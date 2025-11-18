#pragma once

#include <string>

class AesCrypto {
public:
    static bool LoadKey(const std::string& keyHex32); 
    static std::string Encrypt(const std::string& data);
    static std::string Decrypt(const std::string& hexCipher);

private:
    static unsigned char key[16];
    static unsigned char extKey[176];
    static bool keyLoaded;
    static void extendKey();
};