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

#include "optiga_command_library.h"
#include "ifx_i2c.h"
#include <string.h> // memcpy

#ifndef IFX_OPTIGA_LOG
#define IFX_OPTIGA_LOG(...)
#endif
#ifndef IFX_OPTIGA_DUMP
#define IFX_OPTIGA_DUMP(...)
#endif

// Command headers
#define HEADER_SPACE                            0x00, 0x00, 0x00, 0x00
#define OPTIGA_CMD_FLAG_FLUSH_LAST_ERROR        0x80

// Command status codes
#define OPTIGA_CMD_STATUS_SUCCESS               0x00

// Command Open Application
#define OPTIGA_CMD_OPEN_APPLICATION             0x70
#define APP_ID                                  0xD2, 0x76, 0x00, 0x00, 0x04, 0x47, 0x65, 0x6E, \
                                                0x41, 0x75, 0x74, 0x68, 0x41, 0x70, 0x70, 0x6C

// Command Set Auth Scheme
#define OPTIGA_CMD_SET_AUTH_SCHEME              0x10
#define OPTIGA_AUTH_DTLS_ECDHE_ECDSA_AES_128_CCM_8 0x99

// Command Get Random
#define OPTIGA_CMD_GET_RANDOM                   0x0C

// Command Get/Set DTLS message
#define OPTIGA_CMD_PROC_UP_LINK_MSG             0x1A
#define OPTIGA_CMD_PROC_DOWN_LINK_MSG           0x1B

// Command Get Data Object
#define OPTIGA_CMD_GET_DATA_OBJECT              0x01
#define OPTIGA_PARAM_READ_DATA                  0x00
#define OPTIGA_PARAM_READ_METADATA              0x01

// Command Set Data Object
#define OPTIGA_CMD_SET_DATA_OBJECT              0x02
#define OPTIGA_PARAM_WRITE_AND_EREASE_DATA      0x40

// Command Calculate Hash
#define OPTIGA_CMD_CALC_HASH                    0x30

// Command Generate Key Pair (GenKeyPair)
#define OPTIGA_CMD_GEN_KEY_PAIR                 0x38
#define OPTIGA_ALGORITHM_ID_ECC_NIST_P256       0x03
#define OPTIGA_ALGORITHM_ID_ECC_NIST_P384       0x04
#define OPTIGA_KEY_USAGE_KEY_AGREE              0x20
#define OPTIGA_KEY_USAGE_AUTH                   0x01

// Command Verify Signature (VerifySign)
#define OPTIGA_CMD_VERIFY_SIGNATURE             0x32
#define OPTIGA_CMD_VERIFY_SIGNATURE_MAX_PDU_LEN (OPTIGA_CMD_HEADER_LEN + 16 + 2*UINT8_MAX - 2)
						  
// Command Sign Hash (CalcSign)
#define OPTIGA_CMD_SIGN_HASH                    0x31
#define OPTIGA_SIGNATURE_SCHEME_ECDSA           0x11

// Command Calculate Shared Secret (CalcSSec)
#define OPTIGA_CMD_CALC_SHARED_SECRET           0x33
#define OPTIGA_KEY_AGREEMENT_ECDHE              0x01

// Command Derive Key (DeriveKey)
#define OPTIGA_CMD_DERIVE_KEY                   0x34
#define OPTIGA_KDF_TLS_PRF_SHA256               0x01

// Buffer
#define TL_BUFFER_SIZE                          2048
static uint8_t  m_cmdlib_buffer[TL_BUFFER_SIZE];
static uint16_t m_cmblib_buffer_tx_len;
static uint16_t m_cmblib_buffer_rx_len;

// Members to use library in blocking mode
static volatile       uint8_t   m_cmdlib_ifx_i2c_busy = 0;
static volatile       uint8_t   m_cmdlib_ifx_i2c_status;
static const volatile uint8_t * m_cmdlib_rx_buffer;
static volatile       uint16_t  m_cmdlib_rx_length;

// Members to use library in non-blocking mode
static volatile ifx_optiga_callbck m_callback = NULL;
static volatile void* m_ctx = NULL;

