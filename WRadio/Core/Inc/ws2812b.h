/**
******************************************************************************
* @file           : ws2812b.c
* @brief          : handles ws2812b led animations of the matrix
******************************************************************************
*
* ██████╗ ███████╗ ██████╗████████╗██████╗  ██████╗ ███╗   ██╗██╗ ██████╗███████╗
* ██╔══██╗██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗████╗  ██║██║██╔════╝██╔════╝
* ██║  ██║█████╗  ██║        ██║   ██████╔╝██║   ██║██╔██╗ ██║██║██║     ███████╗
* ██║  ██║██╔══╝  ██║        ██║   ██╔══██╗██║   ██║██║╚██╗██║██║██║     ╚════██║
* ██████╔╝███████╗╚██████╗   ██║   ██║  ██║╚██████╔╝██║ ╚████║██║╚██████╗███████║
* ╚═════╝ ╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝ ╚═════╝╚══════╝
*
******************************************************************************
* @author         : Tiebe Declercq
* @copyright      : Copyright (c) 2025 DECTRONICS. All rights reserved.
* @version        : 1.0.0
* @date           : 2025-05-24
******************************************************************************
*/
#ifndef SRC_WS2812B_H_
#define SRC_WS2812B_H_

#include "main.h"

/* User configuration */
#define LED_COUNT	76 //76 WRADIO
#define WS2812B_RESET_LEN   50
#define WS2812B_BUFFER_SIZE (LED_COUNT * 24 + WS2812B_RESET_LEN)

/* WR Logo pixel ranges */
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
    MODE_COMET,
    MODE_FILL,
    MODE_SCANNER,
    MODE_COLOR_SHIFT,
    MODE_STROBE,
    MODE_COUNT
} effect_mode_t;

/* Color structure */
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} LED_Color;

extern uint8_t globalBrightness;
extern uint8_t baseBrightness;

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
void WS2812B_StaticLogoEffect(void);
void WS2812B_BreatheEffect(void);
void WS2812B_SparkleEffect(void);
void WS2812B_WaveEffect(void);
void WS2812B_PulseEffect(void);
void WS2812B_RainbowEffect(void);
void WS2812B_CometEffect(void);
void WS2812B_FillEffect(void);
void WS2812B_ScannerEffect(void);
void WS2812B_ColorShiftEffect(void);
void WS2812B_StrobeEffect(void);
uint32_t WS2812B_Wheel(uint8_t wheelPos);
void WS2812B_RunEffect(effect_mode_t mode);

/* Utility functions */
uint32_t ws_random_byte(uint32_t max);
uint32_t WS2812B_Color(uint8_t r, uint8_t g, uint8_t b);
void WS2812B_TriggerStaticLogoUpdate(void);

#endif
