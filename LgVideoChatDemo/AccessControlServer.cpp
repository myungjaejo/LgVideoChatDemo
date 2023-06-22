#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "definition.h"
#include < iostream >
#include <new>
#include "AccessControlServer.h"
#include <vector>
#include <chrono>


#include "LgVideoChatDemo.h"
#include "TcpSendRecv.h"
#include "filemanager.h"
#include "VideoServer.h"
#include "TwoFactorAuth.h"

static HANDLE hACServerListenerEvent = INVALID_HANDLE_VALUE;
static HANDLE hEndACServerEvent = INVALID_HANDLE_VALUE;
static HANDLE hAccepterEvent = INVALID_HANDLE_VALUE;
static HANDLE hThreadACServer = INVALID_HANDLE_VALUE;
static HANDLE hTimer = INVALID_HANDLE_VALUE;

static SOCKET Listener = INVALID_SOCKET;
static SOCKET Accepter = INVALID_SOCKET;


static DWORD ThreadACServerID;
static int NumEvents;

typedef struct oSocketManager {
	char	IPAddr[IP_BUFFSIZE];
	char	Owner[NAME_BUFSIZE];
	SOCKET	ASocket;
	HANDLE	AcceptEvent;
}TSocketManager;

static std::vector<TRegistration *> controlDevices;
static std::vector<TSocketManager> sockmng;

static void CleanUpACServer(void);
static DWORD WINAPI ThreadACServer(LPVOID ivalue);
static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen);
bool RegistrationUserData(TRegistration* data);
bool RegistrationToFile(void);
char* getFromAddr(SOCKET __InputSock);
void setMacro(SOCKET __InputSock, TRegistration* regData);
bool sendCommandOnlyMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, bool answer);
bool sendStatusMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, char* cid, TStatus status);
int findReceiverIP(char* recID, sockaddr_in* out);


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
						//if (sockmng.size() == 0); //if (Accepter == INVALID_SOCKET)
						//{
							std::cout << "Try Accepted Connection " << std::endl;
							TSocketManager tmp{};

							struct sockaddr_storage sa;
							socklen_t sa_len = sizeof(sa);
							char RemoteIp[INET6_ADDRSTRLEN];

							LARGE_INTEGER liDueTime;

							// Accept a new connection, and add it to the socket and event lists
							tmp.ASocket = accept(Listener, (struct sockaddr*)&sa, &sa_len);

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
						char testText[1024];
						int result;
						sockaddr_in saFrom;
						int nFromLen = sizeof(sockaddr_in);

						if ((result = recvfrom(sockmng[dwEvent - 2].ASocket, testText, sizeof(testText), 0, (sockaddr *)&saFrom, &nFromLen)) != SOCKET_ERROR)
						{
							//std::cout << "Received : " << testText << std::endl;
							RecvHandler(sockmng[dwEvent - 2].ASocket, testText, result, saFrom, nFromLen);							
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


static int RecvHandler(SOCKET __InputSock, char* data, int datasize, sockaddr_in sockip, int socklen)
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
			
			// macro
			setMacro(__InputSock, regData);

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

			char myCID[NAME_BUFSIZE] = "None";
			std::vector<TRegistration*>::iterator iter;
			for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
			{
				// need to implement compare email and pwhash !!
				TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
				
				if (!strncmp((*iter)->email, LoginData->email, (*iter)->EmailSize))
				{
					if (!strncmp((*iter)->password, LoginData->passwordHash, (*iter)->PasswordSize))
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

						resp = VaildTwoFactor;
						strcpy_s(myCID, (*iter)->ContactID);
						SendTFA(LoginData->email);
						break;
					}
				}
			}
			std::cout << "send login response to "<< resp << " in "  << myCID << " and " << sockmng.size() << ", evt : "  << NumEvents << std::endl;
			sendStatusMsg(__InputSock, sockip, socklen, myCID, resp);

			break;
		}
		case TwoFactorRequest:
		{
			TTwoFactor* tMsg = (TTwoFactor *) data;
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
					if ((*iitt).ASocket == __InputSock)
					{
						std::vector<TRegistration*>::iterator iter;
						for (iter = controlDevices.begin(); iter != controlDevices.end(); iter++)
						{
							if (!strcmp((*iter)->ContactID, myCID))
							{
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
					sendto(__InputSock, (char*)feedback, sizeof(TStatusInfo), 0, (sockaddr*)&sockip, socklen);
					free(feedback);
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
					sendto(__InputSock, (char*)feedback, sizeof(TStatusInfo), 0, (sockaddr*)&sockip, socklen);
					free(feedback);
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
			sendto(__InputSock, (char*)&clist, sizeof(clist), 0, (sockaddr*)&sockip, socklen);	

			break;
		}
		case RequestCall:
		{
			TDeviceID* tmp = (TDeviceID*)data;
			char* fromDev = tmp->FromDevID;
			char* dev_id = tmp->ToDevID;
			std::cout << "Call request to "<< dev_id << std::endl;
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
							int ret = send((*iitt).ASocket, (char*)&msg, sizeof(msg), 0);
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
			std::cout << "Call Accepted from "<< fromDev<< " to " << dev_id << std::endl;

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
					
					send((*iter).ASocket, (char*)&msg, sizeof(msg), 0);
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
				sendto(__InputSock, (char*)&msg, sizeof(msg), 0, (sockaddr*)&sockto, iResult);
			}
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
			sMsg->MessageType = ResetPasswordResponse;
			sMsg->status = Disconnected;
			if (Status)
			{
				SendTFA(msg->email);
				strcpy_s(sMsg->myCID, msg->email);
				sMsg->status = ResetPassword;
			}
			send(__InputSock, (char*)sMsg, sizeof(TStatusInfo), 0);
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
				feedback->status = Disconnected;
				std::cout << "Wrong Value - ret : " << iResult << std::endl;
			}

			if (feedback != NULL)
			{
				feedback->MessageType = ChangePasswordResponse;
				sendto(__InputSock, (char*)feedback, sizeof(TStatusInfo), 0, (sockaddr*)&sockip, socklen);
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
					break;
				}
			}

			TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));


			if (feedback != NULL)
			{
				feedback->MessageType = ChangePasswordResponse;
				sendto(__InputSock, (char*)feedback, sizeof(TStatusInfo), 0, (sockaddr*)&sockip, socklen);
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

bool sendStatusMsg(SOCKET __InputSock, sockaddr_in sockip, int socklen, char* cid, TStatus status)
{
	TStatusInfo* feedback = (TStatusInfo*)std::malloc(sizeof(TStatusInfo));
	if (feedback != NULL)
	{
		feedback->MessageType =LoginResponse;
		feedback->status = status;
		strcpy_s(feedback->myCID, cid);
		sendto(__InputSock, (char*)feedback, sizeof(TStatusInfo), 0, (sockaddr*)&sockip, socklen);
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
	ctime_s(buf,128, &end_time);
	strcpy_s(regData->LastRegistTime, buf);

	char ipbuf[32];
	sockaddr_in sock_a;
	int size = sizeof(sockaddr_in);
	memset(&sock_a, 0x00, sizeof(sock_a));
	getpeername(__InputSock, (sockaddr*)&sock_a, &size);
	inet_ntop(AF_INET, &sock_a.sin_addr, ipbuf, sizeof(ipbuf));

	std::cout << "IP address - " << ipbuf << std::endl;
	strcpy_s(regData->LastIPAddress, 32, ipbuf);
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