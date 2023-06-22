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


/*
// write
bool WriteToFile(const char* filename, const char* buffer, int len)
{
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "wb");

    if (fp == nullptr)
    {
        return false;
    }

    size_t fileSize = fwrite(buffer, 1, len, fp);

    fclose(fp);

    return true;
}

// read
bool ReadFromFile(const char* filename, char* buffer, int len)
{
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "rb");

    if (fp == nullptr)
    {
        return false;
    }

    size_t fileSize = fread(buffer, 1, len, fp);

    fclose(fp);

    return true;
}
*/

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

    /* todo: encryption */

    std::ofstream fout(DATA_FILE_NAME);
    fout << root;

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

    /* todo: encryption */

    std::ofstream fout(DATA_FILE_NAME);
    fout << root;

    //bool result = WriteToFile("test.json", outputConfig.c_str(), outputConfig.length());
    return true;
}

bool LoadData(TRegistration* data, int idx)
{
    std::ifstream fin(DATA_FILE_NAME);

    /* todo decryption */

    Json::Value root;
    fin >> root; //���⼭�� >> ����

    Json::Value user = root[idx];

    strncpy_s(data->email, user[STR_EMAIL].asString().c_str(), 128);
    strncpy_s(data->ContactID, user[STR_CONTACT_ID].asString().c_str(), 128);
    strncpy_s(data->password, user[STR_PASSWORD].asString().c_str(), 128);
    strncpy_s(data->lastName, user[STR_LAST_NAME].asString().c_str(), 128);
    strncpy_s(data->firstName, user[STR_FIRST_NAME].asString().c_str(), 128);
    strncpy_s(data->Address, user[STR_ADDRESS].asString().c_str(), 256);
    strncpy_s(data->LastIPAddress, user[STR_IPADDRESS].asString().c_str(), 512);
    strncpy_s(data->LastRegistTime, user[STR_LASTACCESS].asString().c_str(), 512);

    /*
    const int BufferLength = 1024;
    char readBuffer[BufferLength] = { 0, };

    if (ReadFromFile("test.json", readBuffer, BufferLength) == false)
    {
        return -1;
    }

    std::string config_doc = readBuffer;
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(config_doc, root);

    if (parsingSuccessful == false)
    {
        std::cout << "Failed to parse configuration\n" << reader.getFormattedErrorMessages();
        return -1;
    }
    */

    //for (unsigned int i = 0; i < root.size(); ++i)
    //{
    //    if (i > size) return false;

    //    Json::Value user = root[i];
  
    //    strncpy_s(data[i].email, user[STR_EMAIL].asString().c_str(), 128);
    //    strncpy_s(data[i].ContactID, user[STR_CONTACT_ID].asString().c_str(), 128);
    //    strncpy_s(data[i].password, user[STR_PASSWORD].asString().c_str(), 128);
    //    strncpy_s(data[i].lastName, user[STR_LAST_NAME].asString().c_str(), 128);
    //    strncpy_s(data[i].firstName, user[STR_FIRST_NAME].asString().c_str(), 128);
    //    strncpy_s(data[i].Address, user[STR_ADDRESS].asString().c_str(), 128);
    //}

    return true;
}

//void test_filemanager()
//{
//    TRegistration data[5];
//    memset(data, 0, sizeof(data));
//
//    strcpy_s(data[0].email, "user0@email.net");
//    strcpy_s(data[0].ContactID, "nick0");
//    strcpy_s(data[0].password, "password0");
//    strcpy_s(data[0].lastName, "lastname0");
//    strcpy_s(data[0].firstName, "firstname0");
//    strcpy_s(data[0].Address, "address0");
//
//    strcpy_s(data[1].email, "user1@email.net");
//    strcpy_s(data[1].ContactID, "nick1");
//    strcpy_s(data[1].password, "password1");
//    strcpy_s(data[1].lastName, "lastname1");
//    strcpy_s(data[1].firstName, "firstname1");
//    strcpy_s(data[1].Address, "address1");
//
//    strcpy_s(data[2].email, "user2@email.net");
//    strcpy_s(data[2].ContactID, "nick2");
//    strcpy_s(data[2].password, "password2");
//    strcpy_s(data[2].lastName, "lastname2");
//    strcpy_s(data[2].firstName, "firstname2");
//    strcpy_s(data[2].Address, "address2");
//
//    strcpy_s(data[3].email, "user3@email.net");
//    strcpy_s(data[3].ContactID, "nick3");
//    strcpy_s(data[3].password, "password3");
//    strcpy_s(data[3].lastName, "lastname3");
//    strcpy_s(data[3].firstName, "firstname3");
//    strcpy_s(data[3].Address, "address3");
//
//    strcpy_s(data[4].email, "user4@email.net");
//    strcpy_s(data[4].ContactID, "nick4");
//    strcpy_s(data[4].password, "password4");
//    strcpy_s(data[4].lastName, "lastname4");
//    strcpy_s(data[4].firstName, "firstname4");
//    strcpy_s(data[4].Address, "address4");
//
//    StoreData(data, 5);
//
//    TRegistration read_data[5];
//    memset(read_data, 0, sizeof(read_data));
//
//    LoadData(read_data, 5);
//    for (unsigned int i = 0; i < 5; ++i)
//    {
//        std::cout << read_data[i].email << std::endl;
//        std::cout << read_data[i].ContactID << std::endl;
//        std::cout << read_data[i].password << std::endl;
//        std::cout << read_data[i].lastName << std::endl;
//        std::cout << read_data[i].firstName << std::endl;
//        std::cout << read_data[i].Address << std::endl;
//    }
//}


int getLengthJSON(void)
{
    std::ifstream fin(DATA_FILE_NAME);

    if (!fin)
        return 0;

    Json::Value root;
    fin >> root; //���⼭�� >> ����

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