static void optiga_create_header(uint8_t* header, uint8_t command, uint8_t param,
    uint16_t payload_len)
{
    header[0] = command | OPTIGA_CMD_FLAG_FLUSH_LAST_ERROR;
    header[1] = param;
    header[2] = payload_len >> 8;
    header[3] = payload_len;
}

static uint16_t optiga_send_apdu(void* ctx, ifx_optiga_callbck callback, const uint8_t* header,  uint16_t header_len,
                                                                         const uint8_t* payload, uint16_t payload_len)
{
    uint16_t response_len;

    IFX_OPTIGA_LOG("[IFX-OPTIGA] TX APDU\n");
    IFX_OPTIGA_DUMP(header, header_len);
    IFX_OPTIGA_DUMP(payload, payload_len);

    // Buffer merge
    if (header_len + payload_len > sizeof(m_cmdlib_buffer))
    {
        return IFX_I2C_STACK_ERROR;
    }
    memcpy(m_cmdlib_buffer, header, header_len);
    memcpy(m_cmdlib_buffer + header_len, payload, payload_len);
    m_cmblib_buffer_tx_len = header_len + payload_len;

    // Set RX buffer size
    m_cmblib_buffer_rx_len = sizeof(m_cmdlib_buffer);

    // Asynchronous mode
    if (callback)
    {
        m_callback = callback;
        m_ctx = ctx;
        return ifx_i2c_transceive(&ifx_i2c_context_0, m_cmdlib_buffer, &m_cmblib_buffer_tx_len, m_cmdlib_buffer, &m_cmblib_buffer_rx_len);
    }

    // Synchronous mode
    m_cmdlib_ifx_i2c_busy = 1;
    if (ifx_i2c_transceive(&ifx_i2c_context_0, m_cmdlib_buffer, &m_cmblib_buffer_tx_len, m_cmdlib_buffer, &m_cmblib_buffer_rx_len))
    {
        return IFX_I2C_STACK_ERROR;
    }

    while (!callback && m_cmdlib_ifx_i2c_busy)
    {
        // Wait
    }

    if (m_cmdlib_ifx_i2c_status != IFX_I2C_STACK_SUCCESS)
    {
        return IFX_I2C_STACK_ERROR;
    }

    if (m_cmdlib_rx_length < OPTIGA_CMD_HEADER_LEN)
    {
        return IFX_I2C_STACK_ERROR;
    }

    if (m_cmdlib_rx_buffer[0] != OPTIGA_CMD_STATUS_SUCCESS)
    {
        return IFX_I2C_STACK_ERROR;
    }

    response_len = (m_cmdlib_rx_buffer[2] << 8) | ((const uint8_t *) m_cmdlib_rx_buffer)[3];
    if (OPTIGA_CMD_HEADER_LEN + response_len != m_cmdlib_rx_length)
    {
        return IFX_I2C_STACK_ERROR;
    }

    return IFX_I2C_STACK_SUCCESS;
}

static void ifx_i2c_tl_event_handler(void* upper_layer_ctx, host_lib_status_t event)
{
    if (event == OPTIGA_CMD_STATUS_SUCCESS)
    {
        IFX_OPTIGA_LOG("[IFX-OPTIGA] RX APDU\n");
        IFX_OPTIGA_DUMP(data, data_len);
        IFX_OPTIGA_DUMP(payload, payload_len);
    }
    else
    {
        IFX_OPTIGA_LOG("[IFX-OPTIGA] RX APDU failed, event %d\n", event);
    }

    if (m_callback) // Asynchronous mode
    {
        m_callback((void*)m_ctx, event, m_cmdlib_buffer, m_cmblib_buffer_rx_len);
    }
    else // Synchronous mode
    {
        m_cmdlib_ifx_i2c_busy = 0;
        m_cmdlib_rx_buffer = m_cmdlib_buffer;
        m_cmdlib_rx_length = m_cmblib_buffer_rx_len;
        m_cmdlib_ifx_i2c_status = event;
    }
}

