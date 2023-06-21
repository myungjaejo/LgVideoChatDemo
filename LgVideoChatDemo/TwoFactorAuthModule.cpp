#include <windows.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <Commctrl.h>
#include "LgVideoChatDemo.h"
#include "AccessControlClient.h"
#include "TwoFactorAuth.h"


#define BUTTON_OK	    550
#define BUTTON_CANCEL	551
#define EDIT_TFA        552

HWND hOKButton, hCancelButton, hInputEdit;
static char getTFAMsg[128];

LRESULT CALLBACK WindowTFAProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int exchangeTCHToChar(TCHAR* target, char* outbuf)
{
    int nLength = WideCharToMultiByte(CP_ACP, 0, target, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, target, -1, outbuf, nLength, NULL, NULL);

    return nLength;
}

int WINAPI CreateTwoFactorAuthWindow(HWND Phwnd)
{
    if (devStatus == Disconnected)
    {
        MessageBox(NULL, L"Please Login First", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    // ������ Ŭ���� ���
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowTFAProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TFAWindowClass";
    RegisterClass(&wc);

    // ������ ����
    HWND hwnds = CreateWindowEx(0, L"TFAWindowClass", L"Two Factor Authentication", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, Phwnd, NULL, wc.hInstance, NULL);

    if (hwnds == NULL)
    {
        MessageBox(NULL, L"Failed to create the window.", L"Error", MB_OK | MB_ICONERROR);
        std::cout << "ERROR : " << GetLastError() << std::endl;
        return 1;
    }

    hOKButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"ACCEPT",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        50, 180, 100, 60,
        hwnds, (HMENU)BUTTON_OK, NULL, NULL
    );

    hCancelButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"DENY",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        230, 180, 100, 60,
        hwnds, (HMENU)BUTTON_CANCEL, NULL, NULL
    );

    HWND hwndMentionLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Please Insert Received TFA Message",
        WS_VISIBLE | WS_CHILD,
        50, 20, 300, 50,
        hwnds, NULL, (HINSTANCE)GetWindowLongPtr(hwnds, GWLP_HINSTANCE), NULL
    );

    HFONT hFont = CreateFont(
        32,           // ���ϴ� �۲� ũ��
        0,                     // �۲� �ʺ�
        0,                     // ����
        0,                     // ���ؼ��� ����
        FW_NORMAL,             // �۲� �β�
        FALSE,                 // ����� ���� ����
        FALSE,                 // ���� ���� ����
        FALSE,                 // ��Ҽ� ���� ����
        DEFAULT_CHARSET,       // ���� ����
        OUT_DEFAULT_PRECIS,    // ��� ���е�
        CLIP_DEFAULT_PRECIS,   // Ŭ���� ���е�
        DEFAULT_QUALITY,       // ��� ǰ��
        DEFAULT_PITCH | FF_DONTCARE,  // �۲� ����
        L"Arial"               // �۲� �̸�
    );
    SendMessage(hwndMentionLabel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

    hInputEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        100, 20, 220, 20,
        hwnds, NULL, NULL, NULL
    );

    // ������ ǥ��
    ShowWindow(hwnds, SW_SHOW);
    UpdateWindow(hwnds);

    // �޽��� ����
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WindowTFAProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case BUTTON_OK:
        {
            TCHAR InputTFA[RECEVER_LENTH] = TEXT("");            
            GetWindowText(hInputEdit, InputTFA, RECEVER_LENTH);
            TTwoFactor* tMsg = (TTwoFactor*)std::malloc(sizeof(TTwoFactor));
            tMsg->MessageType = TwoFactorResponse;
            strcpy_s(tMsg->myCID, MyID);
            exchangeTCHToChar(InputTFA, tMsg->TFA);
            
            sendMsgtoACS((char*)tMsg, sizeof(TTwoFactor));
            free(tMsg);        

            DestroyWindow(hwnd);
        }
        case BUTTON_CANCEL:
        {
            TTwoFactor* tMsg = (TTwoFactor*)std::malloc(sizeof(TTwoFactor));
            tMsg->MessageType = TwoFactorResponse;

            sendMsgtoACS((char*)tMsg, sizeof(TTwoFactor));
            free(tMsg);
            //strcpy_s(MyID. NULL);
            DestroyWindow(hwnd);
        }
        }
    }
    break;
    case WM_CREATE:
    {
        //wchar_t* pStr;
        //int Size = MultiByteToWideChar(CP_ACP, 0, callMsg, -1, NULL, NULL);
        //pStr = new WCHAR[Size];
        //MultiByteToWideChar(CP_ACP, 0, callMsg, strlen(callMsg) + 1, pStr, Size);
    }
    break;

    case WM_DESTROY:
    {
        DestroyWindow(hOKButton);
        DestroyWindow(hCancelButton);
        DestroyWindow(hInputEdit);
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_CLOSE:
    {
        DestroyWindow(hOKButton);
        DestroyWindow(hCancelButton);
        DestroyWindow(hInputEdit);
        DestroyWindow(hwnd);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
