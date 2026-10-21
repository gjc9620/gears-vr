#define GEARS_DLSS_OWNED_DOF_HOOK_TEST 1
#define main original_control_test_main
#include "dlss_output_control_test.cpp"
#undef main
namespace {
uint64_t __fastcall dof_original(void* pass,uintptr_t unused,void* view){
    return 0xa1b2c3d400000000ull|uint64_t(original_output(pass,unused,view));
}
}
int main(){
    assert(original_control_test_main()==0);
    using namespace gears_dlss_output;
    assert(bind_caller(true)==0xd7ff84&&bind_caller(false)==0xd831c8);
    assert(setter_caller(true,0xd8084e)&&setter_caller(true,0xd80925)&&!setter_caller(true,0xd83b69));
    assert(quad_caller(true,0xd8084e,0xd808d8)&&quad_caller(true,0xd80925,0xd80a7b));
    assert(!quad_caller(true,0xd8084e,0xd80a7b)&&!quad_caller(false,0xd8084e,0xd808d8));
    control_reset();mode=83;dlss_route_observer::original_owned_dof=dof_original;
    {dlss_output_control::ConsumeScope consume(9,7,true);
        assert(dlss_route_observer::hook_owned_dof(expected_pass,0xfeed,expected_view)==0xa1b2c3d4fedcba98ull);}
    assert(output_calls==1&&dlss_route_observer::recorder.rows[0].owned_dof);
    assert(dlss_output_control::publication(9,7)&&!dlss_output_control::publication(9,7,true));
    assert(!control_draw_count&&!control_forward_count&&!dlss_output_control::current_output&&!dlss_route_observer::current);
    control_reset();mode=2;dlss_route_observer::original_owned_dof=dof_original;
    try{dlss_output_control::ConsumeScope consume(9,7,true);dlss_route_observer::hook_owned_dof(expected_pass,0xfeed,expected_view);assert(false);}
    catch(std::runtime_error const&){}
    assert(output_calls==1&&!dlss_output_control::current_output&&!dlss_route_observer::current&&!dlss_output_control::active_key);
    control_reset();mode=83;dlss_route_observer::original_owned_dof=dof_original;
    for(unsigned i=0;i<10;++i)assert(dlss_route_observer::hook_owned_dof(expected_pass,0xfeed,expected_view)==0xa1b2c3d4fedcba98ull);
    assert(output_calls==10&&dlss_route_observer::recorder.size==8&&dlss_route_observer::recorder.limited==2);
    std::puts("Owned DOF hook: full 64-bit return, passthrough without SDK receipt, route pairing, exceptions and exhausted observer budget passed (synthetic only)");
    return 0;
}
