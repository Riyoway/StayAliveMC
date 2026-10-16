#pragma once
#include <string_view>

namespace stayalive {
enum class Mode { Window, Background, Check, Help, Invalid };
inline Mode ParseMode(int count, const wchar_t* const* arguments) {
    if (count == 1) return Mode::Window;
    if (count != 2) return Mode::Invalid;
    const std::wstring_view argument(arguments[1]);
    if (argument == L"--background") return Mode::Background;
    if (argument == L"--check") return Mode::Check;
    if (argument == L"--help" || argument == L"-h") return Mode::Help;
    return Mode::Invalid;
}
}
