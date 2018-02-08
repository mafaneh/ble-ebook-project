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

#include "central.h"

#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define CENTRAL_SCANNING_LED            BSP_BOARD_LED_0
#define CENTRAL_CONNECTED_LED           BSP_BOARD_LED_1

#define SCAN_INTERVAL                   0x00A0                                      /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                     0x0050                                      /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_TIMEOUT                    0

#define MIN_CONNECTION_INTERVAL         (uint16_t) MSEC_TO_UNITS(7.5, UNIT_1_25_MS) /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL         (uint16_t) MSEC_TO_UNITS(30, UNIT_1_25_MS)  /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY                   0                                           /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT             (uint16_t) MSEC_TO_UNITS(4000, UNIT_10_MS)  /**< Determines supervision time-out in units of 10 milliseconds. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

/**@brief   Priority of the application BLE event handler.
 * @note    You shouldn't need to modify this value.
 */
#define APP_BLE_OBSERVER_PRIO           3

BLE_DB_DISCOVERY_ARRAY_DEF(m_db_discovery, 2);                      /**< Database discovery module instances. */

// TODO: Use Thingy:52, and Playbulb Candle names(?)
/**@brief names which the central applications will scan for, and which will be advertised by the peripherals.
 *  if these are set to empty strings, the UUIDs defined below will be used
 */
static char const m_target_periph_name[] = "Thingy";

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
    //ble_hrs_on_db_disc_evt(&m_hrs_c, p_evt);
    //ble_rscs_on_db_disc_evt(&m_rscs_c, p_evt);
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

/**@brief Garage Sensor (Thingy:52) Client initialization.
 */
 //TODO
//static void thingy_client_init(void)
//{
//    ret_code_t       err_code;
//    thingy_client_init_t thingy_client_init_obj;
//
//    thingy_client_init_obj.evt_handler = hrs_c_evt_handler;
//
//    err_code = ble_hrs_c_init(&m_hrs_c, &thingy_client_init_obj);
//    APP_ERROR_CHECK(err_code);
//}


/**@brief Playbulb Central connection initialization.
 */


/**@brief Remote Control Central connection initialization.
 */


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

    switch (p_ble_evt->header.evt_id)
    {
        // Upon connection, check which peripheral has connected (HR or RSC), initiate DB
        // discovery, update LEDs status and resume scanning if necessary.
        case BLE_GAP_EVT_CONNECTED:
        {
            NRF_LOG_INFO("Central connected");
            //TODO
            // If no Heart Rate sensor or RSC sensor is currently connected, try to find them on this peripheral.
//            if (   (m_conn_handle_hrs_c  == BLE_CONN_HANDLE_INVALID)
//                || (m_conn_handle_rscs_c == BLE_CONN_HANDLE_INVALID))
//            {
//                NRF_LOG_INFO("Attempt to find HRS or RSC on conn_handle 0x%x", p_gap_evt->conn_handle);
//
//                err_code = ble_db_discovery_start(&m_db_discovery[0], p_gap_evt->conn_handle);
//                if (err_code == NRF_ERROR_BUSY)
//                {
//                    err_code = ble_db_discovery_start(&m_db_discovery[1], p_gap_evt->conn_handle);
//                    APP_ERROR_CHECK(err_code);
//                }
//                else
//                {
//                    APP_ERROR_CHECK(err_code);
//                }
//            }

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
            //TODO
//            if (p_gap_evt->conn_handle == m_conn_handle_hrs_c)
//            {
//                NRF_LOG_INFO("HRS central disconnected (reason: %d)",
//                             p_gap_evt->params.disconnected.reason);
//
//                m_conn_handle_hrs_c = BLE_CONN_HANDLE_INVALID;
//            }
//            if (p_gap_evt->conn_handle == m_conn_handle_rscs_c)
//            {
//                NRF_LOG_INFO("RSC central disconnected (reason: %d)",
//                             p_gap_evt->params.disconnected.reason);
//
//                m_conn_handle_rscs_c = BLE_CONN_HANDLE_INVALID;
//            }
//
//            if (   (m_conn_handle_rscs_c == BLE_CONN_HANDLE_INVALID)
//                || (m_conn_handle_hrs_c  == BLE_CONN_HANDLE_INVALID))
//            {
//                // Start scanning
//                scan_start();
//
//                // Update LEDs status.
//                bsp_board_led_on(CENTRAL_SCANNING_LED);
//            }
//
//            if (ble_conn_state_n_centrals() == 0)
//            {
//                bsp_board_led_off(CENTRAL_CONNECTED_LED);
//            }
        } break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GAP_EVT_ADV_REPORT:
        {
            // Find the devices of interest: Thingy:52, Playbulb Candle, Remote Control
            if (strlen(m_target_periph_name) != 0)
            {
                if (find_adv_name(&p_gap_evt->params.adv_report, m_target_periph_name))
                {
                    NRF_LOG_INFO("We found a device named: %s", m_target_periph_name);

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
            }
            else
            {
                // We do not want to connect to two peripherals offering the same service, so when
                // a UUID is matched, we check that we are not already connected to a peer which
                // offers the same service.
                //TODO
//                if (   (find_adv_uuid(&p_gap_evt->params.adv_report, BLE_UUID_HEART_RATE_SERVICE)
//                        && (m_conn_handle_hrs_c == BLE_CONN_HANDLE_INVALID))
//                    || (find_adv_uuid(&p_gap_evt->params.adv_report, BLE_UUID_RUNNING_SPEED_AND_CADENCE)
//                        && (m_conn_handle_rscs_c == BLE_CONN_HANDLE_INVALID)))
//                {
//                    // Initiate connection.
//                    err_code = sd_ble_gap_connect(&p_gap_evt->params.adv_report.peer_addr,
//                                                  &m_scan_params,
//                                                  &m_connection_param,
//                                                  APP_BLE_CONN_CFG_TAG);
//                    if (err_code != NRF_SUCCESS)
//                    {
//                        NRF_LOG_WARNING("Connection Request Failed, reason %d", err_code);
//                    }
//                }
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
