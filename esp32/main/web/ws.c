#include "cJSON.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "proto_l0.h"
#include "safety.h"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <stdarg.h>
#include <sys/param.h>

// https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/restful_server/main/rest_server.c
// https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/restful_server/README.md

extern void start_dns_server(void);
extern uint32_t restart_timer;

extern const char root_start[] asm("_binary_index_html_start");
extern const char root_end[] asm("_binary_index_html_end");

#define SCRATCH_BUFSIZE 1512
char scratch[SCRATCH_BUFSIZE];
static esp_ota_handle_t handle = 0;

char ws_console_buffer[512];
uint32_t ws_console_ptr = 0;

void ws_console(const char *s, ...)
{
	if(sizeof(ws_console_buffer) - ws_console_ptr < 2) return;

	va_list args;
	va_start(args, s);
	ws_console_ptr += vsnprintf(&ws_console_buffer[ws_console_ptr], sizeof(ws_console_buffer) - ws_console_ptr, s, args);
	va_end(args);
}

static esp_err_t update_handler(httpd_req_t *req)
{
	safety_disable_power_head();

	if(req->content_len == 0 /*|| req->content_len > MAX_FILE_SIZE*/)
	{
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File size error");
		return ESP_FAIL;
	}

	if(handle != 0)
	{
		esp_ota_end(handle);
		handle = 0;
	}

	if(esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &handle))
	{
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "OTA Bagin failure");
		return ESP_FAIL;
	}

	int remaining = req->content_len;
	while(remaining > 0)
	{
		int received = httpd_req_recv(req, scratch, MIN(remaining, sizeof(scratch)));
		if(received <= 0)
		{
			if(received == HTTPD_SOCK_ERR_TIMEOUT) continue;
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
			return ESP_FAIL;
		}

		if(esp_ota_write(handle, (const void *)scratch, received))
		{
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
			return ESP_FAIL;
		}

		remaining -= received;
	}
	if(esp_ota_end(handle))
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA finish failure");
		return ESP_FAIL;
	}
	if(esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)))
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Boot partition error");
		return ESP_FAIL;
	}

	httpd_resp_set_status(req, HTTPD_200);
	httpd_resp_set_type(req, "text/plain");
	httpd_resp_sendstr(req, "Download success");
	restart_timer = 1000;
	return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
	const uint32_t root_len = root_end - root_start;

	ESP_LOGI("WS", "Serve root");
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, root_start, root_len);

	return ESP_OK;
}

static esp_err_t rtd_get_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root, "i_24", (int)(stm_sts.i_24 * 1000.0f));
	cJSON_AddNumberToObject(root, "u_p24", (int)(stm_sts.u_p24 * 1000.0f));
	cJSON_AddNumberToObject(root, "u_n24", (int)(stm_sts.u_n24 * 1000.0f));
	cJSON_AddNumberToObject(root, "u_in", (int)(stm_sts.u_in * 1000.0f));
	cJSON_AddNumberToObject(root, "t_amp", (int)(stm_sts.t_amp * 10.0f));
	cJSON_AddNumberToObject(root, "t_inv_p", (int)(stm_sts.t[0] * 10.0f));
	cJSON_AddNumberToObject(root, "t_inv_n", (int)(stm_sts.t[1] * 10.0f));
	cJSON_AddNumberToObject(root, "t_head", (int)(stm_sts.t_head * 10.0f));
	cJSON_AddNumberToObject(root, "t_stm", (int)(stm_sts.t_mcu * 10.0f));
	cJSON_AddNumberToObject(root, "fan", (int)stm_sts.vel_fan);
	cJSON_AddNumberToObject(root, "lum", (int)stm_sts.lum);
	cJSON_AddNumberToObject(root, "stm_boot", stm_sts.in_boot);
	cJSON_AddNumberToObject(root, "stm_err", stm_sts.err);
	cJSON_AddNumberToObject(root, "stm_err_l", stm_sts.err_latched);
	cJSON_AddNumberToObject(root, "xlx", (int)(stm_sts.xl[0] * 1000.0f));
	cJSON_AddNumberToObject(root, "xly", (int)(stm_sts.xl[1] * 1000.0f));
	cJSON_AddNumberToObject(root, "xlz", (int)(stm_sts.xl[2] * 1000.0f));
	cJSON_AddNumberToObject(root, "gx", (int)(stm_sts.g[0] * 1000.0f));
	cJSON_AddNumberToObject(root, "gy", (int)(stm_sts.g[1] * 1000.0f));
	cJSON_AddNumberToObject(root, "gz", (int)(stm_sts.g[2] * 1000.0f));

	if(ws_console_ptr)
	{
		cJSON_AddStringToObject(root, "console", ws_console_buffer);
		ws_console_ptr = 0;
	}

	const char *sys_info = cJSON_Print(root);
	httpd_resp_sendstr(req, sys_info);
	free((void *)sys_info);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
	int total_len = req->content_len;
	ESP_LOGI("ws", "req: %s (%d)", req->uri, total_len);
	httpd_resp_sendstr(req, "OK");
	return ESP_OK;
}

static esp_err_t console_handler(httpd_req_t *req)
{
	// int total_len = req->content_len;
	int len = strlen(&req->uri[13]);
	ESP_LOGI("ws", "console: %s (%d)", &req->uri[13], len);
	// if(strcmp(req->uri[13]))
	httpd_resp_sendstr(req, "OK");
	return ESP_OK;
}

static const httpd_uri_t root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = root_get_handler,
};
static const httpd_uri_t update = {
	.uri = "/update*",
	.method = HTTP_POST,
	.handler = update_handler,
};
static const httpd_uri_t rtd = {
	.uri = "/api/rtd",
	.method = HTTP_GET,
	.handler = rtd_get_handler,
};
static const httpd_uri_t cmd = {
	.uri = "/api/cmd",
	.method = HTTP_GET,
	.handler = cmd_handler,
};
static const httpd_uri_t console = {
	.uri = "/api/console",
	.method = HTTP_GET,
	.handler = console_handler,
};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
	// Set status
	httpd_resp_set_status(req, "302 Temporary Redirect");
	// Redirect to the "/" root directory
	httpd_resp_set_hdr(req, "Location", "/");
	// iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
	httpd_resp_send(req, "Redirect to the home", HTTPD_RESP_USE_STRLEN);

	ESP_LOGI("WS", "Redirecting to root");
	return ESP_OK;
}

void ws_init(void)
{
	httpd_handle_t web_server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_open_sockets = 7;
	config.lru_purge_enable = true;

	ESP_LOGI("WS", "Starting httpd server on port: '%d'", config.server_port);
	if(httpd_start(&web_server, &config) == ESP_OK)
	{
		ESP_LOGI("WS", "Registering URI handlers");
		httpd_register_uri_handler(web_server, &root);
		httpd_register_uri_handler(web_server, &update);
		httpd_register_uri_handler(web_server, &rtd);
		httpd_register_uri_handler(web_server, &cmd);
		httpd_register_uri_handler(web_server, &console);
		httpd_register_err_handler(web_server, HTTPD_404_NOT_FOUND, http_404_error_handler);
	}
	else
	{
		ESP_LOGE("WS", "httpd_start failed");
	}

	// start_dns_server();
}