#pragma once
#include <openxr/openxr.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "../vr-common/controller_input_frame.h"
#include "../vr-common/pose_bridge.h"

namespace gears_controller_xr {
class Actions {
    XrInstance instance_{XR_NULL_HANDLE};
    XrSession session_{XR_NULL_HANDLE};
    XrActionSet set_{XR_NULL_HANDLE};
    std::array<XrPath,2> hands_{};
    XrAction grip_{XR_NULL_HANDLE},aim_{XR_NULL_HANDLE},trigger_{XR_NULL_HANDLE},squeeze_{XR_NULL_HANDLE};
    XrAction stick_x_{XR_NULL_HANDLE},stick_y_{XR_NULL_HANDLE},stick_click_{XR_NULL_HANDLE};
    std::array<XrAction,4> face_{}; // A B X Y
    std::array<XrSpace,2> grip_spaces_{},aim_spaces_{};
    std::uint64_t profile_generation_{1};
    gears_vr::ControllerEdgeCounter edges_;
    static void check(XrResult result,const char* operation) {
        if(XR_FAILED(result))throw std::runtime_error(std::string(operation)+" failed: "+std::to_string(result));
    }
    XrPath path(const char* value) const {
        XrPath result=XR_NULL_PATH;check(xrStringToPath(instance_,value,&result),"xrStringToPath");return result;
    }
    XrAction action(const char* name,const char* label,XrActionType type,
                    XrPath const* subactions,std::uint32_t count) const {
        XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
        strcpy_s(info.actionName,sizeof(info.actionName),name);
        strcpy_s(info.localizedActionName,sizeof(info.localizedActionName),label);
        info.actionType=type;info.countSubactionPaths=count;info.subactionPaths=subactions;
        XrAction result=XR_NULL_HANDLE;check(xrCreateAction(set_,&info,&result),"xrCreateAction");return result;
    }
    XrSpace action_space(XrAction action_handle,XrPath hand) const {
        XrActionSpaceCreateInfo info{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        info.action=action_handle;info.subactionPath=hand;info.poseInActionSpace.orientation.w=1;
        XrSpace space=XR_NULL_HANDLE;check(xrCreateActionSpace(session_,&info,&space),"xrCreateActionSpace");return space;
    }
    XrPath component(unsigned hand,const char* suffix) const {
        std::string full=hand==0?"/user/hand/left":"/user/hand/right";
        full+=suffix;return path(full.c_str());
    }
    bool suggest_touch() const {
        std::vector<XrActionSuggestedBinding> bindings;
        auto add=[&](XrAction action_handle,unsigned hand,const char* suffix){
            bindings.push_back({action_handle,component(hand,suffix)});
        };
        for(unsigned hand=0;hand<2;++hand){
            add(grip_,hand,"/input/grip/pose");add(aim_,hand,"/input/aim/pose");
            add(trigger_,hand,"/input/trigger/value");add(squeeze_,hand,"/input/squeeze/value");
            add(stick_x_,hand,"/input/thumbstick/x");add(stick_y_,hand,"/input/thumbstick/y");
            add(stick_click_,hand,"/input/thumbstick/click");
        }
        add(face_[0],1,"/input/a/click");add(face_[1],1,"/input/b/click");
        add(face_[2],0,"/input/x/click");add(face_[3],0,"/input/y/click");
        XrInteractionProfileSuggestedBinding suggestion{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestion.interactionProfile=path("/interaction_profiles/oculus/touch_controller");
        suggestion.countSuggestedBindings=static_cast<std::uint32_t>(bindings.size());
        suggestion.suggestedBindings=bindings.data();
        const auto result=xrSuggestInteractionProfileBindings(instance_,&suggestion);
        if(result==XR_ERROR_PATH_UNSUPPORTED)return false;
        check(result,"xrSuggestInteractionProfileBindings(touch)");return true;
    }
    bool suggest_simple() const {
        std::array<XrActionSuggestedBinding,4> bindings{{
            {grip_,component(0,"/input/grip/pose")},{aim_,component(0,"/input/aim/pose")},
            {grip_,component(1,"/input/grip/pose")},{aim_,component(1,"/input/aim/pose")}
        }};
        XrInteractionProfileSuggestedBinding suggestion{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestion.interactionProfile=path("/interaction_profiles/khr/simple_controller");
        suggestion.countSuggestedBindings=static_cast<std::uint32_t>(bindings.size());
        suggestion.suggestedBindings=bindings.data();
        const auto result=xrSuggestInteractionProfileBindings(instance_,&suggestion);
        if(result==XR_ERROR_PATH_UNSUPPORTED)return false;
        check(result,"xrSuggestInteractionProfileBindings(simple)");return true;
    }
    XrActionStatePose pose_state(XrAction action_handle,XrPath hand) const {
        XrActionStateGetInfo query{XR_TYPE_ACTION_STATE_GET_INFO};query.action=action_handle;query.subactionPath=hand;
        XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
        check(xrGetActionStatePose(session_,&query,&state),"xrGetActionStatePose");return state;
    }
    XrActionStateFloat float_state(XrAction action_handle,XrPath hand) const {
        XrActionStateGetInfo query{XR_TYPE_ACTION_STATE_GET_INFO};query.action=action_handle;query.subactionPath=hand;
        XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
        check(xrGetActionStateFloat(session_,&query,&state),"xrGetActionStateFloat");return state;
    }
    XrActionStateBoolean bool_state(XrAction action_handle,XrPath hand) const {
        XrActionStateGetInfo query{XR_TYPE_ACTION_STATE_GET_INFO};query.action=action_handle;query.subactionPath=hand;
        XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
        check(xrGetActionStateBoolean(session_,&query,&state),"xrGetActionStateBoolean");return state;
    }
    void locate(XrSpace action_space_handle,XrSpace local,XrTime at,std::uint32_t active,
                gears_vr::Pose& pose,std::uint64_t& flags) const {
        if(!active)return;
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        check(xrLocateSpace(action_space_handle,local,at,&location),"xrLocateSpace(controller)");
        flags=location.locationFlags;
        if((flags&3)==3){const auto& p=location.pose;
            pose={{p.position.x,p.position.y,p.position.z},
                  {p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}};}
    }
public:
    Actions()=default;Actions(Actions const&)=delete;Actions& operator=(Actions const&)=delete;
    ~Actions(){close();}
    void initialize(XrInstance instance,XrSession session) {
        if(set_||!instance||!session)throw std::runtime_error("Controller actions already initialized or session missing");
        instance_=instance;session_=session;
        try {
            hands_[0]=path("/user/hand/left");hands_[1]=path("/user/hand/right");
            XrActionSetCreateInfo set_info{XR_TYPE_ACTION_SET_CREATE_INFO};
            strcpy_s(set_info.actionSetName,sizeof(set_info.actionSetName),"gears_weapon_hands");
            strcpy_s(set_info.localizedActionSetName,sizeof(set_info.localizedActionSetName),"Gears VR Weapon Hands");
            check(xrCreateActionSet(instance_,&set_info,&set_),"xrCreateActionSet");
            const auto* both=hands_.data();
            grip_=action("weapon_grip_pose","Weapon Grip Pose",XR_ACTION_TYPE_POSE_INPUT,both,2);
            aim_=action("weapon_aim_pose","Weapon Aim Pose",XR_ACTION_TYPE_POSE_INPUT,both,2);
            trigger_=action("weapon_trigger","Weapon Trigger",XR_ACTION_TYPE_FLOAT_INPUT,both,2);
            squeeze_=action("weapon_squeeze","Weapon Squeeze",XR_ACTION_TYPE_FLOAT_INPUT,both,2);
            stick_x_=action("weapon_stick_x","Weapon Stick X",XR_ACTION_TYPE_FLOAT_INPUT,both,2);
            stick_y_=action("weapon_stick_y","Weapon Stick Y",XR_ACTION_TYPE_FLOAT_INPUT,both,2);
            stick_click_=action("weapon_stick_click","Weapon Stick Click",XR_ACTION_TYPE_BOOLEAN_INPUT,both,2);
            face_[0]=action("weapon_a","Weapon A",XR_ACTION_TYPE_BOOLEAN_INPUT,&hands_[1],1);
            face_[1]=action("weapon_b","Weapon B",XR_ACTION_TYPE_BOOLEAN_INPUT,&hands_[1],1);
            face_[2]=action("weapon_x","Weapon X",XR_ACTION_TYPE_BOOLEAN_INPUT,&hands_[0],1);
            face_[3]=action("weapon_y","Weapon Y",XR_ACTION_TYPE_BOOLEAN_INPUT,&hands_[0],1);
            const bool touch=suggest_touch(),simple=suggest_simple();
            if(!touch&&!simple)throw std::runtime_error("No supported controller interaction profile");
            XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
            attach.countActionSets=1;attach.actionSets=&set_;
            check(xrAttachSessionActionSets(session_,&attach),"xrAttachSessionActionSets");
            for(unsigned i=0;i<2;++i){grip_spaces_[i]=action_space(grip_,hands_[i]);aim_spaces_[i]=action_space(aim_,hands_[i]);}
        }catch(...){close();throw;}
    }
    void profile_changed(){++profile_generation_;if(!profile_generation_)profile_generation_=1;edges_.profile_changed();}
    bool sample(XrSpace local,XrTime predicted_time,bool focused,gears_vr::ControllerInputFrame& frame) {
        if(!set_||!local||predicted_time<=0)return false;
        XrActiveActionSet active{set_,XR_NULL_PATH};
        XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;
        const XrResult result=xrSyncActions(session_,&sync);
        frame.sync_ns=gears_vr::monotonic_ns();
        if(result==XR_SESSION_NOT_FOCUSED||!focused){
            if(result!=XR_SUCCESS&&result!=XR_SESSION_NOT_FOCUSED)
                throw std::runtime_error("xrSyncActions did not return a usable focus state: "+std::to_string(result));
            frame.focused=0;edges_.update(frame);frame.received_ns=gears_vr::monotonic_ns();return true;
        }
        // XR_SESSION_NOT_FOCUSED is a success-class result. Never treat any
        // other non-XR_SUCCESS result as an actionable controller sample.
        if(result!=XR_SUCCESS)throw std::runtime_error("xrSyncActions did not return XR_SUCCESS: "+std::to_string(result));
        frame.focused=1;
        for(unsigned i=0;i<2;++i){
            auto& hand=frame.hands[i];
            hand.grip_active=pose_state(grip_,hands_[i]).isActive;
            hand.aim_active=pose_state(aim_,hands_[i]).isActive;
            locate(grip_spaces_[i],local,predicted_time,hand.grip_active,hand.grip,hand.grip_flags);
            locate(aim_spaces_[i],local,predicted_time,hand.aim_active,hand.aim,hand.aim_flags);
            const auto trigger=float_state(trigger_,hands_[i]);
            hand.trigger_active=trigger.isActive;hand.trigger=trigger.isActive?trigger.currentState:0;
            const auto squeeze=float_state(squeeze_,hands_[i]);
            hand.squeeze_active=squeeze.isActive;hand.squeeze=squeeze.isActive?squeeze.currentState:0;
            const auto x=float_state(stick_x_,hands_[i]),y=float_state(stick_y_,hands_[i]);
            hand.stick_active=x.isActive||y.isActive;
            hand.stick_x=x.isActive?x.currentState:0;hand.stick_y=y.isActive?y.currentState:0;
            const auto button=[&](XrAction action_handle,std::uint32_t bit){
                const auto state=bool_state(action_handle,hands_[i]);
                if(state.isActive){hand.buttons_active|=bit;if(state.currentState)hand.buttons|=bit;}
            };
            button(stick_click_,gears_vr::controller_stick_click);
            if(i==0){button(face_[2],gears_vr::controller_x);button(face_[3],gears_vr::controller_y);}
            else{button(face_[0],gears_vr::controller_a);button(face_[1],gears_vr::controller_b);}
        }
        edges_.update(frame);frame.received_ns=gears_vr::monotonic_ns();return true;
    }
    std::uint64_t profile_generation() const {return profile_generation_;}
    void close() noexcept {
        for(auto& space:grip_spaces_){if(space){xrDestroySpace(space);space=XR_NULL_HANDLE;}}
        for(auto& space:aim_spaces_){if(space){xrDestroySpace(space);space=XR_NULL_HANDLE;}}
        if(set_){xrDestroyActionSet(set_);set_=XR_NULL_HANDLE;}
        instance_=XR_NULL_HANDLE;session_=XR_NULL_HANDLE;
    }
};
}
