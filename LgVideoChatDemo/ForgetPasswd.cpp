#include <windows.h>
#include <stdio.h>
#include "definition.h"

#define BUTTON_OK 500
#define BUTTON_SEND 501

static HWND hwnd, hwndEmailLabel, hwndEmail, hwndSecetLabel, hwndSecret,\
hwndOkButton, hwndSendButton, hwndPasswd, hwndPasswdLabel, hwndPasswd2, hwndPasswd2Label;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
            case BUTTON_SEND:
            {
            
            }
            break;

	        case BUTTON_OK:
    	    {
        	    TCHAR Secret[128] = TEXT("");
                TCHAR Passwd[128] = TEXT("");
                TCHAR Passwd2[128] = TEXT("");

    	        GetWindowText(hwndSecret, Secret, 128);
                GetWindowText(hwndPasswd, Passwd, 128);
                GetWindowText(hwndPasswd2, Passwd2, 128);

	            MessageBox(hwnd, TEXT("BUTTON_OK"), TEXT(""), MB_OK | MB_ICONEXCLAMATION);
       	 	}
            break;

        }
    }
    break;
    case WM_CREATE:
    {

    }
    break;

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
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateForgetPasswd(HWND phwnd)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"CreateForgetPasswd";
    RegisterClass(&wc);

	hwnd = CreateWindowEx(
        0,
        L"CreateForgetPasswd",
        L"Forget Password",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 300,
        NULL, NULL, wc.hInstance, NULL
    );

	hwndEmailLabel = CreateWindowEx(
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

    hwndSendButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"Send",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 50, 145, 30,
        hwnd, (HMENU)BUTTON_SEND, NULL, NULL
    );

    hwndSecetLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Secret",
        WS_VISIBLE | WS_CHILD,
        20, 90, 90, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndSecret = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        100, 90, 220, 20,
        hwnd, NULL, NULL, NULL
    );
	
    hwndPasswdLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Password",
        WS_VISIBLE | WS_CHILD,
        20, 130, 90, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndPasswd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        100, 130, 220, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndPasswd2Label = CreateWindowEx(
        0,
        L"STATIC",
        L"Password",
        WS_VISIBLE | WS_CHILD,
        20, 160, 90, 20,
        hwnd, NULL, NULL, NULL
    );

    hwndPasswd2 = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
        100, 160, 220, 20,
        hwnd, NULL, NULL, NULL
    );

	hwndOkButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"Ok",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 190, 145, 30,
        hwnd, (HMENU)BUTTON_OK, NULL, NULL
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
	
	return;
}
