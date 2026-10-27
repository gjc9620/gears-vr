#pragma once
#include "../vr-common/body_slots.h"
#include <windows.h>
#include <cerrno>
#include <cstdlib>
#include <cwchar>

namespace gears_equipment_runtime {
// The already authenticated and pinned Native module selects the same private
// Capture configuration used by its launcher. Missing calibration never turns
// into an invented player measurement. This function does not register, read
// game objects, change an INI, or create an input producer.
inline bool load_native_armspan(HMODULE native,gears_weapon::ArmspanCalibration& out) noexcept {
    out={};
    if(!native)return false;
    wchar_t path[32768]{};
    const auto length=GetModuleFileNameW(native,path,32768);
    if(!length||length>=32768)return false;
    auto* separator=std::wcsrchr(path,L'\\');
    if(!separator)return false;
    separator[1]=L'\0';
    if(wcscat_s(path,L"capture.ini"))return false;
    wchar_t span_present[2]{},height_present[2]{};
    const auto span_count=GetPrivateProfileStringW(L"Capture",L"weapon_armspan_metres",L"",span_present,2,path);
    const auto height_count=GetPrivateProfileStringW(L"Capture",L"weapon_hands_below_head_metres",L"",height_present,2,path);
    if(!span_count&&!height_count){
        // In the manual unified session a real measurement may complete after
        // Native activation. Borrow that already-observed POD value only;
        // never fabricate calibration or replace a malformed saved setting.
        using Read=DWORD(WINAPI*)(void*);
        const auto read=reinterpret_cast<Read>(GetProcAddress(native,"GearsVrReadBodySlotCalibrationV1"));
        gears_weapon::ArmspanCalibration measured{};
        if(!read||read(&measured)!=ERROR_SUCCESS||!measured.valid())return false;
        out=measured;return true;
    }
    auto number=[&](const wchar_t* key,double& value) noexcept {
        wchar_t text[64]{};
        const auto count=GetPrivateProfileStringW(L"Capture",key,L"",text,64,path);
        if(!count||count>=63)return false;
        wchar_t* end{};errno=0;
        value=std::wcstod(text,&end);
        return !errno&&end&&end!=text&&!*end;
    };
    gears_weapon::ArmspanCalibration candidate{};
    if(!number(L"weapon_armspan_metres",candidate.armspan_metres)||
       !number(L"weapon_hands_below_head_metres",candidate.hands_below_head_metres)||
       !candidate.valid())return false;
    out=candidate;return true;
}
}
