#include <stdio.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lights.h"
#include "wifi.h"

Lights lights;
Wifi wifi;

static void lightsFunc(void *arg);
static void wifiFunc(void *arg);

static const int LIGHTS_STACK_SIZE = 4096;
static const int WIFI_STACK_SIZE = 4096;

extern "C"
void app_main(void)
{
	printf("Lights v1.0\n");

	xTaskCreatePinnedToCore((TaskFunction_t)&wifiFunc, "task_wifi", WIFI_STACK_SIZE, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore((TaskFunction_t)&lightsFunc, "task_lights", LIGHTS_STACK_SIZE, NULL, 1, NULL, 1);

	for (;;) {
		vTaskDelay(1000);
	}
}

static void lightsFunc(void *arg)
{
	lights.setup();

	lights.mode(Lights::MODE_DEMO);

	for (;;)
		lights.loop();
}

static void wifiFunc(void *arg)
{
	wifi.setup();
	wifi.setLights(&lights);

	printf("wifi loop\n");
	for (;;) {
		wifi.loop();
#warning TODO: Handle connection loss. I probably need to un/re-register httpd?
		vTaskDelay(100);
	}
}
