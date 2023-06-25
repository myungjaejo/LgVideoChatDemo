//#include "definition.h"
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <ws2tcpip.h>
#include < cstdlib >
#include <iostream>
#include <Commctrl.h>

#include "AccessControlClient.h"
#include "LgVideoChatDemo.h"
#include "TcpSendRecv.h"
#include "definition.h"
#include "ContactList.h"
#include "NotifyCall.h"
#include "VideoClient.h"
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\opencv.hpp>
#include "Camera.h"
#include "DisplayImage.h"
#include "TwoFactorAuth.h"
#include <openssl/ssl.h>

#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

static HANDLE hClientEvent = INVALID_HANDLE_VALUE;
static HANDLE hEndACClientEvent = INVALID_HANDLE_VALUE;
static HANDLE hThreadACClient = INVALID_HANDLE_VALUE;
static HANDLE hTimer = INVALID_HANDLE_VALUE;

static SOCKET Client = INVALID_SOCKET;
static DWORD ThreadACClientID;

static DWORD WINAPI ThreadACClient(LPVOID ivalue);
static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen);
static int OnConnect(char* IPAddr);

extern HWND hwndCreateRegister, hwndLogin;

SSL* Clientssl;

static void AccessControlClientSetExitEvent(void)
{
	if (hEndACClientEvent != INVALID_HANDLE_VALUE)
		SetEvent(hEndACClientEvent);
}

static void CleanUpAccessControlClient(void)
{
    std::cout << "Cleanup Access Control Client" << std::endl;

    if (hClientEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hClientEvent);
        hClientEvent = INVALID_HANDLE_VALUE;
    }
    if (hEndACClientEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hEndACClientEvent);
        hEndACClientEvent = INVALID_HANDLE_VALUE;
    }

    if (Client != INVALID_SOCKET)
    {
        closesocket(Client);
        Client = INVALID_SOCKET;
    }
}

bool StartAccessControlClient(void)
{
    hThreadACClient = CreateThread(NULL, 0, ThreadACClient, NULL, 0, &ThreadACClientID);
    return true;
}

