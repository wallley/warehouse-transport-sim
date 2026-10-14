#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <cwchar>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

// Windows 从原始宽字符命令行取参数，避免中文路径经过本地代码页后被误当作 UTF-8。
inline std::vector<std::string> utf8Arguments(int argc, char** argv) {
    std::vector<std::string> arguments;
#ifdef _WIN32
    int wideCount = 0;
    wchar_t** wideArguments = CommandLineToArgvW(GetCommandLineW(), &wideCount);
    if (!wideArguments) throw std::runtime_error("Cannot read command line");
    try {
        for (int index = 0; index < wideCount; ++index) {
            const int length = static_cast<int>(std::wcslen(wideArguments[index]));
            const int bytes = WideCharToMultiByte(CP_UTF8, 0, wideArguments[index], length, nullptr, 0, nullptr, nullptr);
            if (length > 0 && bytes == 0) throw std::runtime_error("Cannot decode command line");
            std::string value(static_cast<std::size_t>(bytes), '\0');
            if (bytes) WideCharToMultiByte(CP_UTF8, 0, wideArguments[index], length, value.data(), bytes, nullptr, nullptr);
            arguments.push_back(std::move(value));
        }
    } catch (...) {
        LocalFree(wideArguments);
        throw;
    }
    LocalFree(wideArguments);
    (void)argc;
    (void)argv;
#else
    for (int index = 0; index < argc; ++index) arguments.emplace_back(argv[index]);
#endif
    return arguments;
}