uint16_t optiga_init(void)
{
    m_cmdlib_ifx_i2c_busy = 1;

    ifx_i2c_context_0.upper_layer_event_handler = ifx_i2c_tl_event_handler;
    if (ifx_i2c_open(&ifx_i2c_context_0) != IFX_I2C_STACK_SUCCESS)
    {
        return IFX_I2C_STACK_ERROR;
    }

    while(m_cmdlib_ifx_i2c_busy);
    return m_cmdlib_ifx_i2c_status;
}

uint16_t optiga_open_application(void)
{
    uint8_t header[] = { HEADER_SPACE };
    uint8_t payload[] = { APP_ID };
    optiga_create_header(header, OPTIGA_CMD_OPEN_APPLICATION, 0x00, sizeof(payload));

    return optiga_send_apdu(0, 0, header, sizeof(header), payload, sizeof(payload));
}

uint16_t optiga_get_data_object(OptigaOID oid, uint8_t** data, uint16_t* len)
{
    uint8_t apdu[] = { HEADER_SPACE, oid >> 8, oid };
    optiga_create_header(apdu, OPTIGA_CMD_GET_DATA_OBJECT, OPTIGA_PARAM_READ_DATA,
        sizeof(apdu) - OPTIGA_CMD_HEADER_LEN);

    if (optiga_send_apdu(0, 0, apdu, sizeof(apdu), 0, 0))
    {
        return IFX_I2C_STACK_ERROR;
    }

    *data = (uint8_t*)m_cmdlib_rx_buffer + OPTIGA_CMD_HEADER_LEN;
    *len = m_cmdlib_rx_length - OPTIGA_CMD_HEADER_LEN;
    return IFX_I2C_STACK_SUCCESS;
}

uint16_t optiga_set_data_object(OptigaOID oid, const uint8_t* data, uint16_t len)
{
    uint8_t apdu[] = { HEADER_SPACE, oid >> 8, oid, 0x00, 0x00 };
    optiga_create_header(apdu, OPTIGA_CMD_SET_DATA_OBJECT, OPTIGA_PARAM_WRITE_AND_EREASE_DATA,
                         sizeof(apdu) - OPTIGA_CMD_HEADER_LEN + len);

    return optiga_send_apdu(0, 0, apdu, sizeof(apdu), data, len);
}

uint16_t optiga_get_random(uint8_t* p_random, uint16_t length)
{
    uint8_t apdu[] = { HEADER_SPACE, length >> 8, length };
    optiga_create_header(apdu, OPTIGA_CMD_GET_RANDOM, 0x00, 2);

    if (p_random == NULL || length < 0x0008 || length > 0x100)
    {
        return IFX_I2C_STACK_ERROR;
    }

    if (optiga_send_apdu(0, 0, apdu, sizeof(apdu), 0, 0))
    {
        return IFX_I2C_STACK_ERROR;
    }

    memcpy(p_random, (uint8_t*)(m_cmdlib_rx_buffer + OPTIGA_CMD_HEADER_LEN), length);
    return IFX_I2C_STACK_SUCCESS;
}

// OPTIGA Toolbox Commands

uint16_t optiga_calc_hash(OptigaHash type, const uint8_t* msg, uint16_t len, uint8_t* digest)
{
    uint8_t apdu[] = { HEADER_SPACE, 0x01, len >> 8, len };
    optiga_create_header(apdu, OPTIGA_CMD_CALC_HASH, type, sizeof(apdu) - OPTIGA_CMD_HEADER_LEN + len);

    if (optiga_send_apdu(0, 0, apdu, sizeof(apdu), msg, len))
    {
        return IFX_I2C_STACK_ERROR;
    }
    memcpy(digest, (uint8_t*)m_cmdlib_rx_buffer + OPTIGA_CMD_HEADER_LEN + 3,
        m_cmdlib_rx_length - OPTIGA_CMD_HEADER_LEN - 3);
    return IFX_I2C_STACK_SUCCESS;
}

