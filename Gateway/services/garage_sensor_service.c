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

#include <string.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "garage_sensor_service.h"


static const uint8_t doorStatusCharName[] = "Garage Door Status";
static const uint8_t temperatureCharName[] = "Garage Temperature";
static const uint8_t humidityCharName[] = "Garage Humidity";


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_garage_sensor_service  Garage Sensor Service structure.
 * @param[in]   p_ble_evt                Event received from the BLE stack.
 */
static void on_connect(ble_garage_sensor_service_t * p_garage_sensor_service, ble_evt_t const * p_ble_evt)
{
    p_garage_sensor_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_garage_sensor_service  Garage Sensor Service structure.
 * @param[in]   p_ble_evt                Event received from the BLE stack.
 */
static void on_disconnect(ble_garage_sensor_service_t * p_garage_sensor_service, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_garage_sensor_service->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_garage_sensor_service  Garage Sensor Service structure.
 * @param[in]   p_ble_evt                Event received from the BLE stack.
 */
static void on_write(ble_garage_sensor_service_t * p_garage_sensor_service, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    // Handle enabling/disabling notifications
    // Notifications are enabled/disabled by writing to the
    // Client Characteristic Configuration Descriptor (CCCD)
    if (p_evt_write->len == 2)
    {
        // CCCD written, update notification state
        if (p_evt_write->handle == p_garage_sensor_service->door_status_char_handles.cccd_handle)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                NRF_LOG_INFO("Notification ENABLED for Garage Door Status\r\n");
            }
            else
            {
                NRF_LOG_INFO("Notification DISABLED for Garage Door Status\r\n");
            }
        }
        else if (p_evt_write->handle == p_garage_sensor_service->garage_temp_char_handles.cccd_handle)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                NRF_LOG_INFO("Notification ENABLED for Garage Temperature\r\n");
            }
            else
            {
                NRF_LOG_INFO("Notification DISABLED for Garage Temperature\r\n");
            }
        }
        else if (p_evt_write->handle == p_garage_sensor_service->garage_humidity_char_handles.cccd_handle)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                NRF_LOG_INFO("Notification ENABLED for Garage Humidity\r\n");
            }
            else
            {
                NRF_LOG_INFO("Notification DISABLED for Garage Humidity\r\n");
            }
        }
        else if (p_evt_write->handle == p_garage_sensor_service->battery_level_char_handles.cccd_handle)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                NRF_LOG_INFO("Notification ENABLED for Garage Sensor Battery level\r\n");
            }
            else
            {
                NRF_LOG_INFO("Notification DISABLED for Garage Sensor Battery level\r\n");
            }
        }
    }
}


/**@brief Function for adding the Door Status characteristic.
 *
 */
static uint32_t door_status_char_add(ble_garage_sensor_service_t * p_garage_sensor_service)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    // Clear all the data structures
    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = doorStatusCharName;
    char_md.char_user_desc_size      = sizeof(doorStatusCharName);
    char_md.char_user_desc_max_size  = sizeof(doorStatusCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_GARAGE_SENSOR_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_garage_sensor_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Define the Door Status Characteristic UUID
    ble_uuid.type = p_garage_sensor_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_GARAGE_DOOR_STATUS_CHAR_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute/Characteristic Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_garage_sensor_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_garage_sensor_service->door_status_char_handles);
}

/**@brief Function for adding the Temperature characteristic.
 *
 */
static uint32_t temperature_char_add(ble_garage_sensor_service_t * p_garage_sensor_service)
{
    int8_t initial_temp = 70;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    ble_gatts_char_pf_t presentation_fmt;

    // Clear all the data structures
    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&presentation_fmt, 0, sizeof(presentation_fmt));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // Presentation format
    presentation_fmt.format = BLE_GATT_CPF_FORMAT_SINT8;
    presentation_fmt.unit = 0x272F;

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = temperatureCharName;
    char_md.char_user_desc_size      = sizeof(temperatureCharName);
    char_md.char_user_desc_max_size  = sizeof(temperatureCharName);
    char_md.p_char_pf                = &presentation_fmt;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Temperature Characteristic UUID
    ble_uuid.type = p_garage_sensor_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_GARAGE_TEMP_CHAR_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute/Characteristic Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(int8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(int8_t);
    attr_char_value.p_value      = (uint8_t *)&initial_temp;

    return sd_ble_gatts_characteristic_add(p_garage_sensor_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_garage_sensor_service->garage_temp_char_handles);
}

