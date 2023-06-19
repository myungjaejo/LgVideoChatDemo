#include <windows.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include "LgVideoChatDemo.h"
#include "AccessControlClient.h"

#define BUTTON_CALL 400

HWND hwnd, hComboBox, hCallButton;

extern bool IsLogin;

std::vector<char*> ContactList;
//char ContactList[5][128];
//int contactListSize = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI CreateContactList(HWND Phwnd)
{
    if (devStatus == Disconnected)
    {
        MessageBox(NULL, L"Please Login First", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    // 윈도우 클래스 등록
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MainWindowClass";
    RegisterClass(&wc);
    

    // 윈도우 생성
    hwnd = CreateWindowEx(0, L"MainWindowClass", L"Contact List", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, wc.hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, L"Failed to create the window.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    hCallButton = CreateWindowEx(
        0,
        L"BUTTON",
        L"Call",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        250, 10, 100, 30,
        hwnd, (HMENU)BUTTON_CALL, NULL, NULL
    );

    // 윈도우 표시
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // 메시지 루프
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
                case BUTTON_CALL:
                {
                    int selectedIndex = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
                    if (selectedIndex != CB_ERR)
                    {
                        // 선택된 항목의 길이 가져오기
                        int textLength = SendMessage(hComboBox, CB_GETLBTEXTLEN, selectedIndex, 0);
                        if (textLength != CB_ERR)
                        {
                            // 선택된 항목의 텍스트 가져오기
                            wchar_t* selectedText = new wchar_t[textLength + 1];
                            SendMessage(hComboBox, CB_GETLBTEXT, selectedIndex, (LPARAM)selectedText);

                            // 선택된 항목 출력
                            MessageBox(hwnd, selectedText, L"Selected Item", MB_OK);

                            // call request
                            std::cout << "make a call to " << selectedText << std::endl;
                            TDeviceID msg{};
                            msg.MessageType = RequestCall;
                            //int nLength = WideCharToMultiByte(CP_ACP, 0, target, -1, NULL, 0, NULL, NULL);
                            char* outbuf = (char*)std::malloc(sizeof(char) * NAME_BUFSIZE);
                            WideCharToMultiByte(CP_ACP, 0, selectedText, -1, outbuf, NAME_BUFSIZE, NULL, NULL);
                            strcpy_s(msg.DevID, NAME_BUFSIZE, outbuf);
                            sendMsgtoACS((char*)&msg, sizeof(msg));
                        }
                    }
                }
            }
        }
        break;
        case WM_CREATE:
        {
            // ComboBox 생성
            hComboBox = CreateWindowEx(0, L"ComboBox", NULL, WS_CHILD | WS_VISIBLE | CBS_SIMPLE,
                10, 10, 200, 200, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        
            // ComboBox에 항목 추가
            TCHAR Contact[128];

            std::vector<char*>::iterator iter;

            for (iter = ContactList.begin(); iter != ContactList.end(); iter++)
            {
                MultiByteToWideChar(CP_ACP, 0, *iter, -1, Contact, 128);
                SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)Contact);
            }
        }
        break;
        
        case WM_DESTROY:
        {
            DestroyWindow(hCallButton);
            DestroyWindow(hComboBox);
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_CLOSE:
        {
            DestroyWindow(hCallButton);
            DestroyWindow(hComboBox);
            DestroyWindow(hwnd);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void makeContactList(char* contactID)
{
    ContactList.push_back(contactID);
    //char* cid = (char*)std::malloc(sizeof(char)*128);
    //if (cid != NULL)
    //{
    //    //strncpy_s(cid, contactID, 128);
    //    strncpy_s(cid, 128, contactID, 128);
    //    ContactList.push_back(cid);
    //}
}