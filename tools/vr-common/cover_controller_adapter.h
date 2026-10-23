#pragma once
#include "controller_input_frame.h"
#include "cover_interaction.h"

namespace gears_vr {

// The shared session is an identity, not a monotonic generation. The native
// owner maps its accepted stereo session/space to these kernel generations.
struct CoverControllerReference { std::uint64_t session_epoch=0,space_epoch=0; };
enum class CoverControllerStatus {
    accepted,baseline,needs_release,unavailable,mismatch,stale,untracked,invalid,replayed,ambiguous
};
struct CoverControllerResult {
    CoverHandSample hand;
    CoverControllerStatus status=CoverControllerStatus::unavailable;
    std::uint64_t continuity_generation=0;
};

// Pure adapter for the byte-identical controller v3 interface pinned at
// da775ed7d12118910b140c156b9750a0c678a9bd. It consumes the EXACT stereo-selected
// frame; pass nullptr when read_matching fails, never substitute newer hands.
//
// Feed EVERY result.hand to CoverInteraction, including inactive results: they
// release its ownership and force rearm while retaining its legal virtual
// offset. Input/profile changes always emit an inactive sample first. Reflect
// continuity_generation in the owner's reference generation when rebuilding
// axes, or preserve that mandatory inactive update; never silently retain a
// grip across it. The owner still certifies native cover geometry, maps the
// raw tracking axes, rebuilds references and arbitrates left-hand request IDs.
//
// Hand position is current raw grip.position in metres. No head transform,
// recenter, body offset, pose filter, unit conversion or physical HMD motion is
// applied here. Compose physical HMD motion once in the later camera path.
class CoverControllerAdapter {
public:
    CoverControllerResult update(ControllerInputFrame const* input,StereoSample const& stereo,
                                CoverControllerReference reference,std::int64_t now_ns,bool allow_sim=false){
        if(now_ns<=0||now_ns<last_now_ns_)return fail(CoverControllerStatus::stale,stereo,reference);
        last_now_ns_=now_ns;
        if(!reference.session_epoch||!reference.space_epoch)return fail(CoverControllerStatus::invalid,stereo,reference);
        if(!input)return fail(CoverControllerStatus::unavailable,stereo,reference);
        if(!controller_frame_matches(*input,stereo,now_ns,allow_sim))
            return fail(CoverControllerStatus::mismatch,stereo,reference);
        auto const& frame=*input;
        if(stereo.received_ns<=0||stereo.received_ns>now_ns||now_ns-stereo.received_ns>100000000||
           now_ns-frame.sync_ns>100000000)return fail(CoverControllerStatus::stale,stereo,reference);
        if(!stereo_render_active(stereo)||!stereo_tracking_valid(stereo,allow_sim))
            return fail(CoverControllerStatus::untracked,stereo,reference);
        if(!stereo_geometry_valid(stereo))return fail(CoverControllerStatus::invalid,stereo,reference);

        const bool initial=!last_;
        const bool new_session=last_&&(frame.session!=last_->session||frame.source!=last_->source);
        const bool new_space=last_&&frame.space_epoch!=last_->space_epoch;
        const bool new_reference=last_&&!same(reference,last_reference_);
        const bool boundary=initial||new_session||new_space||new_reference||
            frame.profile_generation!=last_->profile_generation||frame.input_epoch!=last_->input_epoch;
        if(last_){
            if(reference.session_epoch<last_reference_.session_epoch||reference.space_epoch<last_reference_.space_epoch||
               (new_session&&reference.session_epoch<=last_reference_.session_epoch)||
               (!new_session&&new_space&&reference.space_epoch<=last_reference_.space_epoch))
                return fail(CoverControllerStatus::replayed,stereo,reference);
            if(!new_session){
                if(frame.sequence<last_->sequence||frame.received_ns<last_->received_ns||
                   stereo.received_ns<last_stereo_.received_ns||
                   frame.sync_ns<last_->sync_ns||frame.predicted_time<last_->predicted_time||
                   frame.space_epoch<last_->space_epoch||frame.profile_generation<last_->profile_generation||
                   frame.input_epoch<last_->input_epoch)
                    return fail(CoverControllerStatus::replayed,stereo,reference);
                if(frame.sequence==last_->sequence){
                    if(!same(frame,*last_)||!stereo_same(stereo,last_stereo_))
                        return fail(CoverControllerStatus::replayed,stereo,reference);
                    if(!new_reference&&!needs_baseline_&&cached_)return *cached_;
                }
            }
        }
        const auto old_pressed=last_?last_->hands[0].pressed[control_squeeze]:0;
        const auto old_released=last_?last_->hands[0].released[control_squeeze]:0;
        const auto old_sequence=last_?last_->sequence:0;
        const bool was_outage=outage_;
        last_=frame;last_stereo_=stereo;last_reference_=reference;
        auto const& hand=frame.hands[0];
        if(!controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)||hand.squeeze_active!=1)
            return fail(CoverControllerStatus::untracked,stereo,reference);

