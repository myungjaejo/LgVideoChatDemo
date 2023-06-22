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
#define EDIT_EMAIL      552

HWND hFOKButton, hFInputEdit;
static char getTFAMsg[128];

LRESULT CALLBACK WindowResetPWProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int exchangeTCToC(TCHAR* target, char* outbuf)
{
    int nLength = WideCharToMultiByte(CP_ACP, 0, target, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, target, -1, outbuf, nLength, NULL, NULL);

    return nLength;
}

int WINAPI CreateResetPasswordWindow(HWND Phwnd)
{
    //if (devStatus == Disconnected)
    //{
    //    MessageBox(NULL, L"Please Login First", L"Error", MB_OK | MB_ICONERROR);
    //    return 1;
    //}
    // 윈도우 클래스 등록
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowResetPWProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ResetPasswordWindowClass";
    RegisterClass(&wc);

    // 윈도우 생성
    HWND hwnds = CreateWindowEx(0, L"ResetPasswordWindowClass", L"Reset Password", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, Phwnd, NULL, wc.hInstance, NULL);

    if (hwnds == NULL)
    {
        MessageBox(NULL, L"Failed to create the window.", L"Error", MB_OK | MB_ICONERROR);
        std::cout << "ERROR : " << GetLastError() << std::endl;
        return 1;
    }

    hFOKButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"Send",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        140, 180, 100, 60,
        hwnds, (HMENU)BUTTON_OK, NULL, NULL
    );

    //hCancelButton = CreateWindowEx(
    //    0,
    //    L"BUTTON",
    //    L"DENY",
    //    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    //    230, 180, 100, 60,
    //    hwnds, (HMENU)BUTTON_CANCEL, NULL, NULL
    //);

    HWND hwndMentionLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"Please Insert Your Email",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 20, 300, 40,
        hwnds, NULL, (HINSTANCE)GetWindowLongPtr(hwnds, GWLP_HINSTANCE), NULL
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

    hFInputEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        80, 120, 220, 40,
        hwnds, NULL, NULL, NULL
    );

    hFont = CreateFont(
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
    SendMessage(hFInputEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

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

LRESULT CALLBACK WindowResetPWProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            GetWindowText(hFInputEdit, InputTFA, RECEVER_LENTH);
            TLogin* tMsg = (TLogin*)std::malloc(sizeof(TLogin));
            tMsg->MessageType = ResetPasswordRequest;           

            //strcpy_s(tMsg->email, MyID);
            exchangeTCToC(InputTFA, tMsg->email);
            int sLen = strlen(tMsg->email);
            tMsg->EmailSize = sLen;
            std::cout << tMsg->email << std::endl;

            sendMsgtoACS((char*)tMsg, sizeof(TLogin));
            free(tMsg);

            DestroyWindow(hwnd);
            break;
        }
        //case BUTTON_CANCEL:
        //{
        //    TTwoFactor* tMsg = (TTwoFactor*)std::malloc(sizeof(TTwoFactor));
        //    tMsg->MessageType = TwoFactorRequest;

        //    sendMsgtoACS((char*)tMsg, sizeof(TTwoFactor));
        //    free(tMsg);
        //    //strcpy_s(MyID. NULL);
        //    DestroyWindow(hwnd);
        //    break;
        //}
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
        DestroyWindow(hFOKButton);
        DestroyWindow(hFInputEdit);
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_CLOSE:
    {
        DestroyWindow(hFOKButton);
        DestroyWindow(hFInputEdit);
        DestroyWindow(hwnd);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
