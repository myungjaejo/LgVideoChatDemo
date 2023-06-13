#include "Register.h"
#include <iostream>
#define BUTTON_JOINUS 400
const int maxLength = 30;

HWND hwndRegisterEmail, hwndRegisterPassword, hwndRegisterConfirmPassword, hwndRegisterFirstName, hwndRegisterLastName, hwndRegisterAddress, hwndJoinUs;


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

                GetWindowText(hwndRegisterEmail, Email, maxLength);
                GetWindowText(hwndRegisterPassword, Passwd, maxLength);
                GetWindowText(hwndRegisterConfirmPassword, ConfirmPasswd, maxLength);
                GetWindowText(hwndRegisterFirstName, FirstName, maxLength);
                GetWindowText(hwndRegisterLastName, LastName, maxLength);
                GetWindowText(hwndRegisterAddress, Address, maxLength);

                printf("ID : %ls\nPASSWD : %ls\nConfirmPasswd : %ls\nFirstName : %ls\nLastName : %ls\nAddress : %ls\n", Email, Passwd, ConfirmPasswd, FirstName, LastName, Address);
                MessageBox(hwnd, TEXT("BUTTON_JOINUS"), TEXT("TEST"), MB_OK | MB_ICONEXCLAMATION);
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


void RegisterCreateForm(HWND parentHwnd)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = RegisterProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RegisterWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"RegisterWindowClass",
        L"Register",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 500,
        NULL, NULL, wc.hInstance, NULL
    );

    HWND hwndRegisterEmailLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Email",
        WS_VISIBLE | WS_CHILD,
        20, 20, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterEmail = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 50, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndRegisterPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Password",
        WS_VISIBLE | WS_CHILD,
        20, 80, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        20, 110, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndRegisterConfirmPasswordLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Confirm Password",
        WS_VISIBLE | WS_CHILD,
        20, 140, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndRegisterConfirmPassword = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        20, 170, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    HWND hwndRegisterFirstNameLabel = CreateWindowEx(
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
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL ,
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
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL ,
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
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL ,
        20, 350, 300, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndJoinUs = CreateWindowEx(
        0,
        L"BUTTON",
        L"Join Us",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 380, 300, 30,
        hwnd, (HMENU)BUTTON_JOINUS, NULL, NULL
    );
    // Show the main window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
}
