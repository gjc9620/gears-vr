#include "weapon_main_mount_source.h"
// Reuse the owned synthetic hand/Rig fixture without copying its payload.
#define main weapon_rig_binding_inherited_tests
#include "weapon_rig_binding_test.cpp"
#undef main
namespace {
namespace source=gears_weapon_main_mount_source;
namespace policy=gears_weapon_main_mount;
source::BodyContext body_context(Fixture const& f){
    source::BodyContext c{};c.known=true;c.native=f.rig.native;
    c.publication_sequence=f.rig.publication_sequence;c.scene_invocation=f.rig.scene_invocation;
    c.world_generation=f.rig.world_generation;c.binding_generation=f.rig.binding_generation;
    c.reference_generation=f.rig.reference_generation;c.mesh=f.rig.mesh;
    c.geometry=gears_hand_authored_mount::marcus115_signature;c.model_scale=1.7;c.units_per_metre=f.rig.units_per_metre;
    c.revision=7;c.basis_revision=policy::supported_basis_revision;c.basis=policy::Basis::marcus_mesh_face_forward_y;return c;
}
bool empty(policy::BodyBinding const& b){return !b.known&&wh::same(b.key,wh::FrameKey{})&&
    wh::same(b.mesh,wh::ObjectIdentity{})&&b.geometry==source::policy::lancer::Signature{}&&
    b.model_scale==0&&b.units_per_metre==0&&!b.revision&&!b.basis_revision&&b.basis==policy::Basis::unknown;}
template<class Change>void body_rejection(Change change,source::SourceStatus expected){
    Fixture f;auto c=body_context(f);policy::BodyBinding out{};er::Status agreement{};
    CHECK(source::bind(f.hand,f.rig,c,now,0,out,agreement)==source::SourceStatus::ready&&out.known);
    change(c);CHECK(source::bind(f.hand,f.rig,c,now,0,out,agreement)==expected);
    CHECK(agreement==er::Status::matched&&empty(out));
}
template<class Change>void source_rejection(Change change,er::Status expected){
    Fixture f;const auto c=body_context(f);policy::BodyBinding out{};er::Status agreement{};
    CHECK(source::bind(f.hand,f.rig,c,now,0,out,agreement)==source::SourceStatus::ready);
    change(f);CHECK(source::bind(f.hand,f.rig,c,now,0,out,agreement)==source::SourceStatus::source_mismatch);
    CHECK(agreement==expected&&empty(out));
}
static_assert(noexcept(source::bind(wh::Frame{},rt::Frame{},source::BodyContext{},now,0,
    std::declval<policy::BodyBinding&>(),std::declval<er::Status&>())));
}
int main(){
    for(unsigned selected=0;selected<2;++selected){Fixture f(selected);auto c=body_context(f);
        policy::BodyBinding body{};er::Status agreement{};
        CHECK(source::bind(f.hand,f.rig,c,now,selected,body,agreement)==source::SourceStatus::ready);
        CHECK(agreement==er::Status::matched&&body.known&&wh::same(body.key,f.hand.key));
        CHECK(wh::same(body.mesh,c.mesh)&&body.geometry==c.geometry&&body.model_scale==c.model_scale&&
            body.units_per_metre==c.units_per_metre&&body.revision==c.revision&&body.basis==c.basis&&body.basis_revision==c.basis_revision);
        wh::GripGameState game{};game.known=game.main_hand_held=true;game.kind=wh::WeaponKind::firearm;
        policy::AssetBinding asset{};asset.certified=asset.right_attached=true;asset.key=f.hand.key;asset.weapon=f.hand.weapon;
        asset.component=object(0x1800,8);asset.asset=object(0x1900,9);policy::Candidate candidate{};
        CHECK(policy::prepare(f.hand,game,3,asset,body,now,candidate)==policy::Status::ready&&candidate.right.known);
        CHECK(wh::same(candidate.key,f.hand.key)&&candidate.geometry==c.geometry&&!game.mount_known);
        asset.certified=false;CHECK(policy::prepare(f.hand,game,3,asset,body,now,candidate)==policy::Status::unsupported_asset&&!candidate.right.known);
        CHECK(source::bind(f.hand,f.rig,c,now+rt::maximum_age_ns,selected,body,agreement)==source::SourceStatus::source_mismatch&&empty(body));
    }
    source_rejection([](auto& f){++f.rig.scene_invocation;},er::Status::scene_mismatch); // Same original GVC sample.
    source_rejection([](auto& f){++f.hand.key.registration_cookie;},er::Status::native_mismatch);
    source_rejection([](auto& f){++f.hand.mesh.name_number;},er::Status::objects_mismatch);
    source_rejection([](auto& f){f.rig.published_ns=now-100000001;},er::Status::invalid_rig);
    source_rejection([](auto& f){f.hand.stereo.source=gears_vr::StereoSource::MetaOperatorSimulated;},er::Status::source_mismatch);
    body_rejection([](auto& c){c.known=false;},source::SourceStatus::body_unavailable);
    body_rejection([](auto& c){c.revision=0;},source::SourceStatus::body_unavailable);
    for(unsigned field=0;field<5;++field)body_rejection([field](auto& c){
        if(field==0)++c.native.game_pid;if(field==1)++c.native.game_thread;if(field==2)++c.native.game_start;
        if(field==3)++c.native.native_started_qpc;if(field==4)++c.native.registration_cookie;},source::SourceStatus::body_mismatch);
    for(unsigned field=0;field<5;++field)body_rejection([field](auto& c){
        if(field==0)++c.publication_sequence;if(field==1)++c.scene_invocation;if(field==2)++c.world_generation;
        if(field==3)++c.binding_generation;if(field==4)++c.reference_generation;},source::SourceStatus::body_mismatch);
    for(unsigned field=0;field<7;++field)body_rejection([field](auto& c){
        if(field==0)c.mesh.address+=8;if(field==1)c.mesh.outer+=8;if(field==2)c.mesh.klass+=8;
        if(field==3)++c.mesh.registry_index;if(field==4)++c.mesh.name_index;if(field==5)++c.mesh.name_number;
        if(field==6)c.mesh.known=0;},source::SourceStatus::body_mismatch);
    body_rejection([](auto& c){c.basis=policy::Basis::unknown;},source::SourceStatus::body_mismatch);
    body_rejection([](auto& c){c.basis_revision=0;},source::SourceStatus::body_mismatch);
    body_rejection([](auto& c){c.basis_revision=1;},source::SourceStatus::body_mismatch);
    body_rejection([](auto& c){c.geometry[0]^=1;},source::SourceStatus::unsupported_geometry);
    for(double value:{0.,-1.,(std::numeric_limits<double>::quiet_NaN)(),(std::numeric_limits<double>::infinity)()})
        body_rejection([value](auto& c){c.model_scale=value;},source::SourceStatus::unsupported_geometry);
    for(double value:{0.,9.,101.,501.,(std::numeric_limits<double>::quiet_NaN)(),(std::numeric_limits<double>::infinity)()})
        body_rejection([value](auto& c){c.units_per_metre=value;},source::SourceStatus::body_mismatch);
    std::printf("WeaponMainMountSourceTest: %u checks passed; synthetic data only, no Native certification or writer\n",checks);return 0;
}
