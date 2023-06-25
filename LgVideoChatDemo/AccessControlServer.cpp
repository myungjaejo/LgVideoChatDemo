#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "definition.h"
#include < iostream >
#include <new>
#include "AccessControlServer.h"
#include <vector>
#include "filemanager.h"
#include <openssl/ssl.h>
#include <chrono>


#include "LgVideoChatDemo.h"
#include "TcpSendRecv.h"
#include "filemanager.h"
#include "VideoServer.h"
#include "TwoFactorAuth.h"
#include "Logger.h"
#include "TimeUtil.h"

#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "Ws2_32.lib")

static HANDLE hACServerListenerEvent = INVALID_HANDLE_VALUE;
static HANDLE hEndACServerEvent = INVALID_HANDLE_VALUE;
static HANDLE hAccepterEvent = INVALID_HANDLE_VALUE;
static HANDLE hThreadACServer = INVALID_HANDLE_VALUE;
static HANDLE hTimer = INVALID_HANDLE_VALUE;

static SOCKET Listener = INVALID_SOCKET;
static SOCKET Accepter = INVALID_SOCKET;

static bool Loopback = false;

static DWORD ThreadACServerID;
static int NumEvents;

typedef struct oSocketManager {
	char	IPAddr[IP_BUFFSIZE];
	char	Owner[NAME_BUFSIZE];
	SOCKET	ASocket;
	HANDLE	AcceptEvent;
    SSL *ssl;
}TSocketManager;

static std::vector<TRegistration *> controlDevices;
static std::vector<TSocketManager> sockmng;

static void CleanUpACServer(void);
static DWORD WINAPI ThreadACServer(LPVOID ivalue);
static int RecvHandler(TSocketManager *smgr, char* data, int datasize, sockaddr_in sockip, int socklen);
bool RegistrationUserData(TRegistration* data);
bool RegistrationToFile(void);
char* getFromAddr(SOCKET __InputSock);
void setMacro(SOCKET __InputSock, TRegistration* regData);
bool sendCommandOnlyMsg(TSocketManager *smgr, sockaddr_in sockip, int socklen, bool answer);
bool sendStatusMsg(TSocketManager *smgr, sockaddr_in sockip, int socklen, char* cid, TStatus status);
int findReceiverIP(char* recID, sockaddr_in* out);
SSL* ssl[10];
SSL_CTX* sslContext[10];
int sslInx = 0;
int sslCTXInx = 0;

int verify_callback(int preverify_ok, X509_STORE_CTX* ctx);
void log_callback(const SSL* ssl, int where, int ret);
int allowEarlyDataCallback(SSL* ssl, void* arg)
{
	printf("allowEarlyDataCallback\n");
	return SSL_EARLY_DATA_ACCEPTED;
}

// start Server
bool StartACServer(bool &Loopback)
{
	SSL_library_init();
	SSL_load_error_strings();

	if (hThreadACServer == INVALID_HANDLE_VALUE)
	{
		hThreadACServer = CreateThread(NULL, 0, ThreadACServer, 0, 0, &ThreadACServerID);
	}
	return true;
}

bool StopAccessControlClient(void);

// stop Server
bool StopACServer(void)
{
	if (hEndACServerEvent != INVALID_HANDLE_VALUE)
		SetEvent(hEndACServerEvent);

	if (hThreadACServer != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(hThreadACServer, INFINITE);
		hThreadACServer = INVALID_HANDLE_VALUE;
	}
	CleanUpACServer();
	StopAccessControlClient();

	return true;
}

// check server status
bool IsRunningACServer(void)
{
	if (hThreadACServer == INVALID_HANDLE_VALUE) return false;
	else return true;
}

// clear resource
static void CleanUpACServer(void)
{
	if (hACServerListenerEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hACServerListenerEvent);
		hACServerListenerEvent = INVALID_HANDLE_VALUE;
	}
	if (hAccepterEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hAccepterEvent);
		hAccepterEvent = INVALID_HANDLE_VALUE;
	}
	if (hEndACServerEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEndACServerEvent);
		hEndACServerEvent = INVALID_HANDLE_VALUE;
	}

	if (Listener != INVALID_SOCKET)
	{
		closesocket(Listener);
		Listener = INVALID_SOCKET;
	}
	if (Accepter != INVALID_SOCKET)
	{
		closesocket(Accepter);
		Accepter = INVALID_SOCKET;
	}

	static std::vector<TSocketManager>::iterator iter;
	for (iter = sockmng.begin(); iter != sockmng.end(); iter++)
	{
		closesocket((*iter).ASocket);
		CloseHandle((*iter).AcceptEvent);
	}
	sockmng.clear();

	RegistrationToFile();
	controlDevices.clear();
}