bool StopAccessControlClient(void)
{
    AccessControlClientSetExitEvent();
    if (hThreadACClient != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(hThreadACClient, INFINITE);
        CloseHandle(hThreadACClient);
        hThreadACClient = INVALID_HANDLE_VALUE;
    }
    ;
    return true;
}
bool IsACClientRunning(void)
{
    if (hThreadACClient == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    else return true;
}

int verify_callback(int preverify_ok, X509_STORE_CTX* ctx)
{
    if (preverify_ok == 0)
    {
        int depth = X509_STORE_CTX_get_error_depth(ctx);
        int err = X509_STORE_CTX_get_error(ctx);

        if (depth == 0 && err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
        {
            return 1;
        }

        return 0;
    }

    return 1;
}

bool ConnectToACSever(const char* remotehostname, unsigned short remoteport)
{
    int iResult;
    struct addrinfo   hints;
    struct addrinfo* result = NULL;
    char remoteportno[128];

    sprintf_s(remoteportno, sizeof(remoteportno), "%d", remoteport);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Initialize SSL
    SSL_library_init();
    SSL_CTX* sslContext = SSL_CTX_new(TLS_client_method());

    if (sslContext == NULL) {
        printf("Failed to create SSL context.\n");
        closesocket(Client);
        WSACleanup();
        return 1;
    }
    SSL_CTX_set_verify(sslContext, SSL_VERIFY_PEER, verify_callback);
    Clientssl = SSL_new(sslContext);

    if (Clientssl == NULL) {
        printf("Failed to create SSL.\n");
        closesocket(Client);
        WSACleanup();
        return 1;
    }

    iResult = getaddrinfo(remotehostname, remoteportno, &hints, &result);
    if (iResult != 0)
    {
        std::cout << "getaddrinfo: Failed" << std::endl;
        return false;
    }
    if (result == NULL)
    {
        std::cout << "getaddrinfo: Failed" << std::endl;
        return false;
    }

    if ((Client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        std::cout << "video client socket() failed with error " << WSAGetLastError() << std::endl;
        return false;
    }

    //----------------------
    // Connect to server.
    iResult = connect(Client, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    if (iResult == SOCKET_ERROR) {
        std::cout << "connect function failed with error : " << WSAGetLastError() << std::endl;
        iResult = closesocket(Client);
        Client = INVALID_SOCKET;
        if (iResult == SOCKET_ERROR)
            std::cout << "closesocket function failed with error :" << WSAGetLastError() << std::endl;
        return false;
    }
    
    SSL_set_fd(Clientssl, Client);

    // Perform SSL handshake

    if (SSL_connect(Clientssl) != 1) {
        printf("SSL handshake failed.\n");
        SSL_free(Clientssl);
        closesocket(Client);
        SSL_CTX_free(sslContext);
        WSACleanup();
        return 1;
    }

    return true;
}


static DWORD WINAPI ThreadACClient(LPVOID ivalue)
{
    HANDLE ghEvents[3];
    int NumEvents;
    int iResult;
    DWORD dwEvent;
    LARGE_INTEGER liDueTime;

    char* InputBuffer = NULL;
    char* InputBufferWithOffset = NULL;
    unsigned int CurrentInputBufferSize = 1024 * 10;
    unsigned int InputBytesNeeded = 1024 * 10;

    InputBuffer = (char*)std::realloc(InputBuffer, CurrentInputBufferSize);
    InputBufferWithOffset = InputBuffer;

    if (InputBuffer == NULL)
    {
        std::cout << "InputBuffer Realloc failed" << std::endl;
        return 1;
    }

    liDueTime.QuadPart = 0LL;

    hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (NULL == hTimer)
    {
        std::cout << "CreateWaitableTimer failed " << GetLastError() << std::endl;
        return 2;
    }

    if (!SetWaitableTimer(hTimer, &liDueTime, ACS_DELAY, NULL, NULL, 0))
    {
        std::cout << "SetWaitableTimer failed  " << GetLastError() << std::endl;
        return 3;
    }

    hClientEvent = WSACreateEvent();
    hEndACClientEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (WSAEventSelect(Client, hClientEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
    {
        std::cout << "WSAEventSelect() failed with error " << WSAGetLastError() << std::endl;
        iResult = closesocket(Client);
        Client = INVALID_SOCKET;
        if (iResult == SOCKET_ERROR)
            std::cout << "closesocket function failed with error : " << WSAGetLastError() << std::endl;
        return 4;
    }
    ghEvents[0] = hEndACClientEvent;
    ghEvents[1] = hClientEvent;
    ghEvents[2] = hTimer;
    
    NumEvents = 2;

    while (1) {
        dwEvent = WaitForMultipleObjects(
            NumEvents,        // number of objects in array
            ghEvents,       // array of objects
            FALSE,           // wait for any object
            INFINITE);  // INFINITE) wait

        if (dwEvent == WAIT_OBJECT_0) 
            break;
        else if (dwEvent == WAIT_OBJECT_0 + 1)
        {
            WSAResetEvent(hClientEvent);
            WSANETWORKEVENTS NetworkEvents;
            if (SOCKET_ERROR == WSAEnumNetworkEvents(Client, hClientEvent, &NetworkEvents))
            {
                std::cout << "WSAEnumNetworkEvent: " << WSAGetLastError() << "dwEvent " << dwEvent << " lNetworkEvent " << std::hex << NetworkEvents.lNetworkEvents << std::endl;
                NetworkEvents.lNetworkEvents = 0;
            }
            else
            {
                if (NetworkEvents.lNetworkEvents & FD_READ)
                {
                    if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
                    {
                        std::cout << "FD_READ failed with error " << NetworkEvents.iErrorCode[FD_READ_BIT] << std::endl;
                    }
                    else
                    {
                        int iResult = 0;
                        sockaddr_in safrom{};
                        int socklen = sizeof(sockaddr_in);
                        size_t len;

                        iResult = SSL_read_ex(Clientssl, InputBufferWithOffset, InputBytesNeeded, &len);

                        if (iResult != SOCKET_ERROR)
                        {
                            //std::cout << "AC client recevied : " << InputBufferWithOffset << std::endl;
                            RecvHandler(Client, InputBufferWithOffset, len, safrom, socklen);
                        }
                        else 
                            std::cout << "ReadDataTcpNoBlock buff failed " << WSAGetLastError() << std::endl;

                    }
                }
                if (NetworkEvents.lNetworkEvents & FD_WRITE)
                {
                    if (NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
                    {
                        std::cout << "FD_WRITE failed with error " << NetworkEvents.iErrorCode[FD_WRITE_BIT] << std::endl;
                    }
                    else
                    {
                        std::cout << "FD_WRITE" << std::endl;
                    }
                }

                if (NetworkEvents.lNetworkEvents & FD_CLOSE)
                {
                    if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)

                    {
                        std::cout << "FD_CLOSE failed with error " << NetworkEvents.iErrorCode[FD_CLOSE_BIT] << std::endl;
                    }
                    else
                    {
                        std::cout << "FD_CLOSE" << std::endl;
                        PostMessage(hWndMain, WM_CLIENT_LOST, 0, 0);
                        break;
                    }
                }
            }
        }
        else if (dwEvent == WAIT_OBJECT_0 + 2)
        {
            char testText[1000];
            memcpy(testText, "Test timer from client", 22);
            if(sendMsgtoACS((char*)testText, sizeof(testText)))
            { 
                std::cout << "send Test Text to AC Server" << std::endl;
            }
            else
            {
                std::cout << "WriteDataTcp Failed " << WSAGetLastError() << std::endl;
                PostMessage(hWndMain, WM_CLIENT_LOST, 0, 0);
                break;
            }
        }

    }
    if (InputBuffer)
    {
        std::free(InputBuffer);
        InputBuffer = nullptr;
    }
    CleanUpAccessControlClient();
    std::cout << "Access Control Client Exiting" << std::endl;
    return 0;
}

int sendMsgtoACS(char* data, int len)
{
    size_t sent;
	SSL_write_ex(Clientssl, data, len, &sent);
    return sent;
}


//static int OnConnectACS(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
int OnConnectACS(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static char IpAdressACS[512] = ACS_IP;

    if (!IsACClientRunning())
    {
        if (ConnectToACSever(IpAdressACS, ACS_PORT))
        {
            std::cout << "Connected to Server" << std::endl;
            StartAccessControlClient();
            std::cout << "Client Started.." << std::endl;

            return 1;
        }
        else
        {
            std::cout << "Connection AC server Failed!" << std::endl;
            MessageBox(hWnd, TEXT("Connection AC server Failed!"), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
            return 0;
        }

    }
    else
    {
        std::cout << "AC client running" << std::endl;
        return 0;
    }

    return 0;
}

//static int OnDisconnectACS(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
int OnDisconnectACS(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsACClientRunning())
    {
        StopAccessControlClient();
        std::cout << "AC Client Stopped" << std::endl;
    }
    return 1;
}

static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen)
{
    oCommandOnly* getMsg = (oCommandOnly*)data;

    switch (getMsg->MessageType)
    {
    case RegistrationResponse:
    {
        // Registration
        std::cout << "registed User Information " << std::endl;

        if (getMsg->answer) {
            SendMessage(hwndCreateRegister, WM_DESTROY, 0, 0);
            SendMessage(hwndLogin, WM_DESTROY, 0, 0);
            MessageBox(NULL, L"Registered Successfully", L"", MB_OK);
        }
        else {
            TRspResultWithMessage* rsp = (TRspResultWithMessage*)data;

            LPCWSTR lpcwstr;

            int size = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)rsp->Message, -1, NULL, 0);
            wchar_t* wbuffer = new wchar_t[size];
            MultiByteToWideChar(CP_UTF8, 0, (LPCCH)rsp->Message, -1, wbuffer, size);
            lpcwstr = wbuffer;

            MessageBox(NULL, lpcwstr, L"", MB_OK);
        }

        break;
    }
    case LoginResponse:
    {
        TStatusInfo* sMsg = (TStatusInfo*)data;
        if (sMsg->status == VaildTwoFactor)
        {
            //devStatus = Connected;
            //PostMessage(hWndMainToolbar, TB_SETSTATE, IDM_CALL_REQUEST,
            //    (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
            //PostMessage(hWndMainToolbar, TB_SETSTATE, IDM_LOGIN,
            //    (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
            //PostMessage(hWndMainToolbar, TB_SETSTATE, IDM_LOGOUT,
            //    (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

            //strcpy_s(MyID, sMsg->myCID);
            //std::cout << "Lonin Success - ID : "<< MyID << std::endl;

            //TCommandOnly* msg = (TCommandOnly*)std::malloc(sizeof(TCommandOnly));
            //if (msg != NULL)
            //{
            //    msg->MessageType = RequestContactList;
            //    msg->answer = true;
            //    sendMsgtoACS((char*)msg, sizeof(TCommandOnly));
            //}

            //open Two factor popup
            //SendTFA(sMsg->myCID);
            strcpy_s(MyID, sMsg->myCID);
            PostMessage(hWndMain, WM_OPEN_TWOFACTORAUTH, 0, 0);
        }
        else
        {
            //devStatus = Disconnected;
            //PostMessage(hWndMain, WM_COMMAND, (WPARAM)IDM_LOGOUT, 0);
            PostMessage(hwndLogin, WM_DESTROY, 0, 0);
            std::cout << "Lonin Failed " << std::endl;
            //MessageBox(NULL, L"Login Failed", L"", MB_OK);
            TRspWithMessage *rsp = (TRspWithMessage * )data;

            LPCWSTR lpcwstr;


            int size = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)rsp->Message, -1, NULL, 0);
            wchar_t* wbuffer = new wchar_t[size];
            MultiByteToWideChar(CP_UTF8, 0, (LPCCH)rsp->Message, -1, wbuffer, size);
            lpcwstr = wbuffer;

            MessageBox(NULL, lpcwstr, L"", MB_OK);
        }

        break;
    }
	case TwoFactorResponse:
    {
        TStatusInfo* sMsg = (TStatusInfo*)data;
        if (sMsg->status == Connected)
        {
            devStatus = Connected;
            PostMessage(hWndMainToolbar, TB_SETSTATE, IDM_CALL_REQUEST,
                (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
            PostMessage(hWndMainToolbar, TB_SETSTATE, IDM_LOGIN,
                (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
            PostMessage(hWndMainToolbar, TB_SETSTATE, IDM_LOGOUT,
                (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

            strcpy_s(MyID, sMsg->myCID);
            std::cout << "Lonin Success - ID : " << MyID << std::endl;

            TCommandOnly* msg = (TCommandOnly*)std::malloc(sizeof(TCommandOnly));
            if (msg != NULL)
            {
                msg->MessageType = RequestContactList;
                msg->answer = true;
                sendMsgtoACS((char*)msg, sizeof(TCommandOnly));
                free(msg);
            }

            PostMessage(hwndLogin, WM_DESTROY, 0, 0);
            MessageBox(NULL, L"Login Success", L"", MB_OK);
        }
        else
        {
            devStatus = Disconnected;
            PostMessage(hWndMain, WM_COMMAND, (WPARAM)IDM_LOGOUT, 0);
            MessageBox(NULL, L"Login Failed", L"", MB_OK);
            std::cout << "Lonin Failed " << std::endl;
        }
        break;
    }
    case ResetPasswordResponse:
    {
        TStatusInfo* rMsg = (TStatusInfo*)data;
        if (rMsg->status == ResetPassword)
        {
            devStatus = ResetPassword;
            strcpy_s(MyID, rMsg->myCID);
            PostMessage(hWndMain, WM_OPEN_TWOFACTORAUTH, 0, 0);
        }

        break;
    }
    case ChangePasswordResponse:
    {
        PostMessage(hWndMain, WM_OPEN_REREGPASSWORD, 0, 0);
        break;
    }
    case ReRegPasswordResponse:
    {
        std::cout << "passwd chaged" << std::endl;
        break;
    }

    case LogoutResponse:
    {
        break;
    }
    case RequestStatus:
    {
        break;
    }
    case SendStatus:
    {
        break;
    }

    case SendContactList:
    {
        // Load stored data
        TContactList* clist = (TContactList*)data;
        int size = int(clist->ListSize);
        if (size == 0)
        {
            std::cout << "You're Only User in this System" << std::endl;
        }
        else
        {   
            char* buf;
            char* parse = strtok_s(clist->ListBuf, "/", &buf);
            makeContactList(parse, true);
            for (int i = 1; i < size; i++)
            {
                parse = strtok_s(NULL, "/", &buf);
                makeContactList(parse, false);
            }
           
            PostMessage(hWndMain, WM_OPEN_CONTACTLIST, 0, 0);
            std::cout << "make contact list" << std::endl;
        }

        break;
    }
    case RequestCall:
    {
        TDeviceID* tmp = (TDeviceID*)data;
        PostMessage(hWndMain, WM_OPEN_CALLREQUEST, 0, (LPARAM)tmp->FromDevID);

        break;
    }
    case AcceptCall:
    {
        TAcceptCall* tmp = (TAcceptCall*)data;

        std::cout << "make a call - status(" << int(devStatus) << ")" << std::endl;
        if (devStatus == Caller)
        {
            // OnConnect(tmp->IPAddress);
            PostMessage(hWndMain, WM_OPEN_VIDEOCLIENT, 0, (LPARAM)tmp->IPAddress);
            devStatus = Calling;
        }

        break;
    }
    case RejectCall:
    {
        //TDeviceID* tmp = (TDeviceID*)data;
        devStatus = Connected;
        break;
    }
    case MissedCall:
    {
        TMissedCall* req = (TMissedCall*)data;

        // need to discuss missed call structure
        for (int i = 0; i < MAX_DEVSIZE; i++) {
            printf("%s\n", req->ListBuf[i]);
        }

        break;
    }

    default:
        break;
    }
    return 0;
}

static int OnConnect(char* IPAddr)
{
    if (!IsVideoClientRunning())
    {
        if (OpenCamera())
        {
            if (ConnectToSever(IPAddr, VIDEO_PORT))
            {
                std::cout << "Connected to Server" << std::endl;
                StartVideoClient();
                std::cout << "Video Client Started.." << std::endl;
                VoipVoiceStart(IPAddr, VOIP_LOCAL_PORT, VOIP_REMOTE_PORT, VoipAttr);
                std::cout << "Voip Voice Started.." << std::endl;
                return 1;
            }
            else
            {
                std::cout << "Connection Failed!" << std::endl;
                return 0;
            }

        }
        else
        {
            std::cout << "Open Camera Failed" << std::endl;
            return 0;
        }
    }
    return 0;
}
