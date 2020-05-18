#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#ifndef PI
#define PI 3.14159265359
#endif

#include "lights.h"

static constexpr int led_pins[Lights::CH_COUNT] = {
	[Lights::CH_DBG] = 15,
	[Lights::CH_CW] = 2,
	[Lights::CH_WW] = 0,
	[Lights::CH_RED] = 4,
	[Lights::CH_GREEN] = 16,
	[Lights::CH_BLUE] = 17,
};

static constexpr uint32_t PWM_FREQ_HZ = 10000;
static constexpr float PWM_SPREAD_PCT = 0.05;
static constexpr ledc_timer_t PWM_TIMER = LEDC_TIMER_0;
static constexpr ledc_mode_t PWM_MODE = LEDC_LOW_SPEED_MODE;
static constexpr ledc_timer_bit_t PWM_RESOLUTION_BITS = LEDC_TIMER_12_BIT;
static constexpr int32_t PWM_DUTY_MAX = (1U << PWM_RESOLUTION_BITS) - 1;
static constexpr float DUTY_STEP_DV_DT = 1.0 / 0.250;

Lights::Lights(void) :
	_freq(PWM_FREQ_HZ),
	_mode(MODE_OFF)
{
	for (int ch = 0; ch < CH_COUNT; ch++) {
		this->_state[ch] = {
			.current = 0,
			.target = 0,
			.last_ms = 0,
		};
	}
}

void Lights::freq(uint32_t freq)
{
	this->_freq = freq;
	this->_freq_spread = freq * PWM_SPREAD_PCT;
}

uint32_t Lights::freq(void)
{
	return this->_freq;
}

float Lights::duty(int ch)
{
	return this->_state[ch].current;
}

void Lights::target(int ch, float duty)
{
	this->_state[ch].target = duty;
}

float Lights::target(int ch)
{
	return this->_state[ch].target;
}

Lights::mode_t Lights::mode(void)
{
	return _mode;
}

void Lights::mode(mode_t mode)
{
	this->_mode = mode;
}

// #define USE_DSSS
void Lights::loop(void)
{
	long now_ms = this->timeNow();

#ifdef USE_DSSS
	{
		static float x = 0;
		x += 0.01;
		if (x > 2*PI)
			x -= 2*PI;
		uint32_t f = (float)this->_freq + cos(x*2*PI) * this->_freq_spread;
		this->ledFreq(f);
	}
#endif

	if (this->_mode == MODE_OFF) {
		for (int ch = 0; ch < CH_COUNT; ch++) {
			Lights::state_t &state = this->_state[ch];

			if (state.current <= 0) {
				state.last_ms = 0;
				continue;
			}

			if (state.last_ms == 0) {
				state.last_ms = now_ms;
				continue;
			}

			float dv = this->calculateStep(now_ms, state.last_ms);
			if (dv == 0)
				continue;

			if (state.current > dv)
				state.current -= dv;
			else
				state.current = 0;

			state.last_ms = now_ms;
			this->ledDuty(ch, state.current);
			this->ledUpdate(ch);
		}
	} else if (this->_mode == MODE_DEMO) {
		constexpr float period_ms = 2500.0;
		constexpr float e = exp(1);
		float &duty = this->_state[Lights::CH_DBG].current;
		duty = (exp(cos(2 * PI * now_ms / period_ms)) - (1/e)) * 1/(e-(1/e));
		this->ledDuty(Lights::CH_DBG, duty);
		this->ledUpdate(Lights::CH_DBG);
	} else if (this->_mode == MODE_USER) {
		for (int ch = 0; ch < CH_COUNT; ch++) {
			Lights::state_t &state = this->_state[ch];

			if (state.current == state.target) {
				state.last_ms = 0;
				continue;
			}

			if (state.last_ms == 0) {
				state.last_ms = now_ms;
				continue;
			}

			float dv = this->calculateStep(now_ms, state.last_ms);
			if (dv == 0)
				continue;

			if (state.current < state.target - dv) {
				state.current += dv;
			} else if (state.current > state.target + dv) {
				state.current -= dv;
			} else {
				state.current = state.target;
				state.last_ms = 0;
			}

			state.last_ms = now_ms;
			this->ledDuty(ch, state.current);
			this->ledUpdate(ch);
		}
	}
}

float Lights::calculateStep(long now_ms, long last_ms) const
{
	float dt = (now_ms - last_ms)/*ms*/ / 1000.0/*ms/s*/;
	if (dt >= 0.01)
		return dt * DUTY_STEP_DV_DT;
	else
		return 0;
}

long Lights::timeNow(void) const
{
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void Lights::ledConfig(void)
{
	this->ledFreq(PWM_FREQ_HZ);

	for (int ch = 0; ch < CH_COUNT; ch++) {
		ledc_channel_config_t ledc_channel = {
			.gpio_num   = led_pins[ch],
			.speed_mode = PWM_MODE,
			.channel    = static_cast<ledc_channel_t>(ch),
			.intr_type  = LEDC_INTR_DISABLE,
			.timer_sel  = PWM_TIMER,
			.duty       = static_cast<uint32_t>(this->_state[ch].current * PWM_DUTY_MAX),
			.hpoint     = 0,
		};
		ledc_channel_config(&ledc_channel);
	}
}

void Lights::ledFreq(uint32_t freq)
{
	ledc_timer_config_t ledc_timer = {
		.speed_mode = PWM_MODE,
		.duty_resolution = PWM_RESOLUTION_BITS,
		.timer_num = PWM_TIMER,
		.freq_hz = freq,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ledc_timer_config(&ledc_timer);
}

void Lights::ledDuty(int ch, float duty)
{
	ledc_set_duty(PWM_MODE, static_cast<ledc_channel_t>(ch), static_cast<uint32_t>(duty * PWM_DUTY_MAX));
}

void Lights::ledUpdate(int ch)
{
	ledc_update_duty(PWM_MODE, static_cast<ledc_channel_t>(ch));
}