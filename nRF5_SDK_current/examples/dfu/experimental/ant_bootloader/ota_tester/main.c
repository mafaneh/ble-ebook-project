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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "app_error.h"
#include "ant_interface.h"
#include "ant_boot_settings_api.h"
#include "ant_channel_config.h"
#include "app_util_platform.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


// Application's ANT observer priority.
#define APP_ANT_OBSERVER_PRIO           1

// Channel configuration.
#define ANT_CHANNEL_NUMBER              0x00  /**< ANT Channel Number*/
#define ANT_RF_FREQUENCY                0x32u /**< Channel RF Frequency = (2400 + 50)MHz */
#define ANT_CHANNEL_PERIOD              8192u /**< Channel period 4 Hz. */
#define ANT_EXT_ASSIGN                  0x00  /**< ANT Ext Assign. */
#define ANT_NETWORK_NUMBER              0x00  /**< Network Number */
#define ANT_TRANSMIT_POWER              0u    /**< ANT Custom Transmit Power (Invalid/Not Used). */

// Channel ID configuration.
#define ANT_DEV_TYPE                    0x20u                   /**< Device type = 32. */
#define ANT_TRANS_TYPE                  0x05u                   /**< Transmission type. */
#define ANT_DEV_NUM                     (NRF_FICR->DEVICEID[0]) /**< Device number. */

// Test broadcast data
#define BROADCAST_PAYLOAD               {0x01, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xEE}
#define BROADCAST_DATA_BUFFER_SIZE      8

// Version string
#define VERSION_STRING                  "BFM1.00B01"

// Message definitions
#define COMMAND_ID                      0x02u
#define COMMAND_RESTART_BOOTLOADER      0x01u

static const uint8_t m_version_string[]      = VERSION_STRING; // Version string
static volatile bool m_restart_in_bootloader = false;          // Flag indicating to start bootloader


/**@brief Reset the device, and start bootloader
*/
static void restart_in_bootloader(void)
{
    ret_code_t                            err_code;
    bool                                  enter_boot_mode = true;
    __ALIGN(4) static ant_boot_settings_t ant_boot_settings;

    // Clear and set FFs to the memory block.
    err_code = ant_boot_settings_clear(&ant_boot_settings);
    APP_ERROR_CHECK(err_code);

    // Fill ant_boot_settings structure.
    memcpy((void *) ant_boot_settings.app_version, m_version_string, sizeof(m_version_string));
    ant_boot_settings.app_size = 2000; // Estimated current application size used to try to preserve itself

    // Save ant_boot_settings structure.
    err_code = ant_boot_settings_save(&ant_boot_settings);
    APP_ERROR_CHECK(err_code);

    // Sets flag to indicate restarting in bootloader mode. Must be done last before the reset!!!
    ant_boot_settings_validate(enter_boot_mode);

    NRF_LOG_FINAL_FLUSH();
    NVIC_SystemReset();
}


/**@brief Function for setting up the ANT module to be ready for TX broadcast.
 */
static void ant_channel_tx_broadcast_setup(void)
{
    ret_code_t err_code;
    uint8_t    broadcast_data[] = BROADCAST_PAYLOAD;

    ant_channel_config_t broadcast_channel_config =
    {
        .channel_number    = ANT_CHANNEL_NUMBER,
        .channel_type      = CHANNEL_TYPE_MASTER,
        .ext_assign        = ANT_EXT_ASSIGN,
        .rf_freq           = ANT_RF_FREQUENCY,
        .transmission_type = ANT_TRANS_TYPE,
        .device_type       = ANT_DEV_TYPE,
        .device_number     = ANT_DEV_NUM,
        .channel_period    = ANT_CHANNEL_PERIOD,
        .network_number    = ANT_NETWORK_NUMBER,
    };

    err_code = ant_channel_init(&broadcast_channel_config);
    APP_ERROR_CHECK(err_code);

    // Setup broadcast payload
    err_code = sd_ant_broadcast_message_tx(ANT_CHANNEL_NUMBER,
                                           BROADCAST_DATA_BUFFER_SIZE,
                                           broadcast_data);
    if (err_code != NRF_ANT_ERROR_CHANNEL_IN_WRONG_STATE)
    {
        APP_ERROR_CHECK(err_code);
    }

    // Open channel.
    err_code = sd_ant_channel_open(ANT_CHANNEL_NUMBER);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling ANT TX channel events.
 *
 * @param[in] p_ant_evt  ANT stack event.
 * @param[in] p_context  Context.
 */
static void ant_evt_handler(ant_evt_t * p_ant_evt, void * p_context)
{
    uint8_t page_num;
    uint8_t command;

    switch (p_ant_evt->event)
    {
        case EVENT_RX:
            switch (p_ant_evt->message.ANT_MESSAGE_ucMesgID)
            {
                case MESG_BROADCAST_DATA_ID:
                case MESG_ACKNOWLEDGED_DATA_ID:
                    page_num = p_ant_evt->message.ANT_MESSAGE_aucPayload[0];
                    command  = p_ant_evt->message.ANT_MESSAGE_aucPayload[7];
                    if (page_num == COMMAND_ID && command == COMMAND_RESTART_BOOTLOADER)
                    {
                        NRF_LOG_INFO("Received ANT command to start bootloader");
                        m_restart_in_bootloader = true;
                    }
                    break;
            }
            break;
        default:
            break;
    }
}

NRF_SDH_ANT_OBSERVER(m_ant_observer, APP_ANT_OBSERVER_PRIO, ant_evt_handler, NULL);

/**@brief Function for ANT stack initialization.
 */
static void softdevice_setup(void)
{
    ret_code_t err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    ASSERT(nrf_sdh_is_enabled());

    err_code = nrf_sdh_ant_enable();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for application main entry. Does not return.
 */
int main(void)
{
    // Initialize.
    log_init();
    softdevice_setup();
    ant_channel_tx_broadcast_setup();

    // Start execution.
    NRF_LOG_INFO("ANT OTA tester example started.");

    // Main loop.
    for (;;)
    {
        if (NRF_LOG_PROCESS() == false)
        {
            // Put CPU in sleep if possible.
            ret_code_t err_code = sd_app_evt_wait();
            APP_ERROR_CHECK(err_code);
            
            if (m_restart_in_bootloader)
            {
                restart_in_bootloader();
            }
        }
    }
}

