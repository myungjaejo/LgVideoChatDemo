#pragma once

#include "resource.h"
#include "VoipVoice.h"
#include "definition.h"

#define WM_CLIENT_LOST         WM_USER+1
#define WM_REMOTE_CONNECT      WM_USER+2
#define WM_REMOTE_LOST         WM_USER+3
#define WM_VAD_STATE           WM_USER+4

#define WM_OPEN_CONTACTLIST	   WM_USER+6
#define WM_OPEN_CALLREQUEST	   WM_USER+7
#define WM_OPEN_VIDEOSERVER	   WM_USER+8
#define WM_OPEN_VIDEOCLIENT	   WM_USER+9
#define WM_CLOSE_ACSCONNECT	   WM_USER+10
#define WM_OPEN_TWOFACTORAUTH  WM_USER+11

#define VIDEO_PORT				10000
#define VOIP_LOCAL_PORT			10001
#define VOIP_REMOTE_PORT		10001
#define VIDEO_FRAME_DELAY		100

#define IDM_CALL_REQUEST       1014
#define IDM_CALL_DENY          1015
#define IDM_START_SERVER       1016
#define IDM_STOP_SERVER        1017

#define IDM_LOGIN              1022
#define IDM_LOGOUT             1023

extern HWND hWndMain;
extern TVoipAttr VoipAttr;
extern char LocalIpAddress[512];
extern TStatus devStatus;
extern HWND hWndMainToolbar;
extern char MyID[NAME_BUFSIZE];

//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------