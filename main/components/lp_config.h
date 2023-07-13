#ifndef LP_CONFIG_H__
#define LP_CONFIG_H__

#if 0
#define _ESP_ERROR_CHECK(...) ESP_ERROR_CHECK(__VA_ARGS__)
#else
#define _ESP_ERROR_CHECK(...) __VA_ARGS__
#endif

#endif // LP_CONFIG_H__