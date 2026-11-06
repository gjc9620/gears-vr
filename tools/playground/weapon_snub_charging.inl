// The original coupled slide/wing closing tail, driven by the actual wrist.
// Inventory, reward and firing debt belong to snub_runtime, never this pose.
namespace snub_charging {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;
using V=gears_vr::Vector;using P=gears_vr::Pose;
using Stroke=gears_weapon::SnubSlideStroke;
snub_animation::Curve curve;
Stroke stroke;
gears_weapon::SnubSlideGeometry geometry;
gears_weapon::SnubSlideCurve::Sampling sampling{};
std::uint64_t sequence{},bone_frame{};unsigned law_matches{};
double last_actual{},last_wanted{-1};bool release_seen{},issued_full{};
P root{};double root_scale{};
V contact_world{};lancerreload_inventory::Identity contact_identity{};bool contact_known{};
void reset() noexcept {
    curve={};stroke.reset();geometry={};sampling={};sequence=bone_frame=0;law_matches=0;
    last_actual=0;last_wanted=-1;release_seen=issued_full=false;contact_known=false;
}
bool parts(hand::Frame const& f,double& actual) noexcept {
    snub_animation::Parts bones{};P slide{};double size{};
    if(!snub_animation::read_parts(f,bones)||!hammerburst_runtime::matrix(bones.world[0],root,root_scale)||
       !hammerburst_runtime::matrix(bones.world[1],slide,size)||std::abs(size-root_scale)>1e-3)return false;
    const auto relative=snub_visual::mul(gears_weapon::rotate(gears_vr::inverse(root.orientation),
        snub_visual::sub(slide.position,root.position)),1/(root_scale*nativelancerfollow::bound_units));
    const double units=nativelancerfollow::bound_units;
    const V closed{7.087201118469238/units,.014453720301389694/units,10.67758846282959/units};
    if(!gears_vr::finite(relative)||std::hypot(relative.y-closed.y,relative.z-closed.z)>.001)return false;
    actual=closed.x-relative.x;
    contact_world=slide.position;contact_identity=lancerreload_inventory::identity(f);contact_known=true;
    geometry.closed_contact=closed;geometry.axis={-1,0,0};
    geometry.travel=curve.slide.travel();geometry.current_instance_verified=curve.slide.bound();
    return nativeequipment::current(nullptr,f);
}
bool hand_sample(hand::Frame const& f,gears_weapon::HammerburstLatchSample& out) noexcept {
    P wrist{};if(!snub_visual::wrist(f,wrist))return false;
    const auto local=snub_visual::mul(gears_weapon::rotate(gears_vr::inverse(root.orientation),
        snub_visual::sub(wrist.position,root.position)),1/(root_scale*nativelancerfollow::bound_units));
    out={local,f.controllers.sequence,std::uint64_t(f.controllers.sync_ns/1000000),f.press_token,
        bool(f.left_tracked),bool(f.left_grip_known&&f.left_grip_down)};return gears_vr::finite(local);
}
enum class Result {waiting,complete,recovery_required};
Result tick(hand::Frame const& f,hand::Decision const& d,std::uint64_t serial) noexcept {
    if(!curve.slide.bound()){
        if(!anim::ownership.active||!anim::ownership.stopped)return Result::waiting;
        snub_animation::Curve value{};
        if(!snub_animation::read_curve(f,anim::ownership.last,value))return Result::waiting;
        curve=value;return Result::waiting;
    }
    float now{};double actual{};
    if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",now)||!parts(f,actual))return Result::waiting;
    if(!f.left_tracked||!f.left_grip_known)return Result::recovery_required;
    const auto phase=stroke.phase();
    if(!f.left_grip_down){
        release_seen=true;
        if(phase==Stroke::Phase::pulling){
            // waiting_open may only await a full setter already dispatched.
            if(stroke.wanted_distance()>=curve.slide.travel()-.0015&&!issued_full){
                if(!stroke.cancel(now))return Result::recovery_required;
            }else if(!stroke.release(now))return Result::recovery_required;
        }
    }
    if(stroke.phase()==Stroke::Phase::idle){
        if(!release_seen||!f.left_grip_down||!snub_visual::granted(f,d)||!d.new_grant)return Result::waiting;
        gears_weapon::HammerburstLatchSample sample{};
        if(!hand_sample(f,sample)||!stroke.begin(geometry,sample,actual))return Result::waiting;
        sequence=sample.sequence;bone_frame=0;release_seen=false;issued_full=false;last_wanted=-1;
        return Result::waiting;
    }
    if(stroke.phase()==Stroke::Phase::pulling&&f.left_grip_down){
        if(!snub_visual::granted(f,d))return Result::recovery_required;
        gears_weapon::HammerburstLatchSample sample{};
        if(hand_sample(f,sample)&&sample.sequence>sequence){
            sequence=sample.sequence;
            if(!stroke.move(sample))return Result::recovery_required;
        }
    }
    if(f.key.frame>bone_frame){
        bone_frame=f.key.frame;
        double endpoint{};
        if(sampling==gears_weapon::SnubSlideCurve::Sampling::reference_keys&&
           curve.slide.distance_at_time(anim::ownership.last.time,endpoint,gears_weapon::SnubSlideCurve::Sampling::endpoint_intervals)&&
           std::abs(actual-endpoint)<.0008&&std::abs(actual-stroke.wanted_distance())>.0015&&std::abs(actual-last_actual)<.0003){
            if(++law_matches>=2){sampling=gears_weapon::SnubSlideCurve::Sampling::endpoint_intervals;last_wanted=-1;}
        }else law_matches=0;
        last_actual=actual;
        if(!stroke.acknowledge_bone(actual,f.key.frame,now))return Result::recovery_required;
    }
    if(stroke.phase()==Stroke::Phase::complete)return Result::complete;
    if(stroke.phase()==Stroke::Phase::idle){last_wanted=-1;return Result::waiting;}
    if(stroke.phase()==Stroke::Phase::returning&&!stroke.advance_return(now))return Result::recovery_required;
    const auto wanted=stroke.wanted_distance();
    if(std::abs(wanted-last_wanted)>1e-7){
        if(!nativeequipment::current(nullptr,f))return Result::waiting;
        const bool settling=stroke.phase()==Stroke::Phase::returning||stroke.phase()==Stroke::Phase::waiting_open;
        if(!snub_animation::seek_distance(f,serial,curve,wanted,sampling,settling))
            return anim::ownership.seek_pending?Result::recovery_required:Result::waiting;
        last_wanted=wanted;if(wanted>=curve.slide.travel()-.0015)issued_full=true;
    }
    return Result::waiting;
}
} // namespace snub_charging
