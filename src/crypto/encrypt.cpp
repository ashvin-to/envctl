#include "crypto/encrypt.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace envctl {

static std::string bytes_to_hex(const unsigned char* data, size_t len) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) ss << std::setw(2) << (int)data[i];
    return ss.str();
}

static std::string hex_to_bytes(const std::string& hex) {
    std::string result;
    for (size_t i = 0; i < hex.size(); i += 2) {
        result += (char)std::stoi(hex.substr(i, 2), nullptr, 16);
    }
    return result;
}

std::string Crypto::derive_key(const std::string& passphrase) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)passphrase.c_str(), passphrase.size(), hash);
    return std::string((char*)hash, SHA256_DIGEST_LENGTH);
}

std::string Crypto::get_machine_key() {
    std::string seed = "envctl-secret-key-v1";
    const char* host = std::getenv("HOSTNAME");
    if (!host) host = std::getenv("HOST");
    if (!host) host = "default";
    seed += std::string(":") + host;
    return derive_key(seed);
}

std::string Crypto::encrypt(const std::string& plaintext, const std::string& key) {
    std::string derived = derive_key(key);

    unsigned char iv[16];
    RAND_bytes(iv, 16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    std::string ciphertext;
    unsigned char out_buf[1024];
    int out_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                       (const unsigned char*)derived.c_str(), iv);
    EVP_EncryptUpdate(ctx, out_buf, &out_len,
                      (const unsigned char*)plaintext.c_str(), plaintext.size());
    ciphertext.append((char*)out_buf, out_len);

    EVP_EncryptFinal_ex(ctx, out_buf, &out_len);
    ciphertext.append((char*)out_buf, out_len);
    EVP_CIPHER_CTX_free(ctx);

    std::string iv_hex = bytes_to_hex(iv, 16);
    std::string ct_hex = bytes_to_hex((const unsigned char*)ciphertext.data(), ciphertext.size());
    return iv_hex + ":" + ct_hex;
}

std::string Crypto::decrypt(const std::string& ciphertext, const std::string& key) {
    auto colon = ciphertext.find(':');
    if (colon == std::string::npos) throw std::runtime_error("Invalid ciphertext format");

    std::string iv_hex = ciphertext.substr(0, colon);
    std::string ct_hex = ciphertext.substr(colon + 1);
    std::string iv_str = hex_to_bytes(iv_hex);
    std::string ct_str = hex_to_bytes(ct_hex);

    std::string derived = derive_key(key);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char out_buf[1024];
    int out_len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                       (const unsigned char*)derived.c_str(),
                       (const unsigned char*)iv_str.c_str());
    EVP_DecryptUpdate(ctx, out_buf, &out_len,
                      (const unsigned char*)ct_str.c_str(), ct_str.size());

    std::string result((char*)out_buf, out_len);
    EVP_DecryptFinal_ex(ctx, out_buf, &out_len);
    result.append((char*)out_buf, out_len);
    EVP_CIPHER_CTX_free(ctx);

    return result;
}

} // namespace envctl
