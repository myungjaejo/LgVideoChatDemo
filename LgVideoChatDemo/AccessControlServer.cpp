#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "definition.h"
#include < iostream >
#include <new>
#include "AccessControlServer.h"
#include <vector>


#include "LgVideoChatDemo.h"
#include "TcpSendRecv.h"
#include "filemanager.h"
#include "VideoServer.h"
//#include "definition.h"

static HANDLE hACServerListenerEvent = INVALID_HANDLE_VALUE;
static HANDLE hEndACServerEvent = INVALID_HANDLE_VALUE;
static HANDLE hAccepterEvent = INVALID_HANDLE_VALUE;
static HANDLE hThreadACServer = INVALID_HANDLE_VALUE;
static HANDLE hTimer = INVALID_HANDLE_VALUE;

static SOCKET Listener = INVALID_SOCKET;
static SOCKET Accepter = INVALID_SOCKET;


static DWORD ThreadACServerID;
static int NumEvents;

static std::vector<TRegistration *> controlDevices;

static void CleanUpACServer(void);
static DWORD WINAPI ThreadACServer(LPVOID ivalue);
static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen);
bool RegistrationUserData(TRegistration* data);
bool sendCommandOnlyMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, bool answer);
bool sendStatusMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, TStatus status);


// start Server
bool StartACServer(bool& Loopback)
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
		PostMessage(hWndMain, WM_REMOTE_LOST, 0, 0);
	}
	NumEvents = 2;
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

							int err = getnameinfo((struct sockaddr*)&sa, sa_len, RemoteIp, sizeof(RemoteIp), 0, 0, NI_NUMERICHOST);
							if (err != 0) {
								snprintf(RemoteIp, sizeof(RemoteIp), "invalid address");
							}
							else
							{
								std::cout << "Accepted Connection " << RemoteIp << std::endl;
							}

							PostMessage(hWndMain, WM_REMOTE_CONNECT, 0, 0);
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
						int result;
						sockaddr_in saFrom;
						int nFromLen = sizeof(sockaddr_in);

						if ((result = recvfrom(Accepter, testText, sizeof(testText), 0, (sockaddr *)&saFrom, &nFromLen)) != SOCKET_ERROR)
						{
							//std::cout << "Received : " << testText << std::endl;
							RecvHandler(Accepter, testText, result, saFrom, nFromLen);
							
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

			if(send(Accepter, testText, 24, 0))
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
				if (sendCommandOnlyMsg(__InputSock, sockip, socklen, true) != NULL)
				{

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
			TStatus resp = Disconnected;
			std::cout << "email :" << LoginData->email << ", pwhash : " << LoginData->passwordHash << std::endl;

			std::vector<TRegistration*>::iterator iter;
			for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
			{
				// need to implement compare email and pwhash !!
				TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
				
				if (!strncmp((*iter)->email, LoginData->email, (*iter)->EmailSize))
				{
					if (!strncmp((*iter)->password, LoginData->passwordHash, (*iter)->PasswordSize))
					{						
						resp = Connected;
					}
				}
			}
			std::cout << "send login response " << resp << std::endl;
			sendStatusMsg(__InputSock, sockip, socklen, resp);
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


bool sendCommandOnlyMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, bool answer)
{
	TCommandOnly* feedback = (TCommandOnly*)std::malloc(sizeof(TCommandOnly));
	if (feedback != NULL)
	{
		feedback->MessageType = RegistrationResponse;
		feedback->answer = answer;
		sendto(__InputSock, (char*)feedback, sizeof(TCommandOnly), 0, (sockaddr*)&sockip, socklen);
		free(feedback);
		return true;
	}
	return false;
}

bool sendStatusMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, TStatus status)
{
	TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
	if (feedback != NULL)
	{
		feedback->MessageType =LoginResponse;
		feedback->status = status;
		sendto(__InputSock, (char*)feedback, sizeof(TStatusInfo), 0, (sockaddr*)&sockip, socklen);
		free(feedback);
		return true;
	}
	return false;
}