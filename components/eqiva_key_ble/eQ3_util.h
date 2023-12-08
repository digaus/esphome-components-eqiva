//
// Created by marius on 16.12.18.
//

#ifndef DOOR_OPENER_UTIL_H
#define DOOR_OPENER_UTIL_H

std::string string_to_hex(const std::string &input);
std::string hexstring_to_string(const std::string &hex_chars);

// input should be padded array
std::string encrypt_aes_ecb(std::string &data, std::string &key);
std::string xor_array(std::string data, std::string xor_array, int offset = 0);
std::string compute_nonce(char msg_type_id, std::string session_open_nonce, uint16_t security_counter);
int generic_ceil(int value, int step, int minimum);
std::string compute_auth_value(std::string data, char msg_type_id, std::string session_open_nonce, uint16_t security_counter, std::string key);
std::string crypt_data(std::string data, char msg_type_id, std::string session_open_nonce, uint16_t security_counter, std::string key);
uint64_t string_to_mac(std::string const& s);

#endif //DOOR_OPENER_UTIL_H