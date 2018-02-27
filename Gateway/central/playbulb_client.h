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

 //TODO
/**@file
 *
 * @defgroup ble_hrs_c Playbulb Client
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Playbulb Service Client module.
 *
 * @details  This module contains the APIs and types exposed by the Heart Rate Service Client
 *           module. These APIs and types can be used by the application to perform discovery of
 *           Heart Rate Service at the peer and interact with it.
 *
 * @warning  Currently this module only has support for Heart Rate Measurement characteristic. This
 *           means that it will be able to enable notification of the characteristic at the peer and
 *           be able to receive Heart Rate Measurement notifications from the peer. It does not
 *           support the Body Sensor Location and the Heart Rate Control Point characteristics.
 *           When a Heart Rate Measurement is received, this module will decode only the
 *           Heart Rate Measurement Value (both 8 bit and 16 bit) field from it and provide it to
 *           the application.
 *
 * @note    The application must register this module as BLE event observer using the
 *          NRF_SDH_BLE_OBSERVER macro. Example:
 *          @code
 *              ble_hrs_c_t instance;
 *              NRF_SDH_BLE_OBSERVER(anything, BLE_HRS_C_BLE_OBSERVER_PRIO,
 *                                   ble_hrs_c_on_ble_evt, &instance);
 *          @endcode
 */

#ifndef PLAYBULB_C_H
#define PLAYBULB_C_H

#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"
#include "sdk_config.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

// <o> PLAYBULB_C_BLE_OBSERVER_PRIO
// <i> Priority with which BLE events are dispatched to the Playbulb Client.

#ifndef PLAYBULB_C_BLE_OBSERVER_PRIO
#define PLAYBULB_C_BLE_OBSERVER_PRIO 2
#endif

/**@brief   Macro for defining a playbulb_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define PLAYBULB_C_DEF(_name)                                                                        \
static playbulb_clients_t _name;                                                                           \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     PLAYBULB_C_BLE_OBSERVER_PRIO,                                                   \
                     playbulb_client_on_ble_evt, &_name)


/**@brief Playbulb Client event type. */
typedef enum
{
    PLAYBULB_CLIENT_EVT_DISCOVERY_COMPLETE = 1  /**< Event indicating that the Playbulb Service has been discovered at the peer. */
} playbulb_client_evt_type_t;

/**@brief Structure containing the handles related to the Playbulb found on the peer. */
typedef struct
{
    uint16_t color_setting_handle;        /**< Handle of the ON Button characteristic characteristic as provided by the SoftDevice. */
} playbulb_db_t;

/**@brief Playbulb Event structure. */
typedef struct
{
    playbulb_client_evt_type_t evt_type;  /**< Type of the event. */
    uint16_t                 conn_handle;       /**< Connection handle on which the Playbulb was discovered on the peer device..*/
    union
    {
        playbulb_db_t  peer_db;        /**< Button Service related handles found on the peer device.. This will be filled if the evt_type is @ref PLAYBULB_CLIENT_EVT_DISCOVERY_COMPLETE.*/
    } params;
} playbulb_client_evt_t;


typedef struct playbulb_client_s playbulb_client_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module in order to receive events.
 */
typedef void (* playbulb_client_evt_handler_t) (playbulb_client_t * p_playbulb_client, playbulb_client_evt_t * p_evt);


/**@brief Playbulb Client structure.
 */
struct playbulb_client_s
{
    uint16_t                    conn_handle;        /**< Connection handle as provided by the SoftDevice. */
    playbulb_db_t               peer_playbulb_db;   /**< Handles related to Playbulb Button Service on the peer*/
    ble_uuid_t                  service_uuid;       /**< Playbulb Button Service UUID */
    playbulb_client_evt_handler_t evt_handler;      /**< Application event handler to be called when there is an event related to the Playbulb Button service. */
};

/**@brief Playbulb Client initialization structure.
 */
typedef struct
{
    playbulb_client_evt_handler_t evt_handler;  /**< Event handler to be called by the Playbulb Client module whenever there is an event related to the Playbulb Service. */
} playbulb_client_init_t;



/**@brief     Function for initializing the playbulb client module.
 *
 * @details   This function will register with the DB Discovery module. There it
 *            registers for the Playbulb Button Service. Doing so will make the DB Discovery
 *            module look for the presence of a Playbulb Button Service instance at the peer when a
 *            discovery is started.
 *
 * @param[in] p_playbulb_client      Pointer to the playbulb client structure.
 * @param[in] p_playbulb_client_init Pointer to the playbulb initialization structure containing the
 *                                 initialization information.
 *
 * @retval    NRF_SUCCESS On successful initialization. Otherwise an error code. This function
 *                        propagates the error code returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t playbulb_client_init(playbulb_client_t * p_playbulb_client, playbulb_client_init_t * p_playbulb_client_init);


/**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function will handle the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the Playbulb Client module, then it uses it to update
 *            interval variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_evt     Pointer to the BLE event.
 * @param[in] p_context     Pointer to the playbulb client structure.
 */
void playbulb_client_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief     Function for handling events from the database discovery module.
 *
 * @details   Call this function when getting a callback event from the DB discovery module.
 *            This function will handle an event from the database discovery module, and determine
 *            if it relates to the discovery of Environment service at the peer. If so, it will
 *            call the application's event handler indicating that the Temperature service has been
 *            discovered at the peer. It also populates the event with the service related
 *            information before providing it to the application.
 *
 * @param[in] p_playbulb_client Pointer to the heart rate client structure instance to associate.
 * @param[in] p_evt Pointer to the event received from the database discovery module.
 *
 */
void playbulb_on_db_disc_evt(playbulb_client_t * p_playbulb_client, const ble_db_discovery_evt_t * p_evt);


/**@brief     Function for assigning handles to this instance of playbulb_client.
 *
 * @details   Call this function when a link has been established with a peer to
 *            associate this link to this instance of the module. This makes it
 *            possible to handle several link and associate each link to a particular
 *            instance of this module.The connection handle and attribute handles will be
 *            provided from the discovery event @ref BLE_HRS_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_playbulb_client        Pointer to the heart rate client structure instance to associate.
 * @param[in] conn_handle            Connection handle to associated with the given Playbulb Client Instance.
 * @param[in] p_peer_playbulb_handles  Attribute handles for the Playbulb server you want this Playbulb client to
 *                                   interact with.
 */
uint32_t playbulb_client_handles_assign(playbulb_client_t * p_playbulb_client,
                                  uint16_t conn_handle,
                                  const playbulb_db_t * p_peer_playbulb_handles);


#ifdef __cplusplus
}
#endif

#endif // PLAYBULB_C_H