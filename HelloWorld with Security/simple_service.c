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
#include "boards.h"
#include "simple_service.h"

static const uint8_t Button1CharName[]   = "Button 1 press";
static const uint8_t StoreValueCharName[] = "Store Value";
static bool button_notifications_enabled = false;

static void on_connect(ble_simple_service_t * p_simple_service, ble_evt_t const * p_ble_evt)
{
    p_simple_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

static void on_disconnect(ble_simple_service_t * p_simple_service, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_simple_service->conn_handle = BLE_CONN_HANDLE_INVALID;
    button_notifications_enabled = false;
}

static void on_write(ble_simple_service_t * p_simple_service, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    // Custom Value Characteristic Written to.
    if (p_evt_write->handle == p_simple_service->store_value_char_handles.value_handle)
    {
        NRF_LOG_INFO("Store Value written to: %d", *p_evt_write->data);
    }

    if (    (p_evt_write->handle == p_simple_service->button_1_press_char_handles.cccd_handle)
        &&  (p_evt_write->len == 2))
    {
        ble_simple_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            NRF_LOG_INFO("Notifications ENABLED for button 1 press");
            evt.evt_type = BLE_BUTTON_1_PRESS_EVT_NOTIFICATION_ENABLED;
            button_notifications_enabled = true;
        }
        else
        {
            NRF_LOG_INFO("Notifications DISABLED for button 1 press");
            evt.evt_type = BLE_BUTTON_1_PRESS_EVT_NOTIFICATION_DISABLED;
            button_notifications_enabled = false;
        }

        if (p_simple_service->evt_handler != NULL)
        {
            // CCCD written, call application event handler.
            p_simple_service->evt_handler(p_simple_service, &evt);
        }
    }
}

static uint32_t button_1_press_char_add(ble_simple_service_t * p_simple_service)
{
    uint32_t err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    uint8_t init_value = 0;

    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    // Require LESC with MITM (Numeric Comparison)
    BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    // Characteristic Metadata
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = Button1CharName;
    char_md.char_user_desc_size      = sizeof(Button1CharName);
    char_md.char_user_desc_max_size  = sizeof(Button1CharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Button ON press Characteristic UUID
    ble_uuid.type = p_simple_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_BUTTON_1_PRESS_CHAR_UUID;

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
    attr_char_value.p_value      = &init_value;

    return sd_ble_gatts_characteristic_add(p_simple_service->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_simple_service->button_1_press_char_handles);
}

static uint32_t store_value_char_add(ble_simple_service_t * p_simple_service)
{
    uint8_t local_variable_initial_value = 0;
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
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    char_md.char_props.read          = 1;
    char_md.char_props.write         = 1;
    char_md.p_char_user_desc         = StoreValueCharName;
    char_md.char_user_desc_size      = sizeof(StoreValueCharName);
    char_md.char_user_desc_max_size  = sizeof(StoreValueCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = NULL;
    char_md.p_sccd_md                = NULL;

    // Define the Button OFF press Characteristic UUID
    ble_uuid.type = p_simple_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_STORE_VALUE_CHAR_UUID;

    // Attribute Metadata settings
    //attr_md.vloc       = BLE_GATTS_VLOC_USER;
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
    attr_char_value.p_value      = &local_variable_initial_value;

    return sd_ble_gatts_characteristic_add(p_simple_service->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_simple_service->store_value_char_handles);
}

uint32_t ble_simple_service_init(ble_simple_service_t * p_simple_service, ble_simple_evt_handler_t app_evt_handler)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_simple_service->conn_handle = BLE_CONN_HANDLE_INVALID;
    if (app_evt_handler != NULL)
    {
        p_simple_service->evt_handler = app_evt_handler;
    }

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_SIMPLE_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_simple_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Set up the UUID for the service (base + service-specific)
    ble_uuid.type = p_simple_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_SIMPLE_SERVICE_UUID;

    // Set up and add the service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_simple_service->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Add the different characteristics in the service:
    //   Button 1 press characteristic:   E54B0002-67F5-479E-8711-B3B99198CE6C
    //   Store Value characteristic:      E54B0003-67F5-479E-8711-B3B99198CE6C 
    err_code = button_1_press_char_add(p_simple_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    err_code = store_value_char_add(p_simple_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    return NRF_SUCCESS;  
}

void ble_simple_service_on_ble_evt(ble_simple_service_t * p_simple_service, ble_evt_t const * p_ble_evt)
{
    if (p_simple_service == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_simple_service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_simple_service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_simple_service, p_ble_evt);
            break;
        default:
            // No implementation needed.
            break;
    }
}

void button_1_characteristic_update(ble_simple_service_t * p_simple_service, uint8_t *button_action)
{
    uint32_t err_code = NRF_SUCCESS;

    ble_gatts_value_t gatts_value;

    if (p_simple_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        // Initialize value struct.
        memset(&gatts_value, 0, sizeof(gatts_value));

        gatts_value.len     = sizeof(uint8_t);
        gatts_value.offset  = 0;
        gatts_value.p_value = button_action;
    
        // Update database.
        err_code = sd_ble_gatts_value_set(p_simple_service->conn_handle,
                                          p_simple_service->button_1_press_char_handles.value_handle,
                                          &gatts_value);
        APP_ERROR_CHECK(err_code);

        if (button_notifications_enabled)
        {
            NRF_LOG_INFO("Sending notification for button 1 press/release");
            uint16_t               len = sizeof (uint8_t);
            ble_gatts_hvx_params_t hvx_params;
            memset(&hvx_params, 0, sizeof(hvx_params));

            hvx_params.handle = p_simple_service->button_1_press_char_handles.value_handle;

            hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.offset = 0;
            hvx_params.p_len  = &len;
            hvx_params.p_data = (uint8_t*)button_action;

            err_code = sd_ble_gatts_hvx(p_simple_service->conn_handle, &hvx_params);
            APP_ERROR_CHECK(err_code);
        }
    }
}
