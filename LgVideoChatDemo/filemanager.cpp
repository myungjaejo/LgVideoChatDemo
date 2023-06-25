#include "filemanager.h"

#include "json/json.h"
#include <fstream>
#include <iostream>


#define DATA_FILE_NAME "data.dat"

#define STR_EMAIL       "email"
#define STR_CONTACT_ID  "contactID"
#define STR_PASSWORD    "password"
#define STR_LAST_NAME   "lastName"
#define STR_FIRST_NAME  "firstName"
#define STR_ADDRESS     "address"
#define STR_IPADDRESS   "IP"
#define STR_LASTACCESS  "lastAccess"

#include "include\openssl\aes.h"
#include "include\openssl\rand.h"
const int AES_KEY_SIZE = 32;
const int AES_IV_SIZE = 16;

std::string key("1234567890123456789012345678901");
std::string iv("123456789012345");

std::string generateRandomBytes(int size)
{
    std::string randomBytes(size, '\0');
    RAND_bytes(reinterpret_cast<unsigned char*>(&randomBytes[0]), size);
    return randomBytes;
}

std::string aes256cbc_encrypt(const std::string& plainText, const std::string& key, const std::string& iv)
{
    std::string cipherText;
    AES_KEY aesKey;
    AES_set_encrypt_key(reinterpret_cast<const unsigned char*>(key.c_str()), AES_KEY_SIZE * 8, &aesKey);
    unsigned char initializationVector[AES_BLOCK_SIZE];
    memcpy(initializationVector, iv.c_str(), AES_IV_SIZE);
    int paddedLength = (plainText.length() / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
    unsigned char* paddedPlainText = new unsigned char[paddedLength];
    memcpy(paddedPlainText, plainText.c_str(), plainText.length());
    memset(paddedPlainText + plainText.length(), paddedLength - plainText.length(), paddedLength - plainText.length());
    for (int i = 0; i < paddedLength; i += AES_BLOCK_SIZE)
    {
        unsigned char encryptedBlock[AES_BLOCK_SIZE];
        AES_cbc_encrypt(paddedPlainText + i, encryptedBlock, AES_BLOCK_SIZE, &aesKey, initializationVector, AES_ENCRYPT);
        cipherText.append(reinterpret_cast<char*>(encryptedBlock), AES_BLOCK_SIZE);
    }
    delete[] paddedPlainText;
    return cipherText;
}

std::string aes256cbc_decrypt(const std::string& cipherText, const std::string& key, const std::string& iv)
{
    std::string decryptedText;
    AES_KEY aesKey;
    AES_set_decrypt_key(reinterpret_cast<const unsigned char*>(key.c_str()), AES_KEY_SIZE * 8, &aesKey);
    unsigned char initializationVector[AES_BLOCK_SIZE];
    memcpy(initializationVector, iv.c_str(), AES_IV_SIZE);
    std::cout << "cipherText.length()" << cipherText.length() << std::endl;
    for (int i = 0; i < cipherText.length(); i += AES_BLOCK_SIZE)
    {
        unsigned char decryptedBlock[AES_BLOCK_SIZE];
        AES_cbc_encrypt(reinterpret_cast<const unsigned char*>(cipherText.c_str() + i), decryptedBlock, AES_BLOCK_SIZE, &aesKey, initializationVector, AES_DECRYPT);
        decryptedText.append(reinterpret_cast<char*>(decryptedBlock), AES_BLOCK_SIZE);
    }
    int paddingLength = decryptedText[decryptedText.length() - 1];
    decryptedText.erase(decryptedText.length() - paddingLength);
    return decryptedText;
}

void save_encrypted_json(const Json::Value& root, const std::string& filename) {
    // JsonCpp�� JValue�� ���ڿ��� ��ȯ
    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, root);

    // ��ȣȭ
    std::string jsonStrEnc = aes256cbc_encrypt(jsonStr, key, iv);

    // ��ȣȭ�� ������ ���Ͽ� ����
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return;
    }
    file << jsonStrEnc;
    file.close();

    std::cout << "Encrypted data saved to " << filename << std::endl;
}