static void CloseConnectionACS(SOCKET __InputSock)
{	
	static std::vector<TSocketManager>::iterator iter;
	for (iter = sockmng.begin(); iter != sockmng.end(); iter++)
	{
		if ((*iter).ASocket == __InputSock)
			break;
	}

	if ((*iter).ASocket != INVALID_SOCKET)
	{
		closesocket((*iter).ASocket);
		(*iter).ASocket = INVALID_SOCKET;
		//PostMessage(hWndMain, WM_REMOTE_LOST, 0, 0);
	}
	sockmng.erase(iter);
	NumEvents -= 1;
}



static void CloseConnection(SOCKET __InputSock)
{
	if (__InputSock != INVALID_SOCKET)
	{
		closesocket(__InputSock);
		__InputSock = INVALID_SOCKET;
		PostMessage(hWndMain, WM_REMOTE_LOST, 0, 0);
	}
	NumEvents -= 1;
}

void log_callback(const SSL* ssl, int where, int ret)
{
	const char* str;
	int w;

	w = where & ~SSL_ST_MASK;
	if (w & SSL_ST_CONNECT) str = "SSL_connect";
	else if (w & SSL_ST_ACCEPT) str = "SSL_accept";
	else str = "undefined";

	if (where & SSL_CB_LOOP)
	{
		std::cout << str << ": " << SSL_state_string_long(ssl) << std::endl;
	}
	else if (where & SSL_CB_ALERT)
	{
		str = (where & SSL_CB_READ) ? "read" : "write";
		std::cout << "SSL3 alert " << str << ": " << SSL_alert_type_string_long(ret) << ": " << SSL_alert_desc_string_long(ret) << std::endl;
	}
	else if (where & SSL_CB_EXIT)
	{
		if (ret == 0)
		{
			std::cout << str << ": failed in " << SSL_state_string_long(ssl) << std::endl;
		}
		else if (ret < 0)
		{
			std::cout << str << ": error in " << SSL_state_string_long(ssl) << std::endl;
		}
	}
}


