/*
 * ws2812b.c
 *
 *  Created on: Apr 17, 2025
 *      Author: tiebe
 */

#include "ws2812b.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

/* WS2812B protocol timing requires specific timing values */
#define WS2812B_ONE_PULSE   48      // For period 71
#define WS2812B_ZERO_PULSE  24      // For period 71
#define WS2812B_RESET_LEN   50      // Keep same

// Global variables
uint16_t ledBuffer[WS2812B_BUFFER_SIZE];
LED_Color ledColors[LED_COUNT];
volatile bool transferComplete = false;

// External HAL handles - These should be defined in main.c
extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;

/**
  * @brief Initialize WS2812B LED driver
  * @note  TIM3 and DMA must be configured in CubeMX
  */
void WS2812B_Init(void)
{
  // Initialize the LED buffer with all LEDs off
  memset(ledColors, 0, sizeof(ledColors));
  WS2812B_PrepareBuffer();

  // Turn off all LEDs at startup
  WS2812B_SendToLEDs();
}

/**
  * @brief Set a specific LED to a given color
  * @param index LED index (0 to LED_COUNT-1)
  * @param red Red component (0-255)
  * @param green Green component (0-255)
  * @param blue Blue component (0-255)
  */
void WS2812B_SetLED(uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
  if (index < LED_COUNT) {
    ledColors[index].red = red;
    ledColors[index].green = green;
    ledColors[index].blue = blue;
  }
}

/**
  * @brief Clear all LEDs and send immediately
  */
void WS2812B_Clear(void)
{
  memset(ledColors, 0, sizeof(ledColors));
  WS2812B_PrepareBuffer();
  WS2812B_SendToLEDs();
}

/**
  * @brief Set all LEDs to the same color
  * @param red Red component (0-255)
  * @param green Green component (0-255)
  * @param blue Blue component (0-255)
  */
void WS2812B_SetAllLED(uint8_t red, uint8_t green, uint8_t blue)
{
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    ledColors[i].red = red;
    ledColors[i].green = green;
    ledColors[i].blue = blue;
  }
}

/**
  * @brief Prepare the buffer according to the WS2812B protocol
  * @note  The WS2812B protocol sends data in GRB order, MSB first
  */
void WS2812B_PrepareBuffer(void)
{
  uint16_t bufferIndex = 0;

  // For each LED
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    // Green byte (sent first in WS2812B)
    for (int8_t bit = 7; bit >= 0; bit--) {
      if (bufferIndex >= WS2812B_BUFFER_SIZE) return; // Safety check
      ledBuffer[bufferIndex++] = (ledColors[i].green & (1 << bit)) ?
                                  WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
    }

    // Red byte
    for (int8_t bit = 7; bit >= 0; bit--) {
      if (bufferIndex >= WS2812B_BUFFER_SIZE) return; // Safety check
      ledBuffer[bufferIndex++] = (ledColors[i].red & (1 << bit)) ?
                                  WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
    }

    // Blue byte
    for (int8_t bit = 7; bit >= 0; bit--) {
      if (bufferIndex >= WS2812B_BUFFER_SIZE) return; // Safety check
      ledBuffer[bufferIndex++] = (ledColors[i].blue & (1 << bit)) ?
                                  WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
    }
  }

  // Reset pulse (all zeros, will output 0V)
  for (uint16_t i = 0; i < WS2812B_RESET_LEN && bufferIndex < WS2812B_BUFFER_SIZE; i++) {
    ledBuffer[bufferIndex++] = 0;
  }
}

/**
  * @brief Send the prepared buffer to the LEDs
  */
void WS2812B_SendToLEDs(void)
{
  transferComplete = false;

  // Stop the timer and DMA if they're running
  HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);

  // Start the PWM with DMA
  HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)ledBuffer, WS2812B_BUFFER_SIZE);

  // Wait for transfer completion
  // In a real application, you might want to use a non-blocking approach
  uint32_t timeout = HAL_GetTick() + 100; // 100ms timeout
  while (!transferComplete && HAL_GetTick() < timeout) {
    // Could do other tasks here or use HAL_Delay
  }
}

/**
  * @brief Handle DMA transfer complete callback
  * @note This function must be called from HAL_TIM_PWM_PulseFinishedCallback
  */
