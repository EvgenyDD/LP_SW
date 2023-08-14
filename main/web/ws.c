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
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <stdarg.h>
#include <sys/param.h>

#include "i2c_adc.h"

// https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/restful_server/main/rest_server.c
// https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/restful_server/README.md

extern void start_dns_server(void);

extern const char root_start[] asm("_binary_index_html_start");
extern const char root_end[] asm("_binary_index_html_end");

#define TAG "WS"

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

static esp_err_t root_get_handler(httpd_req_t *req)
{
	const uint32_t root_len = root_end - root_start;

	ESP_LOGI(TAG, "Serve root");
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, root_start, root_len);

	return ESP_OK;
}

static esp_err_t rtd_get_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root, "i_p", adc_val.i_p);
	cJSON_AddNumberToObject(root, "v_p", adc_val.v_p);
	cJSON_AddNumberToObject(root, "v_n", adc_val.v_n);
	cJSON_AddNumberToObject(root, "v_i", adc_val.v_i);
	cJSON_AddNumberToObject(root, "t_drv", adc_val.t_drv);
	cJSON_AddNumberToObject(root, "t_inv_p", adc_val.t_inv_p);
	cJSON_AddNumberToObject(root, "t_inv_n", adc_val.t_inv_n);
	cJSON_AddNumberToObject(root, "t_lsr", adc_val.t_lsr_head);
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

	ESP_LOGI(TAG, "Redirecting to root");
	return ESP_OK;
}

void ws_init(void)
{
	// #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
	// ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
	// ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));
	// ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
	// #endif // CONFIG_EXAMPLE_CONNECT_WIFI
	// #ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
	// 	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
	// 	ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
	// #endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

	// start webserver
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_open_sockets = 7;
	config.lru_purge_enable = true;

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if(httpd_start(&server, &config) == ESP_OK)
	{
		// Set URI handlers
		ESP_LOGI(TAG, "Registering URI handlers");
		httpd_register_uri_handler(server, &root);
		httpd_register_uri_handler(server, &rtd);
		httpd_register_uri_handler(server, &cmd);
		httpd_register_uri_handler(server, &console);
		httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
	}
	else
	{
		ESP_LOGE(TAG, "httpd_start failed");
	}

	// Start DNS server
	// start_dns_server();
}