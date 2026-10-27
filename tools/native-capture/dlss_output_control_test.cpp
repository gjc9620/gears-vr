#define GEARS_DLSS_OUTPUT_CONTROL 1
#define GEARS_DLSS_OUTPUT_CONTROL_TEST 1
#define GEARS_DLSS_INPUT_BIND_OBSERVER 1
#define GEARS_DLSS_BIND_TEST_RUNTIME 1
#include "dlss_route_observer_test.cpp"

namespace {
unsigned control_forward_count{},control_draw_count{};
alignas(8) std::array<unsigned char,8> actual_list{},foreign_list{};
alignas(8) std::array<unsigned char,0x20> list_owner{};
std::array<uintptr_t,13> list_vtable{};
alignas(8) std::array<unsigned char,0xc0> control_shader{};
alignas(8) std::array<unsigned char,0x180> control_public{};
unsigned control_bind_calls{};
const uint16_t* control_descriptor(){return reinterpret_cast<const uint16_t*>(control_shader.data()+0xb4);}
uint64_t __fastcall control_original_bind(const uint16_t* desc,void* view,void* shader,uint32_t selection,int32_t alternate){
    assert_frame_mutex_unlocked();++control_bind_calls;
    assert(desc==control_descriptor()&&view==expected_view&&shader==control_shader.data()&&!selection&&!alternate);
    if(mode==43)throw std::runtime_error("control bind");
    put(context,0x768+uintptr_t(mode==45?6:desc[0])*8,uintptr_t(0x777));
    if(mode==46)put(control_shader,0xb4,gears_dlss_route::Descriptor{6,1,0});
    if(mode==47)put(control_shader,0,uintptr_t(0x999));
    return 0xfedcba9876543210ull;
}
void control_bind(){fake_caller=mode==50||mode==82?0xd831c9:0xd831c8;
    assert(dlss_route_observer::hook_bind(control_descriptor(),expected_view,control_shader.data(),0,0)==0xfedcba9876543210ull);}
void control_reset(){
    reset();dlss_output_control::first_exit_failure={};dlss_output_control::exit_failure_claimed=false;dlss_output_control::exit_failure_ready=false;
    dlss_output_control::store={};dlss_output_control::attested=true;dlss_output_control::active_key={};
    dlss_output_control::current_output=nullptr;dlss_output_control::current_quad=nullptr;
    input_public_missing=false;
    source_available=true;source_width=2560;source_format=26;control_forward_count=control_draw_count=control_bind_calls=0;mode=10;
    source_native=0xabc;source_canonical=0x1234;
    dlss_route_observer::input_attested=true;dlss_route_observer::original_bind=control_original_bind;
    fake_list_generation=4;fake_list_canonical=0x500;fake_system_methods=true;drift_list_generation=false;
    dlss_output_control::other_setters=dlss_output_control::list_rejected=0;
    dlss_output_control::non_target_setters={};
    dlss_output_control::non_target_quads={};
    actual_list={};foreign_list={};list_owner={};list_vtable={};list_vtable[12]=0x1337;viewport_targets[3]=reinterpret_cast<void*>(list_vtable[12]);
    put(actual_list,0,reinterpret_cast<uintptr_t>(list_vtable.data()));put(foreign_list,0,reinterpret_cast<uintptr_t>(list_vtable.data()));
    put(list_owner,0x14,reinterpret_cast<uintptr_t>(actual_list.data()));put(context,0x127c,reinterpret_cast<uintptr_t>(list_owner.data()));
    put(owner,8,uint32_t(12));put(owner,0xaa,gears_dlss_route::Descriptor{0xffff,0xffff,0});put(family,0x4c,uint32_t(1));
    control_shader={};control_public={};put(control_shader,0,uintptr_t(0xabcde));put(control_shader,0x50,uintptr_t(0xabc123));
    put(control_shader,0xb4,gears_dlss_route::Descriptor{5,1,0});put(control_public,0x138,uint32_t(2));put(control_public,0x38+2*8,uintptr_t(0x777));
    input_public_cell=reinterpret_cast<uintptr_t>(control_public.data())+0x13c;input_alternate_cell=input_public_cell;
    put(context,0x65d,uint8_t(10));put(context,0x768+5*8,uintptr_t(0x444));
    for(unsigned eye=0;eye<2;++eye){const auto offset=eye*0xd30;
        put(views,offset+0x48,gears_dlss_route::Float4{float(eye*1280),0,1280,1080});
        put(views,offset+0x58,std::array<int32_t,4>{int32_t(eye*1280),0,1280,1080});}
}
void controlled_quad(unsigned selected_mode){
    using namespace gears_dlss_output;
    const Args raw{4294966784.0f,35,2534,1010,0,0,2534,1010};Args args=raw;Dimensions sizes{2560,1080,2560,1080};
    const uintptr_t caller=selected_mode==24?0xd83b1c:0xd83cbf;
    dlss_output_control::QuadScope quad(true,caller,args,sizes);
    quad.forward(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
    if(quad.candidate()){
        ++control_forward_count;const unsigned eye=dlss_route_observer::view(reinterpret_cast<uintptr_t>(expected_view)).eye;
        assert(args==Args({float(eye*1280),0,1280,1080,float(eye*1280),0,1280,1080}));
    }else assert(args==raw);
    late_quad_candidate={true,args,sizes,caller};if(selected_mode==26)late_quad_candidate.args[7]=1079;
    if(selected_mode==51)put(context,0x768+5*8,uintptr_t(0x888)); // Between candidate forwarding and actual draw.
    if(selected_mode!=11){
        dlss_output_control::DrawCompletion completion(reinterpret_cast<uintptr_t>(actual_list.data()));
        gears_viewport::TargetDraw binding{0,1,{0x5678,2560,1080,24,0},0};
        gears_frame_link::ListRef ref{0x500,selected_mode==25?0ull:4ull};
        gears_viewport::DrawState rectangles{true,{0,0,2560,1080},{0,0,2560,1080},1,1,true,0,1};
        const auto proof=dlss_output_control::draw(reinterpret_cast<uintptr_t>(actual_list.data()),base+0x1af92b,1,6,1,binding,ref,rectangles);
        if(proof.candidate){++control_draw_count;assert(gears_final_transfer::eligible_eye_target(caller,true,7,2560,1080,binding,true,proof,9));
            auto stale=proof;stale.ticket=8;assert(!gears_final_transfer::eligible_eye_target(caller,true,7,2560,1080,binding,true,stale,9));
            stale=proof;stale.world=10;assert(!gears_final_transfer::eligible_eye_target(caller,true,7,2560,1080,binding,true,stale,9));}
        if(selected_mode==23)dlss_output_control::draw(reinterpret_cast<uintptr_t>(actual_list.data()),base+0x1af92b,1,6,1,binding,ref,rectangles);
        if(selected_mode==49)put(context,0x768+5*8,uintptr_t(0x888));
        if(selected_mode==52)++fake_list_generation; // Drift during the actual native draw invocation.
        if(selected_mode==53)throw std::runtime_error("actual draw");
        if(selected_mode==66)put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));
    }
    if(selected_mode==67)put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));
    if(selected_mode==30)throw std::runtime_error("quad");
}
uint32_t test_control_output(){
    const auto selected_mode=mode;
    if(selected_mode==83){put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));return 0xfedcba98u;}
    if(selected_mode==19){mode=10;dlss_route_observer::hook_output(expected_pass,0xfeed,expected_view);mode=19;}
    {
        // d82fd9 calls 194950, which can issue this 1024x32 preprocessing
        // quad (call 194c8e / return 194c93) before the generated final quad.
        const std::array<float,8> before{0,0,1024,32,0,0,1024,32};auto args=before;
        dlss_output_control::QuadScope preprocessing(true,0x194c93,args,{1024,32,1024,32});
        preprocessing.forward(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
        assert(args==before&&!preprocessing.candidate()&&!dlss_output_control::scoped());
        assert(dlss_output_control::current_output->quads==0);
    }
    if(selected_mode!=40)control_bind();
    if(selected_mode==41)control_bind();
    if(selected_mode==44)put(control_public,0x38+2*8,uintptr_t(0x999));
    if(selected_mode==48)put(context,0x768+5*8,uintptr_t(0x888));
    // Real preceding setters are unrelated to the one generated output setter.
    for(auto caller:{uintptr_t(0xd8388e),uintptr_t(0xd8397d)}){fake_caller=caller;
        assert(dlss_route_observer::hook_setter(0xdead,descriptor(),payload.data(),0)==0x123456789abcdef0ull);}
    if(selected_mode!=18){fake_caller=0xd83b69;assert(dlss_route_observer::hook_setter(0xdead,descriptor(),payload.data(),0)==0x123456789abcdef0ull);
        if(selected_mode==29)assert(dlss_route_observer::hook_setter(0xdead,descriptor(),payload.data(),0)==0x123456789abcdef0ull);}
    if(selected_mode==14)put(owner,0xaa,gears_dlss_route::Descriptor{0xffff,0xfffe,0});
    if(selected_mode==15)consuming_frame_ticket=8;
    if(selected_mode==32)put(list_owner,0x14,reinterpret_cast<uintptr_t>(foreign_list.data())); // Same canonical, wrong raw list/context.
    if(selected_mode==33)drift_list_generation=true; // Reset/reuse during SRV query must be caught after it.
    if(selected_mode==34)fake_list_generation=5;
    if(selected_mode==35)put(context,0x127c,uintptr_t(0));
    if(selected_mode==36)fake_list_canonical=0x501;
    if(selected_mode==37)list_vtable[12]=0x9999;
    if(selected_mode==65)put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));
    controlled_quad(selected_mode);
    if(selected_mode==42)control_bind(); // Duplicate after actual draw must invalidate the ticket too.
    if(selected_mode==55)put(control_shader,0xb4,gears_dlss_route::Descriptor{6,1,0});
    if(selected_mode==12)controlled_quad(selected_mode);
    if(selected_mode==13)throw std::runtime_error("output after draw");
    if(selected_mode==22)put(views,0x48,gears_dlss_route::Float4{0,0,1279,1080});
    // Native marker1 d83d5b tail updates only this selector after the final quad.
    if(selected_mode==56)put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));
    if(selected_mode==57)put(context,0x768+5*8,uintptr_t(0x888));
    if(selected_mode==58)dlss_output_control::current_output->binding_capture.descriptor_address=1;
    if(selected_mode==59)++fake_list_generation;
    if(selected_mode>=60&&selected_mode<=82)put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(selected_mode==60?2:1));
    if(selected_mode==61)put(family,0x4c,uint32_t(0));
    if(selected_mode==62)put(control_public,0x38+2*8,uintptr_t(0x999));
    if(selected_mode==63)put(context,0x768+5*8,uintptr_t(0x888));
    if(selected_mode==64)++fake_list_generation;
    if(selected_mode==68)put(control_shader,0xb4,gears_dlss_route::Descriptor{6,1,0});
    if(selected_mode==69)put(control_shader,0x50,uintptr_t(0xabc124));
    if(selected_mode==70)input_public_cell=0;
    if(selected_mode==71)source_format=24;
    if(selected_mode==72)committed_world_generation=10;
    if(selected_mode==73)input_public_cell+=8;
    if(selected_mode==74)put(context,0x65d,uint8_t(11));
    if(selected_mode==75)put(owner,0xaa,gears_dlss_route::Descriptor{0xffff,0xfffe,0});
    if(selected_mode==76)put(control_shader,0,uintptr_t(0xabcdf));
    if(selected_mode==77)put(list_owner,0x14,reinterpret_cast<uintptr_t>(foreign_list.data()));
    if(selected_mode==78)drift_list_generation=true;
    if(selected_mode==79)source_width=1280;
    if(selected_mode==80)source_available=false;
    if(selected_mode==81)input_public_missing=true;
    if(selected_mode==84){put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));throw std::runtime_error("output tail");}
    if(selected_mode==85||selected_mode==86){put(views,uintptr_t(expected_view)-reinterpret_cast<uintptr_t>(views.data())+0x3e8,uint32_t(1));
        if(selected_mode==85)source_canonical=0x4321;else source_native=0xdef;}
    assert(!dlss_output_control::publication(9,7)); // Output/consume are still incomplete.
    return 0xfedcba98u;
}
void eye(unsigned index){expected_view=views.data()+index*0xd30;put(context,0x14c0,reinterpret_cast<uintptr_t>(expected_view));assert(call()==0xfedcba98u);}
void run_control_cases(){
    using namespace gears_dlss_output;
    assert(!gears_final_transfer::eye_caller(0xd83cbf,true));
    assert(gears_final_transfer::eye_caller(0xd83cbf,false,true));
    gears_viewport::TargetDraw binding{0,1,{0x5678,2560,1080,24,0},0};
    assert(!gears_final_transfer::eligible_eye_target(0xd83cbf,true,7,2560,1080,binding,true));
    control_reset();{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}
    assert(dlss_output_control::publication(9,7)&&control_forward_count==2&&control_draw_count==2);
    assert(!dlss_output_control::exit_failure_ready.load(std::memory_order_acquire));
    assert(control_bind_calls==2&&dlss_output_control::last_allowed.proof.eyes[0].binding.slot==5&&
        dlss_output_control::last_allowed.proof.eyes[0].binding.highwater==10);
    assert(dlss_output_control::other_setters==4&&setter_calls==6); // Two preceding setters plus the sole target in each eye.
    assert((dlss_output_control::non_target_setters==std::array<uint64_t,3>{2,2,0}));
    assert((dlss_output_control::non_target_quads==std::array<uint64_t,2>{2,0}));
    assert((data==std::array<unsigned char,0x100>{})); // Sentinel setter is unchanged and remains a no-op.
    assert(!dlss_output_control::publication(10,7)&&!dlss_output_control::publication(9,8));
    dlss_output_control::retire();assert(!dlss_output_control::publication(9,7));
    control_reset();dlss_route_observer::recorder.size=8;
    {dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}
    assert(dlss_output_control::publication(9,7)&&control_forward_count==2&&dlss_route_observer::recorder.limited==2);
    for(unsigned failure:{11u,12u,14u,15u,18u,19u,22u,23u,24u,25u,26u,29u,32u,33u,34u,35u,36u,37u,40u,41u,42u,44u,45u,46u,47u,48u,49u,50u,51u,52u,55u}){
        control_reset();mode=failure;{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}
        assert(!dlss_output_control::publication(9,7));
        if(failure>=32&&failure<=37)assert(!control_draw_count);
    }
    for(unsigned exception:{13u,30u,43u,53u,84u}){control_reset();mode=exception;
        try{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);assert(false);}catch(std::runtime_error const&){}
        assert(!dlss_output_control::publication(9,7)&&!dlss_output_control::current_output&&!dlss_output_control::current_quad&&!dlss_output_control::active_key);}
    control_reset();source_available=false;{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7));
    control_reset();source_width=1280;{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7));
    control_reset();put(context,0x65d,uint8_t(5));{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7));
    control_reset();source_format=24;{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7));
    control_reset();put(owner,8,uint32_t(4));{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7)&&!control_forward_count);
    control_reset();put(family,0x4c,uint32_t(0));{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7)&&!control_forward_count);
    control_reset();put(views,0x58,std::array<int32_t,4>{-1,0,1280,1080});{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7)&&!control_forward_count);
    control_reset();put(views,0x48,gears_dlss_route::Float4{0,0,2560,1080});{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7)&&!control_forward_count);
    control_reset();{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);}assert(!dlss_output_control::publication(9,7));
    control_reset();{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(0);eye(1);}assert(!dlss_output_control::publication(9,7));
    control_reset();{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);
        put(control_shader,0x50,uintptr_t(0xabc124));eye(1);}assert(!dlss_output_control::publication(9,7)&&control_draw_count==2);
    control_reset();{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);
        put(control_public,0x138,uint32_t(3));put(control_public,0x38+3*8,uintptr_t(0x777));eye(1);}
    assert(!dlss_output_control::publication(9,7)&&control_draw_count==2);
    for(unsigned failure:{57u,58u,59u}){
        control_reset();mode=failure;
        {dlss_output_control::ConsumeScope consume(9,7,true);eye(0);}
        assert(!dlss_output_control::publication(9,7)&&dlss_output_control::exit_failure_ready.load(std::memory_order_acquire));
        auto first=dlss_output_control::first_exit_failure;
        assert(first.valid&&!first.failed&&!first.nested&&!first.exception&&first.setter_proved&&first.quad_complete&&first.draw_candidate&&first.draw_complete);
        assert(first.setters==1&&first.quads==1&&first.draws==1&&first.stable&&first.descriptor_stable&&!first.binding_current);
        assert(control_draw_count==1&&control_forward_count==1&&first.binding.selector==0&&first.before.marker==1&&first.fresh_view.marker==1);
        if(failure==57)assert(first.binding.resource.object==0x777&&first.trace.sample.resource.object==0x888&&!first.binding_equal&&!first.binding_snapshot);
        if(failure==58)assert(!(first.trace.sample.mask&1)&&!first.binding_snapshot&&first.trace.sample.resource.object==0x777);
        if(failure==59)assert(first.binding.generation==4&&first.trace.after.generation==5&&!first.binding_equal&&first.binding_snapshot);
        // A different later eye failure cannot replace the published first row.
        mode=57;{dlss_output_control::ConsumeScope consume(9,7,true);eye(1);}
        assert(dlss_output_control::first_exit_failure.key==first.key&&dlss_output_control::first_exit_failure.view==first.view&&
            dlss_output_control::first_exit_failure.trace.sample.selector==first.trace.sample.selector);
        FILE* receipt{};assert(!tmpfile_s(&receipt)&&receipt);dlss_output_control::print_exit_failure(receipt);
        assert_report(receipt,"dlss_output_exit_failure",false);assert_report(receipt,"diagnostics_authorize=0",false);
        rewind(receipt);unsigned lines{};for(int ch;(ch=fgetc(receipt))!=EOF;)if(ch=='\n')++lines;assert(lines==5);fclose(receipt);
    }
    for(bool unreadable_new_bank:{false,true}){control_reset();mode=56;if(unreadable_new_bank)input_alternate_cell=0;
        {dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}
        assert(dlss_output_control::publication(9,7)&&control_forward_count==2&&control_draw_count==2);
        auto const& receipt=dlss_output_control::first_exit_failure;
        assert(dlss_output_control::exit_failure_ready.load(std::memory_order_acquire)&&!receipt.binding_current&&receipt.tail_transition_validated&&receipt.output_admitted);
        assert(receipt.trace.sample.selector==1&&receipt.trace.sample.public_global==base+0x1c3b488&&receipt.tail_trace.sample.selector==1&&receipt.tail_trace.sample.public_global==base+0x1c3b410);
        assert(dlss_output_control::last_allowed.proof.eyes[0].tail_transition_validated&&dlss_output_control::last_allowed.proof.eyes[1].tail_transition_validated);
        FILE* printed{};assert(!tmpfile_s(&printed)&&printed);dlss_output_control::print_exit_failure(printed);
        assert_report(printed,"tail_transition_validated=1",false);assert_report(printed,"sampled_sourceglobal=141c3b410",false);fclose(printed);
    }
    for(unsigned failure:{60u,61u,62u,63u,64u,65u,66u,67u,68u,69u,70u,71u,72u,73u,74u,75u,76u,77u,78u,79u,80u,81u,82u,85u,86u}){
        control_reset();mode=failure;{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);}
        assert(!dlss_output_control::publication(9,7)&&dlss_output_control::exit_failure_ready.load(std::memory_order_acquire));
        assert(!dlss_output_control::first_exit_failure.output_admitted&&!dlss_output_control::first_exit_failure.tail_transition_validated);
    }
    control_reset();mode=83;{dlss_output_control::ConsumeScope consume(9,7,true);eye(0);eye(1);}
    assert(dlss_output_control::publication(9,7)&&!control_forward_count&&!control_draw_count&&!dlss_output_control::exit_failure_ready.load(std::memory_order_acquire));
    control_reset();gears_dlss_route::InputBind raw;raw.shader_owner=reinterpret_cast<uintptr_t>(control_shader.data());raw.view_address=reinterpret_cast<uintptr_t>(views.data());
    raw.descriptor_address=reinterpret_cast<uintptr_t>(control_descriptor());raw.tracked_slot_known=true;raw.tracked_slot=5;
    put(views,0x3e8,uint32_t(1));
    const auto actual=dlss_route_observer::input_sample(raw,true),pinned=dlss_route_observer::input_sample(raw,true,base+0x1c3b410);
    const auto unsupported=dlss_route_observer::input_sample(raw,true,base+0x1c3b488);
    assert(actual.selector==1&&actual.public_global==base+0x1c3b488&&pinned.selector==1&&pinned.public_global==base+0x1c3b410);
    assert(unsupported.selector==1&&!(unsupported.mask&64)&&!unsupported.public_global);
    control_reset();FILE* identity{};assert(!tmpfile_s(&identity)&&identity);
    assert(!dlss_output_control::enable(identity,false));assert(!dlss_output_control::enable(identity,true)); // Missing live bytes cannot enable control.
    input_hash_fixture=true;input_hash_bytes={};assert(!dlss_output_control::enable(identity,true)); // Complete synthetic bytes are not the runtime identity.
    input_hash_tail_missing=true;assert(!dlss_output_control::enable(identity,true));input_hash_tail_missing=input_hash_fixture=false;
    owned_bindings_enabled=true;assert(!dlss_output_control::enable(identity,true));owned_bindings_enabled=false;fclose(identity);
    std::puts("DlssOutputControlTest passed (candidate and completed Output proofs remain separate)");
}
}
