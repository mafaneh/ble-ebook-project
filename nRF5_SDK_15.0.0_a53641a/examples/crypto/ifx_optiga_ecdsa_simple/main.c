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

/**
 * @file
 * @defgroup ifx_optiga_ecdsa_simple_example Infineon ECDSA Simple Example (ifx_optiga_ecdsa_simple)
 * @{
 * @addtogroup ifx_optiga
 * @brief Infineon OPTIGA(TM) Trust X ECDSA simple example application main file.
 *
 * This file contains the source code for a sample application
 * using Infineon OPTIGA(TM) Trust X hardware security module.
 *
 * For more information about the Infineon OPTIGA(TM) Trust X please visit:
 * https://www.infineon.com/cms/en/product/security-smart-card-solutions/optiga-embedded-security-solutions/optiga-trust/optiga-trust-x-sls-32aia/
 */


#include <stdio.h>

#include "app_error.h"
#include "app_util_platform.h"
#include "nrf_drv_clock.h"
#include "nrf_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "sdk_config.h"

#include "mbedtls/platform.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/sha256.h"

#include "optiga_command_library.h"


// Prototypes
static int verify_signature_with_certificate(
    const uint8_t   *certificate,
    const uint32_t   certificate_len,
    const uint8_t   *asn_sig,
    const uint32_t   asn_sig_len,
    const uint8_t   *data,
    const uint32_t   data_len);

static int verify_signature_with_key(
    const uint8_t   *key,
    const uint32_t   key_len,
    const uint8_t   *asn_sig,
    const uint32_t   asn_sig_len,
    const uint8_t   *data,
    const uint32_t   data_len);

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info);

void example_one_way_authentication(void);

void example_random_number_generation(void);

void example_message_authentication(void);

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    ret_code_t   err_code;

    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);

    // Start internal LFCLK XTAL oscillator
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_RAW_INFO("Infineon OPTIGA(TM) Trust X (\"Trust X\") example for ECDSA\r\n");
    NRF_LOG_RAW_INFO("Initialize Trust X host library and I2C protocol.\r\n");
    err_code = optiga_init();
    APP_ERROR_CHECK(err_code);

    NRF_LOG_RAW_INFO("Open Trust X application.\r\n");
    err_code = optiga_open_application();
    APP_ERROR_CHECK(err_code);

    example_random_number_generation();

    example_one_way_authentication();

    example_message_authentication();

    NRF_LOG_RAW_INFO("Trust X example application finished.\r\n");

    while (true)
    {
        nrf_pwr_mgmt_run();
        NRF_LOG_FLUSH();
    }
}


void example_message_authentication(void)
{
    ret_code_t err_code;

    NRF_LOG_RAW_INFO("Create key pair with Trust X in slot OID_DEVICE_PRIVATE_KEY_2 and print public key:\r\n");
    uint8_t* p_public_key;
    uint8_t  public_key_len;
    err_code = optiga_generate_key_pair(ECC_NIST_P256, OID_DEVICE_PRIVATE_KEY_2, &p_public_key, &public_key_len);
    APP_ERROR_CHECK(err_code);
    // Copy public key into application memory to make it persistent, and print it
    uint8_t public_key[UINT8_MAX];
    memcpy(public_key, p_public_key, public_key_len);
    NRF_LOG_RAW_HEXDUMP_INFO(public_key, public_key_len);

    NRF_LOG_RAW_INFO("Calculate message digest (SHA-256) with Trust X:\r\n");
    uint8_t digest[OPTIGA_HASH_LEN_SHA256];
    uint8_t msg[] = {'m', 'e', 's', 's', 'a', 'g', 'e'};
    err_code = optiga_calc_hash(HASH_SHA256, msg, sizeof(msg), digest);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_RAW_HEXDUMP_INFO(digest, sizeof(digest));

    NRF_LOG_RAW_INFO("Sign digest with generated private key OID_DEVICE_PRIVATE_KEY_2 inside Trust X:\r\n");
    uint8_t* p_sig2;
    uint8_t  sig2_len;
    err_code = optiga_calc_sign(OID_DEVICE_PRIVATE_KEY_2, digest, sizeof(digest), &p_sig2, &sig2_len);
    APP_ERROR_CHECK(err_code);
    if (sig2_len > SIGNATURE_ECDSA_ECC_NIST_P256_MAX_LEN)
    {
        APP_ERROR_CHECK(err_code);
    }
    // Copy Signature into application memory to make it persistent, and print it
    uint8_t sig2[SIGNATURE_ECDSA_ECC_NIST_P256_MAX_LEN];
    memcpy(sig2, p_sig2, sig2_len);
    NRF_LOG_RAW_HEXDUMP_INFO(sig2, sig2_len);

    NRF_LOG_RAW_INFO("Verify signature with Trust X using the generated public key:\r\n");
    err_code = optiga_verify_signature(ECC_NIST_P256, digest, sizeof(digest), sig2, sig2_len, public_key, public_key_len);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_RAW_INFO(" OK - successfully verified!\r\n");

    NRF_LOG_RAW_INFO("Verify signature with mbed TLS using the generated public key:\r\n");
    if (verify_signature_with_key(&public_key[0], public_key_len, sig2, sig2_len, msg, sizeof(msg)) == 0)
    {
        NRF_LOG_RAW_INFO(" OK - successfully verified!\r\n");
    }
    else
    {
        NRF_LOG_RAW_INFO(" Signature verification failed - verification failed.\r\n");
    }
}

