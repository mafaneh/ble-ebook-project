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
 * File:   button_service.h
 * Author: mafaneh
 *
 * Created on October 20, 2017, 11:01 AM
 */

#ifndef BUTTON_SERVICE_H
#define BUTTON_SERVICE_H

#include <stdint.h>
#include "boards.h"
#include "ble.h"
#include "ble_srv_common.h"

// Button service:                     E54B0001-67F5-479E-8711-B3B99198CE6C
//   ON Button press characteristic:   E54B0002-67F5-479E-8711-B3B99198CE6C
//   OFF Button press characteristic:  E54B0003-67F5-479E-8711-B3B99198CE6C

// The bytes are stored in little-endian format, meaning the
// Least Significant Byte is stored first
// (reversed from the order they're displayed as)

// Base UUID: E54B0000-67F5-479E-8711-B3B99198CE6C
#define BLE_UUID_BUTTON_SERVICE_BASE_UUID  {0x6C, 0xCE, 0x98, 0x91, 0xB9, 0xB3, 0x11, 0x87, 0x9E, 0x47, 0xF5, 0x67, 0x00, 0x00, 0x4B, 0xE5}

// Service & characteristics UUIDs
#define BLE_UUID_BUTTON_SERVICE_UUID         0x0001
#define BLE_UUID_BUTTON_ON_PRESS_CHAR_UUID   0x0002
#define BLE_UUID_BUTTON_OFF_PRESS_CHAR_UUID  0x0003

// Forward declaration of the custom_service_t type.
typedef struct ble_button_service_s ble_button_service_t;


/**@brief Button Service event type. */
typedef enum
{
    BLE_BUTTON_ON_PRESS_EVT_NOTIFICATION_ENABLED,    /**< Button ON press value notification enabled event. */
    BLE_BUTTON_ON_PRESS_EVT_NOTIFICATION_DISABLED,   /**< Button ON press value notification disabled event. */
    BLE_BUTTON_OFF_PRESS_EVT_NOTIFICATION_ENABLED,   /**< Button OFF press value notification enabled event. */
    BLE_BUTTON_OFF_PRESS_EVT_NOTIFICATION_DISABLED   /**< Button OFF press value notification disabled event. */
} ble_button_evt_type_t;

/**@brief Button Service event. */
typedef struct
{
    ble_button_evt_type_t evt_type;        /**< Type of event. */
} ble_button_evt_t;

/**@brief Button Service structure.
 *        This contains various status information
 *        for the service.
 */
typedef struct ble_button_service_s
{
    uint16_t                         conn_handle;
    uint16_t                         service_handle;
    uint8_t                          uuid_type;
//    ble_button_evt_handler_t  evt_handler;
    ble_gatts_char_handles_t         button_on_press_char_handles;
    ble_gatts_char_handles_t         button_off_press_char_handles;

} ble_button_service_t;

/**@brief Function for initializing the Button Service.
 *
 * @param[out]  p_button_service  Button Service structure. This structure will have to be supplied by
 *                                the application. It will be initialized by this function, and will later
 *                                be used to identify this particular service instance.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_button_service_init(ble_button_service_t * p_button_service);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Button Service.
 *
 *
 * @param[in]   p_button_service  Button Service structure.
 * @param[in]   p_ble_evt         Event received from the BLE stack.
 */
void ble_button_service_on_ble_evt(ble_button_service_t * p_button_service, ble_evt_t * p_ble_evt);

void button_characteristic_update(ble_button_service_t * p_button_service, uint8_t pin_no, uint8_t *button_action);

#endif /* BUTTON_SERVICE_H */

