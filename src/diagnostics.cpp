#include "diagnostics.hpp"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <mutex>
#include <string>

namespace {

std::mutex log_mutex;

std::wstring get_log_path()
{
    wchar_t appdata[MAX_PATH] = {};

    DWORD length = GetEnvironmentVariableW(
        L"APPDATA", appdata, static_cast<DWORD>(std::size(appdata)));

    if (length == 0 || length >= std::size(appdata))
        return {};

    std::wstring directory = appdata;
    directory += L"\\obs-studio\\plugin_diagnostics";

    if (!CreateDirectoryW(directory.c_str(), nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
        return {};

    return directory + L"\\win-capture-audio.log";
}

} // namespace

void diagnostic_log(const char *event, const char *format, ...)
{
    char details[1024] = {};

    va_list args;
    va_start(args, format);
    _vsnprintf_s(details, sizeof(details), _TRUNCATE, format, args);
    va_end(args);

    SYSTEMTIME time;
    GetSystemTime(&time);

    char line[1400] = {};

    int length = _snprintf_s(
        line,
        sizeof(line),
        _TRUNCATE,
        "%04u-%02u-%02u %02u:%02u:%02u.%03u UTC "
        "tid=%lu event=%s %s\r\n",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond,
        time.wMilliseconds,
        GetCurrentThreadId(),
        event ? event : "unknown",
        details);

    if (length <= 0)
        return;

    std::lock_guard<std::mutex> lock(log_mutex);

    const std::wstring path = get_log_path();
    if (path.empty())
        return;

    HANDLE file = CreateFileW(
        path.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
        return;

    DWORD written = 0;
    WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr);
    CloseHandle(file);
}
