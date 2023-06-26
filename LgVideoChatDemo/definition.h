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
#define ACS_IP			"192.168.68.103"

#define GENERAL_BUFSIZE		128
#define EMAIL_BUFSIZE		128
#define PASSWORD_BUFSIZE	256
#define ADDRESS_BUFSIZE		256
#define NAME_BUFSIZE		128
#define TIME_STRSIZE		512
#define IP_BUFFSIZE			512
#define PARSE_NAME_BUFSIZE	1024
#define TWOFACTOR_BUFSIZE	256
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
	char MissedCall[PARSE_NAME_BUFSIZE];
	char MissedCallTime[PARSE_NAME_BUFSIZE];
	char LastIPAddress[IP_BUFFSIZE];
	char LastRegistTime[TIME_STRSIZE];
	char LoginAttempt;
	char LastPasswordChange[GENERAL_BUFSIZE];
}TRegistration;

typedef struct oLogin {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordHashSize;
	char passwordHash[PASSWORD_BUFSIZE];
}TLogin;

typedef struct oTwoFactor {
	unsigned char MessageType;
	char myCID[NAME_BUFSIZE];
	char TFA[TWOFACTOR_BUFSIZE];
}TTwoFactor;

typedef struct oReRegistration {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordSize;
	char password[PASSWORD_BUFSIZE];
}TReRegistration;

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

typedef struct oMissedCall {
	unsigned char MessageType;
	unsigned int ListSize;
	char ListBuf[MAX_DEVSIZE][NAME_BUFSIZE];
}TMissedCall;

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
	bool answer;
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
	ResetPasswordRequest,
	ResetPasswordResponse,
	ChangePasswordRequest,
	ChangePasswordResponse,
	ReRegPasswordRequest,
	ReRegPasswordResponse,
	TwoFactorRequest,
	TwoFactorResponse
}TNetworkCommand;


typedef enum {
	Disconnected,
	Connected,
	Caller,
	Callee,
	Calling,
	VaildTwoFactor,
	ResetPassword,
	Server = 31967	
}TStatus;