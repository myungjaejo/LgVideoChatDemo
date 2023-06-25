#include "Logger.h"
#include <cstdarg>

std::mutex logMutex;
std::ofstream LogFile;

void LogF(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::cout << buffer << std::endl;


    std::lock_guard<std::mutex> lock(logMutex);

    if (LogFile.is_open())
    {
        LogFile << buffer << std::endl;
    }
}

// simple message
void Log(const char* message)
{
    std::cout << message << std::endl;

    std::lock_guard<std::mutex> lock(logMutex); 

    if (LogFile.is_open())
    {
        LogFile << message << std::endl;
    }
}

void InitLogger(char* path)
{
    if (path)
        LogFile.open(path, std::ios::app);
    else
    {
        LogFile.open(DEFAULT_LOG_FILE, std::ios::app);
    }
}

void ExitLogger(void)
{
    LogFile.close();
}

