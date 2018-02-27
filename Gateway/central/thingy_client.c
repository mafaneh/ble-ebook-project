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
#include "thingy_client.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"

#include "nrf_log.h"

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */
#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

// Thingy Services & Characteristics
// Base UUID for Enviornment service: EF68xxxx-9B35-4933-9B10-52FFA9740042
// Base UUID: 13BB0000-5884-4C5D-B75B-8768DE741149
#define BLE_UUID_ENVIRONMENT_SERVICE_BASE_UUID  {0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B, \
                                                   0x33, 0x49, 0x35, 0x9B, 0x00, 0x00, 0x68, 0xEF }

// Service & characteristics UUIDs
#define BLE_UUID_ENVIRONMENT_SERVICE_UUID   0x0200
#define BLE_UUID_TEMPERATURE_CHAR_UUID      0x0201
#define BLE_UUID_HUMIDITY_CHAR_UUID         0x0203

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


/**@brief     Function for handling write response events.
 *
 * @param[in] p_thingy_client Pointer to the Heart Rate Client structure.
 * @param[in] p_ble_evt       Pointer to the BLE event received.
 */
static void on_write_rsp(thingy_client_t * p_thingy_client, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_thingy_client->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
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
 * @param[in] p_thingy_client Pointer to the Thingy Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(thingy_client_t * p_thingy_client, const ble_evt_t * p_ble_evt)
{
    // Check if the event is on the link for this instance
    if (p_thingy_client->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
//        NRF_LOG_DEBUG("Received HVX on link 0x%x, not associated to this instance, ignore",
//                      p_ble_evt->evt.gattc_evt.conn_handle);
        return;
    }

    // Check if this is a temperature notification.
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_thingy_client->peer_thingy_db.temp_handle)
    {
        thingy_client_evt_t thingy_client_evt;

        //NRF_LOG_INFO("Temperature notification received");

        thingy_client_evt.evt_type     = THINGY_CLIENT_EVT_TEMP_NOTIFICATION;
        thingy_client_evt.conn_handle  = p_thingy_client->conn_handle;

        thingy_client_evt.params.temp.temp_integer = p_ble_evt->evt.gattc_evt.params.hvx.data[0];
        thingy_client_evt.params.temp.temp_decimal = p_ble_evt->evt.gattc_evt.params.hvx.data[1];

        p_thingy_client->evt_handler(p_thingy_client, &thingy_client_evt);
    }
    else if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_thingy_client->peer_thingy_db.humidity_handle)
    {
        thingy_client_evt_t thingy_client_evt;

        //NRF_LOG_INFO("Humidity notification received");

        thingy_client_evt.evt_type     = THINGY_CLIENT_EVT_HUMIDITY_NOTIFICATION;
        thingy_client_evt.conn_handle  = p_thingy_client->conn_handle;

        thingy_client_evt.params.humidity.humidity = p_ble_evt->evt.gattc_evt.params.hvx.data[0];

        p_thingy_client->evt_handler(p_thingy_client, &thingy_client_evt);
    }
}


/**@brief     Function for handling Disconnected event received from the SoftDevice.
 *
 * @details   This function check if the disconnect event is happening on the link
 *            associated with the current instance of the module, if so it will set its
 *            conn_handle to invalid.
 *
 * @param[in] p_thingy_client Pointer to the Thingy Client structure.
 * @param[in] p_ble_evt       Pointer to the BLE event received.
 */
static void on_disconnected(thingy_client_t * p_thingy_client, const ble_evt_t * p_ble_evt)
{
    if (p_thingy_client->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_thingy_client->conn_handle                         = BLE_CONN_HANDLE_INVALID;
        p_thingy_client->peer_thingy_db.temp_cccd_handle     = BLE_GATT_HANDLE_INVALID;
        p_thingy_client->peer_thingy_db.temp_handle          = BLE_GATT_HANDLE_INVALID;
        p_thingy_client->peer_thingy_db.humidity_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_thingy_client->peer_thingy_db.humidity_handle      = BLE_GATT_HANDLE_INVALID;
    }
}


