/**
******************************************************************************
* @file           : flash_storage.h
* @brief          : handles storage of brightness settings, current animation in flash
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

#ifndef INC_FLASH_STORAGE_H_
#define INC_FLASH_STORAGE_H_

#include "main.h"
#include "ws2812b.h"

/* Flash storage configuration */
#define FLASH_STORAGE_PAGE_ADDR   0x08003C00  // Last 1KB of 16KB flash
#define FLASH_STORAGE_MAGIC       0xDEADBEEF  // Magic number to verify valid data

/* Storage structure */
typedef struct {
    uint32_t magic;           // Magic number for validation
    effect_mode_t mode;       // Current effect mode
    uint8_t brightnessLevel;  // Brightness level index (0-4)
    uint8_t reserved[6];      // Reserved for future use
    uint32_t checksum;        // Simple checksum
} flash_settings_t;

/* Function prototypes */
void Flash_Storage_Init(void);
effect_mode_t Flash_Storage_ReadMode(void);
uint8_t Flash_Storage_ReadBrightnessLevel(void);
HAL_StatusTypeDef Flash_Storage_SaveSettings(effect_mode_t mode, uint8_t brightnessLevel);
uint32_t Flash_Storage_CalculateChecksum(flash_settings_t* settings);

#endif /* INC_FLASH_STORAGE_H_ */
