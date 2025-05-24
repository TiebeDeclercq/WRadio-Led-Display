/*
 * flash_storage.c
 *
 *  Created on: May 24, 2025
 *      Author: tiebe
 */

#include "flash_storage.h"
#include <string.h>

static flash_settings_t current_settings;

void Flash_Storage_Init(void)
{
    // Initialize with defaults
    current_settings.magic = FLASH_STORAGE_MAGIC;
    current_settings.mode = MODE_STATIC_LOGO;
    current_settings.brightnessLevel = 2;  // Medium brightness
    memset(current_settings.reserved, 0, sizeof(current_settings.reserved));
    current_settings.checksum = Flash_Storage_CalculateChecksum(&current_settings);
}

uint32_t Flash_Storage_CalculateChecksum(flash_settings_t* settings)
{
    uint32_t checksum = 0;
    uint32_t* data = (uint32_t*)settings;

    // Calculate checksum of all data except the checksum field itself
    for(int i = 0; i < (sizeof(flash_settings_t) - sizeof(uint32_t)) / sizeof(uint32_t); i++) {
        checksum ^= data[i];
    }

    return checksum;
}

effect_mode_t Flash_Storage_ReadMode(void)
{
    flash_settings_t* stored_settings = (flash_settings_t*)FLASH_STORAGE_PAGE_ADDR;

    // Check if flash contains valid data
    if(stored_settings->magic == FLASH_STORAGE_MAGIC) {
        // Verify checksum
        uint32_t calculated_checksum = Flash_Storage_CalculateChecksum(stored_settings);
        if(calculated_checksum == stored_settings->checksum) {
            // Data is valid, check if mode is within range
            if(stored_settings->mode < MODE_COUNT) {
                return stored_settings->mode;
            }
        }
    }

    // Return default if data is invalid
    return MODE_STATIC_LOGO;
}

uint8_t Flash_Storage_ReadBrightnessLevel(void)
{
    flash_settings_t* stored_settings = (flash_settings_t*)FLASH_STORAGE_PAGE_ADDR;

    // Check if flash contains valid data
    if(stored_settings->magic == FLASH_STORAGE_MAGIC) {
        // Verify checksum
        uint32_t calculated_checksum = Flash_Storage_CalculateChecksum(stored_settings);
        if(calculated_checksum == stored_settings->checksum) {
            // Data is valid, check if brightness level is within range
            if(stored_settings->brightnessLevel < 5) { // 0-4 are valid
                return stored_settings->brightnessLevel;
            }
        }
    }

    // Return default if data is invalid
    return 2; // Medium brightness
}

HAL_StatusTypeDef Flash_Storage_SaveSettings(effect_mode_t mode, uint8_t brightnessLevel)
{
    HAL_StatusTypeDef status = HAL_OK;

    // Prepare settings structure
    current_settings.magic = FLASH_STORAGE_MAGIC;
    current_settings.mode = mode;
    current_settings.brightnessLevel = brightnessLevel;
    current_settings.checksum = Flash_Storage_CalculateChecksum(&current_settings);

    // Unlock flash for writing
    HAL_FLASH_Unlock();

    // Erase the page first
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = FLASH_STORAGE_PAGE_ADDR;
    erase_init.NbPages = 1;

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    if(status == HAL_OK) {
        // Write the settings structure word by word
        uint32_t* data = (uint32_t*)&current_settings;
        uint32_t address = FLASH_STORAGE_PAGE_ADDR;

        for(int i = 0; i < sizeof(flash_settings_t) / sizeof(uint32_t); i++) {
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data[i]);
            if(status != HAL_OK) {
                break;
            }
            address += sizeof(uint32_t);
        }
    }

    // Lock flash
    HAL_FLASH_Lock();

    return status;
}