Json::Value load_encrypted_json(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading." << std::endl;
        return Json::Value();
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string jsonStrEnc = buffer.str();
    std::string jsonStr = aes256cbc_decrypt(jsonStrEnc, key, iv);

    Json::Value root(jsonStr);
    Json::Reader reader;
    bool ret = reader.parse(jsonStr, root);

    std::cout << "load encrypted json: " << std::endl;
    std::cout << root << std::endl;
    return root;
}

bool StoreData(std::vector<TRegistration*> data)
{
    Json::Value root;

    std::vector< TRegistration* >::iterator iter;
    for (iter = data.begin(); iter != data.end(); iter++)
    {
        Json::Value user;
        auto fdata = *iter;
        user[STR_EMAIL] = fdata->email;
        user[STR_CONTACT_ID] = fdata->ContactID;
        user[STR_PASSWORD] = fdata->password;
        user[STR_LAST_NAME] = fdata->lastName;
        user[STR_FIRST_NAME] = fdata->firstName;
        user[STR_ADDRESS] = fdata->Address;
        user[STR_IPADDRESS] = fdata->LastIPAddress;
        user[STR_LASTACCESS] = fdata->LastRegistTime;

        root.append(user);
    }

    Json::StyledWriter writer;
    std::string outputConfig = writer.write(root);

    save_encrypted_json(root, std::string(DATA_FILE_NAME));

    return true;
}

bool StoreData(const TRegistration* data, size_t size)
{
    Json::Value root;

    for (unsigned int i = 0; i < size; i++)
    {
        Json::Value user;
        user[STR_EMAIL] = data[i].email;
        user[STR_CONTACT_ID] = data[i].ContactID;
        user[STR_PASSWORD] = data[i].password;
        user[STR_LAST_NAME] = data[i].lastName;
        user[STR_FIRST_NAME] = data[i].firstName;
        user[STR_ADDRESS] = data[i].Address;
        root.append(user);
    }

    Json::StyledWriter writer;
    std::string outputConfig = writer.write(root);

    save_encrypted_json(root, std::string(DATA_FILE_NAME));
    return true;
}

bool LoadData(TRegistration* data, int idx)
{
    Json::Value root = load_encrypted_json(std::string(DATA_FILE_NAME));

    if (root.size() <= idx)
        return false;

    Json::Value user = root[idx];

    strncpy_s(data->email, user[STR_EMAIL].asString().c_str(), 128);
    strncpy_s(data->ContactID, user[STR_CONTACT_ID].asString().c_str(), 128);
    strncpy_s(data->password, user[STR_PASSWORD].asString().c_str(), 128);
    strncpy_s(data->lastName, user[STR_LAST_NAME].asString().c_str(), 128);
    strncpy_s(data->firstName, user[STR_FIRST_NAME].asString().c_str(), 128);
    strncpy_s(data->Address, user[STR_ADDRESS].asString().c_str(), 256);
    strncpy_s(data->LastIPAddress, user[STR_IPADDRESS].asString().c_str(), 512);
    strncpy_s(data->LastRegistTime, user[STR_LASTACCESS].asString().c_str(), 512);

    return true;
}

int getLengthJSON(void)
{
    Json::Value root = load_encrypted_json(std::string(DATA_FILE_NAME));
    return root.size();
}

void printFileObj(const TRegistration* data)
{
    std::cout << "Stored Data >>>>> " << std::endl;
    std::cout << data->email << std::endl;
    std::cout << data->password << std::endl;
    std::cout << data->ContactID << std::endl;
    std::cout << data->lastName << std::endl;
    std::cout << data->firstName << std::endl;
    std::cout << data->Address << std::endl;
}