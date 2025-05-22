/* ws2812b.h - Updated with new effects */
#ifndef SRC_WS2812B_H_
#define SRC_WS2812B_H_

#include "main.h"

/* User configuration */
#define LED_COUNT       76     // Reduced for STM32F030F4 RAM limits
#define WS2812B_BUFFER_SIZE (LED_COUNT * 24 + 50)

/* WR Logo pixel ranges - back to original */
#define W_START    0
#define W_END      20
#define R_START    20
#define R_END      28

/* Effect modes */
typedef enum {
    MODE_STATIC_LOGO = 0,
    MODE_BREATHE,
    MODE_SPARKLE,
    MODE_WAVE,
    MODE_PULSE,
    MODE_RAINBOW,
    MODE_COUNT
} effect_mode_t;

/* Color structure */
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} LED_Color;

/* Function prototypes */
void WS2812B_Init(void);
void WS2812B_SetLED(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812B_SetAllLED(uint8_t red, uint8_t green, uint8_t blue);
void WS2812B_Clear(void);
void WS2812B_PrepareBuffer(void);
void WS2812B_SendToLEDs(void);
void WS2812B_TIM_DMADelayPulseFinished(void);

/* Logo and effect functions */
void WS2812B_SetLogoColors(void);
void WS2812B_BreatheEffect(void);
void WS2812B_SparkleEffect(void);
void WS2812B_WaveEffect(void);
void WS2812B_PulseEffect(void);
void WS2812B_RainbowEffect(void);
uint32_t WS2812B_Wheel(uint8_t wheelPos);
void WS2812B_RunEffect(effect_mode_t mode);

/* Utility functions */
uint32_t ws_random_byte(uint32_t max);
uint32_t WS2812B_Color(uint8_t r, uint8_t g, uint8_t b);

#endif /* SRC_WS2812B_H_ */
