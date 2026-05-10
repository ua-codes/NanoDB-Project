#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
private:
    FILE* logFile;
    LogLevel minLevel;
    bool consoleEcho;

    const char* levelStr(LogLevel l) {
        switch(l) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            default: return "?????";
        }
    }

    void writeEntry(LogLevel level, const char* msg) {
        if (level < minLevel) return;
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", t);

        if (logFile) {
            fprintf(logFile, "[%s] [%s] %s\n", timeBuf, levelStr(level), msg);
            fflush(logFile);
        }
        if (consoleEcho) {
            fprintf(stdout, "[%s] [%s] %s\n", timeBuf, levelStr(level), msg);
        }
    }

public:
    Logger() : logFile(nullptr), minLevel(LogLevel::DEBUG), consoleEcho(true) {}

    ~Logger() { if (logFile) fclose(logFile); }

    bool open(const char* path, LogLevel level = LogLevel::DEBUG, bool echo = false) {
        logFile = fopen(path, "a");
        minLevel = level;
        consoleEcho = echo;
        return logFile != nullptr;
    }

    void log(LogLevel level, const char* fmt, ...) {
        char buf[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        writeEntry(level, buf);
    }

    void debug(const char* fmt, ...) {
        char buf[2048]; va_list a; va_start(a, fmt);
        vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
        writeEntry(LogLevel::DEBUG, buf);
    }
    void info(const char* fmt, ...) {
        char buf[2048]; va_list a; va_start(a, fmt);
        vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
        writeEntry(LogLevel::INFO, buf);
    }
    void warn(const char* fmt, ...) {
        char buf[2048]; va_list a; va_start(a, fmt);
        vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
        writeEntry(LogLevel::WARN, buf);
    }
    void error(const char* fmt, ...) {
        char buf[2048]; va_list a; va_start(a, fmt);
        vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
        writeEntry(LogLevel::ERROR, buf);
    }

    void close() { if (logFile) { fclose(logFile); logFile = nullptr; } }
};

extern Logger gLogger;
