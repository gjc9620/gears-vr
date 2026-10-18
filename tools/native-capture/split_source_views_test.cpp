#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <vector>
#include "split_source_views.h"
#include "ngx_native_create_source.h"
namespace {
unsigned allocations{},constructs{},destroys{},frees{},allocation_limit=100;
bool bad_matrix{},alias_array{},fail_constructor{};
bool derive_owner_bounds{},wrong_output_owner{};
struct FakeTarget {int width,height;};
gears_vr::StereoMatrices* update_during_constructor{};
unsigned update_at_construct{};
uintptr_t aliased{};
void* allocated[2]{};
bool read(void* out,const void* from,size_t size);
bool failed_read(void* out,const void*,size_t size){memset(out,0xff,size);return false;}
enum class MapMode {normal,source_alias,eye_alias,different_content,mutate_source,after_bucket_alias,after_read_fail};
MapMode map_mode=MapMode::normal;
unsigned map_constructs{},map_nodes_allocated{},map_nodes_freed{},map_buckets_allocated{},map_buckets_freed{};
bool map_test_enabled{};
void* first_map_view{};
unsigned char* map_source_header{};
unsigned after_header_failures{};
struct OwnedMap {void* owner{};void* buckets{};std::vector<void*> nodes;bool freed{};};
std::vector<OwnedMap> owned_maps;
bool read(void* out,const void* from,size_t size){
    if(map_test_enabled&&map_mode==MapMode::after_read_fail&&map_constructs==2&&
        from==map_source_header&&size==24){++after_header_failures;return false;}
    SIZE_T count{};return from&&ReadProcessMemory(GetCurrentProcess(),from,out,size,&count)&&count==size;
}
static void put_word(unsigned char* bytes,size_t offset,uintptr_t value){memcpy(bytes+offset,&value,sizeof(value));}
static void make_map(void* owner,const std::vector<uintptr_t>& payloads,bool reverse=false){
    OwnedMap owned;owned.owner=owner;
    owned.buckets=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,32);if(!owned.buckets)ExitProcess(90);++map_buckets_allocated;
    for(auto payload:payloads){auto node=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,40);if(!node)ExitProcess(91);
        ++map_nodes_allocated;owned.nodes.push_back(node);put_word(static_cast<unsigned char*>(node),0,payload);}
    auto ordered=owned.nodes;if(reverse)std::reverse(ordered.begin(),ordered.end());
    auto header=static_cast<unsigned char*>(owner)+0x140;
    put_word(header,0,ordered.empty()?0:reinterpret_cast<uintptr_t>(ordered[0]));
    const auto count=static_cast<int>(ordered.size());memcpy(header+8,&count,4);
    put_word(header,12,reinterpret_cast<uintptr_t>(owned.buckets));const int bucket_count=4;memcpy(header+20,&bucket_count,4);
    if(!ordered.empty())put_word(static_cast<unsigned char*>(owned.buckets),0,reinterpret_cast<uintptr_t>(ordered[0]));
    for(unsigned i=0;i<ordered.size();++i){auto node=static_cast<unsigned char*>(ordered[i]);
        put_word(node,8,reinterpret_cast<uintptr_t>(i?static_cast<unsigned char*>(ordered[i-1])+16:owned.buckets));
        put_word(node,16,i+1<ordered.size()?reinterpret_cast<uintptr_t>(ordered[i+1]):0);
        put_word(node,24,reinterpret_cast<uintptr_t>(i?static_cast<unsigned char*>(ordered[i-1])+32:header));
        put_word(node,32,i+1<ordered.size()?reinterpret_cast<uintptr_t>(ordered[i+1]):0);}
    owned_maps.push_back(std::move(owned));
}
static void free_owned_map(void* owner){
    for(auto& owned:owned_maps)if(owned.owner==owner&&!owned.freed){
        // Known fixture allocations only. No payload or aliased header traversal.
        for(auto node:owned.nodes){HeapFree(GetProcessHeap(),0,node);++map_nodes_freed;}
        HeapFree(GetProcessHeap(),0,owned.buckets);++map_buckets_freed;owned.freed=true;}
}
static void clear_fixture_maps(){for(auto& owned:owned_maps)if(!owned.freed)free_owned_map(owned.owner);owned_maps.clear();}
void* __fastcall allocate(unsigned size,unsigned alignment) {
    if(size!=0x430 || alignment!=8 || allocations>=allocation_limit)return nullptr;
    void* view=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);allocated[allocations%2]=view;++allocations;return view;
}
void __fastcall destroy(void* view){++destroys;free_owned_map(view);}
void __fastcall free_view(void* view,unsigned size){if(size!=0x430)ExitProcess(80);++frees;HeapFree(GetProcessHeap(),0,view);}
void* __fastcall construct(void* destination,void* owner,void* state,void*,void*,void*,void*,void*,void*,void*,
    float x,float y,float width,float height,const void* view,const void* projection,
    const void*,const void*,const void*,const void* map_input,float,unsigned) {
    ++constructs;if(fail_constructor && constructs%2==0)return nullptr;
    auto bytes=static_cast<unsigned char*>(destination);
    memcpy(bytes,&owner,8);memcpy(bytes+8,&state,8);
    float rectangle[]={x,y,width,height};memcpy(bytes+0x48,rectangle,16);
    if(derive_owner_bounds){
        FakeTarget* target{};memcpy(&target,static_cast<unsigned char*>(owner)+0x10,sizeof(target));
        const int output[]={0,0,target->width,target->height};memcpy(bytes+0x58,output,16);
        const float derived[]={width/target->width,height/target->height,1.f/target->width,1.f/target->height};
        memcpy(bytes+0x3d0,derived,sizeof(derived));
        if(wrong_output_owner){void* invalid=nullptr;memcpy(bytes,&invalid,sizeof(invalid));}
    }
    memcpy(bytes+0x70,view,64);memcpy(bytes+0xb0,projection,64);
    if(update_during_constructor && constructs==update_at_construct){
        update_during_constructor->view[1][12]=999;update_during_constructor->projection[1][8]=999;}
    if(bad_matrix){float value=42;memcpy(bytes+0x70,&value,4);}
    if(alias_array){gears_shadow::Header header{aliased,1,1};memcpy(bytes+0x380,&header,sizeof(header));}
    if(map_test_enabled){
        ++map_constructs;
        if(map_mode==MapMode::source_alias)memcpy(bytes+0x140,map_input,24);
        else if(map_mode==MapMode::eye_alias&&map_constructs==2)
            memcpy(bytes+0x140,static_cast<unsigned char*>(first_map_view)+0x140,24);
        else {
            const auto input=static_cast<const unsigned char*>(map_input);
            auto node=gears_source_map::field<uintptr_t>(input,0);auto count=gears_source_map::field<int>(input,8);
            if(count<1||count>2)ExitProcess(92);
            std::vector<uintptr_t> payloads;
            for(int i=0;i<count;++i){std::array<unsigned char,40> copied{};if(!read(copied.data(),reinterpret_cast<void*>(node),40))ExitProcess(93);
                payloads.push_back(gears_source_map::field<uintptr_t>(copied.data(),0));node=gears_source_map::field<uintptr_t>(copied.data(),32);}
            if(map_mode==MapMode::different_content)payloads[0]=0xbad;
            make_map(destination,payloads,map_constructs==2);
            if(map_mode==MapMode::mutate_source&&map_constructs==2){
                // Modify the already-existing source node after cloning both eyes.
                auto source_head=gears_source_map::field<uintptr_t>(input,0);
                put_word(reinterpret_cast<unsigned char*>(source_head),0,0xcafe);}
            if(map_mode==MapMode::after_bucket_alias&&map_constructs==2){
                // A structurally valid empty source map now owns an interior
                // zero bucket slot from the independently constructed left eye.
                auto left_bucket=gears_source_map::field<uintptr_t>(static_cast<unsigned char*>(first_map_view)+0x140,12);
                put_word(map_source_header,0,0);const int empty_count=0,one_bucket=1;
                memcpy(map_source_header+8,&empty_count,4);put_word(map_source_header,12,left_bucket+8);
                memcpy(map_source_header+20,&one_bucket,4);}
        }
        if(map_constructs==1)first_map_view=destination;
    }
    return destination;
}
}
int main() {
    alignas(8) unsigned char family[0x60]{},source[0x430]{};unsigned char planes[16]{};
    uintptr_t owner=reinterpret_cast<uintptr_t>(family),state=0x1000;aliased=reinterpret_cast<uintptr_t>(planes);
    memcpy(source,&owner,8);memcpy(source+8,&state,8);
    float rectangle[]={0,0,1920,1080};memcpy(source+0x48,rectangle,16);
    float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    memcpy(source+0x70,matrix,64);memcpy(source+0xb0,matrix,64);
    gears_split_sources::API api{allocate,construct,destroy,free_view,read};
    auto run=[&](){return gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000));};
    // First-guard diagnostics do not allocate/construct or classify bytes from
    // a failed/partial read as valid snapshot fields.
    auto original=std::array<unsigned char,0x430>{};memcpy(original.data(),source,sizeof(source));
    const auto allocations_before_guard=allocations,constructs_before_guard=constructs;
    auto guard_rejected=[&](gears_split_sources::Result const& value,unsigned mask){
        return value.rejection==1&&value.checks==0&&value.input_evidence.failure_mask==mask&&
            !value.views[0]&&!value.views[1]&&!value.cleanup_safe&&!value.quarantined&&
            allocations==allocations_before_guard&&constructs==constructs_before_guard;
    };
    auto set_pointer=[&](size_t offset,uintptr_t value){memcpy(source+offset,&value,sizeof(value));};
    using namespace gears_split_sources;
    set_pointer(0,owner+8);auto rejected=run();
    if(!guard_rejected(rejected,family_mismatch)||!rejected.input_evidence.read_succeeded||
       rejected.input_evidence.family!=owner||rejected.input_evidence.observed_family!=owner+8||
       rejected.input_evidence.source!=reinterpret_cast<uintptr_t>(source))return 31;
    memcpy(source,original.data(),sizeof(source));set_pointer(8,0);rejected=run();
    if(!guard_rejected(rejected,missing_original_state)||rejected.input_evidence.original_state)return 32;
    memcpy(source,original.data(),sizeof(source));rejected=create(api,family,source,nullptr);
    if(!guard_rejected(rejected,missing_right_state)||rejected.input_evidence.right_state)return 33;
    rejected=create(api,family,source,reinterpret_cast<void*>(state));
    if(!guard_rejected(rejected,aliased_state)||rejected.input_evidence.original_state!=state||
       rejected.input_evidence.right_state!=state)return 34;
    set_pointer(0x140,0x3000);rejected=run();
    if(!guard_rejected(rejected,field140_nonzero)||rejected.input_evidence.field140!=0x3000)return 35;
    memcpy(source,original.data(),sizeof(source));set_pointer(0x418,0x4000);rejected=run();
    if(!guard_rejected(rejected,field418_nonzero)||rejected.input_evidence.field418!=0x4000)return 36;
    set_pointer(0x140,0x3000);set_pointer(0,owner+8);rejected=create(api,family,source,reinterpret_cast<void*>(state));
    if(!guard_rejected(rejected,family_mismatch|aliased_state|field140_nonzero|field418_nonzero))return 37;
    set_pointer(8,0);rejected=create(api,family,source,nullptr);
    if(!guard_rejected(rejected,family_mismatch|missing_original_state|missing_right_state|field140_nonzero|field418_nonzero))return 38;
    auto no_read=api;no_read.read=failed_read;rejected=create(no_read,family,source,nullptr);
    if(!guard_rejected(rejected,read_failed)||rejected.input_evidence.read_succeeded||
       rejected.input_evidence.observed_family||rejected.input_evidence.original_state||
       rejected.input_evidence.field140||rejected.input_evidence.field418||
       rejected.input_evidence.family!=owner||rejected.input_evidence.source!=reinterpret_cast<uintptr_t>(source))return 39;
    memcpy(source,original.data(),sizeof(source));
    auto result=run();
    if(!result.input_evidence.read_succeeded||result.input_evidence.failure_mask||
       result.input_evidence.original_state!=state||result.input_evidence.right_state!=0x2000||
       result.input_evidence.observed_family!=owner||result.input_evidence.field140||result.input_evidence.field418)return 40;
    if(result.checks!=63 || result.quarantined || !result.cleanup_safe ||
       result.inputs[0].value<float>(0x50)!=960 || result.inputs[1].value<float>(0x48)!=960 ||
       result.inputs[0].value<float>(0xa0)!=1 || result.inputs[1].value<float>(0xa0)!=-1)return 1;
    if(!gears_split_sources::release(api,result) || destroys!=2 || frees!=2 || result.views[0] || result.views[1])return 2;
    float inset[]={640,360,640,360};memcpy(source+0x48,inset,16);result=run();
    if(result.checks!=63 || result.inputs[0].value<float>(0x48)!=640 ||
       result.inputs[1].value<float>(0x48)!=960 || result.inputs[0].value<float>(0x50)!=320 ||
       result.inputs[1].value<float>(0x4c)!=360 || !gears_split_sources::release(api,result))return 8;
    int output_rectangle[]={0,0,1920,1080};memcpy(source+0x58,output_rectangle,16);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),true);
    if(result.checks!=63 || result.inputs[0].value<float>(0x48)!=0 ||
       result.inputs[1].value<float>(0x48)!=960 || result.inputs[0].value<float>(0x50)!=960 ||
       result.inputs[1].value<float>(0x4c)!=0 || result.inputs[1].value<float>(0x54)!=1080 ||
       !gears_split_sources::release(api,result))return 10;
    memcpy(source+0x48,rectangle,16);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,true);
    if(result.checks!=63 || memcmp(result.inputs[0].bytes.data(),source,sizeof(source)) ||
       memcmp(result.inputs[1].bytes.data(),source,sizeof(source)) || !gears_split_sources::release(api,result))return 11;
    float perspective[16]={1.19175363f,0,0,0,0,2.11867309f,0,0,0,0,.999000013f,1,0,0,-9.98999977f,0};
    memcpy(source+0xb0,perspective,64);auto untouched=std::array<unsigned char,0x430>{};memcpy(untouched.data(),source,sizeof(source));
    using gears_projection::Phase;
    const long long frequency=1000,origin=12345;
    const long long times[]={0,4999,5000,9999,10000,14999,15000,19999,20000,29999,30000};
    const Phase phases[]={Phase::a1,Phase::a1,Phase::b,Phase::b,Phase::a2,Phase::a2,Phase::c,Phase::c,Phase::a3,Phase::a3,Phase::a3};
    for(unsigned t=0;t<11;++t){auto selected=gears_projection::phase(origin+times[t],origin,frequency);
        if(selected!=phases[t])return 12;
        result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,selected);
        if(result.checks!=63 || memcmp(source,untouched.data(),sizeof(source)))return 13;
        for(unsigned i=0;i<2;++i){auto expected=perspective[0]*(selected==Phase::b?2.f:1.f);
            if(result.inputs[i].value<float>(0xb0)!=expected || result.inputs[i].value<float>(0xd0)!=(selected==Phase::c?.1f:0.f))return 14;
            for(unsigned j=0;j<16;++j)if(j!=0 && j!=8 && result.inputs[i].value<float>(0xb0+j*4)!=perspective[j])return 15;
            if(memcmp(static_cast<unsigned char*>(result.views[i])+0xb0,result.inputs[i].bytes.data()+0xb0,64))return 16;}
        if(!gears_split_sources::release(api,result))return 17;
    }
    // A caller's frozen decision remains shared even if the second constructor
    // would execute across a phase boundary; constructors receive no clock.
    auto frozen=gears_projection::phase(origin+4999,origin,frequency);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,frozen);
    if(result.inputs[0].value<float>(0xb0)!=perspective[0] || result.inputs[1].value<float>(0xb0)!=perspective[0] || !gears_split_sources::release(api,result))return 18;
    unsigned allocated_before=allocations;float bad=0;memcpy(source+0xdc,&bad,4);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,Phase::b);
    if(result.rejection!=10 || allocations!=allocated_before || result.views[0])return 19;
    memcpy(source,untouched.data(),sizeof(source));float infinite=INFINITY;memcpy(source+0xb0,&infinite,4);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,Phase::a1);
    if(result.rejection!=10 || allocations!=allocated_before)return 20;
    memcpy(source,untouched.data(),sizeof(source));
    // Menu9: derive from the actual completed native family Create, then pass
    // those arguments to the existing constructor (not merely an observer).
    gears_ngx_create::NativeCreateSource menu9{1,1,0x100,0x200,{2560,1440,3840,2160,2,67,0}};
    unsigned menu9_width{},menu9_height{};
    if(!menu9.split_input(3840,2160,menu9_width,menu9_height)||menu9_width!=1280||menu9_height!=1440)return 71;
    float huge=1.e20f;memcpy(source+0xd0,&huge,4);
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,Phase::c);
    if(result.rejection!=10 || allocations!=allocated_before)return 24;
    memcpy(source,untouched.data(),sizeof(source));
    std::array<float,16> expected_projection{};memcpy(expected_projection.data(),perspective,64);
    auto jittered=expected_projection;jittered[8]+=.001f;jittered[9]-=.002f;
    if(gears_projection::matches(jittered,expected_projection,false) || !gears_projection::matches(jittered,expected_projection,true))return 21;
    jittered[14]+=.01f;if(gears_projection::matches(jittered,expected_projection,true))return 22;
    jittered=expected_projection;jittered[8]=INFINITY;if(gears_projection::matches(jittered,expected_projection,true))return 23;
    gears_vr::StereoMatrices custom{};
    for(unsigned i=0;i<2;++i){memcpy(custom.view[i].data(),matrix,64);memcpy(custom.projection[i].data(),perspective,64);
        custom.view[i][12]=i?-3.2f:3.2f;custom.projection[i][0]=1.5f;custom.projection[i][8]=i?.15f:-.12f;}
    const auto frozen_custom=custom;update_during_constructor=&custom;update_at_construct=constructs+1;
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,false,Phase::a1,&custom);
    update_during_constructor=nullptr;
    if(result.checks!=63 || memcmp(source,untouched.data(),sizeof(source)))return 25;
    for(unsigned i=0;i<2;++i)if(memcmp(result.inputs[i].bytes.data()+0x70,frozen_custom.view[i].data(),64) ||
        memcmp(result.inputs[i].bytes.data()+0xb0,frozen_custom.projection[i].data(),64) ||
        memcmp(static_cast<unsigned char*>(result.views[i])+0x70,result.inputs[i].bytes.data()+0x70,128))return 26;
    if(result.inputs[0].value<float>(0x50)!=960 || result.inputs[1].value<float>(0x48)!=960 ||
       !gears_split_sources::release(api,result))return 27;
    custom=frozen_custom;allocated_before=allocations;
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,true,false,Phase::a1,&custom);
    if(result.rejection!=11 || allocations!=allocated_before)return 28;
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,Phase::b,&custom);
    if(result.rejection!=11 || allocations!=allocated_before)return 29;
    custom.projection[1][8]=INFINITY;
    result=gears_split_sources::create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,false,Phase::a1,&custom);
    if(result.rejection!=12 || allocations!=allocated_before)return 30;
    // Fixed constructor bounds use an explicit SBS profile, never the current
    // game source rectangle. Neither family nor any source byte is modified.
    custom=frozen_custom;
    std::array<unsigned char,sizeof(family)> family_before{};memcpy(family_before.data(),family,sizeof(family));
    for(auto fixed: {FixedOutputBounds{4096,2048},FixedOutputBounds{8192,4096},FixedOutputBounds{6144,3264},FixedOutputBounds{4302,2285}}){
        memcpy(source,untouched.data(),sizeof(source));
        const float game_rectangle[]={37,19,1919,1079};memcpy(source+0x48,game_rectangle,16);
        const int game_output[]={11,7,2559,1439};memcpy(source+0x58,game_output,16);
        std::array<unsigned char,0x430> saved{};memcpy(saved.data(),source,sizeof(source));
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&fixed);
        if(result.checks!=63||memcmp(source,saved.data(),sizeof(source))||memcmp(family,family_before.data(),sizeof(family)))return 54;
        const float w=float(fixed.width/2),h=float(fixed.height);
        for(unsigned i=0;i<2;++i){const std::array<float,4> expected{float(i)*w,0,w,h};
            if(memcmp(result.inputs[i].bytes.data()+0x48,expected.data(),16)||
               memcmp(static_cast<unsigned char*>(result.views[i])+0x48,expected.data(),16)||
               memcmp(result.inputs[i].bytes.data()+0x70,custom.view[i].data(),64)||
               memcmp(result.inputs[i].bytes.data()+0xb0,custom.projection[i].data(),64))return 55;
            auto protected_input=result.inputs[i];memcpy(protected_input.bytes.data()+0x48,saved.data()+0x48,16);
            memcpy(protected_input.bytes.data()+0x70,saved.data()+0x70,128);
            if(protected_input.bytes!=saved)return 56;
        }
        if(!release(api,result))return 57;
    }
    memcpy(source,untouched.data(),sizeof(source));
    // Model the certified constructor dependency: output integer bounds and
    // normalized sampling fields come from owner+0x10, not its float arguments.
    // This is a CPU ownership/ABI test, not proof of actual-game scene coverage.
    {
        FakeTarget desktop{3840,2160},owned{5120,2800};
        auto desktop_ptr=&desktop;memcpy(family+0x10,&desktop_ptr,sizeof(desktop_ptr));
        std::array<unsigned char,sizeof(family)> original_family{};memcpy(original_family.data(),family,sizeof(family));
        auto constructor_family=original_family;auto owned_ptr=&owned;
        memcpy(constructor_family.data()+0x10,&owned_ptr,sizeof(owned_ptr));
        const auto saved=untouched;const FixedOutputBounds fixed{5120,2800};
        derive_owner_bounds=true;
        // Default owner remains the original, demonstrating the old derived size.
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&fixed);
        if(result.checks!=63||result.inputs[0].pointer(0)!=family)return 101;
        int old_width{};memcpy(&old_width,static_cast<unsigned char*>(result.views[0])+0x60,4);
        if(old_width!=3840||!release(api,result))return 102;
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&fixed,nullptr,constructor_family.data());
        if(result.checks!=63||memcmp(source,saved.data(),sizeof(source))||
           memcmp(family,original_family.data(),sizeof(family)))return 103;
        for(unsigned i=0;i<2;++i){
            const auto bytes=static_cast<unsigned char*>(result.views[i]);void* actual_owner{};memcpy(&actual_owner,bytes,sizeof(actual_owner));
            int bounds[4]{};memcpy(bounds,bytes+0x58,sizeof(bounds));float derived[4]{};memcpy(derived,bytes+0x3d0,sizeof(derived));
            if(actual_owner!=constructor_family.data()||bounds[0]||bounds[1]||bounds[2]!=5120||bounds[3]!=2800||
               derived[0]!=0.5f||derived[1]!=1.f||derived[2]!=1.f/5120||derived[3]!=1.f/2800||
               result.inputs[i].pointer(0)!=family)return 104;
        }
        if(!release(api,result))return 105;
        wrong_output_owner=true;
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&fixed,nullptr,constructor_family.data());
        if(result.checks==63||!result.cleanup_safe||!release(api,result))return 106;
        wrong_output_owner=false;
        // A valid constructor copy must not authenticate a source whose original
        // owner was replaced: reject before allocating either eye.
        const auto before_allocations=allocations,before_constructs=constructs;
        void* forged_original_owner=constructor_family.data();memcpy(source,&forged_original_owner,sizeof(forged_original_owner));
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&fixed,nullptr,constructor_family.data());
        if(result.rejection!=1||result.input_evidence.failure_mask!=family_mismatch||
           allocations!=before_allocations||constructs!=before_constructs)return 107;
        memcpy(source,saved.data(),sizeof(source));derive_owner_bounds=false;
        memcpy(family,family_before.data(),sizeof(family));
    }
    // Actual native family Create1706x720 -> per-eye853x720. Preserve the
    // 1280 output stride: packing the right input at853 would overlap Copy0.
    // The synthetic native constructor receives the reduced arguments itself.
    for(auto sample:{std::array<unsigned,4>{2560,1080,853,720},
                     std::array<unsigned,4>{4302,2285,1433,1523},
                     std::array<unsigned,4>{menu9.fields[2],menu9.fields[3],menu9_width,menu9_height}}){
        const FixedOutputBounds display{sample[0],sample[1]};
        const RenderInputBounds render{sample[2],sample[3]};
        const auto saved=untouched;
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&display,&render);
        if(result.checks!=63||memcmp(source,saved.data(),sizeof(source))||memcmp(family,family_before.data(),sizeof(family)))return 64;
        for(unsigned i=0;i<2;++i){
            const std::array<float,4> expected{float(i*(display.width/2)),0,float(render.width),float(render.height)};
            if(memcmp(result.inputs[i].bytes.data()+0x48,expected.data(),16)||
               memcmp(static_cast<unsigned char*>(result.views[i])+0x48,expected.data(),16))return 65;
            auto restored=result.inputs[i];memcpy(restored.bytes.data()+0x48,saved.data()+0x48,16);
            memcpy(restored.bytes.data()+0x70,saved.data()+0x70,128);
            if(restored.bytes!=saved)return 66;
        }
        if(!release(api,result))return 67;
    }
    const auto render_allocations=allocations,render_constructs=constructs;
    const FixedOutputBounds display{2560,1080};
    for(auto invalid:{RenderInputBounds{0,0},RenderInputBounds{63,720},RenderInputBounds{853,63},
                     RenderInputBounds{1281,720},RenderInputBounds{853,1081},RenderInputBounds{UINT32_MAX,720}}){
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,&display,&invalid);
        if(result.rejection!=14||result.checks||allocations!=render_allocations||constructs!=render_constructs)return 68;
    }
    const RenderInputBounds render{853,720};
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,false,false,Phase::a1,&custom,nullptr,&render);
    if(result.rejection!=14||allocations!=render_allocations||constructs!=render_constructs)return 69;
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),true,true,false,Phase::a1,&custom,&display,&render);
    if(result.rejection!=11||allocations!=render_allocations)return 70;
    const auto fixed_allocations=allocations,fixed_constructs=constructs;
    for(auto invalid:{FixedOutputBounds{0,0},FixedOutputBounds{126,2048},FixedOutputBounds{8194,4096},
        FixedOutputBounds{8192,8193},FixedOutputBounds{4095,2048},FixedOutputBounds{UINT32_MAX,4096}}){
        result=create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,false,Phase::a1,&custom,&invalid);
        if(result.rejection!=13||result.checks||result.views[0]||result.views[1]||allocations!=fixed_allocations||constructs!=fixed_constructs||
           memcmp(source,untouched.data(),sizeof(source)))return 58;
    }
    const FixedOutputBounds low{4096,2048};
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,false,Phase::a1,nullptr,&low);
    if(result.rejection!=13||allocations!=fixed_allocations||constructs!=fixed_constructs)return 59;
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),false,true,false,Phase::a1,&custom,&low);
    if(result.rejection!=11||allocations!=fixed_allocations)return 60;
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,true,Phase::b,&custom,&low);
    if(result.rejection!=11||allocations!=fixed_allocations)return 61;
    // Explicit nullptr is exactly the default legacy geometry/matrix path.
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,false,Phase::a1,&custom,nullptr);
    if(result.checks!=63||result.inputs[0].value<float>(0x50)!=960||result.inputs[0].value<float>(0x54)!=1080||
       result.inputs[1].value<float>(0x48)!=960||memcmp(source,untouched.data(),sizeof(source))||!release(api,result))return 62;
    unsigned freed_before_partial=frees;
    allocation_limit=allocations+1;result=run();allocation_limit=100;
    if(result.views[0] || result.views[1] || result.cleanup_safe || frees!=freed_before_partial+1)return 3;
    bad_matrix=true;result=run();bad_matrix=false;
    if((result.checks&8) || !result.cleanup_safe || !gears_split_sources::release(api,result))return 4;
    gears_shadow::Header original_array{aliased,1,1};memcpy(source+0x380,&original_array,sizeof(original_array));
    alias_array=true;result=run();alias_array=false;
    if(result.cleanup_safe || !result.quarantined || gears_split_sources::release(api,result))return 5;
    for(void* pointer:result.views)free_view(pointer,0x430); // Mock-known quarantine, never native cleanup.
    fail_constructor=true;result=run();fail_constructor=false;
    if(result.cleanup_safe || !result.quarantined)return 6;
    for(void* pointer:result.views)free_view(pointer,0x430);
    float odd=1919;memcpy(source+0x50,&odd,4);result=run();
    if(result.checks || result.views[0] || result.views[1])return 7;
    if(result.rejection!=3)return 9;
    // Nonempty map ABI path, with real fixture-owned40-byte node/32-byte bucket
    // allocations and opaque payload values never referenced by the destructor.
    auto begin_map_case=[&](MapMode mode,unsigned count){
        clear_fixture_maps();memcpy(source,original.data(),sizeof(source));
        map_mode=mode;map_test_enabled=true;map_constructs=0;first_map_view=nullptr;
        map_source_header=source+0x140;after_header_failures=0;
        std::vector<uintptr_t> payloads(count,0xfeed);make_map(source,payloads);
    };
    for(unsigned count:{1u,2u}){
        begin_map_case(MapMode::normal,count);
        auto before_map=std::make_unique<gears_source_map::Snapshot>();
        if(!gears_source_map::read(read,reinterpret_cast<uintptr_t>(source)+0x140,*before_map))return 41;
        const auto freed_nodes_before=map_nodes_freed,freed_buckets_before=map_buckets_freed;
        result=run();
        if(result.checks!=63||result.map_checks!=63||result.map_nodes!=count||result.rejection||
            !result.cleanup_safe||result.quarantined)return 42;
        auto left=std::make_unique<gears_source_map::Snapshot>(),right=std::make_unique<gears_source_map::Snapshot>();
        if(!gears_source_map::read(read,reinterpret_cast<uintptr_t>(result.views[0])+0x140,*left)||
           !gears_source_map::read(read,reinterpret_cast<uintptr_t>(result.views[1])+0x140,*right)||
           !gears_source_map::same_contents(*before_map,*left)||!gears_source_map::same_contents(*before_map,*right)||
           !gears_source_map::independent(*before_map,*left)||!gears_source_map::independent(*before_map,*right)||
           !gears_source_map::independent(*left,*right))return 43;
        if(!release(api,result)||map_nodes_freed!=freed_nodes_before+count*2||
            map_buckets_freed!=freed_buckets_before+2)return 44;
        auto after_map=std::make_unique<gears_source_map::Snapshot>();
        if(!gears_source_map::read(read,reinterpret_cast<uintptr_t>(source)+0x140,*after_map)||
           !gears_source_map::unchanged(*before_map,*after_map))return 45;
    }
    begin_map_case(MapMode::normal,2);
    auto source_header=source+0x140;
    auto source_node=gears_source_map::field<uintptr_t>(source_header,0);
    put_word(reinterpret_cast<unsigned char*>(source_node),32,source_node);
    const auto alloc_before_bad_map=allocations,construct_before_bad_map=constructs;
    result=run();
    if(result.rejection!=1||result.checks||result.map_checks||result.input_evidence.failure_mask!=field140_nonzero||
        allocations!=alloc_before_bad_map||constructs!=construct_before_bad_map)return 46;
    for(auto mode:{MapMode::source_alias,MapMode::eye_alias}){
        begin_map_case(mode,2);const auto destroyed_before=destroys;
        result=run();
        if(result.cleanup_safe||!result.quarantined||result.rejection!=7||release(api,result)||
            destroys!=destroyed_before)return 47;
        // Fixture-known quarantined allocations only; never invoke native-style
        // destruction on potentially shared map storage.
        clear_fixture_maps();for(void* pointer:result.views)free_view(pointer,0x430);
    }
    begin_map_case(MapMode::different_content,2);result=run();
    if(result.rejection!=8||(result.checks&8)||(result.map_checks&16)||
        !(result.map_checks&8)||!result.cleanup_safe||result.quarantined||!release(api,result))return 48;
    begin_map_case(MapMode::mutate_source,2);result=run();
    if(result.rejection!=9||(result.checks&32)||(result.map_checks&32)||
        result.map_checks!=31||!result.cleanup_safe||result.quarantined||!release(api,result))return 49;
    for(auto mode:{MapMode::after_bucket_alias,MapMode::after_read_fail}){
        begin_map_case(mode,2);const auto destroyed_before=destroys;
        result=run();
        if(result.rejection!=7||result.cleanup_safe||!result.quarantined||(result.checks&16)||
            (result.map_checks&8)||release(api,result)||destroys!=destroyed_before)return 51;
        if(mode==MapMode::after_read_fail&&after_header_failures!=1)return 52;
        if(mode==MapMode::after_bucket_alias){
            auto current=std::make_unique<gears_source_map::Snapshot>(),left=std::make_unique<gears_source_map::Snapshot>();
            if(!gears_source_map::read(read,reinterpret_cast<uintptr_t>(map_source_header),*current)||
                current->node_count!=0||current->bucket_count!=1||
                !gears_source_map::read(read,reinterpret_cast<uintptr_t>(result.views[0])+0x140,*left)||
                current->header.buckets!=left->header.buckets+8||gears_source_map::independent(*current,*left))return 53;
        }
        // Quarantine fixture teardown uses tracked allocations, never the
        // current source's deliberately aliased interior bucket pointer.
        clear_fixture_maps();for(void* pointer:result.views)free_view(pointer,0x430);
    }
    // Fixed bounds retain the existing nonempty-map deep-copy certificate.
    begin_map_case(MapMode::normal,2);
    std::array<unsigned char,0x430> fixed_source{};memcpy(fixed_source.data(),source,sizeof(source));
    auto fixed_map_before=std::make_unique<gears_source_map::Snapshot>();
    if(!gears_source_map::read(read,reinterpret_cast<uintptr_t>(source)+0x140,*fixed_map_before))return 63;
    result=create(api,family,source,reinterpret_cast<void*>(0x2000),false,false,false,Phase::a1,&custom,&low);
    if(result.checks!=63||result.map_checks!=63||result.map_nodes!=2||memcmp(source,fixed_source.data(),sizeof(source)))return 64;
    for(unsigned i=0;i<2;++i){auto cloned_map=std::make_unique<gears_source_map::Snapshot>();
        if(!gears_source_map::read(read,reinterpret_cast<uintptr_t>(result.views[i])+0x140,*cloned_map)||
           !gears_source_map::same_contents(*fixed_map_before,*cloned_map)||!gears_source_map::independent(*fixed_map_before,*cloned_map)||
           result.inputs[i].value<float>(0x48)!=float(i*2048)||result.inputs[i].value<float>(0x50)!=2048||
           result.inputs[i].value<float>(0x54)!=2048)return 65;
    }
    if(!release(api,result))return 66;
    clear_fixture_maps();map_test_enabled=false;
    if(map_nodes_allocated!=map_nodes_freed||map_buckets_allocated!=map_buckets_freed)return 50;
    printf("Split sources: first-guard evidence, map copy/independence/after ownership+read failure/source mutation, rectangles and quarantine passed\n");return 0;
}
