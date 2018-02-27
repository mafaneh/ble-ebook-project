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
 * File:   playbulb_service.h
 * Author: mafaneh
 *
 * Created on October 20, 2017, 11:01 AM
 */

#ifndef PLAYBULB_SERVICE_H
#define PLAYBULB_SERVICE_H

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_bas.h"

// Playbulb service:               19210001-D8A0-49CE-8038-2BE02F099430
//   Light status characteristic:  19210002-D8A0-49CE-8038-2BE02F099430

// The bytes are stored in little-endian format, meaning the
// Least Significant Byte is stored first
// (reversed from the order they're displayed as)

// Base UUID: 19210000-D8A0-49CE-8038-2BE02F099430
#define BLE_UUID_PLAYBULB_SERVICE_BASE_UUID  {0x30, 0x94, 0x09, 0x2F, 0xE0, 0x2B, 0x38, 0x80, 0xCE, 0x49, 0xA0, 0xD8, 0x00, 0x00, 0x21, 0x19}

// Service & characteristics UUIDs
#define BLE_UUID_PLAYBULB_SERVICE_UUID         0x0001
#define BLE_UUID_PLAYBULB_LIGHT_STATUS_UUID    0x0002

// Forward declaration of the custom_service_t type.
typedef struct ble_playbulb_service_s ble_playbulb_service_t;


/**@brief Playbulb Service event type. */
typedef enum
{
    BLE_PLAYBULB_LIGHT_STATUS_EVT_NOTIFICATION_ENABLED,    /**< Playbulb Light status value notification enabled event. */
    BLE_PLAYBULB_LIGHT_STATUS_EVT_NOTIFICATION_DISABLED,   /**< Playbulb Light status value notification disabled event. */
    BLE_PLAYBULB_BATTERY_LEVEL_EVT_NOTIFICATION_ENABLED,   /**< Playbulb Battery value notification enabled event. */
    BLE_PLAYBULB_BATTERY_LEVEL_EVT_NOTIFICATION_DISABLED   /**< Playbulb Battery value notification disabled event. */
} ble_playbulb_evt_type_t;

/**@brief Playbulb Service event. */
typedef struct
{
    ble_playbulb_evt_type_t evt_type;        /**< Type of event. */
} ble_playbulb_evt_t;

/**@brief Playbulb Service structure.
 *        This contains various status information
 *        for the service.
 */
typedef struct ble_playbulb_service_s
{
    uint16_t                         conn_handle;
    uint16_t                         service_handle;
    uint8_t                          uuid_type;
//    ble_playbulb_evt_handler_t  evt_handler;
    ble_gatts_char_handles_t         light_status_char_handles;
    ble_gatts_char_handles_t         battery_level_char_handles;

} ble_playbulb_service_t;

/**@brief Function for initializing the Playbulb Service.
 *
 * @param[out]  p_playbulb_service  Playbulb Service structure. This structure will have to be supplied by
 *                                  the application. It will be initialized by this function, and will later
 *                                  be used to identify this particular service instance.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_playbulb_service_init(ble_playbulb_service_t * p_playbulb_service);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Playbulb Service.
 *
 *
 * @param[in]   p_playbulb_service  Playbulb Service structure.
 * @param[in]   p_ble_evt           Event received from the BLE stack.
 */
void ble_playbulb_service_on_ble_evt(ble_playbulb_service_t * p_playbulb_service, ble_evt_t const * p_ble_evt);

uint32_t playbulb_battery_level_send(ble_playbulb_service_t * p_playbulb_service, uint8_t battery_level);

#endif /* PLAYBULB_SERVICE_H */

