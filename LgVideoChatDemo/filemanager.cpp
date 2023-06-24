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

#include <openssl/aes.h>
#include <openssl/rand.h>
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

#if 0
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
#endif

std::string aes256cbc_encrypt(const std::string& plainText, const std::string& keyStr, const std::string& ivStr) {
    std::string cipherText;

    // ���� unsigned char �迭�� ��ȯ
    std::vector<unsigned char> plainTextData(plainText.begin(), plainText.end());

    // Ű�� IV�� unsigned char �迭�� ��ȯ
    std::vector<unsigned char> key(keyStr.begin(), keyStr.end());
    std::vector<unsigned char> iv(ivStr.begin(), ivStr.end());

    // ��ȣȭ ���ؽ�Ʈ ����
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create encryption context." << std::endl;
        return cipherText;
    }

    // ��ȣȭ �ʱ�ȭ
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, nullptr, nullptr) != 1) {
        std::cerr << "Failed to initialize encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return cipherText;
    }

    // Ű�� IV ����
    if (EVP_CIPHER_CTX_set_key_length(ctx, AES_KEY_SIZE) != 1) {
        std::cerr << "Failed to set key length." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return cipherText;
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        std::cerr << "Failed to set key and IV." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return cipherText;
    }

    // ��ȣȭ ���� ũ�� ���
    int cipherTextLen = plainTextData.size() + EVP_CIPHER_CTX_block_size(ctx);
    cipherText.resize(cipherTextLen);

    // ��ȣȭ ����
    if (EVP_EncryptUpdate(ctx, (unsigned char*)(cipherText.data()), &cipherTextLen, plainTextData.data(), plainTextData.size()) != 1) {
        std::cerr << "Failed to perform encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return cipherText;
    }

    // ��ȣȭ ������
    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx, (unsigned char*)(cipherText.data()) + cipherTextLen, &finalLen) != 1) {
        std::cerr << "Failed to finalize encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return cipherText;
    }

    // ��ȣȭ�� ������ ���� ����
    cipherTextLen += finalLen;
    cipherText.resize(cipherTextLen);

    // ��ȣȭ ���ؽ�Ʈ ����
    EVP_CIPHER_CTX_free(ctx);

    return cipherText;
}

std::string aes256cbc_decrypt(const std::string& cipherText, const std::string& keyStr, const std::string& ivStr) {
    std::string plainText;

    // ��ȣ���� unsigned char �迭�� ��ȯ
    std::vector<unsigned char> cipherTextData(cipherText.begin(), cipherText.end());

    // Ű�� IV�� unsigned char �迭�� ��ȯ
    std::vector<unsigned char> key(keyStr.begin(), keyStr.end());
    std::vector<unsigned char> iv(ivStr.begin(), ivStr.end());

    // ��ȣȭ ���ؽ�Ʈ ����
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create decryption context." << std::endl;
        return plainText;
    }

    // ��ȣȭ �ʱ�ȭ
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, nullptr, nullptr) != 1) {
        std::cerr << "Failed to initialize decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return plainText;
    }

    // Ű�� IV ����
    if (EVP_CIPHER_CTX_set_key_length(ctx, AES_KEY_SIZE) != 1) {
        std::cerr << "Failed to set key length." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return plainText;
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        std::cerr << "Failed to set key and IV." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return plainText;
    }

    // ��ȣȭ ���� ũ�� ���
    int plainTextLen = cipherTextData.size();
    plainText.resize(plainTextLen);

    // ��ȣȭ ����
    if (EVP_DecryptUpdate(ctx, (unsigned char*)(plainText.data()), &plainTextLen, cipherTextData.data(), cipherTextData.size()) != 1) {
        std::cerr << "Failed to perform decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return plainText;
    }

    // ��ȣȭ ������
    int finalLen = 0;
    if (EVP_DecryptFinal_ex(ctx, (unsigned char*)(plainText.data()) + plainTextLen, &finalLen) != 1) {
        std::cerr << "Failed to finalize decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return plainText;
    }

    // ��ȣȭ�� ������ ���� ����
    plainTextLen += finalLen;
    plainText.resize(plainTextLen);

    // ��ȣȭ ���ؽ�Ʈ ����
    EVP_CIPHER_CTX_free(ctx);

    return plainText;
}

#if 0
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
#endif

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
    // ��ȣȭ�� ������ ���� �б�
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

    // ��ȣȭ�� ������ ��ȣȭ �ϱ�
    std::string jsonStrEnc = buffer.str();
    std::string jsonStr = aes256cbc_decrypt(jsonStrEnc, key, iv);

    // Ȥȣȭ�� string�� json���� ��ȯ
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

/*
void test_filemanager()
{
    TRegistration data[5];
    memset(data, 0, sizeof(data));

    strcpy_s(data[0].email, "user0@email.net");
    strcpy_s(data[0].ContactID, "nick0");
    strcpy_s(data[0].password, "password0");
    strcpy_s(data[0].lastName, "lastname0");
    strcpy_s(data[0].firstName, "firstname0");
    strcpy_s(data[0].Address, "address0");

    strcpy_s(data[1].email, "user1@email.net");
    strcpy_s(data[1].ContactID, "nick1");
    strcpy_s(data[1].password, "password1");
    strcpy_s(data[1].lastName, "lastname1");
    strcpy_s(data[1].firstName, "firstname1");
    strcpy_s(data[1].Address, "address1");

    strcpy_s(data[2].email, "user2@email.net");
    strcpy_s(data[2].ContactID, "nick2");
    strcpy_s(data[2].password, "password2");
    strcpy_s(data[2].lastName, "lastname2");
    strcpy_s(data[2].firstName, "firstname2");
    strcpy_s(data[2].Address, "address2");

    strcpy_s(data[3].email, "user3@email.net");
    strcpy_s(data[3].ContactID, "nick3");
    strcpy_s(data[3].password, "password3");
    strcpy_s(data[3].lastName, "lastname3");
    strcpy_s(data[3].firstName, "firstname3");
    strcpy_s(data[3].Address, "address3");

    strcpy_s(data[4].email, "user4@email.net");
    strcpy_s(data[4].ContactID, "nick4");
    strcpy_s(data[4].password, "password4");
    strcpy_s(data[4].lastName, "lastname4");
    strcpy_s(data[4].firstName, "firstname4");
    strcpy_s(data[4].Address, "address4");

    //StoreData(data, 5);

    std::vector<TRegistration*> data_vec;
    for (int i = 0; i < 5; i++)
        data_vec.push_back(&data[i]);

    StoreData(data_vec);

    TRegistration read_data;
    if (LoadData(&read_data, 0))
        printFileObj(&read_data);
    if (LoadData(&read_data, 1))
        printFileObj(&read_data);
    if (LoadData(&read_data, 2))
        printFileObj(&read_data);
    if (LoadData(&read_data, 3))
        printFileObj(&read_data);
    if (LoadData(&read_data, 4))
        printFileObj(&read_data);
    if (LoadData(&read_data, 5))
        printFileObj(&read_data);
}
*/

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