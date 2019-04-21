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

#ifndef REMOTE_CONTROL_C_H
#define REMOTE_CONTROL_C_H

#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"
#include "sdk_config.h"
#include "nrf_sdh_ble.h"

// <o> REMOTE_CONTROL_C_BLE_OBSERVER_PRIO
// <i> Priority with which BLE events are dispatched to the Remote Control Client.

#ifndef REMOTE_CONTROL_C_BLE_OBSERVER_PRIO
#define REMOTE_CONTROL_C_BLE_OBSERVER_PRIO 2
#endif

/**@brief   Macro for defining a remote_control_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define REMOTE_CONTROL_C_DEF(_name)                                                                        \
static remote_control_clients_t _name;                                                                           \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     REMOTE_CONTROL_C_BLE_OBSERVER_PRIO,                                                   \
                     remote_control_client_on_ble_evt, &_name)


/**@brief Remote Control Client event type. */
typedef enum
{
    REMOTE_CONTROL_CLIENT_EVT_DISCOVERY_COMPLETE = 1,  /**< Event indicating that the Thingy Service has been discovered at the peer. */
    REMOTE_CONTROL_EVT_ON_BUTTON_PRESS_NOTIFICATION,       /**< Event indicating that a notification of the Garage Sensor Temperature characteristic has been received from the peer. */
    REMOTE_CONTROL_EVT_OFF_BUTTON_PRESS_NOTIFICATION    /**< Event indicating that a notification of the Garage Sensor Humidity characteristic has been received from the peer. */
} remote_control_client_evt_type_t;


/**@brief Structure containing the temperature measurement received from the peer. */
typedef struct
{
    uint8_t button_pressed;            /**< Button press - 1 for pressed, 0 for released */
} button_press_t;

/**@brief Structure containing the handles related to the Remote Control found on the peer. */
typedef struct
{
    uint16_t on_button_cccd_handle;   /**< Handle of the CCCD of the ON Button characteristic. */
    uint16_t on_button_handle;        /**< Handle of the ON Button characteristic characteristic as provided by the SoftDevice. */
    uint16_t off_button_cccd_handle;  /**< Handle of the CCCD of the OFF Button characteristic. */
    uint16_t off_button_handle;       /**< Handle of the OFF Button characteristic as provided by the SoftDevice. */
} remote_control_db_t;

/**@brief Remote Control Event structure. */
typedef struct
{
    remote_control_client_evt_type_t evt_type;  /**< Type of the event. */
    uint16_t                 conn_handle;       /**< Connection handle on which the Remote Control was discovered on the peer device..*/
    union
    {
        remote_control_db_t  peer_db;        /**< Button Service related handles found on the peer device.. This will be filled if the evt_type is @ref REMOTE_CONTROL_CLIENT_EVT_DISCOVERY_COMPLETE.*/
        button_press_t on_button;            /**< ON Button event received. This will be filled if the evt_type is @ref REMOTE_CONTROL_CLIENT_EVT_ON_BUTTON_NOTIFICATION. */
        button_press_t off_button;           /**< OFF Button event received. This will be filled if the evt_type is @ref REMOTE_CONTROL_CLIENT_EVT_OFF_BUTTON_NOTIFICATION. */
    } params;
} remote_control_client_evt_t;


typedef struct remote_control_client_s remote_control_client_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module in order to receive events.
 */
typedef void (* remote_control_client_evt_handler_t) (remote_control_client_t * p_remote_control_client, remote_control_client_evt_t * p_evt);


/**@brief Remote Control Client structure.
 */
struct remote_control_client_s
{
    uint16_t                    conn_handle;            /**< Connection handle as provided by the SoftDevice. */
    remote_control_db_t         peer_remote_control_db; /**< Handles related to Remote Control Button Service on the peer*/
    ble_uuid_t                  service_uuid;           /**< Remote Control Button Service UUID */
    remote_control_client_evt_handler_t evt_handler;    /**< Application event handler to be called when there is an event related to the Remote Control Button service. */
};

/**@brief Remote Control Client initialization structure.
 */
