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

#include "sdk_common.h"
#include "remote_control_client.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"

#include "nrf_log.h"

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */
#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

// Remote Control Services & Characteristics
// Base UUID: E54B0000-67F5-479E-8711-B3B99198CE6C
#define BLE_UUID_BUTTON_SERVICE_BASE_UUID  {0x6C, 0xCE, 0x98, 0x91, 0xB9, 0xB3, 0x11, 0x87, 0x9E, 0x47, 0xF5, 0x67, 0x00, 0x00, 0x4B, 0xE5}

// Service & characteristics UUIDs
#define BLE_UUID_BUTTON_SERVICE_UUID         0x0001
#define BLE_UUID_BUTTON_ON_PRESS_CHAR_UUID   0x0002
#define BLE_UUID_BUTTON_OFF_PRESS_CHAR_UUID  0x0003

typedef enum
{
    READ_REQ,  /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ  /**< Type identifying that this tx_message is a write request. */
} tx_request_t;

/**@brief Structure for writing a message to the peer, i.e. CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];  /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                       /**< GATTC parameters for this message. */
} write_params_t;

/**@brief Structure for holding data to be transmitted to the connected central.
 */
typedef struct
{
    uint16_t     conn_handle;  /**< Connection handle to be used when transmitting this message. */
    tx_request_t type;         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t       read_handle;  /**< Read request message. */
        write_params_t write_req;    /**< Write request message. */
    } req;
} tx_message_t;


static tx_message_t  m_tx_buffer[TX_BUFFER_SIZE];  /**< Transmit buffer for messages to be transmitted to the central. */
static uint32_t      m_tx_insert_index = 0;        /**< Current index in the transmit buffer where the next message should be inserted. */
static uint32_t      m_tx_index = 0;               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */


/**@brief Function for passing any pending request from the buffer to the stack.
 */
static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            m_tx_index++;
            m_tx_index &= TX_BUFFER_MASK;
        }
        else
        {
            NRF_LOG_DEBUG("SD Read/Write API returns error. This message sending will be "
                          "attempted again..");
        }
    }
}


/**@brief     Function for remote_control write response events.
 *
 * @param[in] p_remote_control_client Pointer to the Heart Rate Client structure.
 * @param[in] p_ble_evt       Pointer to the BLE event received.
 */
static void on_write_rsp(remote_control_client_t * p_remote_control_client, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_remote_control_client->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}


/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function will uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the heart rate measurement from the peer. If
 *            it is, this function will decode the heart rate measurement and send it to the
 *            application.
 *
 * @param[in] p_remote_control_client Pointer to the Remote Control Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(remote_control_client_t * p_remote_control_client, const ble_evt_t * p_ble_evt)
{
    // Check if the event is on the link for this instance
    if (p_remote_control_client->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        NRF_LOG_DEBUG("Received HVX on link 0x%x, not associated to this instance, ignore",
                      p_ble_evt->evt.gattc_evt.conn_handle);
        return;
    }

    // Check if this is an ON Button notification.
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_remote_control_client->peer_remote_control_db.on_button_handle)
    {
        remote_control_client_evt_t remote_control_client_evt;

        NRF_LOG_INFO("ON Button notification received");

        remote_control_client_evt.evt_type     = REMOTE_CONTROL_EVT_ON_BUTTON_PRESS_NOTIFICATION;
        remote_control_client_evt.conn_handle  = p_remote_control_client->conn_handle;

        remote_control_client_evt.params.on_button.button_pressed = p_ble_evt->evt.gattc_evt.params.hvx.data[0];

        p_remote_control_client->evt_handler(p_remote_control_client, &remote_control_client_evt);
    }
    else if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_remote_control_client->peer_remote_control_db.off_button_handle)
    {
        remote_control_client_evt_t remote_control_client_evt;

        NRF_LOG_INFO("OFF Button notification received");

        remote_control_client_evt.evt_type     = REMOTE_CONTROL_EVT_OFF_BUTTON_PRESS_NOTIFICATION;
        remote_control_client_evt.conn_handle  = p_remote_control_client->conn_handle;

        remote_control_client_evt.params.off_button.button_pressed = p_ble_evt->evt.gattc_evt.params.hvx.data[0];

        p_remote_control_client->evt_handler(p_remote_control_client, &remote_control_client_evt);
    }
}


/**@brief     Function for handling Disconnected event received from the SoftDevice.
 *
 * @details   This function check if the disconnect event is happening on the link
 *            associated with the current instance of the module, if so it will set its
 *            conn_handle to invalid.
 *
 * @param[in] p_remote_control_client Pointer to the Remote Control Client structure.
 * @param[in] p_ble_evt       Pointer to the BLE event received.
 */
static void on_disconnected(remote_control_client_t * p_remote_control_client, const ble_evt_t * p_ble_evt)
{
    if (p_remote_control_client->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_remote_control_client->conn_handle                                    = BLE_CONN_HANDLE_INVALID;
        p_remote_control_client->peer_remote_control_db.on_button_cccd_handle   = BLE_GATT_HANDLE_INVALID;
        p_remote_control_client->peer_remote_control_db.on_button_handle        = BLE_GATT_HANDLE_INVALID;
        p_remote_control_client->peer_remote_control_db.off_button_cccd_handle  = BLE_GATT_HANDLE_INVALID;
        p_remote_control_client->peer_remote_control_db.off_button_handle       = BLE_GATT_HANDLE_INVALID;
    }
}