uint16_t optiga_generate_key_pair(OptigaCurve curve, OptigaOID oid_private_key, uint8_t** public_key, uint8_t* public_key_len)
{
    uint8_t apdu[] = { HEADER_SPACE, 0x01, 0x00, 0x02, oid_private_key >> 8, oid_private_key,
        0x02, 0x00, 0x01, OPTIGA_KEY_USAGE_KEY_AGREE | OPTIGA_KEY_USAGE_AUTH };
    optiga_create_header(apdu, OPTIGA_CMD_GEN_KEY_PAIR, curve, sizeof(apdu) - OPTIGA_CMD_HEADER_LEN);

    if (optiga_send_apdu(0, 0, apdu, sizeof(apdu), 0, 0))
    {
        return IFX_I2C_STACK_ERROR;
    }

    if (m_cmdlib_rx_length < OPTIGA_CMD_HEADER_LEN + 6)
    {
        return IFX_I2C_STACK_ERROR;
    }

    // Trim headers (6 byte):
    //     OPTIGA public key header (3 byte): 0x02, length_high_byte, length_low_byte
    //     DER BIT STRING (3 byte): 0x03, length, 0x00 (bits unused in last byte)
    *public_key = (uint8_t*)m_cmdlib_rx_buffer + OPTIGA_CMD_HEADER_LEN + 6;
    *public_key_len = m_cmdlib_rx_length - OPTIGA_CMD_HEADER_LEN - 6;
    return IFX_I2C_STACK_SUCCESS;
}

uint16_t optiga_verify_signature(OptigaCurve curve, const uint8_t* digest, uint8_t digest_size, const uint8_t* sig,
    uint8_t sig_len, const uint8_t* pub_key, uint8_t pub_key_len)
{
    uint8_t apdu[OPTIGA_CMD_VERIFY_SIGNATURE_MAX_PDU_LEN];
    uint8_t* p = apdu + OPTIGA_CMD_HEADER_LEN;

    *p++ = 0x01; // Digest
    *p++ = 0x00;
    *p++ = digest_size;
    memcpy(p, digest, digest_size);
    p += digest_size;

    // Check ASN.1 SEQUENCE tag type and length of signature
    uint8_t asn_tag_type = sig[0];
    uint8_t asn_tag_len = sig[1];
    if (asn_tag_type != 0x30 || asn_tag_len + 2 != sig_len)
    {
        return IFX_I2C_STACK_ERROR;
    }

    *p++ = 0x02; // Signature (without ASN.1 SEQUENCE tag)
    *p++ = 0x00;
    *p++ = (sig_len - 2);
    memcpy(p, sig + 2, sig_len - 2);
    p += (sig_len - 2);

    if (pub_key_len) // Select public key from function arguments
    {
        *p++ = 0x05; // ECC curve
        *p++ = 0x00;
        *p++ = 0x01;
        *p++ = curve;

        *p++ = 0x06; // Public key
        *p++ = 0x00;
        *p++ = pub_key_len + 3;
        *p++ = 0x03;
        *p++ = pub_key_len + 1;
        *p++ = 0x00;
    }
    else // Select public key from certificate in OPTIGA
    {
        *p++ = 0x04; // OID of certificate
        *p++ = 0x02;
        *p++ = 0xE0;
        *p++ = 0xE8;
    }

    optiga_create_header(apdu, OPTIGA_CMD_VERIFY_SIGNATURE, OPTIGA_SIGNATURE_SCHEME_ECDSA,
        (uint16_t)(p - apdu - OPTIGA_CMD_HEADER_LEN + pub_key_len));
    if (optiga_send_apdu(0, 0, apdu, p - apdu, pub_key, pub_key_len))
    {
        return IFX_I2C_STACK_ERROR;
    }

    return IFX_I2C_STACK_SUCCESS;
}

