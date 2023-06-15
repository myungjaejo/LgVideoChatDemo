#pragma once
bool ConnectToACSever(const char* remotehostname, unsigned short remoteport);
bool StartAccessControlClient(void);
bool StopAccessControlClient(void);
bool IsACClientRunning(void);
int sendMsgtoACS(char* data, int len);

int OnConnectACS(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int OnDisconnectACS(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);