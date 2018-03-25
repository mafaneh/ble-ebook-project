/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Novel Bits
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
 * File:   garage_sensor_service.h
 * Author: mafaneh
 *
 * Created on October 20, 2017, 11:00 AM
 */

#ifndef GARAGE_SENSOR_SERVICE_H
#define GARAGE_SENSOR_SERVICE_H


#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

// Garage sensor service:               13BB0001-5884-4C5D-B75B-8768DE741149
//   Garage door status characteristic: 13BB0002-5884-4C5D-B75B-8768DE741149
//   Garage temperature characteristic: 13BB0003-5884-4C5D-B75B-8768DE741149
//   Garage humidity characteristic:    13BB0004-5884-4C5D-B75B-8768DE741149

// The bytes are stored in little-endian format, meaning the
// Least Significant Byte is stored first
// (reversed from the order they're displayed as)

// Base UUID: 13BB0000-5884-4C5D-B75B-8768DE741149
#define BLE_UUID_GARAGE_SENSOR_SERVICE_BASE_UUID  {0x49, 0x11, 0x74, 0xDE, 0x68, 0x87, 0x5B, 0xB7, 0x5D, 0x4C, 0x84, 0x58, 0x00, 0x00, 0xBB, 0x13}

// Service & characteristics UUIDs
#define BLE_UUID_GARAGE_SENSOR_SERVICE_UUID       0x0001
#define BLE_UUID_GARAGE_DOOR_STATUS_CHAR_UUID     0x0002
#define BLE_UUID_GARAGE_TEMP_CHAR_UUID            0x0003
#define BLE_UUID_GARAGE_HUMIDITY_CHAR_UUID        0x0004

// Forward declaration of the custom_service_t type.
typedef struct ble_garage_sensor_service_s ble_garage_sensor_service_t;

/**@brief Garage Sensor Service event type. */
typedef enum
{
    BLE_GARAGE_SENSOR_EVT_DOOR_STATUS_NOTIFICATION_ENABLED,  /**< Garage door status notification enabled event. */
    BLE_GARAGE_SENSOR_EVT_DOOR_STATUS_NOTIFICATION_DISABLED, /**< Garage door status notification disabled event. */
    BLE_GARAGE_SENSOR_EVT_TEMP_NOTIFICATION_ENABLED,         /**< Garage temperature notification enabled event. */
    BLE_GARAGE_SENSOR_EVT_TEMP_NOTIFICATION_DISABLED,        /**< Garage temperature notification disabled event. */
    BLE_GARAGE_SENSOR_EVT_HUMIDITY_NOTIFICATION_ENABLED,     /**< Garage humidity notification enabled event. */
    BLE_GARAGE_SENSOR_EVT_HUMIDITY_NOTIFICATION_DISABLED,    /**< Garage humidity notification disabled event. */
} ble_garage_sensor_evt_type_t;

/**@brief Garage Sensor Service event. */
typedef struct
{
    ble_garage_sensor_evt_type_t evt_type;                                  /**< Type of event. */
} ble_garage_sensor_evt_t;


/**@brief Garage Sensor Service event handler type. */
typedef void (*ble_garage_sensor_evt_handler_t) (ble_garage_sensor_service_t * p_garage_sensor, ble_garage_sensor_evt_t * p_evt);

/**@brief Garage Sensor Service init structure.
 *        This contains all options and data needed for
 *        initialization of the service.
 */
typedef struct
{
    ble_garage_sensor_evt_handler_t         evt_handler;                   /**< Event handler to be called for handling events in the Garage Sensor Service. */
} ble_garage_sensor_service_init_t;

/**@brief Garage Sensor Service structure.
 *        This contains various status information
 *        for the service.
 */
typedef struct ble_garage_sensor_service_s
{
    uint16_t                         conn_handle;
    uint16_t                         service_handle;
    uint8_t                          uuid_type;
    //ble_garage_sensor_evt_handler_t  evt_handler;
    ble_gatts_char_handles_t         door_status_char_handles;
    ble_gatts_char_handles_t         garage_temp_char_handles;
    ble_gatts_char_handles_t         garage_humidity_char_handles;
    ble_gatts_char_handles_t         battery_level_char_handles;
} ble_garage_sensor_service_t;

/**@brief Function for initializing the Garage Sensor Service.
 *
 * @param[out]  p_garage_sensor_service   Garage Sensor Service structure. This structure will have to be supplied by
 *                               the application. It will be initialized by this function, and will later
 *                               be used to identify this particular service instance.
 *
 * @param[in]   ble_garage_sensor_service_t* Garage sensor service pointer.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_garage_sensor_service_init(ble_garage_sensor_service_t * p_garage_sensor_service);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Garage Sensor Service.
 *
 *
 * @param[in]   p_garage_sensor_service      Garage Sensor Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_garage_sensor_service_on_ble_evt(ble_garage_sensor_service_t * p_garage_sensor_service, ble_evt_t const * p_ble_evt);


uint32_t garage_sensor_temperature_send(ble_garage_sensor_service_t * p_garage_sensor_service, int8_t temperature);

uint32_t garage_sensor_humidity_send(ble_garage_sensor_service_t * p_garage_sensor_service, uint8_t humidity);

uint32_t garage_sensor_battery_level_send(ble_garage_sensor_service_t * p_garage_sensor_service, uint8_t battery_level);

#endif /* GARAGE_SENSOR_SERVICE_H */

