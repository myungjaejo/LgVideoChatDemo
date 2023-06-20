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

static std::vector<TRegistration *> controlDevices;

static void CleanUpACServer(void);
static DWORD WINAPI ThreadACServer(LPVOID ivalue);
static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen);
bool RegistrationUserData(TRegistration* data);

SSL* ssl;

// start Server
bool StartACServer(bool &Loopback)
{
	if (hThreadACServer == INVALID_HANDLE_VALUE)
	{
		hThreadACServer = CreateThread(NULL, 0, ThreadACServer, 0, 0, &ThreadACServerID);
	}
	return true;
}

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
}

static void CloseConnection(void)
{
	if (Accepter != INVALID_SOCKET)
	{
		closesocket(Accepter);
		Accepter = INVALID_SOCKET;
	}
	NumEvents = 2;
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

	SSL_library_init();
	SSL_load_error_strings();
	SSL_CTX* sslContext = SSL_CTX_new(SSLv23_server_method());
	SSL_CTX_set_verify(sslContext, SSL_VERIFY_PEER, verify_callback);
	SSL_CTX_set_info_callback(sslContext, log_callback);
	if (sslContext == NULL)
	{
		fprintf(stderr, "Failed to create SSL context.\n");
		return 1;
	}

	if (SSL_CTX_use_certificate_file(sslContext, "server.crt", SSL_FILETYPE_PEM) != 1)
	{
		fprintf(stderr, "Failed to load server certificate.\n");
		//return 1;
	}
	if (SSL_CTX_use_PrivateKey_file(sslContext, "server.key", SSL_FILETYPE_PEM) != 1)
	{
		fprintf(stderr, "Failed to load server private key.\n");
		//return 1;
	}

	InternetAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ACS_IP, &InternetAddr.sin_addr.s_addr);
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
						if (Accepter == INVALID_SOCKET)
						{
							struct sockaddr_storage sa;
							socklen_t sa_len = sizeof(sa);
							char RemoteIp[INET6_ADDRSTRLEN];

							LARGE_INTEGER liDueTime;

							// Accept a new connection, and add it to the socket and event lists
							Accepter = accept(Listener, (struct sockaddr*)&sa, &sa_len);

							ssl = SSL_new(sslContext);

							if (ssl == NULL)
							{
								fprintf(stderr, "Failed to create SSL socket.\n");
								return 1;
							}

							if (SSL_set_fd(ssl, Accepter) != 1)
							{
								fprintf(stderr, "Failed to bind SSL socket.\n");
								return 1;
							}

							int result = SSL_accept(ssl);
							if (result != 1)
							{
								fprintf(stderr, "Failed to perform SSL handshake.\n");
								return 1;
							}

							int err = getnameinfo((struct sockaddr*)&sa, sa_len, RemoteIp, sizeof(RemoteIp), 0, 0, NI_NUMERICHOST);
							if (err != 0) {
								snprintf(RemoteIp, sizeof(RemoteIp), "invalid address");
							}
							else
							{
								std::cout << "Accepted Connection " << RemoteIp << std::endl;
							}

							hAccepterEvent = WSACreateEvent();
							WSAEventSelect(Accepter, hAccepterEvent, FD_READ | FD_WRITE | FD_CLOSE);
							ghEvents[2] = hAccepterEvent;
							NumEvents = 3;
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
						}
						else
						{
							SOCKET Temp = accept(Listener, NULL, NULL);

							if (Temp != INVALID_SOCKET)
							{
								closesocket(Temp);
								std::cout << "Refused-Already Connected" << std::endl;
							}

							ssl = SSL_new(sslContext);
							if (ssl == NULL)
							{
								fprintf(stderr, "Failed to create SSL socket.\n");
								return 1;
							}

							if (SSL_set_fd(ssl, Temp) != 1)
							{
								fprintf(stderr, "Failed to bind SSL socket.\n");
								return 1;
							}

							int result = SSL_accept(ssl);
							if (result != 1)
							{
								fprintf(stderr, "Failed to perform SSL handshake.\n");
								return 1;
							}
						}
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
		else if (dwEvent == WAIT_OBJECT_0 + 2)
		{
			WSANETWORKEVENTS NetworkEvents;
			if (SOCKET_ERROR == WSAEnumNetworkEvents(Accepter, hAccepterEvent, &NetworkEvents))
			{
				std::cout << "WSAEnumNetworkEvent: " << WSAGetLastError() << " dwEvent  " << dwEvent << " lNetworkEvent " << std::hex << NetworkEvents.lNetworkEvents << std::endl;
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
						char testText[1000];
						int result = 0;
						sockaddr_in saFrom{};
						int nFromLen = sizeof(sockaddr_in);
						size_t recieved;

						//if ((result = recvfrom(Accepter, testText, sizeof(testText), 0, (sockaddr *)&saFrom, &nFromLen)) != SOCKET_ERROR)
						//if (SSL_read(ssl, testText, sizeof(testText) > 0))
						if (SSL_read_ex(ssl, testText, sizeof(testText), &recieved) > 0)
						{
							//std::cout << "Received : " << testText << std::endl;
							RecvHandler(Accepter, testText, recieved, saFrom, nFromLen);
							
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
					CloseConnection();
				}
			}
		}
		else if (dwEvent == WAIT_OBJECT_0 + 3)
		{
			unsigned int numbytes;
			char testText[1000];
			memcpy(testText, "Send data from AC Server", 24);

			//if(send(Accepter, testText, 24, 0))
			if (SSL_write(ssl, testText, sizeof(testText)) < 0)
			{
				std::cout << "WriteDataTcp sendbuff.data() Failed " << WSAGetLastError() << std::endl;
				CloseConnection();
			}
			else {
				std::cout << "WriteDataTcp sendbuff.size() Failed " << WSAGetLastError() << std::endl;
				CloseConnection();
			}
	    }
	}

	CleanUpACServer();
	std::cout << "Access Control Server Stopped" << std::endl;
	return 0;
}

