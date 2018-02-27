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

#include "ble_bas.h"
#include "playbulb_service.h"

static const uint8_t lightStatusCharName[] = "Light Status";

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_playbulb_service    Playbulb service structure.
 * @param[in]   p_ble_evt             Event received from the BLE stack.
 */
static void on_connect(ble_playbulb_service_t * p_playbulb_service, ble_evt_t * p_ble_evt)
{
    p_playbulb_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_playbulb_service    Playbulb service structure.
 * @param[in]   p_ble_evt             Event received from the BLE stack.
 */
static void on_disconnect(ble_playbulb_service_t * p_playbulb_service, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_playbulb_service->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_playbulb_service  Playbulb service structure.
 * @param[in]   p_ble_evt           Event received from the BLE stack.
 */
static void on_write(ble_playbulb_service_t * p_playbulb_service, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    // Handle enabling/disabling notifications
    // Notifications are enabled/disabled by writing to the
    // Client Characteristic Configuration Descriptor (CCCD)
    if (p_evt_write->len == 2)
    {
        // CCCD written, update notification state
        if (p_evt_write->handle == p_playbulb_service->light_status_char_handles.cccd_handle)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                NRF_LOG_INFO("Notification ENABLED for Playbulb Light status\r\n");
            }
            else
            {
                NRF_LOG_INFO("Notification DISABLED for Playbulb Light status\r\n");
            }
        }
        else if (p_evt_write->handle == p_playbulb_service->battery_level_char_handles.cccd_handle)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                NRF_LOG_INFO("Notification ENABLED for Playbulb Battery level\r\n");
            }
            else
            {
                NRF_LOG_INFO("Notification DISABLED for Playbulb Battery level\r\n");
            }
        }
    }
}


/**@brief Function for adding the Light Status characteristic.
 *
 */
static uint32_t light_status_char_add(ble_playbulb_service_t * p_playbulb_service)
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

    // CCCD settings
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = lightStatusCharName;
    char_md.char_user_desc_size      = sizeof(lightStatusCharName);
    char_md.char_user_desc_max_size  = sizeof(lightStatusCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Light Status Characteristic UUID
    ble_uuid.type = p_playbulb_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_PLAYBULB_LIGHT_STATUS_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_playbulb_service->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_playbulb_service->light_status_char_handles);
}

/**@brief Function for adding the Battery Level characteristic.
 *
 */
static uint32_t battery_level_char_add(ble_playbulb_service_t * p_playbulb_service)
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

    // CCCD settings
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

    // Attribute Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_playbulb_service->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_playbulb_service->battery_level_char_handles);
}

uint32_t ble_playbulb_service_init(ble_playbulb_service_t * p_playbulb_service)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_playbulb_service->conn_handle = BLE_CONN_HANDLE_INVALID;

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_PLAYBULB_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_playbulb_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set up the UUID for the service (base + service-specific)
    ble_uuid.type = p_playbulb_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_PLAYBULB_SERVICE_UUID;

    // Set up and add the service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_playbulb_service->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the different characteristics in the service:
    // - Playbulb light status characteristic: 19210002-D8A0-49CE-8038-2BE02F099430
    // - Playbulb battery characteristic: 0x2A19 (16-bit UUID)
    err_code = light_status_char_add(p_playbulb_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = battery_level_char_add(p_playbulb_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

void ble_playbulb_service_on_ble_evt(ble_playbulb_service_t * p_playbulb_service, ble_evt_t const * p_ble_evt)
{
    if (p_playbulb_service == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_playbulb_service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_playbulb_service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_playbulb_service, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

uint32_t playbulb_battery_level_send(ble_playbulb_service_t * p_playbulb_service, uint8_t battery_level)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (p_playbulb_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t                batt_level;
        uint16_t               len;
        uint16_t               hvx_len;
        ble_gatts_hvx_params_t hvx_params;

        batt_level = battery_level;
        len     = sizeof(uint8_t);
        hvx_len = len;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_playbulb_service->battery_level_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = &battery_level;

        err_code = sd_ble_gatts_hvx(p_playbulb_service->conn_handle, &hvx_params);
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
