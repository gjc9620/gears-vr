// Included after crosshair_live and before stereo pair construction. All state
// is owned by stereo_mutex; the requested mode persists across world recovery.
namespace first_person_live {
bool enabled{};gears_first_person::Toggle toggle;
HANDLE event{};HMODULE xinput_module{};
struct Pad {unsigned packet{};unsigned short buttons{};unsigned char lt{},rt{};short lx{},ly{},rx{},ry{};};
using GetPad=DWORD(WINAPI*)(DWORD,Pad*);GetPad get_pad{};
unsigned long long transitions{},applied{},excluded{},rejected{},weapon_unknown{},sniper_applied{},sniper_scoped{};
gears_first_person::ZoomHint zoom_hint{};
void close(){if(event){CloseHandle(event);event=nullptr;}}
bool initialize(){
    if(!enabled)return true;
    wchar_t name[128]{};swprintf_s(name,L"Local\\GearsVR.FirstPersonToggle.%lu",GetCurrentProcessId());
    event=CreateEventW(nullptr,FALSE,FALSE,name);
    if(!event||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
    for(auto name:{L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"}){
        xinput_module=LoadLibraryExW(name,nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(xinput_module){get_pad=reinterpret_cast<GetPad>(GetProcAddress(xinput_module,"XInputGetState"));if(get_pad)break;}
    }
    return true;
}
bool ordinary(gears_anchor::Receipt const* anchor){return anchor&&!anchor->anchor.unpossessed&&
    anchor_config.cinematic_enabled&&!anchor_config.mounted_enabled&&!anchor->anchor.cinematic&&
    anchor->anchor.target_identity.address==anchor->anchor.pawn;}
bool weapon_state(gears_anchor::Receipt const* anchor,gears_first_person::WeaponState& state){
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    if(!crosshair_live::enabled){++weapon_unknown;return false;}
    const auto& owned=crosshair_live::current();const auto& c=owned.assignment;
    if(!anchor||!gears_first_person::weapon_state(anchor_config,safe_copy,anchor->anchor.pawn,c.pawn_weapon.offset,state,zoom_hint)){
        ++weapon_unknown;return false;}
    return true;
#else
    (void)anchor;(void)state;++weapon_unknown;return false;
#endif
}
bool prepare(gears_anchor::Receipt const* anchor,gears_vr::StereoMatrix& view){
    if(!enabled)return false;
    const bool context=ordinary(anchor);
    const bool focused=foreground();
    const bool e=focused&&(GetAsyncKeyState('E')&0x8000),r=focused&&(GetAsyncKeyState('R')&0x8000);
    bool pad_chord=false,pad_neutral=true;
    if(focused&&get_pad)for(DWORD i=0;i<4;++i){Pad pad{};if(get_pad(i,&pad)==ERROR_SUCCESS){
        pad_chord=pad_chord||((pad.buttons&0xc0)==0xc0);pad_neutral=pad_neutral&&!(pad.buttons&0xc0);}}
    const bool external=event&&WaitForSingleObject(event,0)==WAIT_OBJECT_0;
    if(toggle.update(context&&(focused||external),(e&&r)||pad_chord,!e&&!r&&pad_neutral,GetTickCount64(),external))++transitions;
    if(!toggle.requested()||!context){++excluded;return false;}
    gears_first_person::WeaponState weapon{};const bool known=weapon_state(anchor,weapon);
    const auto mode=gears_first_person::camera_mode(toggle.requested(),context,known,weapon.sniper,weapon.zoomed);
    if(mode==gears_first_person::CameraMode::unchanged){++excluded;return false;}
    if(mode==gears_first_person::CameraMode::native_scope){
        // Keep the original zoom camera byte-for-byte and prevent camera-closer
        // stacking. The requested preview remains set for release of zoom.
        ++sniper_scoped;return true;
    }
    const auto& a=anchor->anchor;
    gears_first_person::Result result{};
    if(!gears_first_person::prepare(view,{a.location[0],a.location[1],double(a.location[2])+a.eye_height},stereo_units_per_metre,result)){
        ++rejected;return false;}
    view=result.view;++applied;if(weapon.sniper)++sniper_applied;return true;
}
void print(FILE* file){fprintf(file,"first_person_preview enabled=%u requested=%u transitions=%llu applied=%llu excluded=%llu rejected=%llu weapon_unknown=%llu sniper_applied=%llu sniper_scoped=%llu anchor_forward_metres=.35 anchor_up_metres=.10 pitch_height_coupling=0 weapon_animation_modified=0\n",
    unsigned(enabled),unsigned(toggle.requested()),transitions,applied,excluded,rejected,weapon_unknown,sniper_applied,sniper_scoped);}
}
