#include <string.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include "wifi.h"

#define DEFAULT_SSID "changeme"
#define DEFAULT_PASS "changemetoo"
#define WIFI_CONNECT_TIMEOUT_MS 30000
#define WIFI_STATUS_TIMEOUT_MS  30000

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

		ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Wifi::wifi_event_handler, this, NULL));
		ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &Wifi::wifi_event_handler, this, NULL));
		ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &Wifi::wifi_event_handler, this, NULL));

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
		ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
		this->wifi_start();
	}

}

void Wifi::loop()
{
	if ((this->wifi_timeout_ms > 0) && (xTaskGetTickCount() * portTICK_PERIOD_MS >= this->wifi_timeout_ms)) {
		esp_err_t err;
		wifi_ap_record_t ap_info;

		err = esp_wifi_sta_get_ap_info(&ap_info);
		if (err != ESP_OK) {
			printf("Wifi timeout, restart...\n");
			this->wifi_stop();
			this->wifi_start();
		} else {
			printf("Wifi ok: %s %x:%x:%x:%x:%x:%x\n",
				ap_info.ssid,
				ap_info.bssid[0],
				ap_info.bssid[1],
				ap_info.bssid[2],
				ap_info.bssid[3],
				ap_info.bssid[4],
				ap_info.bssid[5]);
			this->wifi_timeout_ms += WIFI_STATUS_TIMEOUT_MS;
		}
	}
}

void Wifi::http_start(void)
{
	if (this->hd != nullptr)
		return;

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_uri_t uri_index = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = &Wifi::handle_index,
		.user_ctx = this,
	};
	httpd_uri_t uri_get = {
		.uri = "/get",
		.method = HTTP_GET,
		.handler = &Wifi::handle_get,
		.user_ctx = this,
	};
	httpd_uri_t uri_set = {
		.uri = "/set",
		.method = HTTP_GET,
		.handler = &Wifi::handle_set,
		.user_ctx = this,
	};

	httpd_start(&this->hd, &config);

	httpd_register_uri_handler(this->hd, &uri_index);
	httpd_register_uri_handler(this->hd, &uri_get);
	httpd_register_uri_handler(this->hd, &uri_set);
}

void Wifi::http_stop(void)
{
	if (this->hd == nullptr)
		return;

	httpd_stop(&this->hd);
	this->hd = nullptr;
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

void Wifi::wifi_start(void)
{
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
	this->wifi_timeout_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) + WIFI_CONNECT_TIMEOUT_MS;
}

void Wifi::wifi_stop(void)
{
	this->wifi_timeout_ms = 0;
	esp_wifi_disconnect();
	esp_wifi_stop();
}

void Wifi::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	Wifi *wifi = (Wifi*)arg;

	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		printf("Got IP, starting HTTP\n");
		wifi->http_start();
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
		printf("Wifi connected\n");
		wifi->wifi_timeout_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS);
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		printf("Wifi disconnected\n");
		wifi->wifi_timeout_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) + WIFI_CONNECT_TIMEOUT_MS;
	}
}
