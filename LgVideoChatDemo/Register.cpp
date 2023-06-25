#include "Register.h"
#include "definition.h"
#include <iostream>
#include <tchar.h>
#include <cstring>
#include "AccessControlClient.h"
#include "Login.h"
extern void SHA256Hash(const char* input, size_t inputLength, char* output);
extern void CopyTCharToChar(TCHAR* tcharString, char* CharString, int length);

#define BUTTON_JOINUS 400
const int maxLength = 255;

HWND hwndCreateRegister, hwndRegisterEmail, hwndRegisterPassword, hwndRegisterConfirmPassword, \
hwndRegisterFirstName, hwndRegisterLastName, hwndRegisterAddress, hwndRegisterCID;

bool checkPasswordRule(HWND hwnd, TCHAR* Passwd, unsigned int maxLength);
bool checkConfirmPasswd(HWND hwnd, const TCHAR* passwd, size_t passwdSize, TCHAR* confirmPasswd, size_t confirmPasswdSize);

int exchangeTCHARToChar(TCHAR* target, char* outbuf)
{
    int nLength = WideCharToMultiByte(CP_ACP, 0, target, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, target, -1, outbuf, nLength, NULL, NULL);

    return nLength;
}


bool checkEmptyField(HWND hwnd, const TCHAR* itemName, TCHAR* itemData, size_t itemDataSize )
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

LRESULT CALLBACK RegisterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
                TCHAR Email[maxLength] = TEXT("");
                TCHAR Passwd[maxLength] = TEXT("");
                TCHAR ConfirmPasswd[maxLength] = TEXT("");
                TCHAR FirstName[maxLength] = TEXT("");
                TCHAR LastName[maxLength] = TEXT("");
                TCHAR Address[maxLength] = TEXT("");
                TCHAR ContactID[maxLength] = TEXT("");

                GetWindowText(hwndRegisterEmail, Email, maxLength);
                GetWindowText(hwndRegisterPassword, Passwd, maxLength);
                GetWindowText(hwndRegisterConfirmPassword, ConfirmPasswd, maxLength);
                GetWindowText(hwndRegisterFirstName, FirstName, maxLength);
                GetWindowText(hwndRegisterLastName, LastName, maxLength);
                GetWindowText(hwndRegisterAddress, Address, maxLength);
                GetWindowText(hwndRegisterCID, ContactID, maxLength);

                /* empty field */
                if (checkEmptyField(hwnd, TEXT("email"), Email, _tcslen(Email)) == false) break;
                if (checkEmptyField(hwnd, TEXT("password"), Passwd, _tcslen(Passwd)) == false) break;
                if (checkEmptyField(hwnd, TEXT("confirm password"), ConfirmPasswd, _tcslen(ConfirmPasswd)) == false) break;
                if (checkEmptyField(hwnd, TEXT("ContactID"), ContactID, _tcslen(ContactID)) == false) break;

                /* check password rule */
                if (checkPasswordRule(hwnd, Passwd, sizeof(Passwd)) == false) break;

                /* check confirm password */
                if (checkConfirmPasswd(hwnd, Passwd, _tcslen(Passwd), ConfirmPasswd, _tcslen(ConfirmPasswd)) == false) break;

                // printf("ID : %ls\nPASSWD : %ls\nConfirmPasswd : %ls\nFirstName : %ls\nLastName : %ls\nAddress : %ls\n", Email, Passwd, ConfirmPasswd, FirstName, LastName, Address);
                TRegistration* msg = (TRegistration *)std::malloc(sizeof(TRegistration));
                memset(msg, 0, sizeof(msg));
                if (msg != NULL)
                {
                    msg->MessageType = Registration;
                    msg->EmailSize = exchangeTCHARToChar(Email, msg->email);

                    memset(msg->password, 0, 256);

                    char PasswdChar[GENERAL_BUFSIZE] = { 0, };
                    char PasswdHash[65] = { 0, };

                    int len = _tcslen(Passwd);
                    CopyTCharToChar(Passwd, PasswdChar, len);

                    SHA256Hash(PasswdChar, _tcslen(Passwd), PasswdHash);

                    std::cout << "SHA-256 : " << PasswdHash << std::endl;

                    msg->PasswordSize = 64;
                    memcpy(msg->password, PasswdHash, 64);

                    exchangeTCHARToChar(ContactID, msg->ContactID);
                    exchangeTCHARToChar(FirstName, msg->firstName);
                    exchangeTCHARToChar(LastName, msg->lastName);
                    exchangeTCHARToChar(Address, msg->Address);
                    msg->EmailSize = _tcslen(Email);

                    sendMsgtoACS((char *)msg, sizeof(TRegistration));
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

bool checkPasswordRule(HWND hwnd, TCHAR* Passwd, unsigned int maxLength)
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

bool checkConfirmPasswd(HWND hwnd, const TCHAR* passwd, size_t passwdSize, TCHAR* confirmPasswd, size_t confirmPasswdSize)
{
    if (passwdSize == confirmPasswdSize && _tcsncmp(passwd, confirmPasswd, passwdSize) == 0)
    {
        return true;
    }

    MessageBox(hwnd, TEXT("Confirm password is not same as password."), TEXT("Warning"), MB_OK | MB_ICONEXCLAMATION);
    return false;
}


void RegisterCreateForm(HWND parentHwnd)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = RegisterProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RegisterWindowClass";
    RegisterClass(&wc);

    hwndCreateRegister = CreateWindowEx(
        0,
        L"RegisterWindowClass",
        L"Register",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 560,
        parentHwnd, NULL, wc.hInstance, NULL
    );

    HWND hwndRegisterEmailLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Email",
        WS_VISIBLE | WS_CHILD,
        20, 20, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterEmail = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 50, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndRegisterPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Password",
        WS_VISIBLE | WS_CHILD,
        20, 80, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        20, 110, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndRegisterConfirmPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Confirm Password",
        WS_VISIBLE | WS_CHILD,
        20, 140, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterConfirmPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        20, 170, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndRegisterFirstNameLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"First Name",
        WS_VISIBLE | WS_CHILD,
        20, 200, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterFirstName = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL ,
        20, 230, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndRegisterLastNameLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Last Name",
        WS_VISIBLE | WS_CHILD,
        20, 260, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterLastName = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL ,
        20, 290, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndRegisterAddressLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Address",
        WS_VISIBLE | WS_CHILD,
        20, 320, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterAddress = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL ,
        20, 350, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndRegisterCIDLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"ContactID",
        WS_VISIBLE | WS_CHILD,
        20, 380, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    hwndRegisterCID = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 410, 300, 20,
        hwndCreateRegister, NULL, NULL, NULL
    );

    HWND hwndJoinUs = CreateWindowEx(
        0,
        L"BUTTON",
        L"Join Us",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 440, 300, 30,
        hwndCreateRegister, (HMENU)BUTTON_JOINUS, NULL, NULL
    );
    // Show the main window
    ShowWindow(hwndCreateRegister, SW_SHOWDEFAULT);
    UpdateWindow(hwndCreateRegister);
}
