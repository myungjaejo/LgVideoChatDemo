#pragma once
bool ConnectToACSever(const char* remotehostname, unsigned short remoteport);
bool StartAccessControlClient(void);
bool StopAccessControlClient(void);
bool IsACClientRunning(void);
