#include <windows.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <Commctrl.h>
#include "LgVideoChatDemo.h"
#include "AccessControlClient.h"
#include "VideoServer.h"

#define BUTTON_ACCEPT	500
#define BUTTON_DENY		501

HWND hCallAcceptButton, hCallDenyButton;
char callMsg[128];

LRESULT CALLBACK WindowNCProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI CreateCallNotification(HWND Phwnd, const char* CallFrom)
{
    if (devStatus == Disconnected)
    {
        MessageBox(NULL, L"Please Login First", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    // 윈도우 클래스 등록
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowNCProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NotifyWindowClass";
    RegisterClass(&wc);

    strcpy_s(callMsg, CallFrom);

    // 윈도우 생성
    HWND hwnds = CreateWindowEx(0, L"NotifyWindowClass", L"Call Notification", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, Phwnd, NULL, wc.hInstance, NULL);

    if (hwnds == NULL)
    {
        MessageBox(NULL, L"Failed to create the window.", L"Error", MB_OK | MB_ICONERROR);
        std::cout << "ERROR : " << GetLastError() << std::endl;
        return 1;
    }

    hCallAcceptButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"ACCEPT",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        50, 180, 100, 60,
        hwnds, (HMENU)BUTTON_ACCEPT, NULL, NULL
    );

    hCallDenyButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"DENY",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        230, 180, 100, 60,
        hwnds, (HMENU)BUTTON_DENY, NULL, NULL
    );

    // 윈도우 표시
    ShowWindow(hwnds, SW_SHOW);
    UpdateWindow(hwnds);

    // 메시지 루프
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WindowNCProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case BUTTON_ACCEPT:
        {
            //Server open
            devStatus = Callee;
            bool loopback = false;
            //StartVideoServer(loopback);
            PostMessage(hWndMain, WM_OPEN_VIDEOSERVER, 0, 0);
            SendMessage(hWndMainToolbar, TB_SETSTATE, IDM_CALL_REQUEST,
                (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
            SendMessage(hWndMainToolbar, TB_SETSTATE, IDM_CALL_DENY,
                (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

            //send msg
            TDeviceID* msg = (TDeviceID*)std::malloc(sizeof(TDeviceID));
            msg->MessageType = AcceptCall;
            strcpy_s(msg->FromDevID, MyID);
            strcpy_s(msg->ToDevID, callMsg);
            sendMsgtoACS((char*)msg, sizeof(TDeviceID));
            free(msg);
            DestroyWindow(hwnd);
        }
        case BUTTON_DENY:
        {
            //send msg
            TDeviceID* msg = (TDeviceID*)std::malloc(sizeof(TDeviceID));
            msg->MessageType = RejectCall;
            strcpy_s(msg->FromDevID, MyID);
            strcpy_s(msg->ToDevID, callMsg);
            sendMsgtoACS((char*)msg, sizeof(TDeviceID));
            free(msg);
            DestroyWindow(hwnd);
        }
        }
    }
    break;
    case WM_CREATE:
    {
        wchar_t* pStr;
        int Size = MultiByteToWideChar(CP_ACP, 0, callMsg, -1, NULL, NULL);
        pStr = new WCHAR[Size];
        MultiByteToWideChar(CP_ACP, 0, callMsg, strlen(callMsg) + 1, pStr, Size);

        HWND hwndMentionLabel = CreateWindowEx(
            0,
            L"STATIC",
            L"Come Call Request form ",
            WS_VISIBLE | WS_CHILD,
            50, 20, 300, 50,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );

        HFONT hFont = CreateFont(
            32,           // 원하는 글꼴 크기
            0,                     // 글꼴 너비
            0,                     // 각도
            0,                     // 기준선의 각도
            FW_NORMAL,             // 글꼴 두께
            FALSE,                 // 기울임 설정 여부
            FALSE,                 // 밑줄 설정 여부
            FALSE,                 // 취소선 설정 여부
            DEFAULT_CHARSET,       // 문자 집합
            OUT_DEFAULT_PRECIS,    // 출력 정밀도
            CLIP_DEFAULT_PRECIS,   // 클리핑 정밀도
            DEFAULT_QUALITY,       // 출력 품질
            DEFAULT_PITCH | FF_DONTCARE,  // 글꼴 종류
            L"Arial"               // 글꼴 이름
        );
        SendMessage(hwndMentionLabel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

        HWND hwndCallLabel = CreateWindowEx(
            0,
            L"STATIC",
            pStr, //L"Email",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            100, 100, 200, 50,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );

        hFont = CreateFont(
            48,           // 원하는 글꼴 크기
            0,                     // 글꼴 너비
            0,                     // 각도
            0,                     // 기준선의 각도
            FW_BOLD,             // 글꼴 두께
            FALSE,                 // 기울임 설정 여부
            FALSE,                 // 밑줄 설정 여부
            FALSE,                 // 취소선 설정 여부
            DEFAULT_CHARSET,       // 문자 집합
            OUT_DEFAULT_PRECIS,    // 출력 정밀도
            CLIP_DEFAULT_PRECIS,   // 클리핑 정밀도
            DEFAULT_QUALITY,       // 출력 품질
            DEFAULT_PITCH | FF_DONTCARE,  // 글꼴 종류
            L"Arial"               // 글꼴 이름
        );
        SendMessage(hwndCallLabel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

        // ComboBox 생성
        //hComboBox = CreateWindowEx(0, L"ComboBox", NULL, WS_CHILD | WS_VISIBLE | CBS_SIMPLE,
        //    10, 10, 200, 200, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        //// ComboBox에 항목 추가
        //TCHAR Contact[128];

        //std::vector<char*>::iterator iter;

        //for (iter = ContactList.begin(); iter != ContactList.end(); iter++)
        //{
        //    MultiByteToWideChar(CP_ACP, 0, *iter, -1, Contact, 128);
        //    SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)Contact);
        //}
    }
    break;

    case WM_DESTROY:
    {
        DestroyWindow(hCallAcceptButton);
        DestroyWindow(hCallDenyButton);
        //DestroyWindow(hComboBox);
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_CLOSE:
    {
        DestroyWindow(hCallAcceptButton);
        DestroyWindow(hCallDenyButton);
        //DestroyWindow(hComboBox);
        DestroyWindow(hwnd);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
