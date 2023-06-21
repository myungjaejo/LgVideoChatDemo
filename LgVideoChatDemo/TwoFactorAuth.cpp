#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include <cstring>

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include "TwoFactorAuth.h"

#pragma comment(lib, "wininet.lib")

namespace ch = std::chrono;

static const char dataset[] =
"0123456789"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";
char random_token[RANDOM_LENTH];
ch::time_point<ch::high_resolution_clock> start;
ch::time_point<ch::high_resolution_clock> end;


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

int CheckToken(char* input_token, int sec)
{
    if (sec >= 0 && sec <= 30)
    {
        if (strcmp(random_token, input_token) == 0)
        {
            return TFA_SUCCESS;
        }
        else
        {
            return TFA_FAIL_WRONG;

        }
    }
    else
    {
        return TFA_FAIL_TIME_OUT;
    }
}

void TFAProcess(const char* reciver)
{
    const char* admin_id = "myungjae.jo";
    const char* admin_passwd = "tismcfzrjuvoadww";
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

    std::string command = "powershell.exe -ExecutionPolicy Bypass -Command \"Send-MailMessage -To '" + std::string(reciver) + "' -From '" + admin_id + "@gmail.com' -Subject 'Two-factor authentication' -Body 'Token : " + body_tokne + "'\"";
    system(command.c_str());
}

void SendTFA(char* reciver)
{
    memset(random_token, NULL, RANDOM_LENTH);
    MakeRandomToken();
    TFAProcess(reciver);
    start = ch::high_resolution_clock::now();
}

int ReadTFA(char* input_token)
{
    int sec;
    int status = 0;
    ch::time_point<ch::high_resolution_clock> end = ch::high_resolution_clock::now();
    auto diff = end - start;
    sec = ch::duration_cast<ch::seconds>(diff).count();
    status = CheckToken(input_token, sec);
    return status;
}