uint16_t optiga_calc_sign(OptigaOID oid_private_key, const uint8_t* digest, uint8_t digest_size, uint8_t** sig, uint8_t* sig_len)
{
    uint8_t apdu[] = { HEADER_SPACE, 0x03, 0x00, 0x02, oid_private_key >> 8, oid_private_key, 0x01, 0, digest_size };
    optiga_create_header(apdu, OPTIGA_CMD_SIGN_HASH, OPTIGA_SIGNATURE_SCHEME_ECDSA,
        sizeof(apdu) - OPTIGA_CMD_HEADER_LEN + digest_size);

    if (optiga_send_apdu(0, 0, apdu, sizeof(apdu), digest, digest_size))
    {
        return IFX_I2C_STACK_ERROR;
    }

    *sig = (uint8_t*)m_cmdlib_rx_buffer + OPTIGA_CMD_HEADER_LEN - 2;
    *sig_len = m_cmdlib_rx_length - OPTIGA_CMD_HEADER_LEN + 2;

    // Write ASN.1 SEQUENCE (required for TLS protocol)
    (*sig)[0] = 0x30;
    (*sig)[1] = m_cmdlib_rx_length - OPTIGA_CMD_HEADER_LEN;
    return IFX_I2C_STACK_SUCCESS;
}

uint16_t optiga_calc_shared_secret(const uint8_t* pub_key, uint8_t pub_key_len)
{
    uint8_t apdu[] = { HEADER_SPACE, 0x01, 0x00, 0x02, OID_SESSION_CONTEXT_1 >> 8, (uint8_t)OID_SESSION_CONTEXT_1, // Private Key OID
        0x05, 0x00, 0x01, OPTIGA_ALGORITHM_ID_ECC_NIST_P256, // Algorithm Identifier
        //0x07, 0x00, 0x00, // Export Shared Secret
        0x08, 0x00, 0x02, OID_SESSION_CONTEXT_1 >> 8, (uint8_t)OID_SESSION_CONTEXT_1, // Shared Secret OID
        0x06, 0x00, 3 + pub_key_len, 0x03, 1 + pub_key_len, 0x00 // Public Key
        };

    optiga_create_header(apdu, OPTIGA_CMD_CALC_SHARED_SECRET, OPTIGA_KEY_AGREEMENT_ECDHE,
        sizeof(apdu) - OPTIGA_CMD_HEADER_LEN + pub_key_len);
    if (optiga_send_apdu(0, 0, apdu, sizeof(apdu), pub_key, pub_key_len))
    {
        return IFX_I2C_STACK_ERROR;
    }
    return IFX_I2C_STACK_SUCCESS;
}

uint16_t optiga_derive_key(const uint8_t* data, uint8_t data_len, uint8_t* output, uint8_t output_len)
{
    uint8_t* apdu;
    uint16_t apdu_len;
    uint8_t apdu_without_export[] = { HEADER_SPACE, 0x01, 0x00, 0x02, 0xE1, 0x00, // OID Master Secret
        0x03, 0x00, 0x02, 0x00, 0x30, // Length of the key to be derived
        0x08, 0x00, 0x02, 0xE1, 0x00, // OID Derived Key
        0x02, 0x00, data_len // Secret Derivation Data
        };
    uint8_t apdu_with_export[] = { HEADER_SPACE, 0x01, 0x00, 0x02, 0xE1, 0x00, // OID Master Secret
        0x03, 0x00, 0x02, 0x00, 0xFF, // Length of the key to be derived
        0x07, 0x00, 0x00, // Export derived key
        0x02, 0x00, data_len // Secret Derivation Data
        };

    if (output)
    {
        apdu = apdu_with_export;
        apdu_len = sizeof(apdu_with_export);
    }
    else
    {
        apdu = apdu_without_export;
        apdu_len = sizeof(apdu_without_export);
    }

    optiga_create_header(apdu, OPTIGA_CMD_DERIVE_KEY, OPTIGA_KDF_TLS_PRF_SHA256,
        apdu_len - OPTIGA_CMD_HEADER_LEN + data_len);
    if (optiga_send_apdu(0, 0, apdu, apdu_len, data, data_len))
    {
        return IFX_I2C_STACK_ERROR;
    }

    if (output)
    {
        if (m_cmdlib_rx_length < OPTIGA_CMD_HEADER_LEN + output_len)
        {
            return IFX_I2C_STACK_ERROR;
        }
        memcpy(output, (uint8_t*)m_cmdlib_rx_buffer + OPTIGA_CMD_HEADER_LEN, output_len);
    }

    return IFX_I2C_STACK_SUCCESS;
}
