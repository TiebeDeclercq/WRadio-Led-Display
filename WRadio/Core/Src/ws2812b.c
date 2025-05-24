/* ws2812b.c - RAM optimized version */
#include "ws2812b.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

#define WS2812B_ONE_PULSE   38
#define WS2812B_ZERO_PULSE  19
#define WS2812B_RESET_LEN   50
#define BASE_BRIGHTNESS     100

#define LED_COUNT 76
#define WS2812B_BUFFER_SIZE (LED_COUNT * 24 + 50)

uint8_t ledBuffer[WS2812B_BUFFER_SIZE];
uint32_t currentColors[LED_COUNT];
volatile bool transferComplete = false;
uint8_t globalBrightness = BASE_BRIGHTNESS;

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

    // Cast uint8_t buffer to uint32_t for DMA (DMA expects uint32_t pointer)
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
    static uint8_t breathBrightness = BASE_BRIGHTNESS / 2;

    if (HAL_GetTick() - lastUpdate < 30) return;
    lastUpdate = HAL_GetTick();

    if (breathDir) {
        breathBrightness++;
        if (breathBrightness >= BASE_BRIGHTNESS * 2) breathDir = 0;
    } else {
        breathBrightness--;
        if (breathBrightness <= BASE_BRIGHTNESS / 2) breathDir = 1;
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

    globalBrightness = BASE_BRIGHTNESS;

    if (sparkleState == 0) {
        WS2812B_SetLogoColors();

        // Only sparkle in the W section (LEDs 0-20)
        sparklePixel = W_START + ws_random_byte(W_END - W_START + 1);

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

    globalBrightness = BASE_BRIGHTNESS;
    WS2812B_SetLogoColors();

    if (waveSection == 0) {
        // Wave in W section - use lighter magenta
        currentColors[wavePos] = WS2812B_Color(255, 150, 200);
        wavePos++;
        if (wavePos > W_END) {
            wavePos = R_START;
            waveSection = 1;
        }
    } else {
        // Wave in R section - use W's magenta color instead of white
        currentColors[wavePos] = WS2812B_Color(255, 0, 100);  // Same as W section
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
        globalBrightness = BASE_BRIGHTNESS * 2.5;
        WS2812B_SetLogoColors();
        pulseState = 1;
    } else {
        globalBrightness = BASE_BRIGHTNESS;
        WS2812B_SetLogoColors();
        pulseState = 0;
    }
}

void WS2812B_RainbowEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint8_t rainbowStep = 0;

    if (HAL_GetTick() - lastUpdate < 60) return;
    lastUpdate = HAL_GetTick();

    globalBrightness = BASE_BRIGHTNESS;

    for(uint16_t i = 0; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Wheel((i + rainbowStep) & 255);
    }

    WS2812B_SendToLEDs();

    rainbowStep += 3;
    // rainbowStep will naturally wrap around from 255 to 0 due to uint8_t overflow
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

// 1. COMET EFFECT - A bright comet with trailing tail
void WS2812B_CometEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint16_t cometPos = W_START;
    static uint8_t direction = 1; // 1 = forward, 0 = backward

    if (HAL_GetTick() - lastUpdate < 80) return;
    lastUpdate = HAL_GetTick();

    globalBrightness = BASE_BRIGHTNESS;

    // Always keep background with its base color
    for(int i = R_END + 1; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Color(30, 30, 150);  // DARK BLUE background
    }

    // Fade W and R sections slightly (create trailing effect)
    for(uint16_t i = W_START; i <= R_END; i++) {
        uint32_t color = currentColors[i];
        uint8_t r = ((color >> 16) & 0xFF) * 0.85;
        uint8_t g = ((color >> 8) & 0xFF) * 0.85;
        uint8_t b = (color & 0xFF) * 0.85;
        currentColors[i] = WS2812B_Color(r, g, b);
    }

    // Bright comet head (only within W and R sections)
    if(cometPos >= W_START && cometPos <= R_END) {
        currentColors[cometPos] = WS2812B_Color(255, 255, 255);
    }

    // Move comet within W and R sections only
    if(direction) {
        cometPos++;
        if(cometPos > R_END) {
            direction = 0;
            cometPos = R_END;
        }
    } else {
        if(cometPos > W_START) {
            cometPos--;
        } else {
            direction = 1;
            cometPos = W_START;
        }
    }

    WS2812B_SendToLEDs();
}

// 2. FILL EFFECT - Fills the logo sections progressively
void WS2812B_FillEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint8_t fillPos = 0;
    static uint8_t fillState = 0; // 0=fill W, 1=fill R, 2=empty all

    if (HAL_GetTick() - lastUpdate < 100) return;
    lastUpdate = HAL_GetTick();

    globalBrightness = BASE_BRIGHTNESS;

    // Always keep background with its base color
    for(int i = R_END + 1; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Color(30, 30, 150);  // DARK BLUE background
    }

    switch(fillState) {
        case 0: // Fill W section sequentially (LEDs 0-19, excluding 20)
            if(fillPos < R_START) { // fillPos goes 0 to 19
                currentColors[W_START + fillPos] = WS2812B_Color(255, 0, 100); // Magenta
                fillPos++;
            } else {
                fillState = 1;
                fillPos = 0;
            }
            break;

        case 1: // Fill R section sequentially (LEDs 20-28, including 20)
            if(fillPos <= (R_END - R_START)) { // fillPos goes 0 to 8 (9 LEDs total)
                currentColors[R_START + fillPos] = WS2812B_Color(255, 255, 255); // White
                fillPos++;
            } else {
                fillState = 2;
                fillPos = 0;
            }
            break;

        case 2: // Empty all W and R sequentially (LEDs 0-28)
            if(fillPos <= R_END) {
                currentColors[W_START + fillPos] = WS2812B_Color(0, 0, 0); // Turn off
                fillPos++;
            } else {
                fillState = 0;
                fillPos = 0;
            }
            break;
    }

    WS2812B_SendToLEDs();
}

