/* ws2812b.c - Fixed Version */
#include "ws2812b.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

/* WS2812B protocol timing - Corrected for 48MHz timer, period 59 */
#define WS2812B_ONE_PULSE   38      // ~0.8us high time
#define WS2812B_ZERO_PULSE  19      // ~0.4us high time
#define WS2812B_RESET_LEN   50      // Reset pulse length

// Global variables
uint16_t ledBuffer[WS2812B_BUFFER_SIZE];
LED_Color ledColors[LED_COUNT];
volatile bool transferComplete = false;

// External HAL handles
extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;

void WS2812B_Init(void)
{
  // Initialize the LED buffer with all LEDs off
  memset(ledColors, 0, sizeof(ledColors));
  WS2812B_PrepareBuffer();

  // Turn off all LEDs at startup
  WS2812B_SendToLEDs();
}

void WS2812B_SetLED(uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
  if (index < LED_COUNT) {
    ledColors[index].red = red;
    ledColors[index].green = green;
    ledColors[index].blue = blue;
  }
}

void WS2812B_Clear(void)
{
  memset(ledColors, 0, sizeof(ledColors));
  WS2812B_PrepareBuffer();
  WS2812B_SendToLEDs();
}

void WS2812B_SetAllLED(uint8_t red, uint8_t green, uint8_t blue)
{
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    ledColors[i].red = red;
    ledColors[i].green = green;
    ledColors[i].blue = blue;
  }
}

void WS2812B_PrepareBuffer(void)
{
  uint16_t bufferIndex = 0;

  // For each LED
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    // Green byte (sent first in WS2812B)
    for (int8_t bit = 7; bit >= 0; bit--) {
      if (bufferIndex >= WS2812B_BUFFER_SIZE) return;
      ledBuffer[bufferIndex++] = (ledColors[i].green & (1 << bit)) ?
                                  WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
    }

    // Red byte
    for (int8_t bit = 7; bit >= 0; bit--) {
      if (bufferIndex >= WS2812B_BUFFER_SIZE) return;
      ledBuffer[bufferIndex++] = (ledColors[i].red & (1 << bit)) ?
                                  WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
    }

    // Blue byte
    for (int8_t bit = 7; bit >= 0; bit--) {
      if (bufferIndex >= WS2812B_BUFFER_SIZE) return;
      ledBuffer[bufferIndex++] = (ledColors[i].blue & (1 << bit)) ?
                                  WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
    }
  }

  // Reset pulse
  for (uint16_t i = 0; i < WS2812B_RESET_LEN && bufferIndex < WS2812B_BUFFER_SIZE; i++) {
    ledBuffer[bufferIndex++] = 0;
  }
}

void WS2812B_SendToLEDs(void)
{
  transferComplete = false;

  // Stop timer and DMA if running (cleanup from previous transfer)
  HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);

  // Start PWM with DMA
  if (HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)ledBuffer, WS2812B_BUFFER_SIZE) != HAL_OK) {
    // Handle error
    return;
  }

  // Wait for transfer completion with timeout
  uint32_t timeout = HAL_GetTick() + 100;
  while (!transferComplete && HAL_GetTick() < timeout) {
    // Wait - DMA will stop automatically when transfer completes
  }

  // Additional small delay to ensure reset pulse completes
  HAL_Delay(1);
}

void WS2812B_TIM_DMADelayPulseFinished(void)
{
  transferComplete = true;
}

// Keep your existing animation functions...
void WS2812B_Rainbow(uint32_t delay_ms)
{
  static uint8_t hue = 0;

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint8_t hsv_h = hue + (i * 256 / LED_COUNT);
    uint8_t hsv_s = 255;
    uint8_t hsv_v = 50; // Reduced brightness

    // Simple HSV to RGB conversion
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
  HAL_Delay(delay_ms);
  hue += 1;
}

uint8_t ws_random_byte(uint8_t max)
{
  static uint32_t seed = 1;
  seed = seed * 1664525 + 1013904223;
  return (seed >> 24) % max;
}

uint8_t ws_random_range(uint8_t min, uint8_t max)
{
  return min + ws_random_byte(max - min);
}