void example_random_number_generation(void)
{
    ret_code_t err_code;

    NRF_LOG_RAW_INFO("Retrieve random number (16 byte) from Trust X:\r\n");
    uint8_t rnd[16];
    err_code = optiga_get_random(rnd, sizeof(rnd));
    APP_ERROR_CHECK(err_code);
    NRF_LOG_RAW_HEXDUMP_INFO(rnd, sizeof(rnd));
}

void example_one_way_authentication(void)
{
    ret_code_t err_code;

    NRF_LOG_RAW_INFO("Calculate message digest (SHA-256) with Trust X:\r\n");
    uint8_t digest[OPTIGA_HASH_LEN_SHA256];
    uint8_t msg[] = {'c', 'h', 'a', 'l', 'l', 'e', 'n', 'g', 'e'};
    err_code = optiga_calc_hash(HASH_SHA256, msg, sizeof(msg), digest);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_RAW_HEXDUMP_INFO(digest, sizeof(digest));

    NRF_LOG_RAW_INFO("Sign digest with protected private key OID_DEVICE_PRIVATE_KEY_1 inside Trust X:\r\n");
    uint8_t* p_sig1;
    uint8_t  sig1_len;
    err_code = optiga_calc_sign(OID_DEVICE_PRIVATE_KEY_1, digest, sizeof(digest), &p_sig1, &sig1_len);
    APP_ERROR_CHECK(err_code);
    if (sig1_len > SIGNATURE_ECDSA_ECC_NIST_P256_MAX_LEN)
    {
        APP_ERROR_CHECK(err_code);
    }
    // Copy Signature into application memory to make it persistent, and print it
    uint8_t sig1[SIGNATURE_ECDSA_ECC_NIST_P256_MAX_LEN];
    memcpy(sig1, p_sig1, sig1_len);
    NRF_LOG_RAW_HEXDUMP_INFO(sig1, sig1_len);

    NRF_LOG_RAW_INFO("Retrieve Infineon public key certificate (OID_INFINEON_CERTIFICATE) from Trust X (output truncated):\r\n");
    uint8_t   *p_cert;
    uint16_t   cert_len;
    err_code = optiga_get_data_object(OID_INFINEON_CERTIFICATE, &p_cert, &cert_len);
    APP_ERROR_CHECK(err_code);
    if (p_cert[0] == 0xC0) // 0x30 = TLS identity tag
    {
        // Strip TLS certificate chain header from certificate chain
        // Please refer to the official documentation for detailed information on public-key certificate data structures).
        //   Example TLS certificate header
        //     C0 01 CA 00 01 C7 00 01 C4 30 82 01 C0 30 82 ... end
        //     <- t1 -> <- va -> <- v1 -> <- X.509 cert  --------->
        p_cert   += 9;
        cert_len -= 9;
    }
    else if (p_cert[0] == 0x30) // 0xC0 = one-way authentication identity
    {
        // Pure ASN.1-coded certificate, no need to strip anything
    }
    else
    {
        APP_ERROR_CHECK(1); // Not supported case
    }
    // Copy certificate into application memory to make it persistent
    if (cert_len > PUBLIC_KEY_CERT_MAX_LEN)
    {
        APP_ERROR_CHECK(1); // Not supported case
    }
    uint8_t cert[PUBLIC_KEY_CERT_MAX_LEN];
    memcpy(cert, p_cert, cert_len);
    NRF_LOG_RAW_HEXDUMP_INFO(cert, cert_len);

    NRF_LOG_RAW_INFO("Verify signature using mbed TLS and the certificate's public key:\r\n");
    if (verify_signature_with_certificate(&cert[0], cert_len, sig1, sig1_len, msg, sizeof(msg)) == 0)
    {
        NRF_LOG_RAW_INFO(" OK - successfully verified!\r\n");
    }
    else
    {
        NRF_LOG_RAW_INFO(" Signature verification failed - verification failed.\r\n");
    }
}


