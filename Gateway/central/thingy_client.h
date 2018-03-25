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

#ifndef THINGY_C_H__
#define THINGY_C_H__

#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"
#include "sdk_config.h"
#include "nrf_sdh_ble.h"

// <o> THINGY_C_BLE_OBSERVER_PRIO
// <i> Priority with which BLE events are dispatched to the Thingy Client.

#ifndef THINGY_C_BLE_OBSERVER_PRIO
#define THINGY_C_BLE_OBSERVER_PRIO 2
#endif

/**@brief   Macro for defining a thingy_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define THINGY_C_DEF(_name)                                                                        \
static thingy_clients_t _name;                                                                           \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     THINGY_C_BLE_OBSERVER_PRIO,                                                   \
                     thingy_client_on_ble_evt, &_name)


/**@brief Thingy Client event type. */
typedef enum
{
    THINGY_CLIENT_EVT_DISCOVERY_COMPLETE = 1,  /**< Event indicating that the Thingy Service has been discovered at the peer. */
    THINGY_CLIENT_EVT_TEMP_NOTIFICATION,       /**< Event indicating that a notification of the Garage Sensor Temperature characteristic has been received from the peer. */
    THINGY_CLIENT_EVT_HUMIDITY_NOTIFICATION    /**< Event indicating that a notification of the Garage Sensor Humidity characteristic has been received from the peer. */
} thingy_client_evt_type_t;


/**@brief Structure containing the temperature measurement received from the peer. */
typedef struct
{
    int8_t  temp_integer;                                   /**< Temperature (in Celsius) - Integer part. */
    uint8_t temp_decimal;                                   /**< Temperature (in Celsius) - Decimal part. */
} thingy_temp_t;

/**@brief Structure containing the humidity measurement received from the peer. */
typedef struct
{
    uint8_t  humidity;                                   /**< Humidity Value. */
} thingy_humidity_t;

/**@brief Structure containing the handles related to the Thingy found on the peer. */
typedef struct
{
    uint16_t temp_cccd_handle;      /**< Handle of the CCCD of the Temperature Measurement characteristic. */
    uint16_t temp_handle;           /**< Handle of the Temperature Measurement characteristic as provided by the SoftDevice. */
    uint16_t humidity_cccd_handle;   /**< Handle of the CCCD of the Humidity Measurement characteristic. */
    uint16_t humidity_handle;       /**< Handle of the Humidity Measurement characteristic as provided by the SoftDevice. */
} thingy_db_t;

/**@brief Thingy Event structure. */
typedef struct
{
    thingy_client_evt_type_t evt_type;    /**< Type of the event. */
    uint16_t                 conn_handle; /**< Connection handle on which the Thingy was discovered on the peer device..*/
    union
    {
        thingy_db_t  peer_db;         /**< Thingy Environment Service related handles found on the peer device.. This will be filled if the evt_type is @ref THINGY_CLIENT_EVT_DISCOVERY_COMPLETE.*/
        thingy_temp_t temp;           /**< Temperature measurement received. This will be filled if the evt_type is @ref THINGY_CLIENT_EVT_TEMP_NOTIFICATION. */
        thingy_humidity_t humidity;   /**< Humidity measurement received. This will be filled if the evt_type is @ref THINGY_CLIENT_EVT_HUMIDITY_NOTIFICATION. */
    } params;
} thingy_client_evt_t;


typedef struct thingy_client_s thingy_client_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module in order to receive events.
 */
typedef void (* thingy_client_evt_handler_t) (thingy_client_t * p_thingy_client, thingy_client_evt_t * p_evt);


/**@brief Thingy Client structure.
 */
struct thingy_client_s
{
    uint16_t                    conn_handle;      /**< Connection handle as provided by the SoftDevice. */
    thingy_db_t                 peer_thingy_db;   /**< Handles related to Thingy Environment Service on the peer*/
    ble_uuid_t                  service_uuid;     /**< Thingy Environment Service UUID */
    thingy_client_evt_handler_t evt_handler;      /**< Application event handler to be called when there is an event related to the Thingy Environment service. */
};