void WS2812B_TIM_DMADelayPulseFinished(void)
{
  transferComplete = true;
}

/**
  * @brief Simple rainbow animation effect
  * @param delay_ms Delay between animation frames in milliseconds
  */
void WS2812B_Rainbow(uint32_t delay_ms)
{
  static uint8_t hue = 0;

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint8_t hsv_h = hue + (i * 256 / LED_COUNT);
    uint8_t hsv_s = 255;
    uint8_t hsv_v = 128;

    // Simple HSV to RGB conversion for rainbow effect
    uint8_t region = hsv_h / 43;
    uint8_t remainder = (hsv_h - (region * 43)) * 6;

    uint8_t p = (hsv_v * (255 - hsv_s)) >> 8;
    uint8_t q = (hsv_v * (255 - ((hsv_s * remainder) >> 8))) >> 8;
    uint8_t t = (hsv_v * (255 - ((hsv_s * (255 - remainder)) >> 8))) >> 8;

    uint8_t r, g, b;

    switch (region) {
      case 0: r = hsv_v; g = t; b = p; break;
      case 1: r = q; g = hsv_v; b = p; break;
      case 2: r = p; g = hsv_v; b = t; break;
      case 3: r = p; g = q; b = hsv_v; break;
      case 4: r = t; g = p; b = hsv_v; break;
      default: r = hsv_v; g = p; b = q; break;
    }

    WS2812B_SetLED(i, r, g, b);
  }

  WS2812B_PrepareBuffer();
  WS2812B_SendToLEDs();

  // Delay a bit
  HAL_Delay(delay_ms);

  // Increment hue for next time
  hue += 1;
}

/**
  * @brief Fire effect animation
  * @param cooling How quickly fire cools down (lower = longer flames)
  * @param sparking Chance of new sparks (higher = more roaring fire)
  */
void WS2812B_Fire(uint8_t cooling, uint8_t sparking)
{
  static uint8_t heat[LED_COUNT];
  uint8_t cooldown;

  // Step 1: Cool down every cell a little
  for (int i = 0; i < LED_COUNT; i++) {
    cooldown = (cooling * 10) / LED_COUNT;

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2: Heat rises from bottom to top
  for (int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3: Randomly ignite new 'sparks' at the bottom
  if (ws_random_byte(255) < sparking) {
    int y = ws_random_byte(7);
    heat[y] = heat[y] + ws_random_range(160, 255);
  }

  // Step 4: Convert heat to LED colors
  for (int j = 0; j < LED_COUNT; j++) {
    WS2812B_SetFromHeatColor(j, heat[j]);
  }

  WS2812B_PrepareBuffer();
  WS2812B_SendToLEDs();
  HAL_Delay(30); // Adjust delay for speed
}

/**
  * @brief Helper for fire effect - converts heat value to color
  * @param ledIndex LED index to set
  * @param temperature Heat value (0-255)
  */
void WS2812B_SetFromHeatColor(uint16_t ledIndex, uint8_t temperature)
{
  // Scale 'heat' down from 0-255 to 0-191
  uint8_t t192 = (temperature * 191) / 255;

  // Calculate ramp up from
  uint8_t heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // Figure out which color range we're in
  if (t192 > 0x80) {                    // hottest
    WS2812B_SetLED(ledIndex, 255, 255, heatramp);
  } else if (t192 > 0x40) {             // middle
    WS2812B_SetLED(ledIndex, 255, heatramp, 0);
  } else {                              // coolest
    WS2812B_SetLED(ledIndex, heatramp, 0, 0);
  }
}

/**
  * @brief Simple PRNG for fire effect
  * @param max Maximum value
  * @return Random number between 0 and max-1
  */
uint8_t ws_random_byte(uint8_t max)
{
  static uint32_t seed = 0;

  // Simple PRNG
  seed = seed * 1664525 + 1013904223;

  return (seed >> 24) % max;
}

/**
  * @brief Overloaded random function for ranges
  * @param min Minimum value
  * @param max Maximum value
  * @return Random number between min and max-1
  */
uint8_t ws_random_range(uint8_t min, uint8_t max)
{
  return min + ws_random_byte(max - min);
}
