#include <string.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include "wifi.h"

#define DEFAULT_SSID "changeme"
#define DEFAULT_PASS "changemetoo"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

Wifi::Wifi()
{
	// Nothing
}

void Wifi::setup()
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	// Net start
	{
		ESP_ERROR_CHECK(esp_netif_init());
		ESP_ERROR_CHECK(esp_event_loop_create_default());
	}

	// Wifi start
	{
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		wifi_config_t wifi_config;
		memset(&wifi_config, 0, sizeof(wifi_config));
		strcpy((char*)wifi_config.sta.ssid, DEFAULT_SSID);
		strcpy((char*)wifi_config.sta.password, DEFAULT_PASS);
		wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

		esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
		ESP_ERROR_CHECK(esp_netif_set_hostname(sta_netif, "lights"));
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));

		// ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
		ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Wifi::wifi_event_handler, this, NULL));

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
		ESP_ERROR_CHECK(esp_wifi_start());
		ESP_ERROR_CHECK(esp_wifi_connect());
	}
}

void Wifi::loop()
{
	// Nothing
}

esp_err_t Wifi::handle_index(httpd_req_t *req)
{
	httpd_resp_send(req, (const char*)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

esp_err_t Wifi::handle_get(httpd_req_t *req)
{
	assert(req != NULL);
	assert(req->user_ctx != NULL);
	Wifi *wifi = (Wifi*)req->user_ctx;
	const int MAX_RESPONSE_LEN = 128;
	char *buf = (char*)malloc(MAX_RESPONSE_LEN);
	int len;

	httpd_resp_set_type(req, "application/json");

	if (wifi->_lights) {
		len = snprintf(buf, MAX_RESPONSE_LEN, "{\"mode\":%d,\"duty\":[", wifi->_lights->mode());
		for (int ch = 0; ch < Lights::CH_COUNT; ch++)
			len += snprintf(buf + len, MAX_RESPONSE_LEN - len, "%s%1.3f", (ch != 0 ? "," : ""), wifi->_lights->duty(ch));
		len += snprintf(buf + len, MAX_RESPONSE_LEN - len, "],\"target\":[");
		for (int ch = 0; ch < Lights::CH_COUNT; ch++)
			len += snprintf(buf + len, MAX_RESPONSE_LEN - len, "%s%1.3f", (ch != 0 ? "," : ""), wifi->_lights->target(ch));
		len += snprintf(buf + len, MAX_RESPONSE_LEN - len, "]}");
	} else
		len = snprintf(buf, MAX_RESPONSE_LEN, "{}");

	httpd_resp_send(req, buf, len);

	return ESP_OK;
}

esp_err_t Wifi::handle_set(httpd_req_t *req)
{
	Wifi *wifi = (Wifi*)req->user_ctx;
	char query[128];
	char val[64];

	if (wifi->_lights == NULL)
		goto complete;

	if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
		goto complete;

	if (httpd_query_key_value(query, "mode", val, sizeof(val)) == ESP_OK)
		wifi->_lights->mode((Lights::mode_t)strtol(val, NULL, 0));

	if (httpd_query_key_value(query, "target", val, sizeof(val)) == ESP_OK) {
		char *tok, *tok_r;
		int ch;

		tok = strtok_r(val, ",", &tok_r);
		for (ch = 0; tok != NULL && ch < Lights::CH_COUNT; ch++) {
			wifi->_lights->target(ch, strtof(tok, NULL));
			tok = strtok_r(NULL, ",", &tok_r);
		}
	}

complete:
	httpd_resp_send(req, "", 0);

	return ESP_OK;
}

void Wifi::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	Wifi *wifi = (Wifi*)arg;
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		// Httpd start
		{
			httpd_handle_t hd;
			httpd_config_t config = HTTPD_DEFAULT_CONFIG();
			httpd_uri_t uri_index = {
				.uri = "/",
				.method = HTTP_GET,
				.handler = &Wifi::handle_index,
				.user_ctx = wifi,
			};
			httpd_uri_t uri_get = {
				.uri = "/get",
				.method = HTTP_GET,
				.handler = &Wifi::handle_get,
				.user_ctx = wifi,
			};
			httpd_uri_t uri_set = {
				.uri = "/set",
				.method = HTTP_GET,
				.handler = &Wifi::handle_set,
				.user_ctx = wifi,
			};

			httpd_start(&hd, &config);

			httpd_register_uri_handler(hd, &uri_index);
			httpd_register_uri_handler(hd, &uri_get);
			httpd_register_uri_handler(hd, &uri_set);
		}
	}
}
