#include <windows.h>
#include "../vr-common/weapon_hand_client.h"
#include "weapon_main_mount_source.h"
#define main weapon_equipment_main_mount_owned_fixture
#define now fixture_now
#include "weapon_rig_binding_test.cpp"
#undef now
#undef main

// Actual main-mount .inl orchestration with owned seams. This is deliberately
// not an asset reader, real Client, Native registration or engine-call test.
namespace {
namespace policy=gears_weapon_main_mount;
namespace source=gears_weapon_main_mount_source;
FILE* log_file{};
enum class Injection {none,copy_fail,soft_after_copy,source_fail,changed_game,
    attachment_fail,asset_fail,asset_changed,component_changed,generation_at_attach,
    generation_at_final_asset,soft_at_final_asset,age_at_final_asset,age_at_final_bind,
    game_at_core,soft_at_core};
Injection injection{};unsigned game_field{};
wh::Frame committed{};wh::GripGameState observed{};
policy::Candidate* active_output{};
std::int64_t clock_value=fixture_now;
unsigned dual_copies{},single_copies{},source_reads{},asset_reads{},attachment_reads{},clock_reads{};
unsigned core_attachment_reads{},standalone_attachment_reads{};
bool on_gt=true,domain=true,armed=true,registration=true;
std::uint64_t fixture_generation=9;
namespace weaponhand {
std::uint32_t current_gt(void*) noexcept {return on_gt?GetCurrentThreadId():0;}
struct Client {
    bool armed() const noexcept {return ::armed;}
    bool registration_alive(std::uint64_t cookie,std::uint64_t started) const noexcept {
        return registration&&cookie==committed.key.registration_cookie&&started==committed.key.native_started_qpc;
    }
    bool copy_frame_on_gt(wh::Frame& out,wh::GripGameState& game) noexcept {
        ++dual_copies;out={};game={};
        if(injection==Injection::copy_fail)return false;
        out=committed;game=observed;
        if(injection==Injection::soft_after_copy)::armed=false;
        return true;
    }
    bool copy_frame_on_gt(wh::Frame& out) noexcept {++single_copies;wh::GripGameState ignored{};return copy_frame_on_gt(out,ignored);}
    bool source_game_on_gt(wh::FrameKey const& key,wh::GripGameState& game) noexcept {
        ++source_reads;game={};
        if(!::armed||!registration||!wh::same(key,committed.key)||injection==Injection::source_fail)return false;
        game=observed;
        if(injection==Injection::changed_game){
            switch(game_field){case 0:game.busy=true;break;case 1:game.animation_owns_hands=true;break;
            case 2:game.main_hand_held=false;break;case 3:++game.mount_revision;break;
            case 4:game.request_ammo=true;break;case 5:game.request_magazine=true;break;
            case 6:game.kind=wh::WeaponKind::hand_thrown;break;
            case 7:++game.object_incumbents[0].id;break;}
        }
        return true;
    }
} client;
}
namespace vrequip {
struct Model {
    std::uint64_t hold_generation() const noexcept {return fixture_generation;}
    bool switch_pending() const noexcept {return false;}
    bool animation_owns_hands() const noexcept {return observed.animation_owns_hands;}
} model;
}
namespace nativeequipment {
bool domain_ready() noexcept {return domain;}
std::int64_t clock_ns(void*) noexcept {
    ++clock_reads;
    if(injection==Injection::age_at_final_bind&&active_output&&active_output->right.known)clock_value=fixture_now+100000001;
    return clock_value;
}
bool attachment_result(wh::Frame const& frame,std::uintptr_t expected) noexcept {
    ++attachment_reads;CHECK(wh::same(frame.key,committed.key)&&expected==0x7000);
    if(injection==Injection::generation_at_attach)++fixture_generation;
    return injection!=Injection::attachment_fail;
}
bool read_attached_frame(wh::Frame const& frame,std::uintptr_t expected) noexcept {
    ++core_attachment_reads;
    if(injection==Injection::game_at_core)observed.busy=true;
    if(injection==Injection::soft_at_core)::armed=false;
    return attachment_result(frame,expected);
}
bool attached_frame(wh::Frame const& frame,std::uintptr_t expected) noexcept {
    ++standalone_attachment_reads;
    return attachment_result(frame,expected);
}
}
namespace lancermain_asset {
enum class Status {ready,invalid_identity,unsupported_weapon};
struct Evidence {wh::ObjectIdentity component{},asset{};};
Status read(wh::Frame const& frame,Evidence& out) noexcept {
    ++asset_reads;CHECK(wh::same(frame.key,committed.key));out={};
    if(injection==Injection::asset_fail)return Status::invalid_identity;
    out.component=object(0x7000,8);out.asset=object(0x7100,9);
    if(asset_reads==2){
        if(injection==Injection::asset_changed)++out.asset.name_number;
        if(injection==Injection::component_changed)++out.component.name_number;
        if(injection==Injection::generation_at_final_asset)++fixture_generation;
        if(injection==Injection::soft_at_final_asset)::armed=false;
        if(injection==Injection::age_at_final_asset)clock_value=fixture_now+100000001;
    }
    return Status::ready;
}
}
#include "weapon_equipment_main_mount.inl"

bool cleared(policy::Candidate const& candidate){
    return !candidate.right.known&&!candidate.key.frame&&!candidate.hold_generation&&
        !candidate.provider_revision&&!candidate.world.address&&!candidate.weapon.address&&!candidate.asset.address;
}
struct Trial {
    Fixture fixture;source::BodyContext context{};policy::BodyBinding body{};policy::Candidate output{};
    nativeequipment_main_mount::RetainedDiagnostic diagnostic{};
    explicit Trial(unsigned selected=0):fixture(selected){
        injection=Injection::none;game_field=0;clock_value=fixture_now;fixture_generation=9;
        dual_copies=single_copies=source_reads=asset_reads=attachment_reads=clock_reads=0;
        core_attachment_reads=standalone_attachment_reads=0;
        on_gt=domain=::armed=registration=true;
        committed=fixture.hand;observed={};observed.known=observed.main_hand_held=true;observed.kind=wh::WeaponKind::firearm;
        const auto& r=fixture.rig;context.known=true;context.native=r.native;
        context.publication_sequence=r.publication_sequence;context.scene_invocation=r.scene_invocation;
        context.world_generation=r.world_generation;context.binding_generation=r.binding_generation;
        context.reference_generation=r.reference_generation;context.mesh=r.mesh;
        context.geometry=gears_hand_authored_mount::marcus115_signature;context.model_scale=1;context.units_per_metre=r.units_per_metre;
        context.revision=r.binding_generation;context.basis_revision=2;context.basis=policy::Basis::marcus_mesh_face_forward_y;
        er::Status agreement{};CHECK(source::bind(committed,r,context,fixture_now,selected,body,agreement)==source::SourceStatus::ready);
        active_output=&output;
    }
    ~Trial(){active_output=nullptr;}
    policy::Status retained(){return nativeequipment_main_mount::prepare_retained(fixture.rig,context,output,diagnostic);}
    policy::Status preview(){nativeequipment_main_mount::Diagnostic d{};
        return nativeequipment_main_mount::prepare(committed,body,output,d);}
};
}
int main(){
    for(unsigned selected=0;selected<2;++selected){Trial t(selected);
        CHECK(t.retained()==policy::Status::ready&&t.output.right.known);
        CHECK(dual_copies==1&&single_copies==0&&source_reads==1&&asset_reads==2&&attachment_reads==1);
        CHECK(core_attachment_reads==1&&standalone_attachment_reads==0);
        CHECK(wh::same(t.output.key,committed.key)&&t.output.hold_generation==9&&t.output.body_revision==t.context.revision);
        CHECK(t.diagnostic.mount.observed_ns==fixture_now&&t.diagnostic.mount.elapsed_ns==0);
        CHECK(!observed.mount_known);}
    {Trial t;CHECK(t.preview()==policy::Status::ready&&t.output.right.known);
        CHECK(dual_copies==0&&single_copies==0&&source_reads==2&&asset_reads==2&&attachment_reads==1);
        CHECK(standalone_attachment_reads==1&&core_attachment_reads==0);}
    for(auto fault:{Injection::copy_fail,Injection::soft_after_copy}){Trial t;t.output.right.known=true;
        injection=fault;CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));
        CHECK(dual_copies==1&&source_reads==0&&asset_reads==0&&attachment_reads==0);
        if(fault==Injection::soft_after_copy)CHECK(weaponhand::client.registration_alive(committed.key.registration_cookie,committed.key.native_started_qpc));}
    for(unsigned field=0;field<8;++field){Trial t;injection=Injection::changed_game;game_field=field;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));CHECK(source_reads==1&&asset_reads==1&&attachment_reads==1);}
    for(auto fault:{Injection::source_fail,Injection::generation_at_attach,Injection::generation_at_final_asset,
        Injection::soft_at_final_asset,Injection::age_at_final_asset}){Trial t;injection=fault;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));CHECK(source_reads==1);}
    {Trial t;injection=Injection::game_at_core;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));
        CHECK(core_attachment_reads==1&&standalone_attachment_reads==0&&source_reads==1&&asset_reads==1);}
    {Trial t;injection=Injection::generation_at_attach;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));
        CHECK(core_attachment_reads==1&&standalone_attachment_reads==0&&source_reads==1&&asset_reads==1);}
    {Trial t;injection=Injection::soft_at_core;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::not_attached&&cleared(t.output));
        CHECK(core_attachment_reads==1&&standalone_attachment_reads==0&&source_reads==0&&asset_reads==1);
        CHECK(weaponhand::client.registration_alive(committed.key.registration_cookie,committed.key.native_started_qpc));}
    {Trial t;injection=Injection::attachment_fail;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::not_attached&&cleared(t.output)&&source_reads==0&&asset_reads==1);}
    for(auto fault:{Injection::asset_fail,Injection::asset_changed,Injection::component_changed}){Trial t;injection=fault;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::unsupported_asset&&cleared(t.output));}
    {Trial t;injection=Injection::age_at_final_bind;t.output.right.known=false;
        CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));
        CHECK(source_reads==1&&asset_reads==2&&t.diagnostic.binding==source::SourceStatus::source_mismatch);}
    for(unsigned guard=0;guard<4;++guard){Trial t;
        if(guard==0)on_gt=false;if(guard==1)domain=false;if(guard==2)::armed=false;if(guard==3)registration=false;
        t.output.right.known=true;CHECK(t.retained()==policy::Status::inactive&&cleared(t.output)&&asset_reads==0);}
    {Trial t;++t.fixture.rig.scene_invocation;t.output.right.known=true;
        CHECK(t.retained()==policy::Status::inactive&&cleared(t.output)&&source_reads==0&&asset_reads==0);}
    {Trial t;CHECK(t.retained()==policy::Status::ready);const auto original=committed;
        injection=Injection::copy_fail;CHECK(t.retained()==policy::Status::inactive&&cleared(t.output));
        CHECK(wh::same(original.key,committed.key));} // No previous candidate survives a failed fresh query.
    std::printf("WeaponEquipmentMainMountTest: %u checks passed; actual .inl with owned host seams, no engine/Native acceptance\n",checks);return 0;
}
