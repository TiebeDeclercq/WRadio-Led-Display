/* ws2812b.c - Simple version with blue background */
#include "ws2812b.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

#define WS2812B_ONE_PULSE   38
#define WS2812B_ZERO_PULSE  19
#define WS2812B_RESET_LEN   50

/* Simple approach - keep original LED count */
#define LED_COUNT 76
#define WS2812B_BUFFER_SIZE (LED_COUNT * 24 + 50)

uint16_t ledBuffer[WS2812B_BUFFER_SIZE];
uint32_t currentColors[LED_COUNT];
volatile bool transferComplete = false;
static uint8_t globalBrightness = 10;

/* Original ranges for 76 LEDs */
#define W_START 0
#define W_END 20
#define R_START 20
#define R_END 28

extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;

void WS2812B_Init(void)
{
    memset(currentColors, 0, sizeof(currentColors));
    WS2812B_SetLogoColors();
}

uint32_t WS2812B_Color(uint8_t r, uint8_t g, uint8_t b)
{
    r = (r * globalBrightness) / 255;
    g = (g * globalBrightness) / 255;
    b = (b * globalBrightness) / 255;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void WS2812B_PrepareBuffer(void)
{
    uint16_t bufferIndex = 0;

    for (uint16_t i = 0; i < LED_COUNT; i++) {
        uint32_t color = currentColors[i];
        uint8_t green = (color >> 8) & 0xFF;
        uint8_t red = (color >> 16) & 0xFF;
        uint8_t blue = color & 0xFF;

        for (int8_t bit = 7; bit >= 0; bit--) {
            ledBuffer[bufferIndex++] = (green & (1 << bit)) ? WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
        }
        for (int8_t bit = 7; bit >= 0; bit--) {
            ledBuffer[bufferIndex++] = (red & (1 << bit)) ? WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
        }
        for (int8_t bit = 7; bit >= 0; bit--) {
            ledBuffer[bufferIndex++] = (blue & (1 << bit)) ? WS2812B_ONE_PULSE : WS2812B_ZERO_PULSE;
        }
    }

    for (uint16_t i = 0; i < WS2812B_RESET_LEN; i++) {
        ledBuffer[bufferIndex++] = 0;
    }
}

void WS2812B_SendToLEDs(void)
{
    transferComplete = false;
    HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);

    WS2812B_PrepareBuffer();

    if (HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)ledBuffer, WS2812B_BUFFER_SIZE) != HAL_OK) {
        return;
    }

    uint32_t timeout = HAL_GetTick() + 100;
    while (!transferComplete && HAL_GetTick() < timeout) {
    }
}

void WS2812B_TIM_DMADelayPulseFinished(void)
{
    transferComplete = true;
}

void WS2812B_SetLogoColors(void)
{
    /* W = Magenta */
    for(int i = W_START; i <= W_END; i++) {
        currentColors[i] = WS2812B_Color(255, 0, 100);
    }

    /* R = White */
    for(int i = R_START; i <= R_END; i++) {
        currentColors[i] = WS2812B_Color(255, 255, 255);
    }

    /* Background = DARK BLUE */
    for(int i = R_END + 1; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Color(30, 30, 150);  // DARK BLUE
    }

    WS2812B_SendToLEDs();
}

void WS2812B_Clear(void)
{
    memset(currentColors, 0, sizeof(currentColors));
    WS2812B_SendToLEDs();
}

void WS2812B_BreatheEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint8_t breathDir = 1;
    static uint8_t breathBrightness = 5;

    if (HAL_GetTick() - lastUpdate < 30) return;
    lastUpdate = HAL_GetTick();

    if (breathDir) {
        breathBrightness++;
        if (breathBrightness >= 20) breathDir = 0;
    } else {
        breathBrightness--;
        if (breathBrightness <= 5) breathDir = 1;
    }

    globalBrightness = breathBrightness;
    WS2812B_SetLogoColors();
}

