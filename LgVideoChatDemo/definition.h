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
#define ACS_IP			"192.168.219.103"

#define GENERAL_BUFSIZE		128
#define EMAIL_BUFSIZE		128
#define PASSWORD_BUFSIZE	256
#define ADDRESS_BUFSIZE		256
#define NAME_BUFSIZE		128
#define NAME_BUFSIZE		128
#define MAX_DEVSIZE			5

#define MAX_ALLOW_LOGIN_ATTEMPT 3

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
	char MissedCall[5][GENERAL_BUFSIZE];
	char LoginAttempt = 0;
	/*sockaddr_in sockaddr;
	int sockaddrSize = sizeof(sockaddr_in);*/
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
	unsigned int ListSize;
	char ListBuf[MAX_DEVSIZE][NAME_BUFSIZE];
}TContactList;

// considering ..
// aaa@aaa.com : 2023-06-19 15:29:33 \n
// bbb@bbb.com : 2023-06-19 16:22:33
typedef struct oMissedCall {
	unsigned char MessageType;
	unsigned int ListSize;
	char ListBuf[MAX_DEVSIZE][NAME_BUFSIZE];
}TMissedCall;

typedef struct oDeviceID {
	unsigned char MessageType;
	char DevID[NAME_BUFSIZE];
}TDeviceID;

typedef struct oStatusInfo {
	unsigned char MessageType;
	unsigned char status;
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

typedef struct oRspWithMessage2 {
	unsigned char MessageType;
	unsigned char MessageLen1;
	unsigned char Message1[GENERAL_BUFSIZE];
	unsigned char MessageLen2;
	unsigned char Message2[GENERAL_BUFSIZE];
}TRspWithMessage2;

typedef struct oRspResultWithMessage {
	unsigned char MessageType;
	int result;
	unsigned char MessageLen;
	unsigned char Message[GENERAL_BUFSIZE];
}TRspResultWithMessage;

typedef struct oRep {
	unsigned char MessageType;
}TReq;

typedef struct oRepwithMessage {
	unsigned char MessageType;
	unsigned char MessageLen;
	unsigned char Message[GENERAL_BUFSIZE];
}TReqwithMessage;

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
	RejectCall,
	MissedCall,
}TNetworkCommand;