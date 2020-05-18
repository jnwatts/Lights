#pragma once

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

	static esp_err_t handle_index(httpd_req_t *req);
	static esp_err_t handle_get(httpd_req_t *req);
	static esp_err_t handle_set(httpd_req_t *req);
	static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};