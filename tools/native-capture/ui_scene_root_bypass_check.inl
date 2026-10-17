// Dedicated loader-process CPU contract check; never a live-session probe.
#if defined(GEARS_UI_SCENE_ROOT_OBSERVER) && GEARS_UI_SCENE_ROOT_OBSERVER
namespace {
namespace ui_root_bypass_check {
constexpr uintptr_t sentinel=0xfedcba9876543210ull;
struct Failure {unsigned value;};
unsigned mode{},depth{},calls{},simple_calls{},material_calls{};
bool good=true;ui_scene_root_live::Row* parent{};
uint32_t __fastcall simple(void* command){++simple_calls;good=good&&command==reinterpret_cast<void*>(0x1234)&&
    stereo_publish_bypass&&ui_scene_root_live::current==parent;return 0x1717;}
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
uint32_t __fastcall material(void* command){++material_calls;good=good&&command==reinterpret_cast<void*>(0x1234)&&
    stereo_publish_bypass&&ui_scene_root_live::current==parent;return 0x1919;}
#endif
uintptr_t __fastcall original(void* client,void* canvas,void* scene){
    ++calls;good=good&&client==reinterpret_cast<void*>(0x1000)&&canvas==reinterpret_cast<void*>(0x2000)&&scene==reinterpret_cast<void*>(0x3000);
    if(!mode){good=good&&!stereo_publish_bypass&&!ui_scene_root_live::current&&world_thread.depth&&active.load();return sentinel;}
    good=good&&stereo_publish_bypass&&ui_scene_root_live::current==parent&&!world_thread.depth&&!active.load();
    good=good&&hook_ui_execute(reinterpret_cast<void*>(0x1234))==0x1717;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    good=good&&hook_ui_material_execute(reinterpret_cast<void*>(0x1234))==0x1919;
#endif
    if(mode==2&&!depth){++depth;const auto before=stereo_publish_bypass;
        {StereoPublishBypass nested;good=good&&ui_scene_root_live::hook(client,canvas,scene)==sentinel&&stereo_publish_bypass==before+1;}
        good=good&&stereo_publish_bypass==before;--depth;}
    if(mode==3)throw Failure{0x231};return sentinel;
}
}
}
#endif
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureUiRootBypassCheck(void*){
#if defined(GEARS_UI_SCENE_ROOT_OBSERVER) && GEARS_UI_SCENE_ROOT_OBSERVER
    using namespace ui_root_bypass_check;
    if(recording.load()||active.load()||stereo_publish_bypass||ui_scene_root_live::current)return 400;
    {Scope admitted;if(!admitted.world()||world_thread.depth!=1)return 401;}
    const auto saved_root=ui_scene_root_live::original;const auto saved_simple=ui_original_execute;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    const auto saved_material=ui_original_material_execute;
#endif
    struct Restore {ui_scene_root_live::Exec root;UiExecute simple;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
        UiExecute material;
#endif
        ~Restore(){ui_scene_root_live::original=root;ui_original_execute=simple;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
            ui_original_material_execute=material;
#endif
            ui_scene_root_live::current=nullptr;recording=false;ui_observer_enabled=false;}
    } restore{saved_root,saved_simple
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
        ,saved_material
#endif
    };
    base=0;recording=true;ui_observer_enabled=true;
    ui_scene_root_live::Row row{};row.simple_executes=7;row.material_executes=9;parent=&row;
    ui_scene_root_live::current=parent;ui_scene_root_live::original=original;ui_original_execute=simple;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    ui_original_material_execute=material;
#endif
    auto state=[&]{return std::array<uint64_t,10>{ui_scene_root_live::calls.load(),ui_scene_root_live::candidates.load(),
        ui_scene_root_live::rejected.load(),ui_scene_root_live::dropped.load(),ui_scene_root_live::slots.load(),
        row.simple_executes,row.material_executes,ui_execute_calls.load(),ui_semantic_consumer_pending,active.load()};};
    const auto before=state();
    for(mode=1;mode<=3;++mode){bool caught=false;
        try{StereoPublishBypass bypass;if(ui_scene_root_live::hook(reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x2000),reinterpret_cast<void*>(0x3000))!=sentinel)return 402;}
        catch(Failure const& e){if(mode!=3||e.value!=0x231)return 403;caught=true;}
        if((mode==3)!=caught||!good||stereo_publish_bypass||world_thread.depth||ui_scene_root_live::current!=parent||state()!=before)return 404+mode;
    }
    if(calls!=4||simple_calls!=4)return 408;
#if defined(GEARS_UI_MATERIAL_ROUTE) && GEARS_UI_MATERIAL_ROUTE
    if(material_calls!=4)return 409;
#endif
    mode=0;
    if(ui_scene_root_live::hook(reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x2000),reinterpret_cast<void*>(0x3000))!=sentinel||
       !good||ui_scene_root_live::current!=parent||ui_scene_root_live::calls.load()!=before[0]+1||
       ui_scene_root_live::rejected.load()!=before[2]+1||ui_scene_root_live::slots.load()!=before[4]||
       row.simple_executes!=7||row.material_executes!=9||active.load()||world_thread.depth)return 410;
    printf("UI root bypass actual hooks: exact forwarding, parent row/counters, simple/material Execute, nested, exception and observer-on control passed\n");return 0;
#else
    return ERROR_NOT_SUPPORTED;
#endif
}
