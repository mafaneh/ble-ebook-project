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

// Needed for including sdk_config.h LOG defines
#include "sdk_common.h"

#define NRF_LOG_MODULE_NAME Central
#if CENTRAL_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL CENTRAL_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR CENTRAL_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR CENTRAL_CONFIG_DEBUG_COLOR
#else //CENTRAL_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL 0
#endif //CENTRAL_CONFIG_LOG_ENABLED
#include "nrf_log.h"

// nRF specific includes
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"

// Application specific includes
#include "playbulb_client.h"

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   4    /**< Length of the write message for the Playbulb color setting (used for ON and OFF as well). */

// Playbulb Services & Characteristics

// Service & characteristics UUIDs
#define BLE_UUID_PLAYBULB_SERVICE_UUID               0xFF02
#define BLE_UUID_PLAYBULB_COLOR_SETTING_CHAR_UUID    0xFFFC

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


/**@brief     Function for playbulb write response events.
 *
 * @param[in] p_playbulb_client Pointer to the Heart Rate Client structure.
 * @param[in] p_ble_evt       Pointer to the BLE event received.
 */
static void on_write_rsp(playbulb_client_t * p_playbulb_client, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_playbulb_client->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        //TODO: Handle errors and responses
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
 * @param[in] p_playbulb_client Pointer to the Playbulb Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(playbulb_client_t * p_playbulb_client, const ble_evt_t * p_ble_evt)
{
    // Check if the event is on the link for this instance
    if (p_playbulb_client->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
//        NRF_LOG_DEBUG("Received HVX on link 0x%x, not associated to this instance, ignore",
//                      p_ble_evt->evt.gattc_evt.conn_handle);
        return;
    }
}


/**@brief     Function for handling Disconnected event received from the SoftDevice.
 *
 * @details   This function check if the disconnect event is happening on the link
 *            associated with the current instance of the module, if so it will set its
 *            conn_handle to invalid.
 *
 * @param[in] p_playbulb_client Pointer to the Playbulb Client structure.
 * @param[in] p_ble_evt       Pointer to the BLE event received.
 */
static void on_disconnected(playbulb_client_t * p_playbulb_client, const ble_evt_t * p_ble_evt)
{
    if (p_playbulb_client->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_playbulb_client->conn_handle                              = BLE_CONN_HANDLE_INVALID;
        p_playbulb_client->peer_playbulb_db.color_setting_handle    = BLE_GATT_HANDLE_INVALID;
    }
}


void playbulb_on_db_disc_evt(playbulb_client_t * p_playbulb_client, const ble_db_discovery_evt_t * p_evt)
{
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
    {
        // Debug log
        NRF_LOG_INFO("BLE DB Discovery complete - Playbulb Client. Discovered UUID:%d, Type: %d",
                      p_evt->params.discovered_db.srv_uuid.uuid, p_evt->params.discovered_db.srv_uuid.type);
    }

    // Check if the Button Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_PLAYBULB_SERVICE_UUID &&
        p_evt->params.discovered_db.srv_uuid.type == p_playbulb_client->service_uuid.type)
    {
        uint32_t i;

        playbulb_client_evt_t evt;

        evt.evt_type    = PLAYBULB_CLIENT_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;

        NRF_LOG_DEBUG("Playbulb Service discovered.");

        // Look for the characteristics of interest
        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_PLAYBULB_COLOR_SETTING_CHAR_UUID)
            {
                // Found Color Setting characteristic. Store handle and continue.
                NRF_LOG_INFO("Found Color Setting characteristic with handle = %d",
                              p_evt->params.discovered_db.charateristics[i].characteristic.handle_value);
                evt.params.peer_db.color_setting_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                continue;
            }
        }

        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_playbulb_client->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if (p_playbulb_client->peer_playbulb_db.color_setting_handle == BLE_GATT_HANDLE_INVALID)
            {
                p_playbulb_client->peer_playbulb_db = evt.params.peer_db;
            }
        }

        p_playbulb_client->evt_handler(p_playbulb_client, &evt);
    }
}


uint32_t playbulb_client_init(playbulb_client_t * p_playbulb_client, playbulb_client_init_t * p_playbulb_client_init)
{
    ret_code_t err_code;

    VERIFY_PARAM_NOT_NULL(p_playbulb_client);
    VERIFY_PARAM_NOT_NULL(p_playbulb_client_init);

    p_playbulb_client->service_uuid.type = BLE_UUID_TYPE_BLE;
    p_playbulb_client->service_uuid.uuid = BLE_UUID_PLAYBULB_SERVICE_UUID;

    p_playbulb_client->evt_handler                             = p_playbulb_client_init->evt_handler;
    p_playbulb_client->conn_handle                             = BLE_CONN_HANDLE_INVALID;
    p_playbulb_client->peer_playbulb_db.color_setting_handle   = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_evt_register(&p_playbulb_client->service_uuid);
}

void playbulb_client_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    playbulb_client_t * p_playbulb_client = (playbulb_client_t *)p_context;

    if ((p_playbulb_client == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_playbulb_client, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_playbulb_client, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_playbulb_client, p_ble_evt);
            break;

        default:
            break;
    }
}

uint32_t playbulb_client_handles_assign(playbulb_client_t * p_playbulb_client,
                                        uint16_t conn_handle,
                                        const playbulb_db_t * p_peer_playbulb_handles)
{
    VERIFY_PARAM_NOT_NULL(p_playbulb_client);

    p_playbulb_client->conn_handle = conn_handle;
    if (p_peer_playbulb_handles != NULL)
    {
        p_playbulb_client->peer_playbulb_db = *p_peer_playbulb_handles;
    }
    return NRF_SUCCESS;
}

uint32_t playbulb_client_turn_on(playbulb_client_t * p_playbulb_client)
{
    uint32_t err_code;
    NRF_LOG_DEBUG("Sending the Turn ON light command to the Playbulb (Handle = %d)", p_playbulb_client->peer_playbulb_db.color_setting_handle);

    tx_message_t * p_msg;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = p_playbulb_client->peer_playbulb_db.color_setting_handle;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_CMD;
    p_msg->req.write_req.gattc_value[0]        = 0xFF;
    p_msg->req.write_req.gattc_value[1]        = 0x00;
    p_msg->req.write_req.gattc_value[2]        = 0x00;
    p_msg->req.write_req.gattc_value[3]        = 0x00;
    p_msg->conn_handle                         = p_playbulb_client->conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}

uint32_t playbulb_client_turn_off(playbulb_client_t * p_playbulb_client)
{
    uint32_t err_code;
    NRF_LOG_DEBUG("Sending the Turn OFF light command to the Playbulb (Handle = %d)", p_playbulb_client->peer_playbulb_db.color_setting_handle);

    tx_message_t * p_msg;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = p_playbulb_client->peer_playbulb_db.color_setting_handle;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_CMD;
    p_msg->req.write_req.gattc_value[0]        = 0x00;
    p_msg->req.write_req.gattc_value[1]        = 0x00;
    p_msg->req.write_req.gattc_value[2]        = 0x00;
    p_msg->req.write_req.gattc_value[3]        = 0x00;
    p_msg->conn_handle                         = p_playbulb_client->conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS; 
}