typedef struct
{
    remote_control_client_evt_handler_t evt_handler;  /**< Event handler to be called by the Remote Control Client module whenever there is an event related to the Remote Control Service. */
} remote_control_client_init_t;



/**@brief     Function for initializing the remote_control client module.
 *
 * @details   This function will register with the DB Discovery module. There it
 *            registers for the Remote Control Button Service. Doing so will make the DB Discovery
 *            module look for the presence of a Remote Control Button Service instance at the peer when a
 *            discovery is started.
 *
 * @param[in] p_remote_control_client      Pointer to the remote_control client structure.
 * @param[in] p_remote_control_client_init Pointer to the remote_control initialization structure containing the
 *                                 initialization information.
 *
 * @retval    NRF_SUCCESS On successful initialization. Otherwise an error code. This function
 *                        propagates the error code returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t remote_control_client_init(remote_control_client_t * p_remote_control_client, remote_control_client_init_t * p_remote_control_client_init);


/**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function will handle the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the Thingy Client module, then it uses it to update
 *            interval variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_evt     Pointer to the BLE event.
 * @param[in] p_context     Pointer to the remote_control client structure.
 */
void remote_control_client_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


/**@brief   Function for requesting the peer to start sending notification of Temperature
 *          Measurement.
 *
 * @details This function will enable to notification of the Temperature Measurement at the peer
 *          by writing to the CCCD of the Temperature Measurement Characteristic.
 *
 * @param   p_remote_control_client Pointer to the remote_control client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice has been requested to write to the CCCD of the peer.
 *                      Otherwise, an error code. This function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t remote_control_client_on_button_notify_enable(remote_control_client_t * p_remote_control_client);

/**@brief   Function for requesting the peer to start sending notification of Humidity
 *          Measurement.
 *
 * @details This function will enable to notification of the Humidity Measurement at the peer
 *          by writing to the CCCD of the Button Off Characteristic.
 *
 * @param   p_remote_control_client Pointer to the remote_control client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice has been requested to write to the CCCD of the peer.
 *                      Otherwise, an error code. This function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t remote_control_client_off_button_notify_enable(remote_control_client_t * p_remote_control_client);

/**@brief   Function for requesting the peer to start sending indications of Button
 *          Off events.
 *
 * @details This function will enable indication of the Button Off events at the peer
 *          by writing to the CCCD of the Button Off Characteristic.
 *
 * @param   p_remote_control_client Pointer to the remote_control client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice has been requested to write to the CCCD of the peer.
 *                      Otherwise, an error code. This function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t remote_control_client_off_button_indicate_enable(remote_control_client_t * p_remote_control_client);

/**@brief     Function for handling events from the database discovery module.
 *
 * @details   Call this function when getting a callback event from the DB discovery module.
 *            This function will handle an event from the database discovery module, and determine
 *            if it relates to the discovery of Environment service at the peer. If so, it will
 *            call the application's event handler indicating that the Temperature service has been
 *            discovered at the peer. It also populates the event with the service related
 *            information before providing it to the application.
 *
 * @param[in] p_remote_control_client Pointer to the heart rate client structure instance to associate.
 * @param[in] p_evt Pointer to the event received from the database discovery module.
 *
 */
void remote_control_on_db_disc_evt(remote_control_client_t * p_remote_control_client, const ble_db_discovery_evt_t * p_evt);


/**@brief     Function for assigning handles to this instance of remote_control_client.
 *
 * @details   Call this function when a link has been established with a peer to
 *            associate this link to this instance of the module. This makes it
 *            possible to handle several link and associate each link to a particular
 *            instance of this module.The connection handle and attribute handles will be
 *            provided from the discovery event @ref BLE_HRS_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_remote_control_client        Pointer to the heart rate client structure instance to associate.
 * @param[in] conn_handle            Connection handle to associated with the given Thingy Client Instance.
 * @param[in] p_peer_remote_control_handles  Attribute handles for the Thingy server you want this Thingy client to
 *                                   interact with.
 */
uint32_t remote_control_client_handles_assign(remote_control_client_t * p_remote_control_client,
                                  uint16_t conn_handle,
                                  const remote_control_db_t * p_peer_remote_control_handles);


#endif // REMOTE_CONTROL_C_H
