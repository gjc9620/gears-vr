#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <vector>
#include "command_line.h"

namespace {
unsigned checks{},failures{};
void check(bool ok,const char* label) {
    ++checks;
    if(!ok){++failures;std::cerr<<"FAIL: "<<label<<'\n';}
}
std::wstring command(const std::vector<std::wstring>& args) {
    std::wstring result;
    for(const auto& arg:args){if(!result.empty())result+=L' ';result+=gears_quiet::quote_argument(arg);}
    return result;
}
void round_trip(const std::vector<std::wstring>& args) {
    const auto text=command(args);
    int count{};
    auto parsed=CommandLineToArgvW(text.c_str(),&count);
    check(parsed!=nullptr,"CommandLineToArgvW succeeds");
    if(!parsed)return;
    check(count>=0 && static_cast<std::size_t>(count)==args.size(),"argument count");
    if(count>=0 && static_cast<std::size_t>(count)==args.size())
        for(int i=0;i<count;++i)check(parsed[i]==args[static_cast<std::size_t>(i)],"argument round trip");
    LocalFree(parsed);
}
}
int main() {
    check(command({L"C:\\Game Dir\\game.exe",L"-NoPauseUnfocus"})==
          L"\"C:\\Game Dir\\game.exe\" -NoPauseUnfocus","exact bare game switch");
    check(gears_quiet::quote_argument(L"plain\\tail\\")==L"plain\\tail\\","bare trailing backslash unchanged");
    check(gears_quiet::quote_argument(L"")==L"\"\"","empty argument quoted");
    const std::vector<std::wstring> values={L"",L"-NoPauseUnfocus",L"plain",L"two words",L"has\ttab",
        L"embedded\"quote",L"\\\"",L"plain\\",L"space tail\\",L"space tail\\\\",
        L"\"",L"\\\\\"end",L"\u6e38\u620f",L"\u6e38\u620f \u8def\u5f84\\"};
    for(const auto& value:values)round_trip({L"C:\\Game Dir\\game.exe",value});
    auto combined=values;combined.insert(combined.begin(),L"C:\\Game Dir\\game.exe");
    round_trip(combined);
    std::cout<<checks<<" checks, "<<failures<<" failures\n";
    return failures?1:0;
}
