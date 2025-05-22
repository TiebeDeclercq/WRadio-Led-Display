/*
 * ws2812b.h
 *
 *  Created on: Apr 17, 2025
 *      Author: tiebe
 */

#ifndef SRC_WS2812B_H_
#define SRC_WS2812B_H_

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* User configuration ---------------------------------------------------------*/
#define LED_COUNT       10     // Number of LEDs in your strip

/* Calculated automatically - do not modify ---------------------------------*/
#define WS2812B_BUFFER_SIZE (LED_COUNT * 24 + 50)  // 24 bits per LED + reset

/* Types ---------------------------------------------------------------------*/
typedef struct {
  uint8_t green;  // Note: WS2812B expects data in GRB order
  uint8_t red;
  uint8_t blue;
} LED_Color;

/* Function prototypes -------------------------------------------------------*/
void WS2812B_Init(void);
void WS2812B_SetLED(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812B_SetAllLED(uint8_t red, uint8_t green, uint8_t blue);
void WS2812B_PrepareBuffer(void);
void WS2812B_SendToLEDs(void);
void WS2812B_TIM_DMADelayPulseFinished(void);
void WS2812B_Rainbow(uint32_t delay_ms);
void WS2812B_Fire(uint8_t cooling, uint8_t sparking);
void WS2812B_SetFromHeatColor(uint16_t ledIndex, uint8_t temperature);
uint8_t ws_random_byte(uint8_t max);
uint8_t ws_random_range(uint8_t min, uint8_t max);


#endif /* SRC_WS2812B_H_ */
