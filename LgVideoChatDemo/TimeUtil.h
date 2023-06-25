#pragma once
#include <Windows.h>

void GetCurrentDateTime(char* buffer, size_t bufferSize);
void ConvertDateTime(char* dateTimeBuffer, SYSTEMTIME* systemTime);
bool IsTimeDifferenceGreaterThanOneHour(const SYSTEMTIME& systemTime1, const SYSTEMTIME& systemTime2);
bool IsTimeDifferenceGreaterThanOneMonth(const SYSTEMTIME& systemTime1, const SYSTEMTIME& systemTime2);