/**@brief Thingy Client initialization structure.
 */
typedef struct
{
    thingy_client_evt_handler_t evt_handler;  /**< Event handler to be called by the Thingy Client module whenever there is an event related to the Thingy Service. */
} thingy_client_init_t;



/**@brief     Function for initializing the thingy client module.
 *
 * @details   This function will register with the DB Discovery module. There it
 *            registers for the Thingy Environment Service. Doing so will make the DB Discovery
 *            module look for the presence of a Thingy Environment Service instance at the peer when a
 *            discovery is started.
 *
 * @param[in] p_thingy_client      Pointer to the thingy client structure.
 * @param[in] p_thingy_client_init Pointer to the thingy initialization structure containing the
 *                                 initialization information.
 *
 * @retval    NRF_SUCCESS On successful initialization. Otherwise an error code. This function
 *                        propagates the error code returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t thingy_client_init(thingy_client_t * p_thingy_client, thingy_client_init_t * p_thingy_client_init);


/**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function will handle the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the Thingy Client module, then it uses it to update
 *            interval variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_evt     Pointer to the BLE event.
 * @param[in] p_context     Pointer to the thingy client structure.
 */
void thingy_client_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


/**@brief   Function for requesting the peer to start sending notification of Temperature
 *          Measurement.
 *
 * @details This function will enable to notification of the Temperature Measurement at the peer
 *          by writing to the CCCD of the Temperature Measurement Characteristic.
 *
 * @param   p_thingy_client Pointer to the thingy client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice has been requested to write to the CCCD of the peer.
 *                      Otherwise, an error code. This function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t thingy_client_temp_notify_enable(thingy_client_t * p_thingy_client);

/**@brief   Function for requesting the peer to start sending notification of Humidity
 *          Measurement.
 *
 * @details This function will enable to notification of the Humidity Measurement at the peer
 *          by writing to the CCCD of the Humidity Measurement Characteristic.
 *
 * @param   p_thingy_client Pointer to the thingy client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice has been requested to write to the CCCD of the peer.
 *                      Otherwise, an error code. This function propagates the error code returned
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t thingy_client_humidity_notify_enable(thingy_client_t * p_thingy_client);

/**@brief     Function for handling events from the database discovery module.
 *
 * @details   Call this function when getting a callback event from the DB discovery module.
 *            This function will handle an event from the database discovery module, and determine
 *            if it relates to the discovery of Environment service at the peer. If so, it will
 *            call the application's event handler indicating that the Temperature service has been
 *            discovered at the peer. It also populates the event with the service related
 *            information before providing it to the application.
 *
 * @param[in] p_thingy_client Pointer to the heart rate client structure instance to associate.
 * @param[in] p_evt Pointer to the event received from the database discovery module.
 *
 */
void thingy_on_db_disc_evt(thingy_client_t * p_thingy_client, const ble_db_discovery_evt_t * p_evt);


/**@brief     Function for assigning handles to this instance of thingy_client.
 *
 * @details   Call this function when a link has been established with a peer to
 *            associate this link to this instance of the module. This makes it
 *            possible to handle several link and associate each link to a particular
 *            instance of this module.The connection handle and attribute handles will be
 *            provided from the discovery event @ref BLE_HRS_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_thingy_client        Pointer to the heart rate client structure instance to associate.
 * @param[in] conn_handle            Connection handle to associated with the given Thingy Client Instance.
 * @param[in] p_peer_thingy_handles  Attribute handles for the Thingy server you want this Thingy client to
 *                                   interact with.
 */
uint32_t thingy_client_handles_assign(thingy_client_t * p_thingy_client,
                                  uint16_t conn_handle,
                                  const thingy_db_t * p_peer_thingy_handles);


#endif // THINGY_C_H__