#include "../vr-common/weapon_boltok_observation_geometry.h"
#include "../vr-common/weapon_boltok_child_pose.h"
// Current original Boltok matrices, through the shared reflected getter and
// identity/freshness chain. This does not enable a second pose writer or use
// Snub's unrelated five animation tracks for the ObjectXX skeleton.
namespace boltok_parts {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
struct Parts {
    hand::ObjectIdentity component{},asset{};hand::FrameKey key{};
    // Source skeleton indices from core-bone-audit.json (source a1f04758...):
    // root=0, support=1, cylinder=3, support child=2, fixed rear plate=11.
    // Only consumed current root/support/cylinder matrices; no child/rear dummies.
    std::array<std::array<float,16>,3> world{};
};
gears_equipment_names::Lookup first_names({L"alow_poly",L"Object03",L"Object02"});
struct ReadCost {
    hand::Frame const& frame;weaponhand::source_validation::Scope* validation{weaponhand::source_validation::active};
    unsigned calls_before{validation?validation->validations:0};
    std::int64_t began{weaponhand::clock_ns(nullptr)};bool okay{};
    explicit ReadCost(hand::Frame const& f) noexcept:frame(f){}
    ~ReadCost() noexcept {
        static unsigned active_rows{},idle_rows{};
        const auto end=weaponhand::clock_ns(nullptr);auto& rows=frame.left_grip_down?active_rows:idle_rows;
        static unsigned late_failed_rows{};
        if(log_file&&!okay&&frame.left_grip_down&&frame.controllers.sync_ns&&
           end-frame.controllers.sync_ns>=80000000&&late_failed_rows++<3){
            auto const* proof=validation?validation->proof:nullptr;
            fwprintf(log_file,L"boltok_parts_late_validation frame=%llu sequence=%llu elapsed_ns=%lld end_age_ns=%lld scope=%u scope_frame=%llu scope_sequence=%llu matches=%u nested=%u disabled=%u sealed=%u validations=%u proof_failure=%u semantic_observer=%u\n",
                frame.key.frame,frame.controllers.sequence,end-began,end-frame.controllers.sync_ns,unsigned(validation!=nullptr),
                validation?validation->frame.key.frame:0ull,validation?validation->frame.controllers.sequence:0ull,
                unsigned(validation&&validation->matches(frame)),unsigned(validation&&validation->previous),
                unsigned(validation&&validation->disabled),unsigned(validation&&validation->sealed),
                validation?validation->validations:0,
                proof?unsigned(proof->failure):0,unsigned(weaponhand::meta::semantic_name_observer!=nullptr));fflush(log_file);
        }
        if(!log_file||rows>=8||!validation||!validation->matches(frame)||
           (!frame.left_grip_down&&okay&&end-began<12000000))return;
        ++rows;auto const* p=validation->proof;
        fwprintf(log_file,L"boltok_parts_validation frame=%llu sequence=%llu passed=%u elapsed_ns=%lld calls=%u calls_total=%u fallbacks=%u sealed=%u disabled=%u reads=%u ranges=%u bytes=%u failure=%u fail_address=%llx fail_bytes=%llu batched=%u batches=%u padding=%u span=%u grip_down=%u end_age_ns=%lld\n",
            frame.key.frame,frame.controllers.sequence,unsigned(okay),end-began,
            validation->validations-calls_before,validation->validations,validation->fallbacks,unsigned(validation->sealed),unsigned(validation->disabled),
            p?p->count:0,p?p->merged:0,p?p->used:0,p?unsigned(p->failure):0,
            static_cast<unsigned long long>(p?p->failure_address:0),static_cast<unsigned long long>(p?p->failure_bytes:0),
            p?unsigned(p->batched):0,p?p->batch_count:0,p?p->padding_bytes:0,p?p->planned_bytes:0,
            unsigned(frame.left_grip_down),end-frame.controllers.sync_ns);fflush(log_file);
    }
};
bool read(hand::Frame const& source,Parts& out) noexcept {
    out={};ReadCost cost(source);anim::Parts a{};
    // One consumed current group only. The shared helper retains full asset
    // eligibility, component/asset identity, metadata and per-part getter checks.
    if(!anim::read_part_group(source,a,lancermain_asset::HeldAsset::boltok,first_names,
           {L"alow_poly",L"Object03",L"Object02"},{0,1,3})||
       !nativeequipment::current(nullptr,source))return false;
    out.component=a.component;out.asset=a.asset;out.key=a.key;
    out.world={a.world[0],a.world[1],a.world[2]};cost.okay=true;return true;
}
struct Mechanical {
    Parts source{};
    gears_vr::Pose root_metres{};
    // Support and cylinder relative to the CURRENT root; metre units.
    // Only consumed support/cylinder poses; no unused child/rear slots.
    std::array<gears_vr::Pose,2> in_root{};
    double component_scale{},units{};
    bool original_cylinder_hidden{};
    bool native_observed{};std::int64_t observed_ns{};std::uint64_t observed_serial{};unsigned observed_epoch{};
};
struct ObservationQuery {
    gears_boltok_original::ObservationRead read{};std::uint64_t handle{},cookie{};bool hidden{};
};
enum class MeasureFailure {none,raw,observation};
bool measure(hand::Frame const& f,Mechanical& out,gears_vr::Pose const* hidden_cylinder_relative=nullptr,
    ObservationQuery const* query=nullptr,MeasureFailure* failure=nullptr) noexcept {
    out={};if(failure)*failure=MeasureFailure::raw;Mechanical value{};
    value.units=nativelancerfollow::bound_units;
    if(!std::isfinite(value.units)||value.units<10||value.units>500||!read(f,value.source))return false;
    gears_vr::Pose root{};
    if(!hammerburst_runtime::matrix(value.source.world[0],root,value.component_scale))return false;
    const auto inverse=gears_vr::inverse(root.orientation);
    gears_boltok_observed_geometry::Geometry geometry{};
    if(query){
        if(failure)*failure=MeasureFailure::observation;
        namespace api=gears_boltok_original;
        api::ObservationRequest request{};request.source=f;request.handle=query->handle;request.cookie=query->cookie;
        request.component=value.source.component;request.asset=value.source.asset;api::Observation native_observation{};
        if(!query->read||!query->read(&request,&native_observation))return false;
        const auto now=weaponhand::clock_ns(nullptr);
        if(native_observation.size!=sizeof native_observation||native_observation.version!=1||!native_observation.verified||!native_observation.published||native_observation.restoring||
           native_observation.handle!=query->handle||!native_observation.publish_serial||!native_observation.epoch||native_observation.source_epoch!=native_observation.epoch||
           native_observation.hidden!=query->hidden||native_observation.observed_ns<=0||now<native_observation.observed_ns||now-native_observation.observed_ns>100000000||
           !hand::same(native_observation.component,value.source.component)||!hand::same(native_observation.asset,value.source.asset)||
           !gears_boltok_observation::same_domain(f,native_observation.source)||f.key.frame<native_observation.source.key.frame||
           !gears_boltok_observed_geometry::convert(native_observation,value.component_scale,value.units,hidden_cylinder_relative,geometry))return false;
        value.native_observed=true;value.observed_ns=native_observation.observed_ns;
        value.observed_serial=native_observation.publish_serial;value.observed_epoch=native_observation.epoch;
        if(failure)*failure=MeasureFailure::raw;
    }
    for(unsigned i=0;i<value.in_root.size();++i){
        gears_vr::Pose part{};double scale{};
        const auto& raw=value.source.world[i+1];
        const bool ordinary=hammerburst_runtime::matrix(raw,part,scale)&&std::abs(scale-value.component_scale)<=1e-5;
        if(!ordinary){
            if(i!=1||!hidden_cylinder_relative)return false;
            for(auto v:raw)if(!std::isfinite(v))return false;
            for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)if(std::abs(raw[row*4+col])>1e-6)return false;
            if(std::abs(raw[15]-1)>1e-5)return false;
            value.original_cylinder_hidden=true;
        }
        auto& local=value.in_root[i];
        if(query&&(i==0||i==1)){
            local=i?geometry.cylinder:geometry.support;
        }else if(i==1&&hidden_cylinder_relative){
            // Native may temporarily restore the hidden child's raw CPU matrix.
            // Always use the native_observation current parent and the neutral rig relation.
            if(!gears_boltok_child_pose::current(value.in_root[0],*hidden_cylinder_relative,local))return false;
        }else{
            local.position=gears_weapon::scaled(gears_weapon::rotate(inverse,
                gears_weapon::difference(part.position,root.position)),1/value.units);
            local.orientation=gears_vr::normalized(gears_vr::multiply(inverse,part.orientation));
        }
        if(!gears_vr::stereo_pose_valid(local))return false;
    }
    value.root_metres=root;value.root_metres.position=gears_weapon::scaled(root.position,1/value.units);
    if(!nativeequipment::current(nullptr,f))return false;
    if(value.native_observed){const auto now=weaponhand::clock_ns(nullptr);
        if(now<value.observed_ns||now-value.observed_ns>100000000){if(failure)*failure=MeasureFailure::observation;return false;}}
    if(failure)*failure=MeasureFailure::none;out=value;return true;
}
}

