#pragma once

//#include <winsock2.h>
//#include <windows.h>
//#include <stdio.h>
//#include <tchar.h>
//
//#include <opencv2\highgui\highgui.hpp>
//#include <opencv2\opencv.hpp>

#define ACS_PORT		12000
#define ACS_DELAY		1000
#define ACS_IP			"192.168.68.104"

#define GENERAL_BUFSIZE		128
#define EMAIL_BUFSIZE		128
#define PASSWORD_BUFSIZE	256
#define ADDRESS_BUFSIZE		256
#define NAME_BUFSIZE		128
#define TIME_STRSIZE		512
#define IP_BUFFSIZE			512
#define PARSE_NAME_BUFSIZE	1024
#define MAX_DEVSIZE			5

typedef struct oRegistration {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordSize;
	char password[PASSWORD_BUFSIZE];
	char ContactID[NAME_BUFSIZE];
	char firstName[NAME_BUFSIZE];
	char lastName[NAME_BUFSIZE];
	char Address[ADDRESS_BUFSIZE];
	char MissedCall[PARSE_NAME_BUFSIZE];
	char MissedCallTime[PARSE_NAME_BUFSIZE];
	char LastIPAddress[IP_BUFFSIZE];
	char LastRegistTime[TIME_STRSIZE];
}TRegistration;

typedef struct oLogin {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordHashSize;
	char passwordHash[PASSWORD_BUFSIZE];
}TLogin;

typedef struct oContactList {
	unsigned char MessageType;
	unsigned char ListSize;
	char ListBuf[PARSE_NAME_BUFSIZE];
}TContactList;

typedef struct oDeviceID {
	unsigned char MessageType;
	char FromDevID[NAME_BUFSIZE];
	char ToDevID[NAME_BUFSIZE];
}TDeviceID;

typedef struct oAcceptCall {
	unsigned char MessageType;
	char FromDevID[NAME_BUFSIZE];
	char ToDevID[NAME_BUFSIZE];
	char IPAddress[IP_BUFFSIZE];
}TAcceptCall;

typedef struct oStatusInfo {
	unsigned char MessageType;
	unsigned char status;
	char myCID[NAME_BUFSIZE];
}TStatusInfo;

typedef struct oCommandOnly {
	unsigned char MessageType;
	bool answer;
}TCommandOnly;

// can be used simple response
typedef struct oRspWithMessage {
	unsigned char MessageType;
	unsigned char MessageLen;
	unsigned char Message[GENERAL_BUFSIZE];
}TRspWithMessage;

typedef enum {
	Registration = 0x41,
	RegistrationResponse,
	Login,
	LoginResponse,
	Logout,
	LogoutResponse,
	RequestStatus,
	SendStatus,
	RequestContactList,
	SendContactList,
	RequestCall,
	AcceptCall,
	RejectCall
}TNetworkCommand;


typedef enum {
	Disconnected,
	Connected,
	Caller,
	Callee,
	Calling,
	Server = 31967
}TStatus;