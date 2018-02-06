/*
 * The MIT License (MIT)
 * Copyright (c) 2017 Novel Bits
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 * 
 */

/* 
 * File:   remote_control_service.h
 * Author: mafaneh
 *
 * Created on October 20, 2017, 11:01 AM
 */

#ifndef REMOTE_CONTROL_SERVICE_H
#define REMOTE_CONTROL_SERVICE_H

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

// Remote Control service:               B49B0001-37C8-4E16-A8C4-49EA4536F44F
//      Battery Level: 0x2A19

// The bytes are stored in little-endian format, meaning the 
// Least Significant Byte is stored first
// (reversed from the order they're displayed as)

// Base UUID: 19210000-D8A0-49CE-8038-2BE02F099430   
#define BLE_UUID_REMOTE_CONTROL_SERVICE_BASE_UUID  {0x4F, 0xF4, 0x36, 0x45, 0xEA, 0x49, 0xC4, 0xA8, 0x16, 0x4E, 0xC8, 0x37, 0x00, 0x00, 0x9B, 0xB4}

// Service & characteristics UUIDs
#define BLE_UUID_REMOTE_CONTROL_SERVICE_UUID         0x0001

// Forward declaration of the custom_service_t type. 
typedef struct ble_remote_control_service_s ble_remote_control_service_t;


/**@brief Remote Control Service event type. */
typedef enum
{
    BLE_REMOTE_CONTROL_BATTERY_LEVEL_EVT_NOTIFICATION_ENABLED,   /**< Remote Control Battery value notification enabled event. */
    BLE_REMOTE_CONTROL_BATTERY_LEVEL_EVT_NOTIFICATION_DISABLED   /**< Remote Control Battery value notification disabled event. */
} ble_remote_control_evt_type_t;

/**@brief Remote Control Service event. */
typedef struct
{
    ble_remote_control_evt_type_t evt_type;        /**< Type of event. */
} ble_remote_control_evt_t;

/**@brief Remote Control Service structure.
 *        This contains various status information 
 *        for the service.
 */
typedef struct ble_remote_control_service_s
{
    uint16_t                         conn_handle;
    uint16_t                         service_handle;
    uint8_t                          uuid_type;
//    ble_remote_control_evt_handler_t  evt_handler;
    ble_gatts_char_handles_t         battery_level_char_handles;
    
} ble_remote_control_service_t;

/**@brief Function for initializing the Remote Control Service.
 *
 * @param[out]  p_remote_control_service  Remote Control Service structure. This structure will have to be supplied by
 *                                        the application. It will be initialized by this function, and will later
 *                                        be used to identify this particular service instance.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_remote_control_service_init(ble_remote_control_service_t * p_remote_control_service);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Remote Control Service.
 *
 *
 * @param[in]   p_remote_control_service  Remote Control Service structure.
 * @param[in]   p_ble_evt                 Event received from the BLE stack.
 */
void ble_remote_control_service_on_ble_evt(ble_remote_control_service_t * p_remote_control_service, ble_evt_t * p_ble_evt);

#endif /* REMOTE_CONTROL_SERVICE_H */

