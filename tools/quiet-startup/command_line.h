#pragma once
#include <string>

namespace gears_quiet {
// Leave simple switches bare: the game's ParseParam also reads the raw command
// line and does not accept a closing quote as a switch terminator.
inline std::wstring quote_argument(const std::wstring& value) {
    if(!value.empty() && value.find_first_of(L" \t\"")==std::wstring::npos)return value;
    std::wstring result=L"\"";
    std::size_t slashes{};
    for(wchar_t ch:value) {
        if(ch==L'\\'){++slashes;continue;}
        result.append(slashes*(ch==L'"'?2:1),L'\\');slashes=0;
        if(ch==L'"')result+=L'\\';
        result+=ch;
    }
    result.append(slashes*2,L'\\');
    return result+L'"';
}
}