void WS2812B_SparkleEffect(void)
{
    static uint32_t lastSparkle = 0;
    static uint16_t sparklePixel = 0;
    static uint8_t sparkleState = 0;

    if (HAL_GetTick() - lastSparkle < 150) return;
    lastSparkle = HAL_GetTick();

    globalBrightness = 10;

    if (sparkleState == 0) {
        WS2812B_SetLogoColors();

        uint8_t section = ws_random_byte(2);
        if (section == 0) {
            sparklePixel = W_START + ws_random_byte(W_END - W_START + 1);
        } else {
            sparklePixel = R_START + ws_random_byte(R_END - R_START + 1);
        }

        currentColors[sparklePixel] = WS2812B_Color(255, 255, 255);
        sparkleState = 1;
    } else {
        WS2812B_SetLogoColors();
        sparkleState = 0;
    }

    WS2812B_SendToLEDs();
}

void WS2812B_WaveEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint16_t wavePos = W_START;
    static uint8_t waveSection = 0;

    if (HAL_GetTick() - lastUpdate < 80) return;
    lastUpdate = HAL_GetTick();

    globalBrightness = 10;
    WS2812B_SetLogoColors();

    if (waveSection == 0) {
        currentColors[wavePos] = WS2812B_Color(255, 150, 200);
        wavePos++;
        if (wavePos > W_END) {
            wavePos = R_START;
            waveSection = 1;
        }
    } else {
        currentColors[wavePos] = WS2812B_Color(255, 255, 255);
        wavePos++;
        if (wavePos > R_END) {
            wavePos = W_START;
            waveSection = 0;
        }
    }

    WS2812B_SendToLEDs();
}

void WS2812B_PulseEffect(void)
{
    static uint32_t lastPulse = 0;
    static uint8_t pulseState = 0;

    if (HAL_GetTick() - lastPulse < 400) return;
    lastPulse = HAL_GetTick();

    if (pulseState == 0) {
        globalBrightness = 25;
        WS2812B_SetLogoColors();
        pulseState = 1;
    } else {
        globalBrightness = 10;
        WS2812B_SetLogoColors();
        pulseState = 0;
    }
}

void WS2812B_RainbowEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint8_t rainbowStep = 0;
    static uint16_t rainbowCounter = 0;

    if (HAL_GetTick() - lastUpdate < 30) return;
    lastUpdate = HAL_GetTick();

    for(uint16_t i = 0; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Wheel((i + rainbowStep) & 255);
    }

    WS2812B_SendToLEDs();

    rainbowStep += 3;
    rainbowCounter++;

    if (rainbowCounter > 150) {
        rainbowStep = 0;
        rainbowCounter = 0;
        globalBrightness = 10;
        WS2812B_SetLogoColors();
    }
}

uint32_t WS2812B_Wheel(uint8_t wheelPos)
{
    wheelPos = 255 - wheelPos;
    if(wheelPos < 85) {
        return WS2812B_Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if(wheelPos < 170) {
        wheelPos -= 85;
        return WS2812B_Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return WS2812B_Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

void WS2812B_RunEffect(effect_mode_t mode)
{
    switch(mode) {
        case MODE_STATIC_LOGO:
            break;
        case MODE_BREATHE:
            WS2812B_BreatheEffect();
            break;
        case MODE_SPARKLE:
            WS2812B_SparkleEffect();
            break;
        case MODE_WAVE:
            WS2812B_WaveEffect();
            break;
        case MODE_PULSE:
            WS2812B_PulseEffect();
            break;
        case MODE_RAINBOW:
            WS2812B_RainbowEffect();
            break;
        case MODE_COUNT:
        default:
            break;
    }
}

uint32_t ws_random_byte(uint32_t max)
{
    static uint32_t seed = 1;
    seed = seed * 1664525 + 1013904223;
    return (seed >> 16) % max;
}
