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

#include <stdint.h>
#include <string.h>

#include "../main.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "../tools.h"
#include "app_util.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "ble_db_discovery.h"
#include "bsp_btn_ble.h"
#include "ble_hci.h"
#include "app_timer.h"
#include "ble_bas_c.h"

#include "thingy_client.h"
#include "remote_control_client.h"
#include "playbulb_client.h"
#include "../peripheral/peripheral.h"
#include "central.h"

#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define CENTRAL_SCANNING_LED            BSP_BOARD_LED_0
#define CENTRAL_CONNECTED_LED           BSP_BOARD_LED_1

#define SCAN_INTERVAL                   0x00A0                                      /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                     0x0050                                      /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_TIMEOUT                    0

#define MIN_CONNECTION_INTERVAL         (uint16_t) MSEC_TO_UNITS(7.5, UNIT_1_25_MS) /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL         (uint16_t) MSEC_TO_UNITS(100, UNIT_1_25_MS)  /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY                   0                                           /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT             (uint16_t) MSEC_TO_UNITS(4000, UNIT_10_MS)  /**< Determines supervision time-out in units of 10 milliseconds. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

BLE_DB_DISCOVERY_ARRAY_DEF(m_db_discovery, 3);                      /**< Database discovery module instances. */

static uint16_t m_conn_handle_thingy_client  = BLE_CONN_HANDLE_INVALID;          /**< Connection handle for the Thingy client application */
static uint16_t m_conn_handle_remote_control_client = BLE_CONN_HANDLE_INVALID;   /**< Connection handle for the Remote Control client application */
static uint16_t m_conn_handle_playbulb_client = BLE_CONN_HANDLE_INVALID;         /**< Connection handle for the Remote Control client application */

//Definition for each of the clients
static thingy_client_t         m_thingy_client;
static remote_control_client_t m_remote_control_client;
static playbulb_client_t       m_playbulb_client;

BLE_BAS_C_DEF(m_bas_client);                                                 /**< Battery Service client module instance. */

/**@brief names which the central applications will scan for, and which will be advertised by the peripherals.
 *  if these are set to empty strings, the UUIDs defined below will be used
 */
 #define NUMBER_OF_TARGET_PERIPHERALS 3
static char const *m_target_periph_names[NUMBER_OF_TARGET_PERIPHERALS] = { "Thingy", "NovelBits RC", "Playbulb Candle"};

/**@brief Parameters used when scanning. */
static ble_gap_scan_params_t const m_scan_params =
{
    .active   = 1,
    .interval = SCAN_INTERVAL,
    .window   = SCAN_WINDOW,
    .timeout  = SCAN_TIMEOUT,
    #if (NRF_SD_BLE_API_VERSION <= 2)
        .selective   = 0,
        .p_whitelist = NULL,
    #endif
    #if (NRF_SD_BLE_API_VERSION >= 3)
        .use_whitelist = 0,
    #endif
};

/**@brief Connection parameters requested for connection. */
static ble_gap_conn_params_t const m_connection_param =
{
    MIN_CONNECTION_INTERVAL,
    MAX_CONNECTION_INTERVAL,
    SLAVE_LATENCY,
    SUPERVISION_TIMEOUT
};

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling database discovery events.
 *
 * @details This function is callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function should forward the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    // Call event handlers for each of the peripherals (Thingy:52, Remote Control, Playbulb Candle)
    thingy_on_db_disc_evt(&m_thingy_client, p_evt);
    remote_control_on_db_disc_evt (&m_remote_control_client, p_evt);
    //playbulb_on_db_disc_evt (&m_playbulb_client, p_evt);
    ble_bas_on_db_disc_evt(&m_bas_client, p_evt);
}

/**
 * @brief Database discovery initialization.
 */
