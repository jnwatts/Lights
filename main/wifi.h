#pragma once

#include <esp_http_server.h>

#include "lights.h"

class Wifi
{
public:
	Wifi();

	void setup();
	void loop();

	void setLights(Lights *lights) { this->_lights = lights; };

private:
	Lights *_lights = nullptr;
	httpd_handle_t hd = nullptr;

	void http_start(void);
	void http_stop(void);
	static esp_err_t handle_index(httpd_req_t *req);
	static esp_err_t handle_get(httpd_req_t *req);
	static esp_err_t handle_set(httpd_req_t *req);
	void wifi_start(void);
	void wifi_stop(void);
	static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

	long wifi_timeout_ms = 0;
};