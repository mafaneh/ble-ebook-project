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


uint8_t m_boot_settings[CODE_PAGE_SIZE]      __attribute__((at(BOOTLOADER_SETTINGS_ADDRESS)))           __attribute__((used));
uint8_t m_boot_settings_pend[CODE_PAGE_SIZE] __attribute__((at(BOOTLOADER_MBR_RETAINING_PAGE_ADDRESS))) __attribute__((used)); 
uint32_t m_uicr_bootloader_start_address     __attribute__((at(NRF_UICR_BOOT_START_ADDRESS))) = BOOTLOADER_REGION_START;
#ifdef NRF52
uint32_t m_uicr_nrffw_1                      __attribute__((at(NRF_UICR_NRFFW_1))) = BOOTLOADER_MBR_RETAINING_PAGE_ADDRESS;
#endif


__ASM void StartApplication(uint32_t start_addr)
{
    LDR   R2, [R0]               ; Get App MSP.
    MSR   MSP, R2                ; Set the main stack pointer to the applications MSP.
    LDR   R3, [R0, #0x00000004]  ; Get application reset vector address.
    BX    R3                     ; No return - stack code is now activated only through SVC and plain interrupts.
    ALIGN
}