void remote_control_on_db_disc_evt(remote_control_client_t * p_remote_control_client, const ble_db_discovery_evt_t * p_evt)
{
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
    {
        // Debug log
        NRF_LOG_INFO("BLE DB Discovery complete - Remote Control Client. Discovered UUID:%d, Type: %d",
                      p_evt->params.discovered_db.srv_uuid.uuid, p_evt->params.discovered_db.srv_uuid.type);
    }

    // Check if the Button Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_BUTTON_SERVICE_UUID &&
        p_evt->params.discovered_db.srv_uuid.type == p_remote_control_client->service_uuid.type)
    {
        uint32_t i;

        remote_control_client_evt_t evt;

        evt.evt_type    = REMOTE_CONTROL_CLIENT_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;

        NRF_LOG_DEBUG("Button Service discovered.");

        // Look for the characteristics of interest
        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_BUTTON_ON_PRESS_CHAR_UUID)
            {
                // Found ON Button characteristic. Store CCCD handle and continue.
                evt.params.peer_db.on_button_cccd_handle =
                    p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.peer_db.on_button_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                continue;
            }
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_BUTTON_OFF_PRESS_CHAR_UUID)
            {
                // Found OFF Button characteristic. Store CCCD handle and continue.
                evt.params.peer_db.off_button_cccd_handle =
                    p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.peer_db.off_button_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                continue;
            }
        }

        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_remote_control_client->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if ((p_remote_control_client->peer_remote_control_db.on_button_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_remote_control_client->peer_remote_control_db.on_button_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_remote_control_client->peer_remote_control_db.off_button_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_remote_control_client->peer_remote_control_db.off_button_handle == BLE_GATT_HANDLE_INVALID))
            {
                p_remote_control_client->peer_remote_control_db = evt.params.peer_db;
            }
        }

        p_remote_control_client->evt_handler(p_remote_control_client, &evt);
    }
}


uint32_t remote_control_client_init(remote_control_client_t * p_remote_control_client, remote_control_client_init_t * p_remote_control_client_init)
{
    ret_code_t err_code;

    VERIFY_PARAM_NOT_NULL(p_remote_control_client);
    VERIFY_PARAM_NOT_NULL(p_remote_control_client_init);

    // Initialize service structure
    p_remote_control_client->conn_handle = BLE_CONN_HANDLE_INVALID;

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_BUTTON_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_remote_control_client->service_uuid.type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set up the UUID for the service (base + service-specific)
    p_remote_control_client->service_uuid.uuid = BLE_UUID_BUTTON_SERVICE_UUID;

    p_remote_control_client->evt_handler                                   = p_remote_control_client_init->evt_handler;
    p_remote_control_client->conn_handle                                   = BLE_CONN_HANDLE_INVALID;
    p_remote_control_client->peer_remote_control_db.on_button_cccd_handle  = BLE_GATT_HANDLE_INVALID;
    p_remote_control_client->peer_remote_control_db.on_button_handle       = BLE_GATT_HANDLE_INVALID;
    p_remote_control_client->peer_remote_control_db.off_button_cccd_handle = BLE_GATT_HANDLE_INVALID;
    p_remote_control_client->peer_remote_control_db.off_button_handle      = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_evt_register(&p_remote_control_client->service_uuid);
}

void remote_control_client_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    remote_control_client_t * p_remote_control_client = (remote_control_client_t *)p_context;

    if ((p_remote_control_client == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_remote_control_client, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_remote_control_client, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_remote_control_client, p_ble_evt);
            break;

        default:
            break;
    }
}


/**@brief Function for creating a message for writing to the CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
    NRF_LOG_DEBUG("Configuring CCCD. CCCD Handle = %d, Connection Handle = %d",
        handle_cccd,conn_handle);

    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB_16(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB_16(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}


uint32_t remote_control_client_on_button_notify_enable(remote_control_client_t * p_remote_control_client)
{
    VERIFY_PARAM_NOT_NULL(p_remote_control_client);

    NRF_LOG_INFO("Enabling notifications for ON Button presses from Remote Control");

    return cccd_configure(p_remote_control_client->conn_handle,
                          p_remote_control_client->peer_remote_control_db.on_button_cccd_handle,
                          true);
}

uint32_t remote_control_client_off_button_notify_enable(remote_control_client_t * p_remote_control_client)
{
    VERIFY_PARAM_NOT_NULL(p_remote_control_client);

    NRF_LOG_INFO("Enabling notifications for OFF Button presses from Remote Control");

    return cccd_configure(p_remote_control_client->conn_handle,
                          p_remote_control_client->peer_remote_control_db.off_button_cccd_handle,
                          true);
}

uint32_t remote_control_client_handles_assign(remote_control_client_t * p_remote_control_client,
                                  uint16_t conn_handle,
                                  const remote_control_db_t * p_peer_remote_control_handles)
{
    VERIFY_PARAM_NOT_NULL(p_remote_control_client);

    p_remote_control_client->conn_handle = conn_handle;
    if (p_peer_remote_control_handles != NULL)
    {
        p_remote_control_client->peer_remote_control_db = *p_peer_remote_control_handles;
    }
    return NRF_SUCCESS;
}