VOID CALLBACK ResetLoginAttempt(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	printf("reset attempt count\n");
	TRegistration* user = (TRegistration*)lpParameter;
	user->LoginAttempt = 0;
}

static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen )
{
	oCommandOnly *getMsg = (oCommandOnly*) data;

	switch(getMsg->MessageType)
	{
		case Registration:
		{
			// Registration
			TRegistration* regData = (TRegistration*)data;
			// Store Data - memory / storage
			std::cout << "get registration infomation" << std::endl;
			if (RegistrationUserData(regData))
			{
				TCommandOnly* feedback = (TCommandOnly*)std::malloc(sizeof(TCommandOnly));
				if (feedback != NULL)
				{
					feedback->MessageType = RegistrationResponse;
					feedback->answer = true;
					//sendto(__InputSock, (char *)feedback, sizeof(TCommandOnly), 0, (sockaddr*)&sockip, socklen);
					SSL_write(ssl, (char*)feedback, sizeof(TCommandOnly));
					free(feedback);
				}
				else
				{
					// TODO : ERROR
					std::cout << "memory error - malloc" << std::endl;
				}
			}
			else
			{
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
			
			std::cout << "email :" << LoginData->email << ", pwhash : " << LoginData->passwordHash << std::endl;

			TRspWithMessage2 rsp{};
			rsp.MessageType = (char)LoginResponse;
			rsp.MessageLen1 = (char)strlen("Login Failed");
			memcpy(rsp.Message1, "Login Failed", strlen("Login Failed"));
			bool Success = false;

			for (TRegistration* user : controlDevices) {
				std::cout << "email :" << user->email << ", pwhash : " << user->password << std::endl;
				if (!strncmp(LoginData->email, user->email, EMAIL_BUFSIZE))
				{
					if (user->LoginAttempt > MAX_ALLOW_LOGIN_ATTEMPT)
					{
						rsp.MessageLen1 = (char)strlen("Login restricted");
						memcpy(&rsp.Message1, "Login restricted", strlen("Login restricted"));
						break;
					}

					if (!strncmp(LoginData->passwordHash, user->password, PASSWORD_BUFSIZE))
					{
						if (user->LoginAttempt < MAX_ALLOW_LOGIN_ATTEMPT)
						{
							printf("login success");
							rsp.MessageLen1 = (char)strlen("Login Success");
							memcpy(&rsp.Message1, "Login Success", strlen("Login Success"));
							rsp.MessageLen2 = (char)strlen(user->email);
							memcpy(&rsp.Message2, user->email, strlen(user->email));
							Success = true;
							user->LoginAttempt = 0;
							break;
						}
					}
				}
			}

			if (!Success)
			{
				for (TRegistration* user : controlDevices) {
					if (!strncmp(LoginData->email, user->email, EMAIL_BUFSIZE))
					{
						user->LoginAttempt++;

						if (user->LoginAttempt == MAX_ALLOW_LOGIN_ATTEMPT)
						{
							HANDLE hTimer = NULL;
							HANDLE hTimerQueue = CreateTimerQueue();

							if (!hTimerQueue) {
								printf("hTimerQueue is null\n");
								break;
							}

							if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, ResetLoginAttempt, user, 3600000, 0, 0)) {
								printf("hTimerQueue is null\n");
								break;
							}
						}
						break;
					}
				}
			}

			//int sent = sendto(__InputSock, (char*)&rsp, sizeof(TRspWithMessage2), 0, (sockaddr*)&sockip, socklen);
			int sent = SSL_write(ssl, (const void *)&rsp, sizeof(TRspWithMessage2));

			// compare stored data

			// compare result
			// if true : status update
			// TStatusInfo *sinfo = (TStatusInfo *)std::malloc(sizeof(TStatusInfo));
			// sinfo->MessageType = SendStatus
			// sinfo->status = "<< connect status >>"
			// sendto(Accepter, (char *)sinfo, sizeof(TStatusInfo), 0, 0, <<sockaddr>>, <<sockaddr_len>>);

			// if false :

			//testlogin();

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
			// Load stored data

			// send contact list
			// TContactList *clist = (TContactList *)std::malloc(sizeof(TContactList));
			// clist->MessageType = SendContactList
			// for i in range(Sizeof stored clist)
			//		(clist->DevID).append(stored_clist[i])
			// sendto(Accepter, (char *)clist, sizeof(TContactList), 0, 0, <<sockaddr>>, <<sockaddr_len>>);
			TContactList rsp{};
			rsp.MessageType = SendContactList;
			rsp.ListSize = MAX_DEVSIZE;

			int i = 0;
			for (TRegistration* user : controlDevices) {
				memcpy(rsp.ListBuf[i], user->email, strlen(user->email));
				i++;
			}

			//int sent = sendto(__InputSock, (char*)&rsp, sizeof(TContactList), 0, (sockaddr*)&sockip, socklen);
			int sent = SSL_write(ssl, (char*)&rsp, sizeof(TContactList));

			break;
		}
		case RequestCall:
		{
			TDeviceID* tmp = (TDeviceID*)data;
			// char* dev_id = tmp->DevID;
			// std::cout << dev_id << std::endl;
			// Load stored IP_addres of Receiver 
			// sendto(Accepter, (char*)tmp, sizeof(TDeviceID), 0, 0, << sockaddr_forward >> , << sockaddr_len >> );
			break;
		}
		case AcceptCall:
		{
			TDeviceID* tmp = (TDeviceID*)data;
			// char* dev_id = tmp->DevID;
			// std::cout << dev_id << std::endl;
			break;
		}
		case RejectCall:
		{
			TDeviceID* tmp = (TDeviceID*)data;
			// char* dev_id = tmp->DevID;
			// std::cout << dev_id << std::endl;
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
					memcpy(rsp.ListBuf[i], user->MissedCall, strlen((const char *)user->MissedCall));
					break;
				}
				i++;
			}

			//int sent = sendto(__InputSock, (char*)&rsp, sizeof(TMissedCall), 0, (sockaddr*)&sockip, socklen);
			int sent = SSL_write(ssl, (char*)&rsp, sizeof(TMissedCall));

			break;
		}


		default:
			break;
	}
	return 0;
}


bool RegistrationToMem(TRegistration* data)
{
	int len = controlDevices.size();
	if (len < MAX_DEVSIZE )
	{
		TRegistration* input = (TRegistration*)std::malloc(sizeof(TRegistration));
		std::memset(input, 0, sizeof(TRegistration));
		if (input != NULL)
		{
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
	int len = controlDevices.size();
	if (len < (MAX_DEVSIZE+1) )
	{
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

#define MAX_PATH 1024

int _tmain(int argc, _TCHAR* argv[])
{
	char currentDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, currentDir);
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