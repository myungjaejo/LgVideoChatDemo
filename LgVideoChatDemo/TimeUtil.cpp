#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>

void GetCurrentDateTime(char* buffer, size_t bufferSize)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);

    int year = systemTime.wYear;
    int month = systemTime.wMonth;
    int day = systemTime.wDay;
    int hour = systemTime.wHour;
    int minute = systemTime.wMinute;
    int second = systemTime.wSecond;

    snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}

void ConvertDateTime(char *dateTimeBuffer, SYSTEMTIME *systemTime)
{
    sscanf_s(dateTimeBuffer, "%04d-%02d-%02d %02d:%02d:%02d", &systemTime->wYear, &systemTime->wMonth, &systemTime->wDay,
    &systemTime->wHour, &systemTime->wMinute, &systemTime->wSecond);
}

bool IsTimeDifferenceGreaterThanOneHour(const SYSTEMTIME& systemTime1, const SYSTEMTIME& systemTime2)
{
    FILETIME fileTime1, fileTime2;
    SystemTimeToFileTime(&systemTime1, &fileTime1);
    SystemTimeToFileTime(&systemTime2, &fileTime2);

    ULARGE_INTEGER time1{}, time2{};
    time1.LowPart = fileTime1.dwLowDateTime;
    time1.HighPart = fileTime1.dwHighDateTime;
    time2.LowPart = fileTime2.dwLowDateTime;
    time2.HighPart = fileTime2.dwHighDateTime;

    const ULONGLONG FILETIME_TICKS_PER_HOUR = 10000000ull * 3600ull;
    ULONGLONG difference = (time1.QuadPart > time2.QuadPart) ? (time1.QuadPart - time2.QuadPart) : (time2.QuadPart - time1.QuadPart);

    return (difference > FILETIME_TICKS_PER_HOUR);
}

bool IsTimeDifferenceGreaterThanOneMonth(const SYSTEMTIME& systemTime1, const SYSTEMTIME& systemTime2)
{
    FILETIME fileTime1, fileTime2;
    SystemTimeToFileTime(&systemTime1, &fileTime1);
    SystemTimeToFileTime(&systemTime2, &fileTime2);

    ULARGE_INTEGER time1{}, time2{};
    time1.LowPart = fileTime1.dwLowDateTime;
    time1.HighPart = fileTime1.dwHighDateTime;
    time2.LowPart = fileTime2.dwLowDateTime;
    time2.HighPart = fileTime2.dwHighDateTime;

    const ULONGLONG FILETIME_TICKS_PER_MONTH = 10000000ull * 3600ull * 24ull * 30ull;
    //const ULONGLONG FILETIME_TICKS_PER_MONTH = 100000000ull; // 10 sec for test perpose
    ULONGLONG difference = (time1.QuadPart > time2.QuadPart) ? (time1.QuadPart - time2.QuadPart) : (time2.QuadPart - time1.QuadPart);

    return (difference > FILETIME_TICKS_PER_MONTH);
}