#define MBEDTLS_CONFIG_FILE "mbedtls/esp_config.h"

#include <mbedtls/aes.h>
#include <string>
#include <string.h>
#include <sstream>
#include <byteswap.h>
#include <cmath>
// #include <Arduino.h>
#include <esp_log.h>

using std::string;

// -----------------------------------------------------------------------------
// --[string_to_hex]------------------------------------------------------------
// -----------------------------------------------------------------------------
std::string string_to_hex(const std::string &input) {
    static const char *const lut = "0123456789ABCDEF";
    size_t len = input.length();
    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

// -----------------------------------------------------------------------------
// --[hexstring_to_string]------------------------------------------------------
// -----------------------------------------------------------------------------
std::string hexstring_to_string(const std::string& hex_chars) {
    std::string bytes;
    // must be int for correct overload!
    unsigned int c;
    for (int i = 0; i < hex_chars.length(); i+=2) {
        std::istringstream hex_chars_stream(hex_chars.substr(i,2));
        hex_chars_stream >> std::hex >> c;
        bytes.append(1, (char) c);
    }
    return bytes;
}

// -----------------------------------------------------------------------------
// --[encrypt_aes_ecb]----------------------------------------------------------
// -----------------------------------------------------------------------------
std::string encrypt_aes_ecb(std::string &data, std::string &key) { // input should be padded array
    if(key.length() != 16 || (data.length() % 16) != 0) {
        ESP_LOGD("eqiva_key_ble", "# Authentifizierungswert Fehler %d | %d", key.length(), (data.length() % 16));
        return data;
    }
   // esp_aes_acquire_hardware();
   // esp_aes_context context;
   // esp_aes_init(&context);
    mbedtls_aes_context context;
    mbedtls_aes_init( &context );

    unsigned char *aeskey = (unsigned char *) key.c_str();
    // esp_aes_setkey(&context, aeskey, 128);
    mbedtls_aes_setkey_enc(&context, aeskey, 128);
    unsigned char input[16];
    unsigned char output[16];
    std::stringstream output_data;
    for (int i = 0; i < data.length(); i += 16) {
        std::string inp = data.substr(i, 16);
        memcpy(input, inp.c_str(), 16);
        // esp_aes_crypt_ecb(&context, ESP_AES_ENCRYPT, input, output);
        mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, input, output);
        output_data.write((char *) output, 16);
    }
    // esp_aes_free(&context);
    // esp_aes_release_hardware();
    mbedtls_aes_free(&context);
    return output_data.str();
}

// ------------------------------------------a-----------------------------------
// --[xor_array]----------------------------------------------------------------
// -----------------------------------------------------------------------------
std::string xor_array(std::string data, std::string xor_array, int offset = 0) {
    for (int i = 0; i < data.length(); i++)
        data[i] = data[i] ^ xor_array[(offset + i) % xor_array.length()];
    return data;
}

// -----------------------------------------------------------------------------
// --[compute_nonce]------------------------------------------------------------
// -----------------------------------------------------------------------------
std::string compute_nonce(char msg_type_id, std::string session_open_nonce, uint16_t security_counter) {
    std::stringstream ss;
    ss.put(msg_type_id);
    ss << session_open_nonce;
    ss.put(0);
    ss.put(0);
    ss.put((char) (security_counter >> 8));
    ss.put((char) security_counter);
    return ss.str();
}

// -----------------------------------------------------------------------------
// --[generic_ceil]-------------------------------------------------------------
// -----------------------------------------------------------------------------
int generic_ceil(int value, int step, int minimum) {
    return (int) ((std::ceil((value - minimum) / step) * step) + minimum);
}

// -----------------------------------------------------------------------------
// --[compute_auth_value]-------------------------------------------------------
// -----------------------------------------------------------------------------
std::string compute_auth_value(std::string data, char msg_type_id, std::string session_open_nonce, uint16_t security_counter, std::string key) {
    ESP_LOGD("eqiva_key_ble", "# Authentifizierungswert berechnen...");
    if(key.length() != 16) {
        ESP_LOGD("eqiva_key_ble", "# Authentifizierungswert Fehler %d", key.length());
    }
    std::string nonce = compute_nonce(msg_type_id, session_open_nonce, security_counter);
    size_t data_length = data.length(); // original data length
    if (data.length() % 16 > 0)
        data.append(16 - (data.length() % 16), 0);
    std::string encrypted_xor_data = "";
    encrypted_xor_data.append(1, 9);
    encrypted_xor_data.append(nonce);
    encrypted_xor_data.append(1, (char) (data_length >> 8));
    encrypted_xor_data.append(1, (char) data_length);
    encrypted_xor_data = encrypt_aes_ecb(encrypted_xor_data, key);
    for (int offset = 0; offset < data.length(); offset += 0x10) {
        std::string xored = xor_array(encrypted_xor_data, data, offset);
        encrypted_xor_data = encrypt_aes_ecb(xored, key);
    }
    std::string extra = "";
    extra.append(1, 1);
    extra.append(nonce);
    extra.append(2, 0);
    std::string ret = xor_array(encrypted_xor_data.substr(0, 4), encrypt_aes_ecb(extra, key));
    ESP_LOGD("eqiva_key_ble", "# fertig...");
    return ret;
}

// -----------------------------------------------------------------------------
// --[crypt_data]---------------------------------------------------------------
// -----------------------------------------------------------------------------
std::string crypt_data(std::string data, char msg_type_id, std::string session_open_nonce, uint16_t security_counter, std::string key) {
    ESP_LOGD("eqiva_key_ble", "# Cryptdata length: %d", data.length());
    if (key.length() != 16 || session_open_nonce.length() != 8) {
        ESP_LOGE("eqiva_key_ble", "# Length error, %d | %d", key.length(), session_open_nonce.length());\
        return data;
    }
    //assert(key.length() == 16);
    //assert(session_open_nonce.length() == 8);
    // is this not basically AES-CTR?
    std::string nonce = compute_nonce(msg_type_id, session_open_nonce, security_counter);
    std::string xor_data;
    int len = data.length();
    if (len % 16 > 0)
        len += 16 - (len % 16);
    len = len / 16;
    for (int i = 0; i < len; i++) {
        std::string to_encrypt = "";
        to_encrypt.append(1, 1);
        to_encrypt.append(nonce);
        to_encrypt.append(1, (char) ((i + 1) >> 8));
        to_encrypt.append(1, (char) (i + 1));
        xor_data.append(encrypt_aes_ecb(to_encrypt, key));
    }
    std::string ret = xor_array(data, xor_data);
    return ret;
}