        if(boundary||needs_baseline_){
            if(!generation_||(!needs_baseline_&&!initial))advance_generation();
            known_=false;down_=true;release_required_=true;needs_baseline_=false;outage_=false;
            zero_neutral_allowed_=hand.squeeze==0.f;
            // Pinned producer: active value exactly zero is released in BOTH
            // hysteresis branches. This is an exact neutral witness, not a
            // second analog threshold. Held/band baselines remain unarmed.
            if(initial&&!was_outage&&hand.squeeze==0.f){
                known_=true;down_=false;release_required_=false;
                return save(frame,reference,CoverControllerStatus::baseline,true);
            }
            return save(frame,reference,CoverControllerStatus::baseline,false);
        }
        const auto pressed=hand.pressed[control_squeeze],released=hand.released[control_squeeze];
        if(pressed<old_pressed||released<old_released||pressed-old_pressed>1||released-old_released>1||
           (pressed!=old_pressed&&released!=old_released))return ambiguous(frame,reference);
        const bool press=pressed!=old_pressed,release=released!=old_released;
        if((press&&known_&&down_)||(release&&known_&&!down_))return ambiguous(frame,reference);
        if(press){
            if(hand.last_squeeze_press_sequence<=old_sequence||
               !controller_pose_usable(hand.last_squeeze_press_grip,hand.last_squeeze_press_flags,1))
                return ambiguous(frame,reference);
            known_=true;down_=true;
        }else if(release){
            known_=true;down_=false;release_required_=false;
        }else if(!known_&&zero_neutral_allowed_&&hand.squeeze==0.f){
            known_=true;down_=false;release_required_=false;
        }
        if(!known_||release_required_)return save(frame,reference,CoverControllerStatus::needs_release,false);
        return save(frame,reference,CoverControllerStatus::accepted,true);
    }

private:
    static bool same(CoverControllerReference a,CoverControllerReference b){
        return a.session_epoch==b.session_epoch&&a.space_epoch==b.space_epoch;
    }
    template<class T> static bool number_same(T a,T b){return std::memcmp(&a,&b,sizeof(T))==0;}
    static bool same(Pose const& a,Pose const& b){
        return number_same(a.position.x,b.position.x)&&number_same(a.position.y,b.position.y)&&
            number_same(a.position.z,b.position.z)&&number_same(a.orientation.x,b.orientation.x)&&
            number_same(a.orientation.y,b.orientation.y)&&number_same(a.orientation.z,b.orientation.z)&&
            number_same(a.orientation.w,b.orientation.w);
    }
    static bool same(ControllerHand const& a,ControllerHand const& b){
        if(!same(a.grip,b.grip)||!same(a.aim,b.aim)||a.grip_flags!=b.grip_flags||a.aim_flags!=b.aim_flags||
           a.grip_active!=b.grip_active||a.aim_active!=b.aim_active||a.buttons!=b.buttons||
           a.buttons_active!=b.buttons_active||!number_same(a.trigger,b.trigger)||!number_same(a.squeeze,b.squeeze)||
           !number_same(a.stick_x,b.stick_x)||!number_same(a.stick_y,b.stick_y)||
           a.trigger_active!=b.trigger_active||a.squeeze_active!=b.squeeze_active||a.stick_active!=b.stick_active||
           !same(a.last_squeeze_press_grip,b.last_squeeze_press_grip)||
           a.last_squeeze_press_flags!=b.last_squeeze_press_flags||a.last_squeeze_press_sequence!=b.last_squeeze_press_sequence)
            return false;
        for(unsigned i=0;i<control_count;++i)if(a.pressed[i]!=b.pressed[i]||a.released[i]!=b.released[i])return false;
        return true;
    }
    static bool same(ControllerInputFrame const& a,ControllerInputFrame const& b){
        return a.source==b.source&&a.session==b.session&&a.sequence==b.sequence&&a.space_epoch==b.space_epoch&&
            a.profile_generation==b.profile_generation&&a.input_epoch==b.input_epoch&&a.predicted_time==b.predicted_time&&
            a.received_ns==b.received_ns&&a.sync_ns==b.sync_ns&&a.focused==b.focused&&
            same(a.hands[0],b.hands[0])&&same(a.hands[1],b.hands[1]);
    }
    static CoverHandSample inactive(StereoSample const& stereo,CoverControllerReference reference){
        CoverHandSample sample{};sample.sequence=stereo.sequence;sample.observed_ns=stereo.received_ns;
        sample.session_epoch=reference.session_epoch;sample.space_epoch=reference.space_epoch;
        return sample;
    }
    void advance_generation(){
        if(generation_!=(std::numeric_limits<std::uint64_t>::max)())++generation_;
    }
    CoverControllerResult fail(CoverControllerStatus status,StereoSample const& stereo,CoverControllerReference reference){
        if(!outage_)advance_generation();
        outage_=true;needs_baseline_=true;known_=false;down_=true;release_required_=true;
        zero_neutral_allowed_=false;cached_.reset();
        return {inactive(stereo,reference),status,generation_};
    }
    CoverControllerResult save(ControllerInputFrame const& frame,CoverControllerReference reference,
                               CoverControllerStatus status,bool usable){
        CoverHandSample sample{};sample.sequence=frame.sequence;sample.observed_ns=frame.received_ns;
        sample.session_epoch=reference.session_epoch;sample.space_epoch=reference.space_epoch;
        if(usable){
            sample.left_position_metres=frame.hands[0].grip.position;
            sample.input_active=true;sample.position_valid=true;sample.position_tracked=true;sample.lgrip_down=down_;
        }
        cached_=CoverControllerResult{sample,status,generation_};return *cached_;
    }
    CoverControllerResult ambiguous(ControllerInputFrame const& frame,CoverControllerReference reference){
        advance_generation();known_=false;down_=true;release_required_=true;
        zero_neutral_allowed_=frame.hands[0].squeeze==0.f;
        return save(frame,reference,CoverControllerStatus::ambiguous,false);
    }
    std::int64_t last_now_ns_=0;
    std::uint64_t generation_=0;
    bool outage_=false,needs_baseline_=false,known_=false,down_=true,release_required_=true;
    bool zero_neutral_allowed_=false;
    CoverControllerReference last_reference_;
    StereoSample last_stereo_;
    std::optional<ControllerInputFrame> last_;
    std::optional<CoverControllerResult> cached_;
};
} // namespace gears_vr