void db_discovery_init(void)
{
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initiating scanning.
 */
void scan_start(void)
{
    ret_code_t err_code;

    (void) sd_ble_gap_scan_stop();

    err_code = sd_ble_gap_scan_start(&m_scan_params);
    // It is okay to ignore this error since we are stopping the scan anyway.
    if (err_code != NRF_ERROR_INVALID_STATE)
    {
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling Battery Level Collector events.
 *
 * @param[in] p_bas_c       Pointer to Battery Service Client structure.
 * @param[in] p_bas_c_evt   Pointer to event structure.
 */
static void bas_c_evt_handler(ble_bas_c_t * p_bas_c, ble_bas_c_evt_t * p_bas_c_evt)
{
    ret_code_t err_code;

    switch (p_bas_c_evt->evt_type)
    {
        case BLE_BAS_C_EVT_DISCOVERY_COMPLETE:
            err_code = ble_bas_c_handles_assign(p_bas_c,
                                                p_bas_c_evt->conn_handle,
                                                &p_bas_c_evt->params.bas_db);
            APP_ERROR_CHECK(err_code);

            // Batttery service discovered. Enable notification of Battery Level.
            NRF_LOG_DEBUG("Battery Service discovered. Reading battery level.");

            err_code = ble_bas_c_bl_read(p_bas_c);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_DEBUG("Enabling Battery Level Notification. ");
            err_code = ble_bas_c_bl_notif_enable(p_bas_c);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_BAS_C_EVT_BATT_NOTIFICATION:
            NRF_LOG_DEBUG("Battery Level received %d %%", p_bas_c_evt->params.battery_level);
            if (p_bas_c_evt->conn_handle == m_conn_handle_thingy_client)
            {
                send_garage_sensor_battery_level_to_client(p_bas_c_evt->params.battery_level);
            }
            else if (p_bas_c_evt->conn_handle == m_conn_handle_remote_control_client)
            {
                send_remote_control_battery_level_to_client (p_bas_c_evt->params.battery_level);
            }
            else if (p_bas_c_evt->conn_handle == m_conn_handle_playbulb_client)
            {
                send_playbulb_battery_level_to_client (p_bas_c_evt->params.battery_level);
            }
            break;

        case BLE_BAS_C_EVT_BATT_READ_RESP:
            NRF_LOG_INFO("Battery Level Read as %d %%", p_bas_c_evt->params.battery_level);
            //TODO: Need to store the battery level value
            break;

        default:
            break;
    }
}

/**@brief Handles events coming from the Thingy central module.
 */
static void thingy_c_evt_handler(thingy_client_t * p_thingy_c, thingy_client_evt_t * p_thingy_c_evt)
{
    switch (p_thingy_c_evt->evt_type)
    {
        case THINGY_CLIENT_EVT_DISCOVERY_COMPLETE:
        {
            if (m_conn_handle_thingy_client == BLE_CONN_HANDLE_INVALID)
            {
                ret_code_t err_code;

                m_conn_handle_thingy_client = p_thingy_c_evt->conn_handle;
                NRF_LOG_INFO("Thingy Environment Service discovered on conn_handle 0x%x", m_conn_handle_thingy_client);

                err_code = thingy_client_handles_assign(p_thingy_c,
                                                    m_conn_handle_thingy_client,
                                                    &p_thingy_c_evt->params.peer_db);
                APP_ERROR_CHECK(err_code);

                // Environment service discovered. Enable notification of Temperature and Humidity readings.
                err_code = thingy_client_temp_notify_enable(p_thingy_c);
                APP_ERROR_CHECK(err_code);

                err_code = thingy_client_humidity_notify_enable(p_thingy_c);
                APP_ERROR_CHECK(err_code);
            }
        } break; // THINGY_CLIENT_EVT_DISCOVERY_COMPLETE

        case THINGY_CLIENT_EVT_TEMP_NOTIFICATION:
        {
            ret_code_t err_code;

            NRF_LOG_INFO("Temperature = %d.%d Celsius", p_thingy_c_evt->params.temp.temp_integer, p_thingy_c_evt->params.temp.temp_decimal);

            // Send value to the Client device
            err_code = send_temperature_to_client(p_thingy_c_evt->params.temp.temp_integer);
            if ((err_code != NRF_SUCCESS) &&
                (err_code != NRF_ERROR_INVALID_STATE) &&
                (err_code != NRF_ERROR_RESOURCES) &&
                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                )
            {
                APP_ERROR_HANDLER(err_code);
            }
        } break; // THINGY_CLIENT_EVT_TEMP_NOTIFICATION

        case THINGY_CLIENT_EVT_HUMIDITY_NOTIFICATION:
        {
            ret_code_t err_code;

            NRF_LOG_INFO("Humidity percentage = %u%", p_thingy_c_evt->params.humidity.humidity);

            // Send value to the Client device
            err_code = send_humidity_to_client(p_thingy_c_evt->params.humidity.humidity);
            if ((err_code != NRF_SUCCESS) &&
                (err_code != NRF_ERROR_INVALID_STATE) &&
                (err_code != NRF_ERROR_RESOURCES) &&
                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                )
            {
                APP_ERROR_HANDLER(err_code);
            }
        } break; // THINGY_CLIENT_EVT_HUMIDITY_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Handles events coming from the Remote Control central module.
 */
static void remote_control_c_evt_handler(remote_control_client_t * p_remote_control_c, remote_control_client_evt_t * p_remote_control_c_evt)
{
    switch (p_remote_control_c_evt->evt_type)
    {
        case REMOTE_CONTROL_CLIENT_EVT_DISCOVERY_COMPLETE:
        {
            if (m_conn_handle_remote_control_client == BLE_CONN_HANDLE_INVALID)
            {
                ret_code_t err_code;

                m_conn_handle_remote_control_client = p_remote_control_c_evt->conn_handle;
                NRF_LOG_INFO("Remote Control Button Service discovered on conn_handle 0x%x", m_conn_handle_remote_control_client);

                err_code = remote_control_client_handles_assign(p_remote_control_c,
                                                    m_conn_handle_remote_control_client,
                                                    &p_remote_control_c_evt->params.peer_db);
                APP_ERROR_CHECK(err_code);

                // Button service discovered. Enable notification of ON and OFF Buttons readings.
                err_code = remote_control_client_on_button_notify_enable(p_remote_control_c);
                APP_ERROR_CHECK(err_code);

                err_code = remote_control_client_off_button_notify_enable(p_remote_control_c);
                APP_ERROR_CHECK(err_code);
            }
        } break; // REMOTE_CONTROL_CLIENT_EVT_DISCOVERY_COMPLETE

        case REMOTE_CONTROL_EVT_ON_BUTTON_PRESS_NOTIFICATION:
        {
            NRF_LOG_INFO("ON Button = %s", p_remote_control_c_evt->params.on_button.button_pressed == 1? "Pressed":"Released");

           //TODO
           // Send command to turn on Playbulb candle when ON Button is pressed
        } break; // REMOTE_CONTROL_EVT_ON_BUTTON_PRESS_NOTIFICATION

        case REMOTE_CONTROL_EVT_OFF_BUTTON_PRESS_NOTIFICATION:
        {
            NRF_LOG_INFO("OFF Button = %s", p_remote_control_c_evt->params.off_button.button_pressed == 1? "Pressed":"Released");

            //TODO
           // Send command to turn OFF Playbulb candle when OFF Button is pressed
        } break; // REMOTE_CONTROL_EVT_OFF_BUTTON_PRESS_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Handles events coming from the Thingy central module.
 */
static void playbulb_c_evt_handler(playbulb_client_t * p_playbulb_c, playbulb_client_evt_t * p_playbulb_c_evt)
{
    switch (p_playbulb_c_evt->evt_type)
    {
        case PLAYBULB_CLIENT_EVT_DISCOVERY_COMPLETE:
        {
            if (m_conn_handle_playbulb_client == BLE_CONN_HANDLE_INVALID)
            {
                ret_code_t err_code;

                m_conn_handle_playbulb_client = p_playbulb_c_evt->conn_handle;
                NRF_LOG_INFO("Playbulb Service discovered on conn_handle 0x%x", m_conn_handle_playbulb_client);

                err_code = playbulb_client_handles_assign(p_playbulb_c,
                                                    m_conn_handle_playbulb_client,
                                                    &p_playbulb_c_evt->params.peer_db);
                APP_ERROR_CHECK(err_code);
            }
        } break; // PLAYBULB_CLIENT_EVT_DISCOVERY_COMPLETE

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Central Clients initialization.
 */
void central_init(void)
{
    ret_code_t                   err_code;
    thingy_client_init_t         thingy_init_obj;
    remote_control_client_init_t remote_control_init_obj;
    playbulb_client_init_t       playbulb_init_obj;
    ble_bas_c_init_t             bas_c_init_obj;

    thingy_init_obj.evt_handler         = thingy_c_evt_handler;
    remote_control_init_obj.evt_handler = remote_control_c_evt_handler;
    playbulb_init_obj.evt_handler       = playbulb_c_evt_handler;
    bas_c_init_obj.evt_handler          = bas_c_evt_handler;

    // Initialize the different clients:

    // Initialize the Thingy Client
    err_code = thingy_client_init(&m_thingy_client, &thingy_init_obj);
    APP_ERROR_CHECK(err_code);

    // Initialize the Remote Control Client
    err_code = remote_control_client_init(&m_remote_control_client, &remote_control_init_obj);
    APP_ERROR_CHECK(err_code);

    // Initialize the Playbulb Candle Client
    err_code = playbulb_client_init(&m_playbulb_client, &playbulb_init_obj);
    APP_ERROR_CHECK(err_code);

    // Initialize the Battery Service client
    err_code = ble_bas_c_init(&m_bas_client, &bas_c_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief   Function for handling BLE events from central applications.
 *
 * @details This function parses scanning reports and initiates a connection to peripherals when a
 *          target UUID is found. It updates the status of LEDs used to report central applications
 *          activity.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
void on_ble_central_evt(ble_evt_t const * p_ble_evt)
{
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

    // Call the event handlers for each of the clients
    thingy_client_on_ble_evt(p_ble_evt, &m_thingy_client);
    remote_control_client_on_ble_evt(p_ble_evt, &m_remote_control_client);
    playbulb_client_on_ble_evt(p_ble_evt, &m_playbulb_client);
    //TODO: Add for each of the clients

    switch (p_ble_evt->header.evt_id)
    {
        // Upon connection, check which peripheral has connected (Thingy, Playbulb, or Remote Control), initiate DB
        // discovery, update LEDs status and resume scanning if necessary.
        case BLE_GAP_EVT_CONNECTED:
        {
            NRF_LOG_INFO("Central connected");
            // If no Thingy  is currently connected, try to find them on this peripheral.
            if (   (m_conn_handle_thingy_client  == BLE_CONN_HANDLE_INVALID)
                || (m_conn_handle_remote_control_client == BLE_CONN_HANDLE_INVALID)
                || (m_conn_handle_playbulb_client == BLE_CONN_HANDLE_INVALID))
            {
                NRF_LOG_INFO("Attempt to find Thingy, Playbulb or Remote Control on conn_handle 0x%x", p_gap_evt->conn_handle);

                err_code = ble_db_discovery_start(&m_db_discovery[0], p_gap_evt->conn_handle);
                if (err_code == NRF_ERROR_BUSY)
                {
                    err_code = ble_db_discovery_start(&m_db_discovery[1], p_gap_evt->conn_handle);
                    if (err_code == NRF_ERROR_BUSY)
                    {
                        err_code = ble_db_discovery_start(&m_db_discovery[2], p_gap_evt->conn_handle);
                        APP_ERROR_CHECK(err_code);
                    }
                }
                else
                {
                    APP_ERROR_CHECK(err_code);
                }
            }

            // Update LEDs status, and check if we should be looking for more peripherals to connect to.
            bsp_board_led_on(CENTRAL_CONNECTED_LED);
            if (ble_conn_state_n_centrals() == NRF_SDH_BLE_CENTRAL_LINK_COUNT)
            {
                bsp_board_led_off(CENTRAL_SCANNING_LED);
            }
            else
            {
                // Resume scanning.
                bsp_board_led_on(CENTRAL_SCANNING_LED);
                scan_start();
            }
        } break; // BLE_GAP_EVT_CONNECTED

        // Upon disconnection, reset the connection handle of the peer which disconnected,
        // update the LEDs status and start scanning again.
        case BLE_GAP_EVT_DISCONNECTED:
        {
            if (p_gap_evt->conn_handle == m_conn_handle_thingy_client)
            {
                NRF_LOG_INFO("Thingy client disconnected (reason: %d)",
                             p_gap_evt->params.disconnected.reason);

                m_conn_handle_thingy_client = BLE_CONN_HANDLE_INVALID;
            }
            if (p_gap_evt->conn_handle == m_conn_handle_remote_control_client)
            {
                NRF_LOG_INFO("Remote Control client disconnected (reason: %d)",
                             p_gap_evt->params.disconnected.reason);

                m_conn_handle_remote_control_client = BLE_CONN_HANDLE_INVALID;
            }
            if (p_gap_evt->conn_handle == m_conn_handle_playbulb_client)
            {
                NRF_LOG_INFO("Playbulb client disconnected (reason: %d)",
                             p_gap_evt->params.disconnected.reason);

                m_conn_handle_playbulb_client = BLE_CONN_HANDLE_INVALID;
            }

            if ((m_conn_handle_thingy_client  == BLE_CONN_HANDLE_INVALID) ||
                (m_conn_handle_remote_control_client == BLE_CONN_HANDLE_INVALID) ||
                (m_conn_handle_playbulb_client == BLE_CONN_HANDLE_INVALID))
            {
                // Start scanning
                scan_start();

                // Update LEDs status.
                bsp_board_led_on(CENTRAL_SCANNING_LED);
            }

            if (ble_conn_state_n_centrals() == 0)
            {
                bsp_board_led_off(CENTRAL_CONNECTED_LED);
            }
        } break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GAP_EVT_ADV_REPORT:
        {
            int8_t index;

            // Find the devices of interest: Thingy:52, Playbulb Candle, Remote Control
            if ((index = find_adv_name(&p_gap_evt->params.adv_report, m_target_periph_names, NUMBER_OF_TARGET_PERIPHERALS)) >= 0)
            {
                NRF_LOG_INFO("We found a device named: %s", m_target_periph_names[index]);

                // Initiate connection.
                err_code = sd_ble_gap_connect(&p_gap_evt->params.adv_report.peer_addr,
                                              &m_scan_params,
                                              &m_connection_param,
                                              APP_BLE_CONN_CFG_TAG);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_INFO("Connection Request Failed, reason %d", err_code);
                }
                else
                {
                    NRF_LOG_INFO("Connection Request SUCCEEDED");
                }
            }
        } break; // BLE_GAP_ADV_REPORT

        case BLE_GAP_EVT_TIMEOUT:
        {
            // We have not specified a timeout for scanning, so only connection attempts can timeout.
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                NRF_LOG_INFO("Connection Request timed out.");
            }
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            // Accept parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                        &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
        } break;

#ifndef S140
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
#endif

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}
