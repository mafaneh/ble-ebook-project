/**
 * Copyright (c) 2018, Infineon Technologies AG
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef OPTIGA_COMMAND_LIBRARY_H__
#define OPTIGA_COMMAND_LIBRARY_H__

#include "ifx_i2c_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ifx_optiga_api Infineon OPTIGA(TM) Trust X Command Library
 * @{
 * @addtogroup ifx_optiga
 *
 * @brief Module for the application-level API to use Infineon OPTIGA(TM) Trust X.
 *
 * @note
 *   This module implements a preliminary high-level API to access a *subset* of Infineon OPTIGA(TM) Trust X features.
 *   This high-level API is an early evaluation version and subject to *major changes* in future releases.
 *
 * For more information about the Infineon OPTIGA(TM) Trust X please visit:
 * https://www.infineon.com/cms/en/product/security-smart-card-solutions/optiga-embedded-security-solutions/optiga-trust/optiga-trust-x-sls-32aia/
 *
 */

/**
 * @brief Initialize the Trust X command library and I2C protocol stack.
 *
 * This function initializes the Trust X high-level API and the Infineon I2C Protocol.
 *
 * @retval  IFX_I2C_STACK_SUCCESS  If the function succeeded.
 * @retval  IFX_I2C_STACK_ERROR    If the operation failed.
 */
uint16_t optiga_init(void);

/**
 * @brief Open the Trust X application.
 *
 * This function initializes the Trust X application by
 * sending the 'open application' command to the device.
 *
 * @retval  IFX_I2C_STACK_SUCCESS  If the function succeeded.
 * @retval  IFX_I2C_STACK_ERROR    If the operation failed.
 */
uint16_t optiga_open_application(void);

/**
 * @brief Get a random number.
 *
 * The function retrieves a cryptographic-quality random number
 * from the Trust X device. This function can be used as entropy
 * source for various security schemes.
 *
 * @param[in]  length       Length of the random number (range 8 to 256).
 * @param[out] p_random     Buffer to store the data.
 *
 * @retval  IFX_I2C_STACK_SUCCESS  If the function succeeded.
 * @retval  IFX_I2C_STACK_ERROR    If the operation failed.
 */
uint16_t optiga_get_random(uint8_t* p_random, uint16_t length);

/**
 * List of OPTIGA Object IDs (OIDs).
 */
