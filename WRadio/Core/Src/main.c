/* USER CODE BEGIN Header */
/**
******************************************************************************
* @file           : main.c
* @brief          : handles button presses, data saved in flash, and animates WR logo hardware
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
* @attention
*
* Copyright (c) 2025 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
* @author         : Tiebe Declercq
* @copyright      : Copyright (c) 2025 DECTRONICS. All rights reserved.
* @version        : 1.0.0
* @date           : 2025-05-24
******************************************************************************
*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ws2812b.h"
#include "flash_storage.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAVE_DELAY_MS    		2000    // Wait 2 seconds before saving to flash
#define LONG_PRESS_TIME_MS      1000    // 1 second for long press detection
#define BRIGHTNESS_LEVELS       5       // Number of brightness levels
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch1_trig;

/* USER CODE BEGIN PV */
/* Global variables */
static effect_mode_t currentMode = MODE_STATIC_LOGO;
static uint32_t lastButtonPress = 0;
static uint32_t lastModeChange = 0;
static uint32_t buttonPressStartTime = 0;
static uint8_t buttonPressed = 0;
static uint8_t buttonReleased = 1;
static uint8_t modePendingSave = 0;
static uint8_t brightnessLevel = 2;

static uint8_t brightnessLevels[BRIGHTNESS_LEVELS] = {50, 100, 150, 200, 255};
/*
Level 0: [████░░░░░░] 50   (20% brightness)
Level 1: [████████░░] 100  (40% brightness)  ◀── Default
Level 2: [██████████] 150  (60% brightness)
Level 3: [██████████] 200  (80% brightness)
Level 4: [██████████] 255  (100% brightness)
*/

uint8_t baseBrightness = 100;  // This will be set based on brightness level
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void HandleButtonPress(void);
void HandleFlashSave(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  HAL_Delay(100);

  // Initialize flash storage
  Flash_Storage_Init();

  // Load saved effect mode and brightness
  currentMode = Flash_Storage_ReadMode();
  brightnessLevel = Flash_Storage_ReadBrightnessLevel();

  // Set the base brightness from the level
  baseBrightness = brightnessLevels[brightnessLevel];
  globalBrightness = baseBrightness;

  // Initialize the WS2812B driver
  WS2812B_Init();

  // Apply the loaded effect immediately
  WS2812B_RunEffect(currentMode);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HandleButtonPress();
    HandleFlashSave();      // Check if we need to save settings
    WS2812B_RunEffect(currentMode);
    HAL_Delay(1);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 59;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HandleButtonPress(void)
{
    uint8_t buttonState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    uint32_t currentTime = HAL_GetTick();

    if (buttonState == GPIO_PIN_SET && buttonReleased) {
        // Button just pressed down
        if (currentTime - lastButtonPress > 50) { // 50ms debounce
            buttonPressed = 1;
            buttonReleased = 0;
            buttonPressStartTime = currentTime;
            lastButtonPress = currentTime;
        }
    }
    else if (buttonState == GPIO_PIN_RESET && !buttonReleased) {
        // Button just released
        buttonReleased = 1;
        buttonPressed = 0;

        uint32_t pressDuration = currentTime - buttonPressStartTime;

        if (pressDuration >= LONG_PRESS_TIME_MS) {
            // LONG PRESS - Change brightness
            brightnessLevel++;
            if (brightnessLevel >= BRIGHTNESS_LEVELS) {
                brightnessLevel = 0;  // Loop back to lowest
            }

            // Update base brightness that will be used by all effects
            baseBrightness = brightnessLevels[brightnessLevel];
            globalBrightness = baseBrightness;

            // Trigger static logo update if we're in static mode
            if (currentMode == MODE_STATIC_LOGO) {
                WS2812B_TriggerStaticLogoUpdate();
            }

            // Mark settings for saving
            modePendingSave = 1;
            lastModeChange = currentTime;

        } else if (pressDuration >= 200) {
            // SHORT PRESS - Change effect (with minimum 200ms press)
            currentMode++;
            if (currentMode >= MODE_COUNT) {
                currentMode = MODE_STATIC_LOGO;
            }

            // Mark mode for saving
            modePendingSave = 1;
            lastModeChange = currentTime;
        }
    }
}

void HandleFlashSave(void)
{
    /* Save to flash after SAVE_DELAY_MS milliseconds of no changes */
    if (modePendingSave) {
        uint32_t currentTime = HAL_GetTick();
        if (currentTime - lastModeChange > SAVE_DELAY_MS) {
            // Save the current mode and brightness level to flash
            if (Flash_Storage_SaveSettings(currentMode, brightnessLevel) == HAL_OK) {
                modePendingSave = 0;  // Clear the pending save flag
            }
        }
    }
}

// Add this function to main.c
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) {
    WS2812B_TIM_DMADelayPulseFinished();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
