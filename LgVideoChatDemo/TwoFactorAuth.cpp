#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <tuple>
#include <algorithm>
#include <cstring>

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include "AccessControlServer.h"
#include "TwoFactorAuth.h"
#include "Logger.h"

#pragma comment(lib, "wininet.lib")

namespace ch = std::chrono;

static const char dataset[] =
"0123456789"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";
char random_token[RANDOM_LENTH];
ch::time_point<ch::high_resolution_clock> start;
ch::time_point<ch::high_resolution_clock> end;

const char* admin_id = " lgss23.team2";
const char* admin_passwd = "bbmugvpajgoanzbu";
static std::vector<std::pair<std::string, std::string>> tokenMng;

void MakeRandomToken()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    memset(random_token, NULL, RANDOM_LENTH);
    std::uniform_int_distribution<int> dist(0, strlen(dataset) - 1);
    for (int i = 0; i < RANDOM_LENTH - 1; i++) {
        random_token[i] = dataset[dist(gen)];
    }
    //std::cout << random_token << std::endl;
}

int CheckToken(char* receiver, char* input_token, int sec)
{
    char recv_email[256];
    strcpy_s((char*)recv_email, 256, searchConnectID(receiver));

    for (auto iter = tokenMng.begin(); iter != tokenMng.end(); iter++)
    {
        if (!strcmp(((*iter).first).c_str(), recv_email))
        {
            //std::cout << "Token : " << ((*iter).second).c_str() << " Recv : " << input_token << std::endl;
            if (sec >= 0 && sec <= 120)
            {
                if (!strcmp(((*iter).second).c_str(), input_token))
                {
                    tokenMng.erase(iter);
                    return TFA_SUCCESS;
                }
                else
                {
                    return TFA_FAIL_WRONG;
                }
            }
            else
            {
                std::cout << "Timeout!!!!!!!! " << std::endl;
                return TFA_FAIL_TIME_OUT;
            }
        }
    }
    return TFA_FAIL_WRONG;
}

void TFAProcess(const char* receiver)
{
    const char* smtp_server = "smtp.gmail.com";
    const char* body_tokne = random_token;

    int smtp_server_port = 587;
    int send_status = 0;

    HINTERNET hInternet = InternetOpenA("Email", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet)
    {
        HINTERNET hConnect = InternetOpenUrlA(hInternet, "https://www.google.com/", NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hConnect)
        {
            char headers[1024];
            DWORD dwLength = sizeof(headers);
            if (HttpQueryInfoA(hConnect, HTTP_QUERY_RAW_HEADERS_CRLF, headers, &dwLength, NULL))
            {
                // Do something with the headers
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }

    //std::string command = "powershell.exe -ExecutionPolicy Bypass -Command \"Send-MailMessage -To '" + std::string(reciver) + "' -From '" + admin_id + "@gmail.com' -Subject 'Two-factor authentication' -Body 'Token : " + body_tokne + "' -SmtpServer '" + smtp_server + "' -Port 587 -UseSsl -Credential (New-Object System.Management.Automation.PSCredential -ArgumentList '" + admin_id + "@gmail.com', (ConvertTo-SecureString -String '" + admin_passwd + "' - AsPlainText - Force))" + "\"";
    std::string command = "powershell.exe -ExecutionPolicy Bypass -Command \"Send-MailMessage -To '";
    command += std::string(receiver);
    command += "' -From '";
    command += std::string(admin_id);
    command += "@gmail.com' -Subject 'Two-factor authentication' -Body 'Token : ";
    command += std::string(body_tokne);
    command += "' -SmtpServer '";
    command += std::string(smtp_server);
    command += "' -Port 587 -UseSsl -Credential (New-Object System.Management.Automation.PSCredential -ArgumentList '";
    command += std::string(admin_id);
    command += "@gmail.com', (ConvertTo-SecureString -String '";
    command += std::string(admin_passwd);
    command += "' -AsPlainText -Force))\"";

    //std::cout << command.c_str() << std::endl;

    int ret = system(command.c_str());
    if (ret)
        LOG("failed to send TFA");
    else
    {
        LOG("succeded to send TFA");
        auto input = std::make_pair(std::string(receiver), std::string(random_token));
        tokenMng.push_back(input);
    }
}

void SendTFA(char* reciver)
{
    memset(random_token, NULL, RANDOM_LENTH);
    MakeRandomToken();
    TFAProcess(reciver);
    start = ch::high_resolution_clock::now();
}

int ReadTFA(char* receiver, char* input_token)
{
    int sec;
    int status = 0;
    ch::time_point<ch::high_resolution_clock> end = ch::high_resolution_clock::now();
    auto diff = end - start;
    sec = ch::duration_cast<ch::seconds>(diff).count();
    status = CheckToken(receiver,input_token, sec);
    return status;
}