/**@brief Function for adding the Humidity characteristic.
 *
 */
static uint32_t humidity_char_add(ble_garage_sensor_service_t * p_garage_sensor_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = humidityCharName;
    char_md.char_user_desc_size      = sizeof(humidityCharName);
    char_md.char_user_desc_max_size  = sizeof(humidityCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Humidity Characteristic UUID
    ble_uuid.type = p_garage_sensor_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_GARAGE_HUMIDITY_CHAR_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute/Characteristic Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_garage_sensor_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_garage_sensor_service->garage_humidity_char_handles);
}

/**@brief Function for adding the Battery Level characteristic.
 *
 */
static uint32_t battery_level_char_add(ble_garage_sensor_service_t * p_garage_sensor_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = NULL;
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Battery Level Characteristic UUID (SIG-adopted)
    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_BATTERY_LEVEL_CHAR);

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute/Characteristic Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_garage_sensor_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_garage_sensor_service->battery_level_char_handles);
}

uint32_t ble_garage_sensor_service_init(ble_garage_sensor_service_t * p_garage_sensor_service)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_garage_sensor_service->conn_handle = BLE_CONN_HANDLE_INVALID;

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_GARAGE_SENSOR_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_garage_sensor_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set up the UUID for the service (base + service-specific)
    ble_uuid.type = p_garage_sensor_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_GARAGE_SENSOR_SERVICE_UUID;

    // Set up and add the service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_garage_sensor_service->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the different characteristics in the service:
    // - Garage door status characteristic: 13BB0002-5884-4C5D-B75B-8768DE741149
    // - Garage temperature characteristic: 13BB0003-5884-4C5D-B75B-8768DE741149
    // - Garage humidity characteristic:    13BB0004-5884-4C5D-B75B-8768DE741149
    // - Garage sensor battery characteristic: 0x2A19 (16-bit UUID  SIG-adopted)
    err_code = door_status_char_add(p_garage_sensor_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = temperature_char_add(p_garage_sensor_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = humidity_char_add(p_garage_sensor_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = battery_level_char_add(p_garage_sensor_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    NRF_LOG_INFO("Completed init of garage door service\r\n");
    return NRF_SUCCESS;
}

void ble_garage_sensor_service_on_ble_evt(ble_garage_sensor_service_t * p_garage_sensor_service, ble_evt_t const * p_ble_evt)
{
    if (p_garage_sensor_service == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_garage_sensor_service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_garage_sensor_service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_garage_sensor_service, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

uint32_t garage_sensor_temperature_send(ble_garage_sensor_service_t * p_garage_sensor_service, int8_t temperature)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (p_garage_sensor_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        int8_t                 temp;
        uint16_t               len;
        uint16_t               hvx_len;
        ble_gatts_hvx_params_t hvx_params;

        temp = temperature;
        len     = sizeof(int8_t);
        hvx_len = len;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_garage_sensor_service->garage_temp_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = &temperature;

        err_code = sd_ble_gatts_hvx(p_garage_sensor_service->conn_handle, &hvx_params);
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

uint32_t garage_sensor_humidity_send(ble_garage_sensor_service_t * p_garage_sensor_service, uint8_t humidity)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (p_garage_sensor_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t                hum;
        uint16_t               len;
        uint16_t               hvx_len;
        ble_gatts_hvx_params_t hvx_params;

        hum = humidity;
        len     = sizeof(uint8_t);
        hvx_len = len;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_garage_sensor_service->garage_humidity_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = &humidity;

        err_code = sd_ble_gatts_hvx(p_garage_sensor_service->conn_handle, &hvx_params);
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}

uint32_t garage_sensor_battery_level_send(ble_garage_sensor_service_t * p_garage_sensor_service, uint8_t battery_level)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (p_garage_sensor_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t                batt_level;
        uint16_t               len;
        uint16_t               hvx_len;
        ble_gatts_hvx_params_t hvx_params;

        batt_level = battery_level;
        len     = sizeof(uint8_t);
        hvx_len = len;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_garage_sensor_service->battery_level_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = &battery_level;

        err_code = sd_ble_gatts_hvx(p_garage_sensor_service->conn_handle, &hvx_params);
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}