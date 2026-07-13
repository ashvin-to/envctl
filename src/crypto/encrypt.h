#pragma once
#include <string>

namespace envctl {

class Crypto {
public:
    static std::string encrypt(const std::string& plaintext, const std::string& key);
    static std::string decrypt(const std::string& ciphertext, const std::string& key);
    static std::string derive_key(const std::string& passphrase);
    static std::string get_machine_key();
};

} // namespace envctl
