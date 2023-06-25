//
// pch.h
//

#pragma once

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <new>
#include <vector>
#include <chrono>
#include <ws2tcpip.h>

#include "gtest/gtest.h"
#include "../LgVideoChatDemo/AccessControlServer.h"
#include "../LgVideoChatDemo/LgVideoChatDemo.h"
#include "../LgVideoChatDemo/TcpSendRecv.h"
#include "../LgVideoChatDemo/filemanager.h"
#include "../LgVideoChatDemo/VideoServer.h"
#include "../LgVideoChatDemo/TwoFactorAuth.h"
#include "../LgVideoChatDemo/AccessControlServer.h"
#include "../LgVideoChatDemo/definition.h"
