#define GEARS_TROIKA_VIEW_LIVE_TEST 1
#include "troika_scene_tracking_test.cpp"

int run_view_cases(gears_vr::StereoResult& stereo){
    namespace live=troika_view_live;namespace tracking=troika_scene_tracking;
    auto const saved_source=tracking::value;auto const saved_basis=tracking::retained_basis;
    auto const saved_memory=fixture->memory;auto const saved_scene=fixture_scene;
    auto const saved_ns=current_ns;auto const saved_revision=rig_tracking_live::reference_revision;
    auto const selections=cover_input_live::selections;
    const gears_vr::StereoMatrix sentinel={1,0,0,0,0,1,0,0,0,0,1,0,42,43,44,1};
    auto view=sentinel;
    tracking::geometry_ready=false;tracking::geometry_value={};
    CHECK(live::prepare(&fixture_scene,stereo,view));
    CHECK(view==rig_tracking_live::original.base_view);
    CHECK(!std::memcmp(&tracking::value,&saved_source,sizeof saved_source));
    CHECK(cover_input_live::selections==selections); // No input reselection/retiming.
    CHECK(!tracking::geometry_ready); // Viewing needs no control/contact lease.
    for(unsigned fault=0;fault<17;++fault){
        fixture->memory=saved_memory;fixture_scene=saved_scene;current_ns=saved_ns;
        rig_tracking_live::reference_revision=saved_revision;weapon_hand_runtime::open=true;
        tracking::active=true;tracking::basis_retained=true;tracking::mapping_ready=true;
        auto sample=stereo;stereo_units_per_metre=100.;recording=true;
        if(fault==0)++fixture_scene.invocation;
        if(fault==1)++fixture_scene.thread;
        if(fault==2)++sample.sample.sequence;
        if(fault==3)++sample.reference_generation;
        if(fault==4)sample.reference.position.x=.1;
        if(fault==5)current_ns+=100000001;
        if(fault==6)rig_tracking_live::reference_revision+=2;
        if(fault==7)weapon_hand_runtime::open=false;
        if(fault==8)stereo_units_per_metre=101.;
        if(fault==9)tracking::basis_retained=false;
        if(fault==10)tracking::mapping_ready=false;
        if(fault==11)tracking::active=false;
        if(fault==12)fixture->put(0x10224,uintptr_t(0x20000)); // PC no longer possesses turret.
        if(fault==13)fixture->put(0x20810,uintptr_t(0)); // Driver mount relation lost.
        if(fault==14)fixture->put(0x80800,uintptr_t(0)); // Original turret Driver lost.
        if(fault==15)fixture->put(saved_source.driver_mesh.address+8,999u); // Reused identity.
        if(fault==16)recording=false;
        view=sentinel;CHECK(!live::prepare(&fixture_scene,sample,view));CHECK(view==sentinel);
        CHECK(!std::memcmp(&tracking::value,&saved_source,sizeof saved_source));
        CHECK(cover_input_live::selections==selections);
    }
    fixture->memory=saved_memory;fixture_scene=saved_scene;current_ns=saved_ns;
    rig_tracking_live::reference_revision=saved_revision;weapon_hand_runtime::open=true;
    tracking::active=tracking::basis_retained=tracking::mapping_ready=true;
    stereo_units_per_metre=100.;recording=true;
    view=sentinel;CHECK(!live::prepare(nullptr,stereo,view));CHECK(view==sentinel);
    // A read race must reject even after the copied current source validated.
    read_change=race;race_done=false;CHECK(!live::prepare(&fixture_scene,stereo,view));
    CHECK(race_done&&view==sentinel);read_change=nullptr;fixture->memory=saved_memory;
    CHECK(live::prepare(&fixture_scene,stereo,view));
    CHECK(tracking::cancel_basis(&saved_basis)==gears_rig_tracking::Reply::accepted);
    view=sentinel;CHECK(!live::prepare(&fixture_scene,stereo,view));CHECK(view==sentinel);
    CHECK(tracking::retain_basis(&saved_basis)==gears_rig_tracking::Reply::unavailable);
    CHECK(weapon_hand_runtime::detail::calls==0);
    std::printf("Troika fixed-view actual adapter: %u checks passed; sparse synthetic world/session, no GPU/game proof.\n",checks);
    return 0;
}
