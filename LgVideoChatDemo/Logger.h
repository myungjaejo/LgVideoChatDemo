#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <mutex>

#define DEFAULT_LOG_FILE "log.txt"
#define LOGF(format, ...) LogF(format, ##__VA_ARGS__)
#define LOG(message) Log(message)

void LogF(const char* format, ...);
void Log(const char* message);
void InitLogger(char* path);
void ExitLogger(void);
