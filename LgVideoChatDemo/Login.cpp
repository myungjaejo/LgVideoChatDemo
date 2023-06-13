#include "Login.h"
#include "Register.h"
#include <iostream>
#define BUTTON_LOGIN 300
#define BUTTON_REGISTER 301
#define BUTTON_FORGETPASSWD 302

const int maxEmailLength = 30;
const int maxPasswdLength = 30;
HWND hwndEmail, hwndPassword, hwndLogin, hwndRegister, hwndForgetPasswd;
HWND hwndParent;

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
                    MessageBox(hwnd, TEXT("BUTTON_LOGIN"), TEXT("TEST"), MB_OK | MB_ICONEXCLAMATION);
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
            PostQuitMessage(0);
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


void LoginCreateForm(HWND parentHwnd)
{
    hwndParent = parentHwnd;
    WNDCLASS wc = {};
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
        NULL, NULL, wc.hInstance, NULL
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
