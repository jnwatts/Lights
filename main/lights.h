#ifndef _LIGHTS_H_
#define _LIGHTS_H_

#include <stdint.h>

class Lights {
public:
  Lights(void);

  enum channel_t {
    CH_DBG,
    CH_CW,
    CH_WW,
    CH_RED,
    CH_GREEN,
    CH_BLUE,
    CH_COUNT,
  };

  enum mode_t {
    MODE_OFF,
    MODE_DEMO,
    MODE_USER,
  };

  void setup() { ledConfig(); }

  void freq(uint32_t freq);
  uint32_t freq(void);

  float duty(int ch);

  void target(int ch, float duty);
  float target(int ch);

  mode_t mode(void);
  void mode(mode_t m);

  void loop(void);

private:
  struct state_t {
    float current;
    float target;
    long last_ms;
  };

  float calculateStep(long now_ms, long last_ms) const;

  long timeNow(void) const;
  void ledConfig(void);
  void ledFreq(uint32_t freq);
  void ledDuty(int ch, float duty);
  void ledUpdate(int ch);

  state_t _state[CH_COUNT];
  uint32_t _freq;
  float _freq_spread;
  mode_t _mode;
};

#endif