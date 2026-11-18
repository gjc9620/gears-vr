// Bow-only yellow one-frame lines through the existing original Actor call.
// Runs AFTER mechanics/input/poses; no new render hook, persistent component,
// input writer, hand grant or collision volume. Failed/late cues are dropped.
namespace torquebow_guide {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace input=torquebow_input;
namespace guide=gears_torque_bow_guide;
guide::Selector selector;hand::Frame drawn_source{};bool drawn_known{};unsigned rows{};guide::Step reported{};
void lost() noexcept {selector.lost();}
bool transport_current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&
        weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc)&&
        hand::frame_active(f,weaponhand::clock_ns(nullptr))&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        input::driver.bound()&&input::driver.matches_source(f,input::backend.hold(f))&&input::backend.geometry_current(f);
}
bool current(hand::Frame const& f) noexcept {return transport_current(f)&&nativeequipment::current(nullptr,f);}
bool line_call(hand::Frame const& f,playground::Args& call,hand::ObjectIdentity& fn) noexcept {
    if(!current(f)||!torquebow_original::original_function(call,L"Actor",L"Engine")||!call.r.size||call.r.size>64||
       !nativeequipment::identify(nullptr,call.r.function,fn)||(get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||
       get<std::uint16_t>(call.r.function+0x108)!=0xffff||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* labels[]={L"LineStart",L"LineEnd",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* types[]={L"StructProperty",L"StructProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned widths[]={12,12,1,1,1,4};std::uint64_t occupied{};std::array<uptr,6> properties{};
    for(unsigned i=0;i<6;++i){const auto p=call.prop(labels[i]);const auto offset=get<unsigned>(p+0x94);
        if(!torquebow_original::input_property(call,labels[i],types[i],widths[i],offset)||
           offset>call.r.size||widths[i]>call.r.size-offset)return false;
        properties[i]=p;
        for(unsigned j=0;j<widths[i];++j){const auto bit=std::uint64_t(1)<<(offset+j);if(occupied&bit)return false;occupied|=bit;}
        if(i<2){const auto type=get<uptr>(p+0xd0);if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    std::array<uptr,32> seen{};unsigned count{},params{};
    for(auto p=get<uptr>(call.r.function+0x80);p;p=get<uptr>(p+0x68)){
        if(count==seen.size()||!registered(p))return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;seen[count++]=p;
        if(!(get<unsigned long long>(p+0x78)&0x80))continue;
        if(std::find(properties.begin(),properties.end(),p)==properties.end())return false;++params;
    }
    return params==6&&call.set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(220))
        .set(L"B",static_cast<unsigned char>(0)).valid&&nativelancerfollow::bool_arg(call,L"bPersistentLines",false);
}
void tick(hand::Frame const& f,hand::GripGameState const& game,bool observed_source) noexcept {
    const auto began=weaponhand::clock_ns(nullptr);auto& backend=input::backend;const auto& model=input::driver.interaction();
    if(!observed_source||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !f.left_tracked||!f.left_grip_known||!backend.last_known||!hand::same_frame(backend.last_source,f)||
       !current(f)||input::driver.inventory().loaded!=backend.last.loaded||input::driver.inventory().reserve!=backend.last.reserve){lost();return;}
    guide::Sample sample{};sample.source=f;sample.hold=backend.hold(f);sample.config=model.instance_config_serial();sample.now=began;
    sample.stage=input::driver.stage();sample.loaded=backend.last.loaded;sample.reserve=backend.last.reserve;
    sample.held_arrow=model.held_arrow_serial()!=0;sample.charging=backend.last.original.charging;
    sample.receipts={model.last_loaded_token(),model.last_confirmed_cancel_edge(),model.last_original_projectile_serial()};
    if(sample.charging){torquebow_guide_source::Progress progress{};sample.progress_known=torquebow_guide_source::progress(f,progress);
        sample.charge_count=progress.count;sample.charge_max=progress.maximum;}
    const auto view=selector.advance(sample);
    if(view.step==guide::Step::none||(drawn_known&&hand::same(drawn_source.key,f.key)))return;
    gears_rig_tracking::Frame raw{};gears_torque_bow_arrow_tracking::RootSample root{};
    vr::Pose shoulder{};guide::Shape shape{};
    if(nativeequipment_rig::acquire(f,raw).status!=nativeequipment_rig::Status::ready||
       !gears_torque_bow_arrow_tracking::root_sample(f,raw,nativelancerfollow::mount,backend.now(),root)||
       std::abs(root.scale-backend.geometry_bow_scale)>1e-6)return;
    if(view.step==guide::Step::shoulder||view.step==guide::Step::no_ammo){vr::Vector raw_center{};
        const vr::Pose body{f.stereo.head.position,f.reference.orientation};
        if(!gears_weapon::shoulder_ammo_center(nativeequipment::calibration,body,raw_center)||
           !vr::rig_tracking::pose(raw.base_view,raw.reference,{raw_center,body.orientation},raw.units_per_metre,shoulder))return;
    }
    if(!guide::make(view,root.world,root.scale,raw.units_per_metre,shoulder,shape)||!current(f))return;
    playground::Args call(f.world.address,L"DrawDebugLine");hand::ObjectIdentity fn{};
    if(!line_call(f,call,fn)||weaponhand::clock_ns(nullptr)-began>=8000000)return;
    // Consume before dispatch: unknown original return or synchronous reentry
    // cannot replay the same batch. A fresh frame can draw its own transient.
    drawn_source=f;drawn_known=true;unsigned drawn{};
    for(unsigned i=0;i<shape.count;++i){hand::ObjectIdentity actual{};
        if(weaponhand::clock_ns(nullptr)-began>=8000000||!transport_current(f)||!registered(f.world.address)||
           !nativeequipment::identify(nullptr,fn.address,actual)||!hand::same(fn,actual))break;
        const auto& edge=shape.lines[i];const playground::Vector a{float(edge.start.x),float(edge.start.y),float(edge.start.z)};
        const playground::Vector b{float(edge.end.x),float(edge.end.y),float(edge.end.z)};
        if(!call.set(L"LineStart",a).set(L"LineEnd",b).valid||!call.run())break;++drawn;
    }
    const bool closed=current(f);
    if(log_file&&rows<24&&reported!=view.step){fwprintf(log_file,L"torquebow_guide frame=%llu step=%u lines=%u/%u progress=%.4f one_frame=1 original_charge_read_only=1 closing_current=%u elapsed_ns=%lld\n",
        f.key.frame,unsigned(view.step),drawn,shape.count,view.progress,unsigned(closed),weaponhand::clock_ns(nullptr)-began);fflush(log_file);++rows;reported=view.step;}
}
}