typedef enum OptigaOID
{
    // Common Objects
    OID_GLOBAL_LIFE_CYCLE_STATUS            = 0xE0C0,//!< OID_GLOBAL_LIFE_CYCLE_STATUS
    OID_GLOBAL_SECURITY_STATUS              = 0xE0C1,//!< OID_GLOBAL_SECURITY_STATUS
    OID_COPROCESSOR_UID                     = 0xE0C2,//!< OID_COPROCESSOR_UID
    OID_SLEEP_MODE_ACTIVATION_DELAY         = 0xE0C3,//!< OID_SLEEP_MODE_ACTIVATION_DELAY
    OID_CURRENT_LIMITATION                  = 0xE0C4,//!< OID_CURRENT_LIMITATION
    OID_SECURITY_EVENT_COUNTER              = 0xE0C5,//!< OID_SECURITY_EVENT_COUNTER
    OID_MAX_COMM_BUFFER_SIZE                = 0xE0C6,//!< OID_MAX_COMM_BUFFER_SIZE

    // Device Certificates
    OID_INFINEON_CERTIFICATE                = 0xE0E0,//!< OID_INFINEON_CERTIFICATE
    OID_PROJECT_CERTIFICATE_1               = 0xE0E1,//!< OID_PROJECT_CERTIFICATE_1
    OID_PROJECT_CERTIFICATE_2               = 0xE0E2,//!< OID_PROJECT_CERTIFICATE_2
    OID_PROJECT_CERTIFICATE_3               = 0xE0E3,//!< OID_PROJECT_CERTIFICATE_3

    // Root CA Certificates (TLS (1) and Platform Integrity (8))
    OID_ROOT_CA_CERTIFICATE_1               = 0xE0E8,//!< OID_ROOT_CA_CERTIFICATE_1
    OID_ROOT_CA_CERTIFICATE_8               = 0xE0EF,//!< OID_ROOT_CA_CERTIFICATE_8

    // Device Private Keys
    OID_DEVICE_PRIVATE_KEY_1                = 0xE0F0,//!< OID_DEVICE_PRIVATE_KEY_1
    OID_DEVICE_PRIVATE_KEY_2                = 0xE0F1,//!< OID_DEVICE_PRIVATE_KEY_2
    OID_DEVICE_PRIVATE_KEY_3                = 0xE0F2,//!< OID_DEVICE_PRIVATE_KEY_3
    OID_DEVICE_PRIVATE_KEY_4                = 0xE0F3,//!< OID_DEVICE_PRIVATE_KEY_4

    // Session Contexts for Toolbox/DTLS
    OID_SESSION_CONTEXT_1                   = 0xE100,//!< OID_SESSION_CONTEXT_1
    OID_SESSION_CONTEXT_2                   = 0xE101,//!< OID_SESSION_CONTEXT_2
    OID_SESSION_CONTEXT_3                   = 0xE102,//!< OID_SESSION_CONTEXT_3
    OID_SESSION_CONTEXT_4                   = 0xE103,//!< OID_SESSION_CONTEXT_4

    // Application Objects
    OID_APP_LIFE_CYCLE_STATUS               = 0xF1C0,//!< OID_APP_LIFE_CYCLE_STATUS
    OID_APP_SECURITY_STATUS                 = 0xF1C1,//!< OID_APP_SECURITY_STATUS
    OID_APP_ERROR_CODES                     = 0xF1C2,//!< OID_APP_ERROR_CODES

    // Application Data Objects Type 1 (100 byte size)
    OID_APP_ARBITRARY_DATA_OBJECT_T1_1      = 0xF1D0,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_1
    OID_APP_ARBITRARY_DATA_OBJECT_T1_2      = 0xF1D1,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_2
    OID_APP_ARBITRARY_DATA_OBJECT_T1_3      = 0xF1D2,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_3
    OID_APP_ARBITRARY_DATA_OBJECT_T1_4      = 0xF1D3,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_4
    OID_APP_ARBITRARY_DATA_OBJECT_T1_5      = 0xF1D4,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_5
    OID_APP_ARBITRARY_DATA_OBJECT_T1_6      = 0xF1D5,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_6
    OID_APP_ARBITRARY_DATA_OBJECT_T1_7      = 0xF1D6,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_7
    OID_APP_ARBITRARY_DATA_OBJECT_T1_8      = 0xF1D7,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_8
    OID_APP_ARBITRARY_DATA_OBJECT_T1_9      = 0xF1D8,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_9
    OID_APP_ARBITRARY_DATA_OBJECT_T1_10     = 0xF1D9,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_10
    OID_APP_ARBITRARY_DATA_OBJECT_T1_11     = 0xF1DA,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_11
    OID_APP_ARBITRARY_DATA_OBJECT_T1_12     = 0xF1DB,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_12
    OID_APP_ARBITRARY_DATA_OBJECT_T1_13     = 0xF1DC,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_13
    OID_APP_ARBITRARY_DATA_OBJECT_T1_14     = 0xF1DD,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_14
    OID_APP_ARBITRARY_DATA_OBJECT_T1_15     = 0xF1DE,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_15
    OID_APP_ARBITRARY_DATA_OBJECT_T1_16     = 0xF1DF,//!< OID_APP_ARBITRARY_DATA_OBJECT_T1_16

    // Application Data Objects Type 2 (1500 byte size)
    OID_APP_ARBITRARY_DATA_OBJECT_T2_1      = 0xF1E0,//!< OID_APP_ARBITRARY_DATA_OBJECT_T2_1
    OID_APP_ARBITRARY_DATA_OBJECT_T2_2      = 0xF1E1 //!< OID_APP_ARBITRARY_DATA_OBJECT_T2_2
} OptigaOID;

/** Maximum length of public key certificate data element length */
#define PUBLIC_KEY_CERT_MAX_LEN               1728
/** Maximum length of root CA public key certificate data element */
#define ROOT_CA_PUBLIC_KEY_CERT_MAX_LEN       1024

/**
 * @brief Retrieves a data object from the Trust X.
 *
 * The functions retrieves the data object specified by its Object ID (OID)
 * from the Trust X device.
 * The data objects that can be retrieved a listed in the enum OptigaOID.
 *
 * @param[in]  oid   Object ID of the data object to retrieve.
 * @param[in]  data  Pointer to buffer that stores the retrieved data object.
 * @param[out] len   Pointer to variable holding the length of the retrieved data object.
 *
 * @retval  IFX_I2C_STACK_SUCCESS  If the function succeeded.
 * @retval  IFX_I2C_STACK_ERROR    If the operation failed.
 */
uint16_t optiga_get_data_object(OptigaOID oid, uint8_t** data, uint16_t* len);

/**
 * @brief Write a data object to the Trust X.
 *
 * The function writes the specified data to the specified Object ID (OID) on the Trust X.
 *
 * @param oid   Object ID of the data object to write.
 * @param data  Data to be written.
 * @param len   Length of data to be written.
 *
 * @retval  IFX_I2C_STACK_SUCCESS  If the function succeeded.
 * @retval  IFX_I2C_STACK_ERROR    If the operation failed.
 */
uint16_t optiga_set_data_object(OptigaOID oid, const uint8_t* data, uint16_t len);

/**
 * @brief Callback for asynchronous command library calls.
 *
 * @param ctx       Pointer to user-defined context object to be returned with callback
 * @param event     Information on event that triggered the callback
 * @param data      Data retrieved
 * @param data_len  Length of data retrieved
 */