void thingy_on_db_disc_evt(thingy_client_t * p_thingy_client, const ble_db_discovery_evt_t * p_evt)
{
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
    {
        // Debug log
        NRF_LOG_INFO("BLE DB Discovery complete - Thingy Client. Discovered UUID:%d, Type: %d",
                      p_evt->params.discovered_db.srv_uuid.uuid, p_evt->params.discovered_db.srv_uuid.type);
    }

    // Check if the Thingy Environment Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_ENVIRONMENT_SERVICE_UUID &&
        p_evt->params.discovered_db.srv_uuid.type == p_thingy_client->service_uuid.type)
    {
        // Find the CCCD Handle of the Temperature Measurement characteristic.
        uint32_t i;

        thingy_client_evt_t evt;

        evt.evt_type    = THINGY_CLIENT_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;

        NRF_LOG_DEBUG("Thingy Environment Service discovered.");

        // Look for the characteristics of interest
        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_TEMPERATURE_CHAR_UUID)
            {
                // Found Temperature characteristic. Store CCCD handle and continue.
                evt.params.peer_db.temp_cccd_handle =
                    p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.peer_db.temp_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                continue;
            }
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_HUMIDITY_CHAR_UUID)
            {
                // Found Humidity characteristic. Store CCCD handle and continue.
                evt.params.peer_db.humidity_cccd_handle =
                    p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.peer_db.humidity_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                continue;
            }
        }

        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_thingy_client->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if ((p_thingy_client->peer_thingy_db.temp_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_thingy_client->peer_thingy_db.temp_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_thingy_client->peer_thingy_db.humidity_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_thingy_client->peer_thingy_db.humidity_handle == BLE_GATT_HANDLE_INVALID))
            {
                p_thingy_client->peer_thingy_db = evt.params.peer_db;
            }
        }

        p_thingy_client->evt_handler(p_thingy_client, &evt);
    }
}


uint32_t thingy_client_init(thingy_client_t * p_thingy_client, thingy_client_init_t * p_thingy_client_init)
{
    ret_code_t err_code;

    VERIFY_PARAM_NOT_NULL(p_thingy_client);
    VERIFY_PARAM_NOT_NULL(p_thingy_client_init);

    // Initialize service structure
    p_thingy_client->conn_handle = BLE_CONN_HANDLE_INVALID;

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_ENVIRONMENT_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_thingy_client->service_uuid.type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set up the UUID for the service (base + service-specific)
    p_thingy_client->service_uuid.uuid = BLE_UUID_ENVIRONMENT_SERVICE_UUID;

    p_thingy_client->evt_handler                         = p_thingy_client_init->evt_handler;
    p_thingy_client->conn_handle                         = BLE_CONN_HANDLE_INVALID;
    p_thingy_client->peer_thingy_db.temp_cccd_handle     = BLE_GATT_HANDLE_INVALID;
    p_thingy_client->peer_thingy_db.temp_handle          = BLE_GATT_HANDLE_INVALID;
    p_thingy_client->peer_thingy_db.humidity_cccd_handle = BLE_GATT_HANDLE_INVALID;
    p_thingy_client->peer_thingy_db.humidity_handle      = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_evt_register(&p_thingy_client->service_uuid);
}

void thingy_client_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    thingy_client_t * p_thingy_client = (thingy_client_t *)p_context;

    if ((p_thingy_client == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_thingy_client, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_thingy_client, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_thingy_client, p_ble_evt);
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


uint32_t thingy_client_temp_notify_enable(thingy_client_t * p_thingy_client)
{
    VERIFY_PARAM_NOT_NULL(p_thingy_client);

    NRF_LOG_INFO("Enabling notifications for Temperature readings from Thingy");

    return cccd_configure(p_thingy_client->conn_handle,
                          p_thingy_client->peer_thingy_db.temp_cccd_handle,
                          true);
}

uint32_t thingy_client_humidity_notify_enable(thingy_client_t * p_thingy_client)
{
    VERIFY_PARAM_NOT_NULL(p_thingy_client);

    NRF_LOG_INFO("Enabling notifications for Humidity readings from Thingy");

    return cccd_configure(p_thingy_client->conn_handle,
                          p_thingy_client->peer_thingy_db.humidity_cccd_handle,
                          true);
}

uint32_t thingy_client_handles_assign(thingy_client_t * p_thingy_client,
                                  uint16_t conn_handle,
                                  const thingy_db_t * p_peer_thingy_handles)
{
    VERIFY_PARAM_NOT_NULL(p_thingy_client);

    p_thingy_client->conn_handle = conn_handle;
    if (p_peer_thingy_handles != NULL)
    {
        p_thingy_client->peer_thingy_db = *p_peer_thingy_handles;
    }
    return NRF_SUCCESS;
}
