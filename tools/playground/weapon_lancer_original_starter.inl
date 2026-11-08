// Read-only current geometry consumer; the existing Native decoder remains
// the sole bone writer. No added T-grip, cord, material or second evaluator.
namespace lancersaw_visual {
namespace api=gears_lancer_original_handle;namespace hand=gears_weapon_hand;
namespace asset=weapon_part_assets;
asset::WeaponPoseOwner owner{};hand::ObjectIdentity node{};hand::FrameKey key{};
api::Control control{};api::Plan closed{};
std::atomic<unsigned> outstanding{};std::atomic<bool> stop_requested{};
bool ready{},visible{},revoking{},unknown_adoption{};unsigned failure{};
std::uint64_t generation{};PhaseCost last_cost{};
std::int64_t last_pose_ns{};double deployment_angle{};
api::DisplayTarget accepted_pose{};bool accepted_pose_known{};
void report() noexcept {
    static unsigned previous{~0u},rows{};const unsigned state=failure*4+unsigned(ready)+2*unsigned(visible);
    if(log_file&&rows<32&&state!=previous){
        fwprintf(log_file,L"lancer_starter_visual gate=%u ready=%u visible=%u generation=%llu original_bone=4 added_cord=0 retained=%u\n",
            failure,unsigned(ready),unsigned(visible),generation,outstanding.load());fflush(log_file);++rows;
    }previous=state;
}

template<class Fn>Fn function(char const* label) noexcept {
    const auto address=weaponhand::native_module?GetProcAddress(weaponhand::native_module,label):nullptr;
    return address&&weaponhand::executable(address,weaponhand::native_module)?reinterpret_cast<Fn>(address):nullptr;
}
bool live() noexcept{return outstanding.load(std::memory_order_acquire)!=0;}
void request_stop() noexcept{stop_requested.store(true,std::memory_order_release);}
bool same_binding(hand::Frame const& f) noexcept {
    return control.handle&&key.registration_cookie==f.key.registration_cookie&&key.lifecycle==f.key.lifecycle&&
        key.game_start==f.key.game_start&&key.native_started_qpc==f.key.native_started_qpc&&
        key.world_generation==f.key.world_generation&&asset::same_weapon(owner,f);
}
// A missing current interaction frame hides the cue/contact, not the owned
// bone resource. Hard binding changes and stop still use the normal drain.
void hide() noexcept {ready=visible=false;}
bool restore() noexcept {
    ready=visible=false;if(!asset::gt())return !live();if(!live())return true;
    revoking=true;if(unknown_adoption||!control.handle)return false;
    const auto call=function<api::Update>(api::restore_export);api::Receipt r{};
    if(!call)return false;const auto code=call(&control,&r);
    if(r.size!=sizeof r||r.version!=2||r.handle!=control.handle||r.result!=code||
       (code!=api::Result::restored&&code!=api::Result::inactive_restored)||
       (!r.cpu_restored&&!r.original_destroyed))return false;
    control={};owner={};node={};closed={};key={};generation=0;last_pose_ns=0;deployment_angle=0;
    accepted_pose={};accepted_pose_known=false;revoking=false;failure=0;
    outstanding.store(0,std::memory_order_release);return true;
}
bool reset_after_drain() noexcept {
    if(live())return false;stop_requested.store(false,std::memory_order_release);return true;
}
struct Observation {
    gears_vr::Vector rest{},grasp{},pivot{},axis{};
    double angle{},pull_raw{},scale{};bool deployed{};std::array<float,16> gun{};unsigned epoch{};
};
// All accesses are bounded reads. In particular this can run in Native's
// proposal callback: no GetBoneMatrix/ProcessEvent or broker reentrancy.
bool observe(hand::Frame const& f,Observation& out,bool calibrate=false) noexcept {
    out={};if(!same_binding(f)||revoking||!asset::object(owner.component)||!asset::object(owner.asset)||!asset::object(node))return false;
    api::Array a{},again{},atoms{};std::array<float,16> root{},handle{};unsigned epoch{},end_epoch{};
    const auto component=owner.component.address;
    if(!copy(&a,component+0x26c,sizeof a)||!api::array(a,64)||
       get<uptr>(component+0x218)!=owner.asset.address||get<uptr>(node.address+0x60)!=component||
       !copy(&epoch,component+0x338,sizeof epoch)||!epoch||
       !copy(root.data(),a.data,sizeof root)||!copy(handle.data(),a.data+4*64,sizeof handle))return false;
    api::Matrix r{},h{};std::copy(root.begin(),root.end(),r.begin());std::copy(handle.begin(),handle.end(),h.begin());
    if(!api::proper(r)||!api::proper(h))return false;
    if(calibrate&&!closed.known){api::Atom atom{};
        if(!copy(&atoms,component+0x27c,sizeof atoms)||!api::array(atoms,32)||
           !copy(&atom,atoms.data+4*32,sizeof atom)||!closed.bind(atom,r,h))return false;}
    if(!closed.known||!api::close(closed.root,r))return false;
    const auto delta=api::multiply(api::inverse(closed.space),h);
    out.angle=std::atan2(delta[1],delta[0]);api::Atom ignored{};api::Matrix expected{};
    if(out.angle<-.001||out.angle>api::maximum_angle+.001)return false;
    out.angle=std::clamp(out.angle,0.,api::maximum_angle);
    if(!closed.target(out.angle,0,ignored,expected))return false;
    out.pull_raw=h[13]-expected[13];
    if(out.pull_raw<-.002||out.pull_raw>api::maximum_pull_raw+.002)return false;
    out.pull_raw=std::clamp(out.pull_raw,0.,api::maximum_pull_raw);
    if(!closed.target(out.angle,out.pull_raw,ignored,expected)||!api::close(h,expected))return false;
    out.deployed=out.angle>=api::maximum_angle-.002;
    vrequip::MeshPoseProbe gun{},end{};
    if(!vrequip::mesh_pose_probe(f.weapon.address,gun)||gun.hidden||gun.mesh!=component||gun.asset!=owner.asset.address)return false;
    std::memcpy(out.gun.data(),gun.local_to_world,sizeof gun.local_to_world);
    const auto world=[&](std::array<double,3> const& p){return gears_vr::Vector{
        p[0]*out.gun[0]+p[1]*out.gun[4]+p[2]*out.gun[8]+out.gun[12],
        p[0]*out.gun[1]+p[1]*out.gun[5]+p[2]*out.gun[9]+out.gun[13],
        p[0]*out.gun[2]+p[1]*out.gun[6]+p[2]*out.gun[10]+out.gun[14]};};
    const auto move=api::multiply(api::inverse(closed.space),h);std::array<double,3> grasp{};
    for(unsigned j=0;j<3;++j){grasp[j]=move[12+j];for(unsigned i=0;i<3;++i)grasp[j]+=api::closed_grasp[i]*move[i*4+j];}
    api::Matrix deployed{};if(!closed.target(api::maximum_angle,0,ignored,deployed))return false;
    const auto deploy_delta=api::multiply(api::inverse(closed.space),deployed);std::array<double,3> rest{};
    for(unsigned j=0;j<3;++j){rest[j]=deploy_delta[12+j];for(unsigned i=0;i<3;++i)rest[j]+=api::closed_grasp[i]*deploy_delta[i*4+j];}
    out.rest=world(rest);out.grasp=world(grasp);out.pivot=world({closed.space[12],closed.space[13],closed.space[14]});
    const double size=std::hypot(std::hypot(double(out.gun[4]),double(out.gun[5])),double(out.gun[6]));
    if(!std::isfinite(size)||size<.1||size>10)return false;
    out.axis={out.gun[4]/size,out.gun[5]/size,out.gun[6]/size};out.scale=size;out.epoch=epoch;
    return copy(&again,component+0x26c,sizeof again)&&gears_weapon_part_visibility::same(a,again)&&
        copy(&end_epoch,component+0x338,sizeof end_epoch)&&epoch==end_epoch&&
        vrequip::mesh_pose_probe(f.weapon.address,end)&&end.mesh==gun.mesh&&end.asset==gun.asset&&
        !std::memcmp(end.local_to_world,gun.local_to_world,sizeof gun.local_to_world)&&
        // The proposal already holds the hand client's arbitration gate.
        // This is a read-only geometry bracket, so never re-enter that client
        // here: a failed nested Gate disarms the actual published input. The
        // ordinary tick/setter separately authenticates its current source.
        same_binding(f)&&asset::object(owner.component)&&asset::object(owner.asset)&&asset::object(node);
}
void tick(hand::Frame const& f,double pull_raw,bool deploy,bool lock) noexcept try {
    struct Review {~Review(){report();}} review;
    last_cost={};const auto begin=weaponhand::clock_ns(nullptr);ready=visible=false;
    if(stop_requested.load()||revoking||(live()&&!same_binding(f))){restore();return;}
    if(!asset::gt()||!unified_weapon_fire::manual_profile()||!std::isfinite(pull_raw)||pull_raw<0||pull_raw>api::maximum_pull_raw)return;
    failure=1;
    if(!live()){
        asset::WeaponPoseOwner selected{};hand::ObjectIdentity selected_node{};
        if(pull_raw!=0||!asset::weapon_owner(f,selected)||!hammerburst_animation::selected_node(f,selected_node))return;
        const auto bind=function<api::Begin>(api::bind_export);if(!bind)return;
        api::Request q{};q.source=f;q.component=selected.component;q.asset=selected.asset;q.node=selected_node;
        if(!nativeequipment::current(nullptr,f))return;
        // Journal before dispatch: an ambiguous adopted lease cannot unload.
        unknown_adoption=true;outstanding.store(1,std::memory_order_release);
        api::Receipt r{};const auto code=bind(&q,&r);
        if(r.size!=sizeof r||r.version!=2||r.result!=code){failure=2;return;}
        if(!r.handle){
            if(code==api::Result::rejected||code==api::Result::unavailable){unknown_adoption=false;outstanding.store(0);failure=100+r.gate;}
            return;
        }
        owner=selected;node=selected_node;key=f.key;generation=r.handle;control={};
        control.handle=r.handle;control.cookie=f.key.registration_cookie;unknown_adoption=false;last_pose_ns=begin;deployment_angle=0;
        accepted_pose={r.angle,r.pull_raw};accepted_pose_known=code==api::Result::pending||code==api::Result::ready;
        if(code==api::Result::restoring){revoking=true;restore();return;}
    }
    // Neutral auto-socket may deploy cosmetic geometry; only authenticated
    // Grip motion changes the slider. Release locks its current pose instead
    // of springing/folding it. Stow/owner loss still restores normally.
    const double dt=last_pose_ns>0&&begin>=last_pose_ns?std::clamp(double(begin-last_pose_ns)/1e9,0.,.1):0;
    last_pose_ns=begin;
    if(!lock)deployment_angle=std::clamp(deployment_angle+((deploy||pull_raw>.001)?1.:-1.)*dt*api::maximum_angle/.2,0.,api::maximum_angle);
    api::DisplayTarget target{};
    if(!api::display_target(lock,{closed.known?deployment_angle:0,pull_raw},accepted_pose,accepted_pose_known,target))return;
    if(lock)deployment_angle=target.angle;
    control.source=f;control.angle=target.angle;
    control.pull_raw=target.pull;const auto pose=function<api::Update>(api::update_export);api::Receipt r{};
    failure=3;if(!pose)return;const auto code=pose(&control,&r);
    if(r.size!=sizeof r||r.version!=2||r.handle!=control.handle||r.result!=code)return;
    if(code==api::Result::restoring||code==api::Result::restored||code==api::Result::inactive_restored){revoking=true;restore();return;}
    if(code!=api::Result::pending&&code!=api::Result::ready)return;
    if(r.angle!=control.angle||r.pull_raw!=control.pull_raw)return;
    accepted_pose={r.angle,r.pull_raw};accepted_pose_known=true;
    Observation actual{};failure=4;
    // Calibrate only with Native's actual evaluated rest pose acknowledged.
    if(!closed.known&&(control.angle!=0||pull_raw!=0||code!=api::Result::ready))return;
    if(!observe(f,actual,true))return;
    visible=true;ready=code==api::Result::ready;
    failure=ready?0:4;last_cost.prepare_ns=weaponhand::clock_ns(nullptr)-begin;
}catch(...){hide();}
}
