/**
 * Copyright (c) 2013 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "bootloader_util.h"
#include <stdint.h>
#include "nordic_common.h"
#include "bootloader_types.h"
#include <dfu_types.h>
#include "compiler_abstraction.h"

/**< This variables reserves a codepage for bootloader specific settings, to ensure the compiler doesn't locate any code or variables at his location. */
extern uint8_t  m_boot_settings[CODE_PAGE_SIZE];
extern uint8_t  m_boot_settings_pend[CODE_PAGE_SIZE];

/**< This variable ensures that the linker script will write the bootloader start address to the UICR register. This value will be written in the HEX file and thus written to UICR when the bootloader is flashed into the chip. */
extern uint32_t m_uicr_bootloader_start_address; 

#ifdef NRF52

/**< This variable ensures that the linker script will write the retaining page address to the UICR register. This value will be written in the HEX file and thus written to UICR when the bootloader is flashed into the chip. */
extern uint32_t m_uicr_nrffw_1;

#endif


/**@brief Function for starting the application (compiler specific).
 *
 * @param[in]  start_addr  Start address.
 */
void StartApplication(uint32_t start_addr);


void bootloader_util_app_start(uint32_t start_addr)
{
    StartApplication(start_addr);
}

void bootloader_util_settings_get(const bootloader_settings_t ** pp_bootloader_settings)
{
    // Read only pointer to bootloader settings in flash.
    static bootloader_settings_t const * const p_bootloader_settings =
        (bootloader_settings_t *)&m_boot_settings[0];

    *pp_bootloader_settings = p_bootloader_settings;
}