typedef void (*ifx_optiga_callbck)(void* ctx, uint8_t event, const uint8_t* data, uint16_t data_len);

#define OPTIGA_CMD_HEADER_LEN                4

// Hash

typedef enum OptigaHash
{
    HASH_SHA256 = 0xE2 // value matches OPTIGA(TM) Algorithm Identifier
} OptigaHash;

#define OPTIGA_HASH_LEN_SHA256              32

/**
 * @brief Calculates the message digest for the specified message.
 *
 * @param type      Message digest type
 * @param msg       Data of message to be hashed
 * @param len       Length of data of message to be hashed
 * @param digest    Message digest (length is implicitly defined by message digest type, see optiga_get_hash_len
 * @return IFX_I2C_STACK_SUCCESS if operation succeeds, otherwise IFX_I2C_STACK_ERROR
 */
uint16_t optiga_calc_hash(OptigaHash type, const uint8_t* msg, uint16_t len, uint8_t* digest);


// ECDSA

/**
 * List of available elliptic curves.
 */
typedef enum OptigaCurve
{
    ECC_NIST_P256 = 0x03,//!< ECC_NIST_P256
    ECC_NIST_P384 = 0x04 //!< ECC_NIST_P384
} OptigaCurve;

/** Maximum length of ASN.1-coded ECDSA signature with ECC_NIST_P256 */
#define SIGNATURE_ECDSA_ECC_NIST_P256_MAX_LEN    (2+2*(2+33))
/** Maximum length of ASN.1-coded ECDSA signature with ECC_NIST_P384 */
#define SIGNATURE_ECDSA_ECC_NIST_P384_MAX_LEN    (2+2*(2+49))

/**
 * @brief Generates a new private/public key pair.
 *
 * @param curve             Elliptic curve domain in which to generate the key pair (see OptigaCurve)
 * @param oid_private_key   Object ID of the slot where the private key shall be stored on Trust X
 * @param public_key        Generated and ASN.1-coded public key
 * @param public_key_len    Length of generated and ASN.1-coded public key
 * @return IFX_I2C_STACK_SUCCESS if operation succeeds, otherwise IFX_I2C_STACK_ERROR
 */
uint16_t optiga_generate_key_pair(OptigaCurve curve, OptigaOID oid_private_key, uint8_t** public_key, uint8_t* public_key_len);

/**
 * @brief Verifies a digital signature over a message using a public key
 *
 * @param curve         Elliptic curve domain of key used for ECDSA-signature
 * @param digest        Message digest of message to be verified
 * @param digest_size   Length of message digest of message to be verified
 * @param sig           ASN.1-coded signature data to be verified
 * @param sig_len       Length of ASN.1-coded signature data to be verified
 * @param pub_key       ASN.1-coded public key for signature verification
 * @param pub_key_len   Length of ASN.1-coded public key
 * @return IFX_I2C_STACK_SUCCESS if operation succeeds, otherwise IFX_I2C_STACK_ERROR
 */
uint16_t optiga_verify_signature(OptigaCurve curve, const uint8_t* digest, uint8_t digest_size, const uint8_t* sig, uint8_t sig_len, const uint8_t* pub_key, uint8_t pub_key_len);

/**
 * @brief Calculates a digital signature over a message digest using a key inside the Trust X
 *
 * @param oid_private_key   Object ID of the slot that contains the private key to use for the signature calculation
 * @param digest            Message digest of message to be signed
 * @param digest_size       Length of message digest of message to be signed
 * @param sig               Resulting ASN.1-coded signature data
 * @param sig_len           Length of resulting ASN.1-coded signature data
 * @return IFX_I2C_STACK_SUCCESS if operation succeeds, otherwise IFX_I2C_STACK_ERROR
 */
uint16_t optiga_calc_sign(OptigaOID oid_private_key, const uint8_t* digest, uint8_t digest_size, uint8_t** sig, uint8_t* sig_len);

// TLS
/**
 * @brief Calculates the shared secret for TLS on the Trust X
 *
 * @param pub_key       ASN
 * @param pub_key       ASN.1-coded public key
 * @param pub_key_len   Length of ASN.1-coded public key
 * @return IFX_I2C_STACK_SUCCESS if operation succeeds, otherwise IFX_I2C_STACK_ERROR
 */
uint16_t optiga_calc_shared_secret(const uint8_t* pub_key, uint8_t pub_key_len);

/**
 * @brief Derives a key from a previously calculated shared secret (see optiga_calc_shared_secret)
 *
 * @param data          Key derivation parameters
 * @param data_len      Length of key derivation parameters
 * @param output        Derived data
 * @param output_len    Length of derived data
 * @return IFX_I2C_STACK_SUCCESS if operation succeeds, otherwise IFX_I2C_STACK_ERROR
 */
uint16_t optiga_derive_key(const uint8_t* data, uint8_t data_len, uint8_t* output, uint8_t output_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 **/

#endif /* OPTIGA_COMMAND_LIBRARY_H__ */