static DWORD WINAPI ThreadACServer(LPVOID ivalue)
{
	SOCKADDR_IN InternetAddr;	// ACServer IP
	HANDLE ghEvents[6];			// start , end , popup_list, callReq, getStatus, makeCall 
	DWORD dwEvent;

	std::cout << "make ACServer process....." << std::endl;
	if ((Listener = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		std::cout << "socket() failed with error " << WSAGetLastError() << std::endl;
		return 1;
	}
	int ret = BIO_set_tcp_ndelay(Listener, 1);
	if (ret < 0) {
		BIO_closesocket(Listener);
		return 0;
	}
	ret = 0;

	int opt = 1;
	setsockopt(Listener, IPPROTO_TCP, /*TCP_FASTOPEN_CONNECT*/30, (char*)&opt, sizeof(opt));

	// create Events handles
	std::cout << "create ACServer event....." << std::endl;
	hACServerListenerEvent = WSACreateEvent();
	hEndACServerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (WSAEventSelect(Listener, hACServerListenerEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		std::cout << "WSAEventSelect() failed with error " << WSAGetLastError() << std::endl;
		std::cout << "Thread handle = " << Listener << std::endl;
		return 1;
	}

	InternetAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "0.0.0.0", &InternetAddr.sin_addr.s_addr);
	InternetAddr.sin_port = htons(ACS_PORT);
	std::cout << "bind ACServer event....." << std::endl;
	if (bind(Listener, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind() failed with error " << WSAGetLastError() << std::endl;
		return 1;
	}

	if (listen(Listener, 5))
	{
		std::cout << "listen() failed with error " << WSAGetLastError() << std::endl;
		return 1;
	}

	ghEvents[0] = hEndACServerEvent;
	ghEvents[1] = hACServerListenerEvent;
	NumEvents = 2;

	// TODO : load register file
	int len_stored = getLengthJSON();
	std::cout << "JSON file obj size : " << len_stored << std::endl;
	for (int i = 0; i < len_stored; i++)
	{
		TRegistration* tmp = (TRegistration*)std::malloc(sizeof(TRegistration));
		std::memset(tmp, 0, sizeof(TRegistration));
		if (tmp != NULL)
		{
			LoadData(tmp, i);
			controlDevices.push_back(tmp);
		}
		else
		{
			std::cout << "Memory is not stable - malloc error" << std::endl;
			exit(-1);
		}
	}

	std::vector<TRegistration*>::iterator iter;
	for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
	{
		printFileObj(*iter);
	}

	while (1)
	{
		dwEvent = WaitForMultipleObjects(
			NumEvents,        // number of objects in array
			ghEvents,       // array of objects
			FALSE,           // wait for any object
			INFINITE);  // INFINITE) wait

		if (dwEvent == WAIT_OBJECT_0)
			break;
		else if (dwEvent == WAIT_OBJECT_0 + 1)
		{
			WSANETWORKEVENTS NetworkEvents;
			if (SOCKET_ERROR == WSAEnumNetworkEvents(Listener, hACServerListenerEvent, &NetworkEvents))
			{
				std::cout << "WSAEnumNetworkEvent: " << WSAGetLastError() << " dwEvent  " << dwEvent << " lNetworkEvent " << std::hex << NetworkEvents.lNetworkEvents << std::endl;
				NetworkEvents.lNetworkEvents = 0;
			}
			else
			{
				if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
				{
					if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
					{
						std::cout << "FD_ACCEPT failed with error " << NetworkEvents.iErrorCode[FD_ACCEPT_BIT] << std::endl;
					}
					else
					{
						//if (sockmng.size() == 0); //if (Accepter == INVALID_SOCKET)
						//{
							std::cout << "Try Accepted Connection " << std::endl;
							TSocketManager tmp{};

							struct sockaddr_storage sa;
							socklen_t sa_len = sizeof(sa);
							char RemoteIp[INET6_ADDRSTRLEN];

							LARGE_INTEGER liDueTime;

							sslContext[sslCTXInx] = SSL_CTX_new(SSLv23_server_method());
							SSL_CTX_set_max_early_data(sslContext[sslCTXInx], 1024);
							SSL_CTX_set_verify(sslContext[sslCTXInx], SSL_VERIFY_PEER, verify_callback);
							SSL_CTX_set_info_callback(sslContext[sslCTXInx], log_callback);
							SSL_CTX_set_allow_early_data_cb(sslContext[sslCTXInx], allowEarlyDataCallback, NULL);
							if (sslContext[sslCTXInx] == NULL)
							{
								fprintf(stderr, "Failed to create SSL context.\n");
								return 1;
							}

							if (SSL_CTX_use_certificate_file(sslContext[sslCTXInx], "server.crt", SSL_FILETYPE_PEM) != 1)
							{
								fprintf(stderr, "Failed to load server certificate.\n");
								return 1;
							}
							if (SSL_CTX_use_PrivateKey_file(sslContext[sslCTXInx], "server.key", SSL_FILETYPE_PEM) != 1)
							{
								fprintf(stderr, "Failed to load server private key.\n");
								return 1;
							}
							std::cout << "load key done" << std::endl;

							ssl[sslInx] = SSL_new(sslContext[sslCTXInx]);

							// Accept a new connection, and add it to the socket and event lists
							tmp.ASocket = accept(Listener, (struct sockaddr*)&sa, &sa_len);
							int opt = 1;
							setsockopt(tmp.ASocket, IPPROTO_TCP, /*TCP_FASTOPEN_CONNECT*/30, (char*)&opt, sizeof(opt));

							int ret = BIO_set_tcp_ndelay(tmp.ASocket, 1);
							if (ret < 0) {
								BIO_closesocket(tmp.ASocket);
								return 0;

							}
							ret = 0;

							if (tmp.ASocket == INVALID_SOCKET)
							{
								std::cout << "Accept Failed " << std::endl;
								return 1;
							}

							if (ssl == NULL)
							{
								fprintf(stderr, "Failed to create SSL socket.\n");
								return 1;
							}

							if (SSL_set_fd(ssl[sslInx], tmp.ASocket) != 1)
							{
								fprintf(stderr, "Failed to bind SSL socket.\n");
								return 1;
							}

							int result = SSL_accept(ssl[sslInx]);
							if (result != 1)
							{
								printf("Failed to perform SSL handshake.\n");
								//return 1;
							}

							tmp.ssl = ssl[sslInx];

							sslInx++;
							sslCTXInx++;

							int err = getnameinfo((struct sockaddr*)&sa, sa_len, RemoteIp, sizeof(RemoteIp), 0, 0, NI_NUMERICHOST);
							if (err != 0) {
								snprintf(RemoteIp, sizeof(RemoteIp), "invalid address");
							}
							else
							{
								std::cout << "Accepted Connection " << RemoteIp << " at " << tmp.ASocket << std::endl;
								strcpy_s(tmp.IPAddr, RemoteIp);
							}

							//PostMessage(hWndMain, WM_REMOTE_CONNECT, 0, 0);
							//hAccepterEvent = WSACreateEvent();
							tmp.AcceptEvent = WSACreateEvent();
							//WSAEventSelect(Accepter, hAccepterEvent, FD_READ | FD_WRITE | FD_CLOSE);
							WSAEventSelect(tmp.ASocket, tmp.AcceptEvent, FD_READ | FD_WRITE | FD_CLOSE);							
							ghEvents[NumEvents] = tmp.AcceptEvent;
							sockmng.push_back(tmp);
							NumEvents += 1;
							/*
							hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
							if (NULL == hTimer)
							{
								std::cout << "CreateWaitableTimer failed " << GetLastError() << std::endl;
								break;
							}

							if (!SetWaitableTimer(hTimer, &liDueTime, VIDEO_FRAME_DELAY, NULL, NULL, 0))
							{
								std::cout << "SetWaitableTimer failed  " << GetLastError() << std::endl;
								break;
							}
							ghEvents[3] = hTimer;
							NumEvents = 4;
							*/
						//}
						/*			else
						{
							SOCKET Temp = accept(Listener, NULL, NULL);
							if (Temp != INVALID_SOCKET)
							{
								closesocket(Temp);
								std::cout << "Refused-Already Connected" << std::endl;
							}
						}*/
					}
				}

				if (NetworkEvents.lNetworkEvents & FD_CLOSE)
				{
					if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
					{
						std::cout << "FD_CLOSE failed with error on Listen Socket" << NetworkEvents.iErrorCode[FD_CLOSE_BIT] << std::endl;
					}

					closesocket(Listener);
					Listener = INVALID_SOCKET;
				}
			}
		}
		else if (dwEvent >= WAIT_OBJECT_0 + 2)
		{
			WSANETWORKEVENTS NetworkEvents;
			if (SOCKET_ERROR == WSAEnumNetworkEvents(sockmng[dwEvent-2].ASocket, sockmng[dwEvent - 2].AcceptEvent, &NetworkEvents))
			{
				std::cout << "WSAEnumNetworkEvent: " << WSAGetLastError() << " dwEvent  " << dwEvent << " lNetworkEvent " << std::hex << NetworkEvents.lNetworkEvents << std::endl;
				NetworkEvents.lNetworkEvents = 0;
			}
			else
			{
				std::cout << "client FD_READ" << std::endl;
				if (NetworkEvents.lNetworkEvents & FD_READ)
				{
					if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
					{
						std::cout << "FD_READ failed with error " << NetworkEvents.iErrorCode[FD_READ_BIT] << std::endl;
					}
					else
					{
						char testText[8192] = {0,};
						int result = 0;
						sockaddr_in saFrom{};
						int nFromLen = sizeof(sockaddr_in);
						size_t recieved;

						if (SSL_read_ex(sockmng[dwEvent - 2].ssl, testText, sizeof(testText) - 1, &recieved) > 0)
						{
							//std::cout << "Received : " << testText << std::endl;
							RecvHandler(&sockmng[dwEvent - 2], testText, recieved, saFrom, nFromLen);
							
						}
						else 
							std::cout << "ReadData failed " << WSAGetLastError() << std::endl;
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
						std::cout << "FD_CLOSE failed with error Connection " << NetworkEvents.iErrorCode[FD_CLOSE_BIT] << std::endl;
					}
					else
					{
						std::cout << "FD_CLOSE" << std::endl;

					}
					CloseConnectionACS(sockmng[dwEvent - 2].ASocket);
				}
			}
		}
		//else if (dwEvent == WAIT_OBJECT_0 + 3)
		//{
		//	unsigned int numbytes;
		//	char testText[1000];
		//	memcpy(testText, "Send data from AC Server", 24);

		//	if(send(Accepter, testText, 24, 0))
		//	{
		//		std::cout << "WriteDataTcp sendbuff.data() Failed " << WSAGetLastError() << std::endl;
		//		CloseConnection();
		//	}
		//	else {
		//		std::cout << "WriteDataTcp sendbuff.size() Failed " << WSAGetLastError() << std::endl;
		//		CloseConnection();
		//	}
	 //   }
	}

	//CleanUpACServer();
	std::cout << "Access Control Server Stopped" << std::endl;
	return 0;
}

VOID CALLBACK ResetLoginAttempt(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	printf("reset attempt count\n");
	TRegistration* user = (TRegistration*)lpParameter;
	user->LoginAttempt = 0;
}

static int RecvHandler(TSocketManager *smgr, char* data, int datasize, sockaddr_in sockip, int socklen )
{
	oCommandOnly *getMsg = (oCommandOnly*) data;

	printf("RecvHandler = %c\n", getMsg->MessageType);

	switch (getMsg->MessageType)
	{
	case Registration:
	{
		// Registration
		TRegistration* regData = (TRegistration*)data;
		// Store Data - memory / storage
		std::cout << "get registration infomation" << std::endl;

		for (TRegistration* user : controlDevices)
		{
			TRspResultWithMessage rsp{};
			rsp.MessageType = RegistrationResponse;
			rsp.answer = false;
			if (!strncmp(user->ContactID, regData->ContactID, NAME_BUFSIZE))
			{
				memcpy((char *)rsp.Message, "Duplicated ConntactID", strlen("Duplicated ConntactID"));
				rsp.MessageLen = strlen("Duplicated ConntactID");
				return SSL_write(smgr->ssl, &rsp, sizeof(rsp));
			}
			if (!strncmp(user->email, regData->email, EMAIL_BUFSIZE))
			{
				memcpy((char*)rsp.Message, "Duplicated Email", strlen("Duplicated ConntactID"));
				rsp.MessageLen = strlen("Duplicated Email");
				return SSL_write(smgr->ssl, &rsp, sizeof(rsp));
			}
		}

		// macro
		setMacro(smgr->ASocket, regData);

		GetCurrentDateTime(regData->LastPasswordChange, sizeof(regData->LastPasswordChange));

		if (RegistrationUserData(regData))
		{
			if (sendCommandOnlyMsg(smgr, sockip, socklen, true) != NULL)
			{

			}
			else
			{
				sendCommandOnlyMsg(smgr, sockip, socklen, false);
				// TODO : ERROR
				std::cout << "memory error - malloc" << std::endl;
			}
		}
		else
		{
			sendCommandOnlyMsg(smgr, sockip, socklen, false);
			//TODO :: ERROR
			std::cout << "Full data error" << std::endl;
		}

		//StoreData()

		// update IP info

		break;
	}
	case Login:
	{
		// Login
		TLogin* LoginData = (TLogin*)data;
		TStatus resp = Disconnected;
		std::cout << "email :" << LoginData->email << ", pwhash : " << LoginData->passwordHash << std::endl;

		char myCID[NAME_BUFSIZE] = "None";

		std::vector<TRegistration*>::iterator iter;
		for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
		{
			if (!strncmp((*iter)->email, LoginData->email, LoginData->EmailSize))
			{
				if (!strncmp((*iter)->password, LoginData->passwordHash, LoginData->PasswordHashSize))
				{
					//resp = Connected;

					//strcpy_s(myCID, (*iter)->ContactID);

					//std::vector<TSocketManager>::iterator iitt;
					//for (iitt = sockmng.begin(); iitt != sockmng.end(); iitt++)
					//{
					//	//std::cout << "search "
					//	if ((*iitt).ASocket == __InputSock)
					//	{
					//		strcpy_s((*iitt).Owner, myCID);
					//		strcpy_s((*iter)->LastIPAddress, (*iitt).IPAddr);
					//		std::cout << "Login Information : " << (*iter)->email << " / " << myCID << " / " << (*iitt).IPAddr << std::endl;
					//		break;
					//	}
					//}

					(*iter)->LoginAttempt = 0;
					resp = VaildTwoFactor;
					strcpy_s(myCID, (*iter)->ContactID);
					SendTFA(LoginData->email);
					break;
				}
				else
				{
					(*iter)->LoginAttempt += 1;
					if ((*iter)->LoginAttempt > MAX_ALLOW_LOGIN_ATTEMPT)
					{
						std::cout << "Login restricted - Too Many Login attempt" << std::endl;
						HANDLE hTimer = NULL;
						HANDLE hTimerQueue = CreateTimerQueue();

						if (!hTimerQueue) {
							printf("hTimerQueue is null\n");
							break;
						}

						//if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, ResetLoginAttempt, user, 3600000, 0, 0)) {
						if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, ResetLoginAttempt, (*iter), 10000, 0, 0)) {
							printf("hTimerQueue is null\n");
							break;
						}

						break;
					}
				}
			}
		}

		std::cout << "send login response to " << resp << " in " << myCID << " and " << sockmng.size() << ", evt : " << NumEvents << std::endl;
		sendStatusMsg(smgr, sockip, socklen, myCID, resp);

		break;
	}
	case TwoFactorRequest:
	{
		TTwoFactor* tMsg = (TTwoFactor*)data;
		TStatus resp = Disconnected;
		char myCID[NAME_BUFSIZE] = "None";
		std::cout << "RECEIVED TOKEN : " << tMsg->TFA << std::endl;
		int iResult = ReadTFA(tMsg->TFA);
		if (iResult == TFA_SUCCESS)
		{
			resp = Connected;
			strcpy_s(myCID, tMsg->myCID);

			std::vector<TSocketManager>::iterator iitt;
			for (iitt = sockmng.begin(); iitt != sockmng.end(); iitt++)
			{
				//std::cout << "search "
				if ((*iitt).ASocket == smgr->ASocket)
				{
					std::vector<TRegistration*>::iterator iter;
					for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
					{
						if (!strcmp((*iter)->ContactID, myCID))
						{
							SYSTEMTIME CurTime, LastPasswordChangedTime;
							GetLocalTime(&CurTime);
							ConvertDateTime((*iter)->LastPasswordChange, &LastPasswordChangedTime);

							if (IsTimeDifferenceGreaterThanOneMonth(CurTime, LastPasswordChangedTime))
							{
								TRspWithMessage rsp{};
								rsp.MessageType = LoginResponse;
								rsp.MessageLen = strlen("Password need to be updated. it changed one month ago.");
								memcpy(rsp.Message, "Password need to be updated. it changed one month ago.", strlen("Password need to be updated.it changed one month ago."));
								return SSL_write(smgr->ssl, &rsp, sizeof(rsp));
							}

							strcpy_s((*iitt).Owner, myCID);
							strcpy_s((*iter)->LastIPAddress, (*iitt).IPAddr);
							std::cout << "Login Information : " << (*iter)->email << " / " << myCID << " / " << (*iitt).IPAddr << std::endl;
							break;
						}
					}
					if (iter == controlDevices.end())
					{
						std::cout << "Can't find user" << std::endl;
					}
					break;
				}
			}
			TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
			if (feedback != NULL)
			{
				feedback->MessageType = TwoFactorResponse;
				feedback->status = Connected;
				strcpy_s(feedback->myCID, myCID);
				SSL_write(smgr->ssl, (char*)feedback, sizeof(TStatusInfo));
				free(feedback);
				return true;
			}
		}
		else
		{
			std::cout << "Wrong Value" << std::endl;
			TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
			if (feedback != NULL)
			{
				feedback->MessageType = TwoFactorResponse;
				feedback->status = Disconnected;
				strcpy_s(feedback->myCID, myCID);
				SSL_write(smgr->ssl, (char*)feedback, sizeof(TStatusInfo));
				free(feedback);
				return true;
			}
		}
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

	case RequestContactList:
	{
		TContactList clist{};
		clist.MessageType = SendContactList;
		clist.ListSize = (char)0;
		strcpy_s(clist.ListBuf, "");
		// Load stored data
		std::vector<TRegistration*>::iterator iter;
		for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
		{
			strcat_s(clist.ListBuf, (*iter)->ContactID);
			strcat_s(clist.ListBuf, "/");
			clist.ListSize += 1;
		}
		std::cout << "merge data : " << clist.ListBuf << std::endl;
		// send contact list
		SSL_write(smgr->ssl, (char*)&clist, sizeof(clist));

		break;
	}
	case RequestCall:
	{
		TDeviceID* tmp = (TDeviceID*)data;
		char* fromDev = tmp->FromDevID;
		char* dev_id = tmp->ToDevID;
		std::cout << "Call request to " << dev_id << std::endl;
		std::vector<TSocketManager>::iterator iit;
		for (iit = sockmng.begin(); iit != sockmng.end(); iit++)
		{
			std::cout << "socket info : " << (*iit).Owner << " -> " << (*iit).IPAddr << std::endl;
		}

		// Load stored IP_addres of Receiver
		std::vector<TRegistration*>::iterator iter;
		for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
		{
			if (!strncmp(dev_id, (*iter)->ContactID, NAME_BUFSIZE))
			{
				TDeviceID msg{};
				msg.MessageType = RequestCall;

				strcpy_s(msg.FromDevID, fromDev);
				strcpy_s(msg.ToDevID, dev_id);

				std::vector<TSocketManager>::iterator iitt;
				for (iitt = sockmng.begin(); iitt != sockmng.end(); iitt++)
				{
					if (!strcmp((*iitt).Owner, (*iter)->ContactID))
					{
						std::cout << "transfer request : " << msg.FromDevID << " -> " << msg.ToDevID << " / " << (*iitt).IPAddr << std::endl;
						int ret = SSL_write((*iitt).ssl, (char*)&msg, sizeof(msg));
						std::cout << (*iitt).ASocket << ", " << ret << std::endl;
						break;
					}
				}
			}
		}

		break;
	}
	case AcceptCall:
	{
		TDeviceID* tmp = (TDeviceID*)data;
		char* fromDev = tmp->FromDevID;
		char* dev_id = tmp->ToDevID;
		std::cout << "Call Accepted from " << fromDev << " to " << dev_id << std::endl;

		TAcceptCall msg{};
		msg.MessageType = AcceptCall;

		std::vector<TSocketManager>::iterator iter;
		for (iter = sockmng.begin(); iter != sockmng.end(); iter++)
		{
			if (!strncmp(fromDev, (*iter).Owner, NAME_BUFSIZE))
			{
				strcpy_s(msg.IPAddress, (*iter).IPAddr);
				break;
			}
		}

		for (iter = sockmng.begin(); iter != sockmng.end(); iter++)
		{
			if (!strncmp(dev_id, (*iter).Owner, NAME_BUFSIZE))
			{
				strcpy_s(msg.FromDevID, fromDev);
				strcpy_s(msg.ToDevID, dev_id);

				SSL_write((*iter).ssl, (char*)&msg, sizeof(msg));
				std::cout << "Accept call : " << msg.FromDevID << " -> " << msg.ToDevID << " / " << msg.IPAddress << std::endl;
				break;
			}
		}

		break;
	}
	case RejectCall:
	{
		TDeviceID* tmp = (TDeviceID*)data;
		char* fromDev = tmp->FromDevID;
		char* dev_id = tmp->ToDevID;
		std::cout << "Call rejected from " << fromDev << " to " << dev_id << std::endl;
		sockaddr_in sockto;
		int iResult = findReceiverIP(fromDev, &sockto);
		if (iResult != 0)
		{
			TDeviceID msg{};
			msg.MessageType = RejectCall;

			strcpy_s(msg.FromDevID, fromDev);
			strcpy_s(msg.ToDevID, dev_id);
			SSL_write(smgr->ssl, (char*)&msg, sizeof(msg));
		}
		break;
	}
	case MissedCall:
	{
		TReqwithMessage* req = (TReqwithMessage*)data;
		TMissedCall rsp{};

		rsp.MessageType = MissedCall;
		rsp.ListSize = MAX_DEVSIZE;

		int i = 0;
		for (TRegistration* user : controlDevices) {
			if (!strncmp((const char*)req->Message, user->email, req->MessageLen)) {
				memcpy(rsp.ListBuf[i], user->MissedCall, strlen((const char*)user->MissedCall));
				break;
			}
			i++;
		}

		//int sent = sendto(__InputSock, (char*)&rsp, sizeof(TMissedCall), 0, (sockaddr*)&sockip, socklen);
		SSL_write(smgr->ssl, (char*)&rsp, sizeof(TMissedCall));
		break;
	}

	case ResetPasswordRequest:
	{
		TLogin* msg = (TLogin*)data;
		bool Status = false;

		std::vector<TRegistration*>::iterator iter;
		for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
		{
			if (!strcmp(msg->email, (*iter)->email))
			{
				Status = true;
				break;
			}
		}

		TStatusInfo* sMsg = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
		if (sMsg)
		{
			sMsg->MessageType = (unsigned char)ResetPasswordResponse;
			sMsg->status = Disconnected;
			if (Status)
			{
				SendTFA(msg->email);
				strcpy_s(sMsg->myCID, msg->email);
				sMsg->status = ResetPassword;
			}
			SSL_write(smgr->ssl, (char*)sMsg, sizeof(TStatusInfo));
			free(sMsg);
		}
		break;
	}
	case ChangePasswordRequest:
	{
		TTwoFactor* tMsg = (TTwoFactor*)data;
		TStatus resp = Disconnected;
		char myCID[NAME_BUFSIZE] = "None";
		std::cout << "RECEIVED TOKEN : " << tMsg->TFA << std::endl;
		TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));

		int iResult = ReadTFA(tMsg->TFA);
		if (iResult == TFA_SUCCESS)
		{
			feedback->status = ResetPassword;
		}
		else
		{
			feedback->status = (unsigned char)Disconnected;
			std::cout << "Wrong Value - ret : " << iResult << std::endl;
		}

		if (feedback != NULL)
		{
			feedback->MessageType = ChangePasswordResponse;
			SSL_write(smgr->ssl, (char*)feedback, sizeof(TStatusInfo));
			free(feedback);
		}

		break;
	}

	case ReRegPasswordRequest:
	{
		TReRegistration* msg = (TReRegistration*)data;
		TStatus resp = Disconnected;

		std::vector<TRegistration*>::iterator iter;
		for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
		{
			if (!strcmp(msg->email, (*iter)->email))
			{
				std::cout << "Re-Registration Password to " << (*iter)->email << std::endl;
				memcpy((*iter)->password, msg->password, msg->PasswordSize);
				GetCurrentDateTime((*iter)->LastPasswordChange, sizeof((*iter)->LastPasswordChange));
				break;
			}
		}
		// ? update
		RegistrationToFile();

		TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));

		if (feedback != NULL)
		{
			feedback->MessageType = ChangePasswordResponse;
			SSL_write(smgr->ssl, (char*)feedback, sizeof(TStatusInfo));
			free(feedback);
		}

		break;
	}

	default:
		break;
	}
	return 0;
}


