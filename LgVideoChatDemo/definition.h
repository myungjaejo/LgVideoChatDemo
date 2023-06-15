#pragma once

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <opencv2\highgui\highgui.hpp>
#include <opencv2\opencv.hpp>

#define ACS_PORT		12000
#define ACS_DELAY		1000
#define ACS_IP			"192.168.68.104"

#define EMAIL_BUFSIZE		128
#define PASSWORD_BUFSIZE	128
#define ADDRESS_BUFSIZE		256
#define NAME_BUFSIZE		128
#define NAME_BUFSIZE		128
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
	sockaddr_in sockaddr;
	int sockaddrSize = sizeof(sockaddr_in);
}TRegistration;

typedef struct oLogin {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordSize;
	char password[PASSWORD_BUFSIZE];
}TLogin;

typedef struct oContactList {
	unsigned char MessageType;
	unsigned int ListSize;
	char ListBuf[MAX_DEVSIZE][NAME_BUFSIZE];
}TContactList;

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

typedef enum {
	Registration,
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