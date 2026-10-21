#define GEARS_DLSS_OWNED_DOF_CONTROL_TEST 1
#define GEARS_NGX_ISOLATED_STEREO_TRIAL 1
#define GEARS_NGX_OWNED_SCENE_TRIAL 1
#define main unused_base_test_main
#include "dlss_output_control_test.cpp"
#undef main
namespace {
unsigned fault{},branch{};bool admitted_quad{};
uint64_t __fastcall owned_original(void* pass,uintptr_t unused,void* input){
    assert_frame_mutex_unlocked();++output_calls;
    assert(pass==expected_pass&&input==expected_view&&unused==0xfeed);
    fake_caller=fault==2?0xd7ff85:0xd7ff84;
    assert(dlss_route_observer::hook_bind(control_descriptor(),input,control_shader.data(),0,0)==0xfedcba9876543210ull);
    fake_caller=branch?0xd80925:0xd8084e;
    assert(dlss_route_observer::hook_setter(0xdead,descriptor(),payload.data(),0)==0x123456789abcdef0ull);
    const uintptr_t site=(branch||(fault==3))?0xd80a7b:0xd808d8;
    const gears_dlss_output::Args raw{0,0,853,720,0,0,853,720};auto args=raw;
    const gears_dlss_output::Dimensions dimensions{2560,1080,2560,1080};
    dlss_route_observer::quad(site,args,dimensions);
    {dlss_output_control::QuadScope quad(true,site,args,dimensions);admitted_quad=quad.candidate();
        if(site==0xd80a7b){assert(!gears_final_transfer::eye_caller(site));
            assert(gears_final_transfer::eye_caller(site,false,admitted_quad)==admitted_quad);}
        quad.forward(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
        if(admitted_quad){const unsigned eye=dlss_output_control::current_output->before.eye;
            auto const& native=dlss_output_control::current_output->native_gap;
            const float sw=native?float(native->input[eye][2]):1280.0f,sh=native?float(native->input[eye][3]):1080.0f;
            assert(args==gears_dlss_output::Args({float(eye*1280),0,1280,1080,float(eye*1280),0,sw,sh}));
            late_quad_candidate={true,args,dimensions,site};
            if(fault==4)vr_dof_compat::zero=false;
            if(fault==6)ngx_capture_control::fresh=false;
            if(fault==10)ngx_capture_control::gap_fresh=false;
            gears_viewport::TargetDraw target{0,1,{fault==5?0x5679ull:0x5678ull,2560,1080,28,0},0};
            gears_viewport::DrawState rectangles{true,{0,0,2560,1080},{0,0,2560,1080},1,1,true,0,1};
            dlss_output_control::DrawCompletion completion(reinterpret_cast<uintptr_t>(actual_list.data()));
            const auto proof=dlss_output_control::draw(reinterpret_cast<uintptr_t>(actual_list.data()),base+0x1af92b,1,6,1,target,{0x500,4},rectangles);
            if(proof.candidate)++control_draw_count;
        }else assert(args==raw);
    }
    if(fault==7)throw std::runtime_error("owned output tail");
    return 0xabcdef9876543210ull;
}
void prepare(unsigned selected_fault,unsigned selected_branch){
    control_reset();fault=selected_fault;branch=selected_branch;admitted_quad=false;
    vr_dof_compat::zero=fault!=1;vr_dof_compat::checks=0;
    auto& out=ngx_capture_control::fixture;out={};out.identity={1,9,3,4,5};out.ticket=7;out.color=0x1234;
    auto& layout=out.layout;layout.full_output={2560,1080};layout.input={853,720};layout.eye_output={1280,1080};layout.stride=1280;
    layout.input_base[1]={1280,0};for(auto& physical:layout.physical)physical=layout.full_output;
    for(unsigned eye=0;eye<2;++eye){auto& copy=out.copies[eye];copy.source_box={0,0,0,1280,1080,1};copy.destination={eye*1280,0,0};
        copy.source=0x7890;copy.target=out.color;copy.list=10+eye;copy.generation=4;copy.ticket=7;copy.ordinal=eye;
        put(views,eye*0xd30+0x48,gears_dlss_route::Float4{float(eye*1280),0,853,720});
        put(views,eye*0xd30+0x58,std::array<int32_t,4>{int32_t(eye*1280),0,853,720});}
    ngx_capture_control::available=fault!=8;ngx_capture_control::fresh=true;
    ngx_capture_control::gap_available=false;ngx_capture_control::gap_fresh=true;
    if(fault==9)out.color=0x4321;
    dlss_route_observer::original_owned_dof=owned_original;
}
void owned_eye(unsigned eye){
    expected_view=views.data()+eye*0xd30;put(context,0x14c0,reinterpret_cast<uintptr_t>(expected_view));
    assert(dlss_route_observer::hook_owned_dof(expected_pass,0xfeed,expected_view)==0xabcdef9876543210ull);
}
void native_gap(bool full){
    ngx_capture_control::available=false;ngx_capture_control::gap_available=true;
    auto& gap=ngx_capture_control::gap;gap={};gap.world=9;gap.ticket=7;gap.prior_sdk_ticket=6;gap.epoch={1,9,3,4,5};
    gap.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);gap.views={reinterpret_cast<uintptr_t>(views.data()),reinterpret_cast<uintptr_t>(views.data()+0xd30)};
    gap.output=gap.texture={2560,1080};
    for(unsigned eye=0;eye<2;++eye){gap.input[eye]={eye*1280,0,full?1280u:853u,full?1080u:720u};
        if(full){put(views,eye*0xd30+0x48,gears_dlss_route::Float4{float(eye*1280),0,1280,1080});
            put(views,eye*0xd30+0x58,std::array<int32_t,4>{int32_t(eye*1280),0,1280,1080});}}
}
void ordinary_output(unsigned flags){
    put(owner,8,uint32_t(flags));
    const gears_dlss_output::Args original{0,0,1280,1080,0,0,1280,1080};auto args=original;
    dlss_output_control::OutputScope output(true,reinterpret_cast<uintptr_t>(expected_pass),reinterpret_cast<uintptr_t>(expected_view));
    assert(!output.value.key&&!output.value.site);
    {dlss_output_control::BindScope bind(true,0xd831c8,control_descriptor(),expected_view,control_shader.data(),0,0);
        assert(!bind.tracking);}
    {dlss_output_control::SetterScope setter(true,0xd83b69,descriptor(),0);assert(!setter.tracking);}
    {dlss_output_control::QuadScope quad(true,0xd83cbf,args,{2560,1080,2560,1080});
        quad.forward(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
        assert(!quad.candidate()&&args==original);}
}
}
int main(){
    // Replay manual VDXR startup: ordinary flag4 Output and its generated
    // setter/quad are passthrough, not failed SDK evidence. Flag8 is likewise
    // outside this owned-DOF transport; legacy control has its own test target.
    for(unsigned flags:{4u,8u}){
        prepare(8,0);
        {dlss_output_control::ConsumeScope consume(9,7,true);ordinary_output(flags);}
        assert(dlss_output_control::publication(9,7)&&!dlss_output_control::publication(9,7,true));
        assert(!dlss_output_control::store.find({9,7})->site&&!control_draw_count);
        assert(!dlss_output_control::current_output&&!dlss_output_control::current_quad);
    }
    for(unsigned selected_branch=0;selected_branch<2;++selected_branch){
        prepare(0,selected_branch);{dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);owned_eye(1);
            assert(!dlss_output_control::publication(9,7,true));consume.finish();}
        assert(dlss_output_control::publication(9,7,true)&&control_draw_count==2&&vr_dof_compat::checks>=4);
        assert(dlss_route_observer::recorder.rows[0].quad_count==1&&dlss_route_observer::recorder.rows[1].quad_count==1);
    }
    // The ordinary pass after complete owned SDK eyes must neither poison nor
    // replace that proof. It cannot rescue an incomplete owned SDK pair.
    for(bool complete:{false,true}){
        prepare(0,0);
        {dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);if(complete)owned_eye(1);ordinary_output(4);}
        assert(dlss_output_control::publication(9,7,true)==complete);
    }
    for(unsigned selected_fault=1;selected_fault<=9;++selected_fault){
        prepare(selected_fault,0);bool threw=false;
        try{dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);}catch(std::runtime_error const&){threw=true;}
        assert(threw==(selected_fault==7));assert(!dlss_output_control::publication(9,7,true));
        if(selected_fault==8)assert(dlss_output_control::publication(9,7)&&!admitted_quad);
        if(selected_fault==1||selected_fault==2||selected_fault==3||selected_fault==9)assert(!admitted_quad);
        if(selected_fault==4||selected_fault==5||selected_fault==6)assert(admitted_quad&&!control_draw_count);
        assert(!dlss_output_control::current_output&&!dlss_output_control::current_quad&&!dlss_route_observer::current&&!dlss_output_control::active_key);
    }
    for(bool full:{false,true})for(unsigned selected_branch=0;selected_branch<2;++selected_branch){
        prepare(0,selected_branch);native_gap(full);source_canonical=0x9876; // Current native Color may differ from cached SDK Color.
        {dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);owned_eye(1);}
        assert(dlss_output_control::publication(9,7)&&control_draw_count==2&&vr_dof_compat::checks>=4);
        assert(!dlss_output_control::publication(9,7,true)); // Native gap proof cannot authorize an SDK frame.
        auto const& e=dlss_output_control::store.find({9,7})->payload.eyes[0];assert(e.owned_dof&&!e.sdk_output&&e.draw.source==0x9876);
    }
    for(unsigned selected_fault:{1u,2u,3u,4u,5u,10u}){
        prepare(selected_fault,0);native_gap(false);
        {dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);}
        assert(!dlss_output_control::publication(9,7)&&!dlss_output_control::publication(9,7,true));
        if(selected_fault==1||selected_fault==2||selected_fault==3)assert(!admitted_quad);
        else assert(admitted_quad&&!control_draw_count);
    }
    prepare(0,0);native_gap(false);
    auto view=dlss_route_observer::view(reinterpret_cast<uintptr_t>(expected_view));gears_dlss_output::Args args;
    auto& physical=ngx_capture_control::gap;physical.texture={};
    assert(!gears_dlss_output::native_geometry(view,{2560,1080,2560,1080},physical,9,7,reinterpret_cast<uintptr_t>(consuming_renderer),args));
    const gears_ngx_stereo::Resource color{0x1234,3,{2560,1080},26,0,1,0};
    assert(gears_dlss_output::set_native_texture(physical,color));
    for(unsigned mutation=0;mutation<7;++mutation){auto bad=color;auto candidate=physical;
        switch(mutation){case 0:bad.identity=0;break;case 1:++bad.device;break;case 2:bad.format=10;break;case 3:++bad.mip;break;
            case 4:bad.samples=2;break;case 5:++bad.subresource;break;case 6:--bad.extent.width;break;}
        assert((!gears_dlss_output::set_native_texture(candidate,bad)&&candidate.texture==std::array<unsigned,2>{}));
    }
    assert(gears_dlss_output::native_geometry(view,{2560,1080,2560,1080},ngx_capture_control::gap,9,7,reinterpret_cast<uintptr_t>(consuming_renderer),args));
    for(unsigned mutation=0;mutation<9;++mutation){auto bad=ngx_capture_control::gap;
        switch(mutation){case 0:bad.prior_sdk_ticket=7;break;case 1:bad.world=10;break;case 2:bad.epoch.world=10;break;
            case 3:++bad.renderer;break;case 4:++bad.views[1];break;case 5:--bad.texture[0];break;
            case 6:++bad.input[0][2];break;case 7:bad.input[1][0]=853;break;case 8:bad.input[0][3]=1081;break;}
        assert(!gears_dlss_output::native_geometry(view,{2560,1080,2560,1080},bad,9,7,reinterpret_cast<uintptr_t>(consuming_renderer),args));
    }
    prepare(0,0);native_gap(false);
    {dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);ngx_capture_control::available=true;ngx_capture_control::gap_available=false;owned_eye(1);}
    assert(!dlss_output_control::publication(9,7,true)); // Mixed native/SDK eyes cannot satisfy SDK publication.
    for(bool full:{false,true}){
        prepare(0,0);native_gap(full);ngx_capture_control::gap.prior_sdk_ticket=7;ngx_capture_control::gap.unpublished_sdk=true;
        {dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);owned_eye(1);}
        assert(control_draw_count==2&&dlss_output_control::unpublished(9,7,ngx_capture_control::gap.epoch));
        auto wrong=ngx_capture_control::gap.epoch;++wrong.epoch;assert(!dlss_output_control::unpublished(9,7,wrong));
        assert(!dlss_output_control::publication(9,7)&&!dlss_output_control::publication(9,7,true));
    }
    prepare(0,0);native_gap(false);ngx_capture_control::gap.prior_sdk_ticket=7;ngx_capture_control::gap.unpublished_sdk=true;
    {dlss_output_control::ConsumeScope consume(9,7,true);owned_eye(0);ngx_capture_control::available=true;ngx_capture_control::gap_available=false;owned_eye(1);}
    assert(dlss_output_control::unpublished(9,7,ngx_capture_control::gap.epoch)); // Late SDK readiness cannot upgrade the candidate image.
    assert(!dlss_output_control::publication(9,7)&&!dlss_output_control::publication(9,7,true));
    std::puts("Owned DOF controller passed: both route pairs, reduced-to-full geometry, complete two-eye publication and nine fail-closed cases (synthetic SDK/DOF seams; no GPU claim)");
    std::puts("Native gap control passed: reduced/full inputs on both routes, current native Color, stale/zero/list/target failures and SDK-proof separation (synthetic only)");
}
