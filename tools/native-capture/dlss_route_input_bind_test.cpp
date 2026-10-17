#define GEARS_DLSS_INPUT_BIND_OBSERVER 1
#define GEARS_DLSS_INPUT_BIND_OBSERVER_TEST 1
#include "dlss_output_control_test.cpp"

namespace {
alignas(8) std::array<unsigned char,0xc0> shader_owner{};
alignas(8) std::array<unsigned char,0x180> public_wrapper{};
unsigned bind_calls{};const uint16_t* expected_descriptor{};void* expected_bind_view{};void* expected_shader_owner{};
uint32_t expected_selection{};int32_t expected_alternate{};
uint64_t __fastcall original_bind(const uint16_t* descriptor,void* input,void* shader,uint32_t selection,int32_t alternate){
    assert_frame_mutex_unlocked();++bind_calls;
    assert(descriptor==expected_descriptor&&input==expected_bind_view&&shader==expected_shader_owner&&selection==expected_selection&&alternate==expected_alternate);
    if(descriptor)put(context,0x768+uintptr_t(descriptor[0])*8,uintptr_t(0x5555));
    put(context,0x65d,uint8_t(10));
    if(mode==106)throw std::runtime_error("bind");
    return 0xfedcba9876543210ull;
}
void bind_reset(){
    control_reset();dlss_route_observer::input_attested=true;dlss_route_observer::original_bind=original_bind;
    shader_owner={};public_wrapper={};bind_calls=source_query_calls=0;mode=100;
    put(shader_owner,0x50,uintptr_t(0xabc123));put(shader_owner,0xb4,gears_dlss_route::Descriptor{5,1,2});
    put(public_wrapper,0x138,uint32_t(2));put(public_wrapper,0x38+2*8,uintptr_t(0x5555));
    input_public_cell=reinterpret_cast<uintptr_t>(public_wrapper.data())+0x13c;input_alternate_cell=input_public_cell;
    expected_descriptor=reinterpret_cast<const uint16_t*>(shader_owner.data()+0xb4);expected_bind_view=expected_view;expected_shader_owner=shader_owner.data();
    expected_selection=0;expected_alternate=0;put(context,0x768+5*8,uintptr_t(0x4444));
}
uint32_t test_input_bind_output(){
    fake_caller=mode==104?0xd831c9:0xd831c8;
    assert(dlss_route_observer::hook_bind(expected_descriptor,expected_bind_view,expected_shader_owner,expected_selection,expected_alternate)==0xfedcba9876543210ull);
    if(mode==105)assert(dlss_route_observer::hook_bind(expected_descriptor,expected_bind_view,expected_shader_owner,expected_selection,expected_alternate)==0xfedcba9876543210ull);
    if(mode==107)put(context,0x768+5*8,uintptr_t(0x6666));
    if(mode==109)put(shader_owner,0xb4,gears_dlss_route::Descriptor{7,1,2});
    dlss_route_observer::quad(0xd83cbf,{0,0,1280,1080,0,0,1280,1080},{2560,1080,2560,1080});
    if(mode==108)put(context,0x768+5*8,uintptr_t(0x7777));
    late_quad_candidate={true,{0,0,1280,1080,0,0,1280,1080},{2560,1080,2560,1080},0xd83cbf};
    dlss_route_observer::draw(reinterpret_cast<uintptr_t>(actual_list.data()),base+0x1af92b,1,6,1);
    return 0xfedcba98u;
}
void run_input_bind_cases(){
    bind_reset();assert(call()==0xfedcba98u&&bind_calls==1);
    const auto& b=dlss_route_observer::recorder.rows[0].input;
    assert(b.present&&b.exited&&!b.unwinding&&b.owner_descriptor&&b.output_view_matches&&b.result==0xfedcba9876543210ull);
    assert(b.before.descriptor==b.after.descriptor&&b.tracked_slot_known&&b.tracked_slot==5);
    assert(b.before.resource.object==0x4444&&b.after.resource.object==0x5555&&b.after.resource.native==0xabc&&b.after.resource.canonical==0x1234);
    assert(b.after.resource.width==2560&&b.after.highwater==10&&b.after.public_object==b.after.resource.object);
    assert(b.quad.present&&b.draw.present&&b.quad.sample.slot==5&&b.draw.sample.resource.object==0x5555&&b.draw.list_matches&&b.draw.generation==4);
    bind_reset();expected_selection=2;expected_alternate=-7;assert(call()==0xfedcba98u&&bind_calls==1);
    assert(dlss_route_observer::recorder.rows[0].input.selection==2&&dlss_route_observer::recorder.rows[0].input.alternate==-7);
    bind_reset();mode=102;expected_descriptor=nullptr;assert(call()==0xfedcba98u&&bind_calls==1);
    assert(dlss_route_observer::recorder.rows[0].input.present&&!(dlss_route_observer::recorder.rows[0].input.before.mask&1));
    bind_reset();expected_shader_owner=nullptr;assert(call()==0xfedcba98u&&bind_calls==1);
    assert(!dlss_route_observer::recorder.rows[0].input.owner_descriptor&&!(dlss_route_observer::recorder.rows[0].input.before.mask&4));
    bind_reset();mode=104;assert(call()==0xfedcba98u&&bind_calls==1&&!dlss_route_observer::recorder.rows[0].input.present&&source_query_calls==0);
    bind_reset();mode=105;assert(call()==0xfedcba98u&&bind_calls==2&&dlss_route_observer::recorder.rows[0].input.duplicates==1);
    bind_reset();mode=106;try{call();assert(false);}catch(std::runtime_error const&){}
    assert(bind_calls==1&&active.load()==0&&!dlss_route_observer::current&&dlss_route_observer::recorder.rows[0].input.unwinding);
    for(unsigned overwrite:{107u,108u}){bind_reset();mode=overwrite;assert(call()==0xfedcba98u);
        const auto& input=dlss_route_observer::recorder.rows[0].input;
        assert(input.after.resource.object==0x5555&&input.draw.sample.resource.object!=input.after.resource.object);
        if(overwrite==107)assert(input.quad.sample.resource.object==0x6666);else assert(input.quad.sample.resource.object==0x5555);}
    bind_reset();mode=109;assert(call()==0xfedcba98u);const auto& changed=dlss_route_observer::recorder.rows[0].input;
    assert(changed.quad.sample.descriptor[0]==7&&changed.quad.sample.slot==5&&changed.draw.sample.slot==5);
    bind_reset();put(shader_owner,0xb4,gears_dlss_route::Descriptor{40,1,2});assert(call()==0xfedcba98u&&bind_calls==1);
    assert(!dlss_route_observer::recorder.rows[0].input.tracked_slot_known&&!(dlss_route_observer::recorder.rows[0].input.draw.sample.mask&32));
    bind_reset();expected_bind_view=views.data()+0xd30;assert(call()==0xfedcba98u&&bind_calls==1&&!dlss_route_observer::recorder.rows[0].input.output_view_matches);
    bind_reset();dlss_route_observer::recorder.size=8;assert(call()==0xfedcba98u&&bind_calls==1&&source_query_calls==0);
    bind_reset();fake_caller=0xd831c8;
    assert(dlss_route_observer::hook_bind(expected_descriptor,expected_bind_view,expected_shader_owner,0,0)==0xfedcba9876543210ull&&bind_calls==1&&!dlss_route_observer::recorder.size);
    bind_reset();assert(call()==0xfedcba98u);recording=false;FILE* report{};assert(!tmpfile_s(&report)&&report);dlss_route_observer::print(report);
    assert_report(report,"shader_sampling_verified=0",true);fclose(report);
    assert(!tmpfile_s(&report)&&report);input_hash_fixture=true;input_hash_tail_missing=false;input_hash_bytes={};
    constexpr auto zero_hash="561dc0533eb1201eb4bdc0a0f90a89929c8ab2ba5fd38d82474b18d3cdaec5fb";
    assert(dlss_route_observer::verify_input_chunks(report,zero_hash));
    input_hash_bytes.back()=1;assert(!dlss_route_observer::verify_input_chunks(report,zero_hash));
    input_hash_bytes.back()=0;input_hash_tail_missing=true;assert(!dlss_route_observer::verify_input_chunks(report,zero_hash));
    input_hash_tail_missing=false;assert(!dlss_route_observer::enable_input_bind(report)); // Production identity never learns synthetic bytes.
    input_hash_fixture=false;fclose(report);
    std::puts("DlssRouteInputBindTest passed (five-argument wrapper, selected nonzero slot, later overwrite; no admission)");
}
}
