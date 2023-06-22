#include <iostream>
#include <tchar.h>
#include <cstring>

#include "definition.h"
#include "Login.h"
#include "AccessControlClient.h"
#include "LgVideoChatDemo.h"

extern void SHA256Hash(const TCHAR* input, size_t inputLength, char* output);

#define BUTTON_JOINUS 400
const int maxLength = 255;

HWND hwndReRegisterPassword, hwndReRegisterConfirmPassword, hwndReJoinUs;

bool checkPasswordRuleR(HWND hwnd, TCHAR* Passwd, unsigned int maxLength);
bool checkConfirmPasswdR(HWND hwnd, const TCHAR* passwd, size_t passwdSize, TCHAR* confirmPasswd, size_t confirmPasswdSize);

//int exchangeTCHARToChar(TCHAR* target, char* outbuf)
//{
//    int nLength = WideCharToMultiByte(CP_ACP, 0, target, -1, NULL, 0, NULL, NULL);
//    WideCharToMultiByte(CP_ACP, 0, target, -1, outbuf, nLength, NULL, NULL);
//
//    return nLength;
//}


bool checkEmptyFieldR(HWND hwnd, const TCHAR* itemName, TCHAR* itemData, size_t itemDataSize)
{
    if (_tcsncmp(itemData, TEXT(""), itemDataSize) == 0)
    {
        TCHAR message[256] = TEXT("");
        _stprintf_s(message, _countof(message), TEXT("%s is empty. Please fill it."), itemName);
        MessageBox(hwnd, message, TEXT("Warning"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    return true;
}

LRESULT CALLBACK ReRegisterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case BUTTON_JOINUS:
        {
            TCHAR Passwd[maxLength] = TEXT("");
            TCHAR ConfirmPasswd[maxLength] = TEXT("");
            //TCHAR ContactID[maxLength] = TEXT("");

            GetWindowText(hwndReRegisterPassword, Passwd, maxLength);
            GetWindowText(hwndReRegisterConfirmPassword, ConfirmPasswd, maxLength);
       
            /* empty field */
            if (checkEmptyFieldR(hwnd, TEXT("password"), Passwd, _tcslen(Passwd)) == false) break;
            if (checkEmptyFieldR(hwnd, TEXT("confirm password"), ConfirmPasswd, _tcslen(ConfirmPasswd)) == false) break;

            /* check password rule */
            if (checkPasswordRuleR(hwnd, Passwd, sizeof(Passwd)) == false) break;

            /* check confirm password */
            if (checkConfirmPasswdR(hwnd, Passwd, _tcslen(Passwd), ConfirmPasswd, _tcslen(ConfirmPasswd)) == false) break;

            // printf("ID : %ls\nPASSWD : %ls\nConfirmPasswd : %ls\nFirstName : %ls\nLastName : %ls\nAddress : %ls\n", Email, Passwd, ConfirmPasswd, FirstName, LastName, Address);
            TReRegistration* msg = (TReRegistration*)std::malloc(sizeof(TReRegistration));
            if (msg != NULL)
            {
                std::cout << " Re-Registration process - myID : " << MyID << std::endl;
                msg->MessageType = ReRegPasswordRequest;

                strcpy_s(msg->email, MyID);
                memset(msg->password, 0, 256);

                char PasswdHash[65];

                SHA256Hash(Passwd, _tcslen(Passwd), PasswdHash);
                std::cout << "SHA-256 : " << PasswdHash << std::endl;

                msg->PasswordSize = 64;
                memcpy(msg->password, PasswdHash, 64);

                sendMsgtoACS((char*)msg, sizeof(TReRegistration));
                free(msg);
                DestroyWindow(hwnd);
            }
            else
            {
                std::cout << "exception for malloc error" << std::endl;
            }

            break;
        }
        default:
            break;
        }
        break;
    }
    case WM_DESTROY:
    {
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_CLOSE:
    {
        DestroyWindow(hwnd);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

bool checkPasswordRuleR(HWND hwnd, TCHAR* Passwd, unsigned int maxLength)
{
    int nLength = WideCharToMultiByte(CP_ACP, 0, Passwd, -1, NULL, 0, NULL, NULL);

    bool numExist = false;
    bool symExist = false;

    char* outbuf = new char[nLength + 1];
    memset(outbuf, 0, nLength + 1);

    WideCharToMultiByte(CP_ACP, 0, Passwd, -1, outbuf, nLength, NULL, NULL);

    for (int i = 0; i < nLength; i++)
    {
        if (outbuf[i] >= 48 && outbuf[i] <= 57)
            numExist = true;
        if (outbuf[i] >= 33 && outbuf[i] <= 47)
            symExist = true;
        if (outbuf[i] >= 58 && outbuf[i] <= 64)
            symExist = true;
        if (outbuf[i] >= 91 && outbuf[i] <= 96)
            symExist = true;
        if (outbuf[i] >= 123 && outbuf[i] <= 126)
            symExist = true;
    }

    delete[] outbuf;

    if (nLength < 11)
    {
        MessageBox(hwnd, TEXT("Lenth of password should over 10 characters"), TEXT("Warning"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    if (numExist == false)
    {
        MessageBox(hwnd, TEXT("Password should contain 1 number"), TEXT("Warning"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    if (symExist == false)
    {
        MessageBox(hwnd, TEXT("Lenth of password should over 1 symbol"), TEXT("Warning"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    return true;
}

bool checkConfirmPasswdR(HWND hwnd, const TCHAR* passwd, size_t passwdSize, TCHAR* confirmPasswd, size_t confirmPasswdSize)
{
    if (passwdSize == confirmPasswdSize && _tcsncmp(passwd, confirmPasswd, passwdSize) == 0)
    {
        return true;
    }

    MessageBox(hwnd, TEXT("Confirm password is not same as password."), TEXT("Warning"), MB_OK | MB_ICONEXCLAMATION);
    return false;
}


void ReRegisterCreateForm(HWND parentHwnd)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = ReRegisterProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ReRegisterWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"ReRegisterWindowClass",
        L"Exchange Password",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 560,
        parentHwnd, NULL, wc.hInstance, NULL
    );

    //HWND hwndRegisterEmailLabel = CreateWindowEx(
    //    0,
    //    L"STATIC",
    //    L"Email",
    //    WS_VISIBLE | WS_CHILD,
    //    20, 20, 300, 20,
    //    hwnd, NULL, NULL, NULL
    //);

    //hwndRegisterEmail = CreateWindowEx(
    //    WS_EX_CLIENTEDGE,
    //    L"EDIT",
    //    L"",
    //    WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
    //    20, 50, 300, 20,
    //    hwnd, NULL, NULL, NULL
    //);

    HWND hwndReRegisterPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Password",
        WS_VISIBLE | WS_CHILD,
        20, 80, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndReRegisterPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        20, 110, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndReRegisterConfirmPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Confirm Password",
        WS_VISIBLE | WS_CHILD,
        20, 140, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndReRegisterConfirmPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        20, 170, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    /*HWND hwndRegisterFirstNameLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"First Name",
        WS_VISIBLE | WS_CHILD,
        20, 200, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterFirstName = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 230, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndRegisterLastNameLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Last Name",
        WS_VISIBLE | WS_CHILD,
        20, 260, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterLastName = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 290, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndRegisterAddressLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Address",
        WS_VISIBLE | WS_CHILD,
        20, 320, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterAddress = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 350, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndRegisterCIDLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"ContactID",
        WS_VISIBLE | WS_CHILD,
        20, 380, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterCID = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 410, 300, 20,
        hwnd, NULL, NULL, NULL
    );*/

    hwndReJoinUs = CreateWindowEx(
        0,
        L"BUTTON",
        L"Exchange",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 440, 300, 30,
        hwnd, (HMENU)BUTTON_JOINUS, NULL, NULL
    );
    // Show the main window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
}