bool RegistrationToMem(TRegistration* data)
{
	size_t len = controlDevices.size();
	if (len < MAX_DEVSIZE )
	{
		TRegistration* input = (TRegistration*)std::malloc(sizeof(TRegistration));

		if (input != NULL)
		{
			std::memset(input, 0, sizeof(TRegistration));
			memcpy(input, data, sizeof(TRegistration));
			controlDevices.push_back(input);
			return true;
		}
		else
		{
			std::cout << "memory error - malloc" << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "isFull regist memory" << std::endl;
		return false;
	}
}

bool RegistrationToFile(void)
{
	size_t len = controlDevices.size();
	if (len < (MAX_DEVSIZE+1) )
	{
		std::cout << "Store File - size :" << len << std::endl;
		StoreData(controlDevices);
		return true;
	}
	else
	{
		std::cout << "isFull regist memory" << std::endl;
		return false;
	}
}

bool RegistrationUserData(TRegistration* data)
{
	if (!RegistrationToMem(data)) return false;
	if (!RegistrationToFile()) return false;
	return true;
}

bool sendCommandOnlyMsg(TSocketManager *smgr, sockaddr_in sockip, int socklen, bool answer)
{
	TCommandOnly* feedback = (TCommandOnly*)std::malloc(sizeof(TCommandOnly));
	if (feedback != NULL)
	{
		feedback->MessageType = RegistrationResponse;
		feedback->answer = answer;
		SSL_write(smgr->ssl, (char*)feedback, sizeof(TCommandOnly));
		free(feedback);
		return true;
	}
	return false;
}

bool sendStatusMsg(TSocketManager *smgr, sockaddr_in sockip, int socklen, char* cid, TStatus status)
{
	TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
	if (feedback != NULL)
	{
		feedback->MessageType =LoginResponse;
		feedback->status = status;
		strcpy_s(feedback->myCID, cid);
		SSL_write(smgr->ssl, (char*)feedback, sizeof(TStatusInfo));
		free(feedback);
		return true;
	}
	return false;
}

void setMacro(SOCKET __InputSock,TRegistration* regData)
{
	auto now = std::chrono::system_clock::now();
	std::time_t end_time = std::chrono::system_clock::to_time_t(now);
	char* buf = (char*)std::malloc(sizeof(char) * 128);
	if (buf)
	{
		ctime_s(buf,128, &end_time);
		strcpy_s(regData->LastRegistTime, buf);
	} else{
		LOG("Failed to alloc buf\n");
		return;
	}

	char ipbuf[32];
	sockaddr_in sock_a;
	int size = sizeof(sockaddr_in);
	memset(&sock_a, 0x00, sizeof(sock_a));
	getpeername(__InputSock, (sockaddr*)&sock_a, &size);
	inet_ntop(AF_INET, &sock_a.sin_addr, ipbuf, sizeof(ipbuf));

	std::cout << "IP address - " << ipbuf << std::endl;
	strcpy_s(regData->LastIPAddress, 32, ipbuf);
	free(buf);
}

char* getFromAddr(SOCKET __InputSock)
{
	char* ipbuf = (char *)std::malloc(sizeof(char)*32);
	sockaddr_in sock_a;
	int size = sizeof(sockaddr_in);
	memset(&sock_a, 0x00, sizeof(sock_a));
	getpeername(__InputSock, (sockaddr*)&sock_a, &size);
	inet_ntop(AF_INET, &sock_a.sin_addr, ipbuf, sizeof(ipbuf));

	return ipbuf;
}

int findReceiverIP(char* recID, sockaddr_in *out)
{
	std::vector<TRegistration*>::iterator iter;
	for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
	{
		if (!strncmp(recID, (*iter)->ContactID, NAME_BUFSIZE))
		{
			int ss = sizeof(sockaddr_in);
			(*out).sin_family = AF_INET;
			inet_pton(AF_INET, (*iter)->LastIPAddress, &((*out).sin_addr.s_addr));
			(*out).sin_port = htons(ACS_PORT);
			return ss;
		}
	}
	return 0;
}



int _tmain(int argc, _TCHAR* argv[])
{
	char currentDir[1024];
	GetCurrentDirectoryA(1024, currentDir);
	std::cout << "Current Directory: " << currentDir << std::endl;

	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	
	WSADATA wsaData;

	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << res << std::endl;
		return 1;
	}

	StartACServer(Loopback);

	printf("Server Runnning\n");
	SuspendThread(GetCurrentThread());
	return 0;
}