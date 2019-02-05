/**
 * This software is subject to the ANT+ Shared Source License
 * www.thisisant.com/swlicenses
 * Copyright (c) Garmin Canada Inc. 2014
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *    1) Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *
 *    2) Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 *    3) Neither the name of Garmin nor the names of its
 *       contributors may be used to endorse or promote products
 *       derived from this software without specific prior
 *       written permission.
 *
 * The following actions are prohibited:
 *
 *    1) Redistribution of source code containing the ANT+ Network
 *       Key. The ANT+ Network Key is available to ANT+ Adopters.
 *       Please refer to http://thisisant.com to become an ANT+
 *       Adopter and access the key. 
 *
 *    2) Reverse engineering, decompilation, and/or disassembly of
 *       software provided in binary form under this license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE HEREBY
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; DAMAGE TO ANY DEVICE, LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE. SOME STATES DO NOT ALLOW 
 * THE EXCLUSION OF INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO THE
 * ABOVE LIMITATIONS MAY NOT APPLY TO YOU.
 *
 */

#include <stdint.h>
#include <string.h>

#include "ant_interface.h"
#include "ant_parameters.h"
#include "nrf_sdh_soc.h"


#include "bootloader_types.h"
#include "ant_boot_settings_api.h"

 /**< This variable reserves a codepage for bootloader specific settings, to ensure the compiler doesn't locate any code or variables at his location. */
#if defined ( __CC_ARM )
uint8_t m_ant_boot_settings[ANT_BOOT_SETTINGS_SIZE] __attribute__((at(ANT_BOOT_SETTINGS_LOCATION)));
#else
uint8_t m_ant_boot_settings[ANT_BOOT_SETTINGS_SIZE] __attribute__((section (".ant_boot_settings"))) __attribute__((used));
#endif

volatile uint8_t mb_flash_busy = false;
/*
 * sd_flash_page_erase() and sd_flash_write() generates an event at SD_EVT_IRQHandler
 * Please include run this function inside SD_EVT_IRQHandler
 *
 */
void ant_boot_settings_sys_event_handler(uint32_t sys_evt, void * p_context)
{
    if ((sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS) || (sys_evt == NRF_EVT_FLASH_OPERATION_ERROR))
    {
        mb_flash_busy = false;
    }
}

NRF_SDH_SOC_OBSERVER(m_soc_evt_observer, 0, ant_boot_settings_sys_event_handler, NULL);

uint32_t ant_boot_settings_save(ant_boot_settings_t * boot_settings)
{
    ret_code_t err_code = NRF_SUCCESS;

    mb_flash_busy = true;
    err_code      = sd_flash_write((uint32_t *) ANT_BOOT_SETTINGS_LOCATION,
                                   (uint32_t*)boot_settings,
                                    ANT_BOOT_SETTINGS_SIZE / 4);
    if (err_code == NRF_SUCCESS)
    {
        while (mb_flash_busy); // wait until it is done
    }
    else
    {
        return err_code;
    }

    return err_code;
}

uint32_t ant_boot_settings_clear(ant_boot_settings_t * boot_settings)
{
    ret_code_t err_code = NRF_SUCCESS;

    // Clears \ presets the bootloader_settings memory
    memset(boot_settings, 0xFF, sizeof(ant_boot_settings_t));

    // Erases entire bootloader_settings in flash
    mb_flash_busy = true;
    err_code      = sd_flash_page_erase(FLASH_LAST_PAGE); // last flash page
    if (err_code == NRF_SUCCESS)
    {
        while (mb_flash_busy); // wait until it is done
    }
    else
    {
        return err_code;
    }

    return err_code;
}

void ant_boot_settings_get(const ant_boot_settings_t ** pp_boot_settings)
{
    // Read only pointer to antfs boot settings in flash.
    static ant_boot_settings_t const * const p_boot_settings =
        (ant_boot_settings_t *)&m_ant_boot_settings[0];

    *pp_boot_settings = p_boot_settings;
}

void ant_boot_settings_validate(bool enter_boot_mode)
{
    ret_code_t err_code  = NRF_SUCCESS;
    uint32_t   param_flags;

    if (enter_boot_mode)
    {
        param_flags = 0xFFFFFFFC;
    }
    else
    {
        param_flags = 0xFFFFFFFE;
    }

    mb_flash_busy = true;
    err_code      = sd_flash_write((uint32_t *) ANT_BOOT_PARAM_FLAGS_BASE ,
                                   (uint32_t *) &param_flags,
                                    1);
    if (err_code == NRF_SUCCESS)
    {
        while (mb_flash_busy); // wait until it is done
    }
}