// 3. SCANNER/KNIGHT RIDER EFFECT - Back and forth scanning
// SCANNER/KNIGHT RIDER EFFECT - W and R sections only
void WS2812B_ScannerEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint16_t scanPos = W_START;
    static uint8_t direction = 1; // 1 = forward, 0 = backward
    static uint8_t scanWidth = 3; // Width of the scanner beam

    if (HAL_GetTick() - lastUpdate < 50) return;
    lastUpdate = HAL_GetTick();

    globalBrightness = BASE_BRIGHTNESS;

    // Clear W and R sections
    for(int i = W_START; i <= R_END; i++) {
        currentColors[i] = WS2812B_Color(0, 0, 0);
    }

    // Always keep background with its base color
    for(int i = R_END + 1; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Color(30, 30, 150);  // DARK BLUE background
    }

    // Create scanner beam within W and R sections only
    for(uint8_t i = 0; i < scanWidth; i++) {
        uint16_t pos = scanPos + i;
        if(pos >= W_START && pos <= R_END) { // Keep within W and R bounds
            uint8_t brightness = 255 - (i * 80); // Fade the tail
            currentColors[pos] = WS2812B_Color(brightness, 0, 0); // Red scanner
        }
    }

    // Move scanner within W and R sections only
    if(direction) {
        scanPos++;
        if(scanPos >= (R_END - scanWidth + 1)) { // Reached end of R section
            direction = 0;
        }
    } else {
        if(scanPos > W_START) {
            scanPos--;
        } else { // Reached start of W section
            direction = 1;
        }
    }

    WS2812B_SendToLEDs();
}

// 4. COLOR SHIFT EFFECT - Smoothly shifts through colors
void WS2812B_ColorShiftEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint8_t hue = 0;

    if (HAL_GetTick() - lastUpdate < 40) return;
    lastUpdate = HAL_GetTick();

    globalBrightness = BASE_BRIGHTNESS;

    // W section
    for(int i = W_START; i <= W_END; i++) {
        currentColors[i] = WS2812B_Wheel(hue);
    }

    // R section - slightly offset hue
    for(int i = R_START; i <= R_END; i++) {
        currentColors[i] = WS2812B_Wheel(hue + 60);
    }

    // Background - different offset
    for(int i = R_END + 1; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Wheel(hue + 120);
    }

    hue += 2;
    WS2812B_SendToLEDs();
}

// 5. STROBE EFFECT - Alternating flash pattern
void WS2812B_StrobeEffect(void)
{
    static uint32_t lastUpdate = 0;
    static uint8_t strobeState = 0;
    static uint8_t strobeCount = 0;

    if (HAL_GetTick() - lastUpdate < 150) return;
    lastUpdate = HAL_GetTick();

    strobeCount++;
    globalBrightness = BASE_BRIGHTNESS * 2;

    if(strobeState == 0) {
        // Flash W section only (LEDs 0-19)
        for(int i = W_START; i < R_START; i++) {  // Changed <= to <
            currentColors[i] = WS2812B_Color(255, 0, 100);
        }
        // Flash R section (LEDs 20-28)
        for(int i = R_START; i <= R_END; i++) {
            currentColors[i] = WS2812B_Color(0, 0, 0);  // Turn off R
        }
    } else {
        // Turn off W section (LEDs 0-19)
        for(int i = W_START; i < R_START; i++) {  // Changed <= to <
            currentColors[i] = WS2812B_Color(0, 0, 0);
        }
        // Flash R section (LEDs 20-28)
        for(int i = R_START; i <= R_END; i++) {
            currentColors[i] = WS2812B_Color(255, 255, 255);  // Turn on R
        }
    }

    // Keep background dim
    for(int i = R_END + 1; i < LED_COUNT; i++) {
        currentColors[i] = WS2812B_Color(10, 10, 50);
    }

    if(strobeCount >= 4) { // Flash 4 times then switch
        strobeState = 1 - strobeState;
        strobeCount = 0;
    }

    WS2812B_SendToLEDs();
}

void WS2812B_RunEffect(effect_mode_t mode)
{
    switch(mode) {
        case MODE_STATIC_LOGO:
            // Static logo - no animation needed
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

        case MODE_COMET:
            WS2812B_CometEffect();
            break;

        case MODE_FILL:
            WS2812B_FillEffect();
            break;

        case MODE_SCANNER:
            WS2812B_ScannerEffect();
            break;

        case MODE_COLOR_SHIFT:
            WS2812B_ColorShiftEffect();
            break;

        case MODE_STROBE:
            WS2812B_StrobeEffect();
            break;

        case MODE_COUNT:
        default:
            // Fallback to static logo for invalid modes
            break;
    }
}

uint32_t ws_random_byte(uint32_t max)
{
    static uint32_t seed = 1;
    seed = seed * 1664525 + 1013904223;
    return (seed >> 16) % max;
}
