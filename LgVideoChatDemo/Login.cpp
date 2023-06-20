#include "Login.h"
#include "Register.h"
#include <iostream>
#include <wincrypt.h>
#include <tchar.h>
#include <regex>
#include <Commctrl.h>
//#include "definition.h"
#include "LgVideoChatDemo.h"
#include "AccessControlClient.h"
#include "AccessControlServer.h"
#include "ContactList.h"

#define BUTTON_LOGIN 300
#define BUTTON_REGISTER 301
#define BUTTON_FORGETPASSWD 302

const int maxEmailLength = 30;
const int maxPasswdLength = 30;
HWND hwndEmail, hwndPassword, hwndLogin, hwndRegister, hwndForgetPasswd;
HWND hwndParent;

bool IsLogin = false;

extern HINSTANCE hInst;

bool isAdmin(const TCHAR* Email, const TCHAR* Passwd)
{
    if (!_tcscmp(Email, TEXT("admin")))
    {
        if (!_tcscmp(Passwd, TEXT("admin123")))
        {
            return true;
        }
    }
    return false;
}

bool HasConsecutiveCharacters(const TCHAR* password) {
    const int consecutiveLimit = 3;
    size_t length = _tcslen(password);

    for (size_t i = 0; i < length - consecutiveLimit + 1; i++) {
        bool isConsecutive = true;
        TCHAR currentChar = password[i];

        for (int j = 1; j < consecutiveLimit; j++) {
            if (password[i + j] != currentChar + j) {
                isConsecutive = false;
                break;
            }
        }

        if (isConsecutive) {
            return true;
        }
    }

    return false;
}

bool ValidateEmailAddress(const TCHAR* email) {
    std::basic_regex<TCHAR> pattern(_T(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"));

    std::basic_string<TCHAR> emailString(email);
    return std::regex_match(emailString, pattern);
}

void SHA256Hash(const TCHAR* input, size_t inputLength, char* output) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[32];
    DWORD hashLen = sizeof(hash);

    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        std::cerr << "CryptAcquireContext Failed: " << GetLastError() << std::endl;
        return;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        std::cerr << "CryptCreateHash Failed: " << GetLastError() << std::endl;
        CryptReleaseContext(hProv, 0);
        return;
    }

    if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input), static_cast<DWORD>(inputLength), 0)) {
        std::cerr << "CryptHashData Failed: " << GetLastError() << std::endl;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        std::cerr << "CryptGetHashParam Failed: " << GetLastError() << std::endl;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    for (DWORD i = 0; i < hashLen; i++) {
        sprintf_s(output + i * 2, 3, "%02X", hash[i]);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

void CopyTCharToChar(TCHAR* tcharString, char* CharString, int length)
{
    for (int i = 0; i < length; i++)
    {
        CharString[i] = (char)(tcharString[i]);
    }
}

LRESULT CALLBACK LoginProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
        case WM_COMMAND: 
        {
            int wmId = LOWORD(wParam);
            switch (wmId) 
            {
                case BUTTON_LOGIN:
                {
                    TCHAR Email[maxEmailLength] = TEXT("");
                    TCHAR Passwd[maxPasswdLength] = TEXT("");
                    GetWindowText(hwndEmail, Email, maxEmailLength);
                    GetWindowText(hwndPassword, Passwd, maxPasswdLength);
                    printf("ID : %ls\nPASSWD : %ls\n", Email, Passwd);

                    if (isAdmin(Email, Passwd))
                    {
                        devStatus = Server;
                        SendMessage(hWndMainToolbar, TB_SETSTATE, IDM_START_SERVER,
                            (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
                        DestroyWindow(hwnd);
                        return 1;
                    }

                    if (!ValidateEmailAddress(Email))
                    {
                        MessageBox(hwnd, TEXT("INVALID EMAIL FORMAT"), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
                        return 1;
                    }

                    if (_tcslen(Passwd) == 0)
                    {
                        MessageBox(hwnd, TEXT("INPUT PASSWORD"), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
                        return 1;
                    }

                    if (HasConsecutiveCharacters(Passwd))
                    {
                        MessageBox(hwnd, TEXT("NOT USE CONSECUTIVE CHAR FOR PASSWORD"), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
                        return 1;
                    }

                    char PasswdHash[65];

                    SHA256Hash(Passwd, _tcslen(Passwd), PasswdHash);
                    std::cout << "SHA-256 : " << PasswdHash << std::endl;

                    TLogin login{};
                    login.MessageType = (char)Login;
                    login.EmailSize = _tcslen(Email);
                    login.PasswordHashSize = 64; // This value is always same
                    CopyTCharToChar(Email, login.email, login.EmailSize);
                    memcpy(login.passwordHash, PasswdHash, 64);
                    // MessageBox(hwnd, TEXT("BUTTON_LOGIN"), TEXT("TEST"), MB_OK | MB_ICONEXCLAMATION);
                    sendMsgtoACS((char*) &login, sizeof(login));

                    break;
                }
                case BUTTON_REGISTER:
                {
                    RegisterCreateForm(hwnd);
                    //MessageBox(hwnd, TEXT("BUTTON_REGISTER"), TEXT("TEST"), MB_OK | MB_ICONEXCLAMATION);
                    break;
                }
                case BUTTON_FORGETPASSWD:
                {
                    MessageBox(hwnd, TEXT("BUTTON_FORGETPASSWD"), TEXT("TEST"), MB_OK | MB_ICONEXCLAMATION);
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
            //PostQuitMessage(0);
            return 0;
        }
        case WM_CLOSE: 
        {
            //PostQuitMessage(0);
            DestroyWindow(hwnd);
            return 0;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


void LoginCreateForm(HWND parentHwnd)
{
    hwndParent = parentHwnd;
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = LoginProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LoginWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"LoginWindowClass",
        L"Login",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 200,
        hwndParent, NULL, wc.hInstance, NULL
    );

    HWND hwndEmailLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Email",
        WS_VISIBLE | WS_CHILD,
        20, 20, 90, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndEmail = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        100, 20, 220, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Password",
        WS_VISIBLE | WS_CHILD,
        20, 50, 90, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        100, 50, 220, 20,
        hwnd, NULL, NULL, NULL
    );
    hwndLogin = CreateWindowEx(
        0,
        L"BUTTON",
        L"Login",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 80, 300, 30,
        hwnd, (HMENU)BUTTON_LOGIN, NULL, NULL
    );

    hwndRegister = CreateWindowEx(
        0,
        L"BUTTON",
        L"Register",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 120, 145, 30,
        hwnd, (HMENU)BUTTON_REGISTER, NULL, NULL
    );

    hwndForgetPasswd = CreateWindowEx(
        0,
        L"BUTTON",
        L"Forget Passwd",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        175, 120, 145, 30,
        hwnd, (HMENU)BUTTON_FORGETPASSWD, NULL, NULL
    );

    // Show the main window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
}
