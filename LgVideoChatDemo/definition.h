#pragma once

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <opencv2\highgui\highgui.hpp>
#include <opencv2\opencv.hpp>

#define EMAIL_BUFSIZE		128
#define PASSWORD_BUFSIZE	128
#define ADDRESS_BUFSIZE		256
#define NAME_BUFSIZE		128
#define NAME_BUFSIZE		128
#define MAX_DEVSIZE			5

typedef struct {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordSize;
	char password[PASSWORD_BUFSIZE];
	char ContactID[NAME_BUFSIZE];
	char firstName[NAME_BUFSIZE];
	char lastName[NAME_BUFSIZE];
	char Address[ADDRESS_BUFSIZE];
}TRegisteration;

typedef struct {
	unsigned char MessageType;
	unsigned int EmailSize;
	char email[EMAIL_BUFSIZE];
	unsigned int PasswordSize;
	char password[PASSWORD_BUFSIZE];
}TLogin;

typedef struct {
	unsigned char MessageType;
	unsigned int ListSize;
	char ListBuf[MAX_DEVSIZE][NAME_BUFSIZE];
}TContractList;

typedef struct {
	unsigned char MessageType;
	char DevID[NAME_BUFSIZE];
}TDeviceID;

typedef struct {
	unsigned char MessageType;
	unsigned char status;
}TStatusInfo;