/**
 * Handles faults.
 */
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_RAW_INFO("\r\nError.\r\n\r\n\r\n");
    NRF_LOG_FLUSH();
    while (true)
    {
        nrf_pwr_mgmt_run();
        // Do not automatically reset, otherwise fast subsequent sign calls might trigger
        // security events on the Trust X device
    }
}


/**
 * @brief Helper function that verifies an ECDSA signature with the signer's public key, using mbed TLS.
 *
 * @param key           ECDSA public key data used for signature verification
 * @param key_len       Length of ECDSA public key data
 * @param sig           Signature coded in ASN.1
 * @param sig_len       Length of signature
 * @param data          Data which was signed
 * @param data_len      Length of data which was signed
 * @return 0 on successful verification, otherwise 1
 */
static int verify_signature_with_key(
    const uint8_t   *key,
    const uint32_t   key_len,
    const uint8_t   *sig,
    const uint32_t   sig_len,
    const uint8_t   *data,
    const uint32_t   data_len)
{
    int                 result;
    uint8_t             digest[32]; // Stores a SHA-256 hash digest of 256 bits = 32 Byte
    mbedtls_pk_context  context;    // Public key container

    // Setup memory management
    mbedtls_platform_set_calloc_free(calloc, free);

    mbedtls_pk_init(&context);

    if ((result = mbedtls_pk_setup(&context, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))) != 0 )
    {
        APP_ERROR_CHECK(result);
    }

    if ((result = mbedtls_ecp_group_load(&mbedtls_pk_ec(context)->grp, MBEDTLS_ECP_DP_SECP256R1)) != 0)
    {
        APP_ERROR_CHECK(result);
    }

    if ((result = mbedtls_ecp_point_read_binary(&mbedtls_pk_ec(context)->grp, &mbedtls_pk_ec(context)->Q, key, key_len)) != 0)
    {
        APP_ERROR_CHECK(result);
    }

    // Calculate the SHA-256 hash digest of the to-be-signed data
    mbedtls_sha256(data, data_len, digest, 0);

    // Do the signature verification using context, hash, and signature
    if ((result = mbedtls_ecdsa_read_signature(mbedtls_pk_ec(context), digest, sizeof(digest), sig, sig_len)) != 0)
    {
        // Error: signature invalid
        mbedtls_pk_free(&context);
        return result;
    }
    else
    {
        // Signature successfully verified
        mbedtls_pk_free(&context);
        return 0;
    }
}


/**
 * @brief Helper function that verifies an ECDSA signature using the public key contained in an X.509 certificate.
 *
 * @param certificate       X.509 certificate data which contains the public ECDSA verification key
 * @param certificate_len   Length of the X.509 certificate data
 * @param sig               Signature coded in ASN.1
 * @param sig_len           Length of signature
 * @param data              Data which was signed
 * @param data_len          Length of data which was signed
 * @return 0 on successful verification, otherwise 1
 */
static int verify_signature_with_certificate(
    const uint8_t   *certificate,
    const uint32_t   certificate_len,
    const uint8_t   *sig,
    const uint32_t   sig_len,
    const uint8_t   *data,
    const uint32_t   data_len)
{
    int                     result;
    uint8_t                 digest[OPTIGA_HASH_LEN_SHA256];
    mbedtls_x509_crt        x509_certificate;
    mbedtls_ecdsa_context   ecdsa_context;
    mbedtls_ecp_keypair    *ecp_keypair;

    // Setup memory management
    mbedtls_platform_set_calloc_free(calloc, free);

    // Parse certificate and prepare verification context with public key
    mbedtls_x509_crt_init(&x509_certificate);
    result = mbedtls_x509_crt_parse(&x509_certificate, certificate, certificate_len);
    if (result != 0)
    {
        APP_ERROR_CHECK(NRF_ERROR_INTERNAL);
    }

    // Prepare ECDSA context with public key information from certificate
    mbedtls_ecdsa_init(&ecdsa_context);
    ecp_keypair = (mbedtls_ecp_keypair *) x509_certificate.pk.pk_ctx;
    if (mbedtls_ecdsa_from_keypair(&ecdsa_context, ecp_keypair) != 0)
    {
        APP_ERROR_CHECK(NRF_ERROR_INTERNAL);
    }

    // Calculate the SHA-256 hash digest of the to-be-signed data
    mbedtls_sha256(data, data_len, digest, 0);

    // Do the signature verification using context, hash, and signature
    if ((result = mbedtls_ecdsa_read_signature(&ecdsa_context, digest, sizeof(digest), sig, sig_len)) != 0)
    {
        mbedtls_x509_crt_free(&x509_certificate);
        mbedtls_ecdsa_free(&ecdsa_context);
        return -1; // Error: signature invalid
    }
    else
    {
        mbedtls_x509_crt_free(&x509_certificate);
        mbedtls_ecdsa_free(&ecdsa_context);
        return 0; // Signature successfully verified
    }
}

/** @} */
