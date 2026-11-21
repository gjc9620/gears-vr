#pragma once
#include "core.h"
#include <openxr/openxr.h>
#include <cstring>

namespace workbench {
struct Controller {
    P aim{},grip{};
    bool aim_valid{},grip_valid{},select{},select_active{},recenter{},recenter_active{};
    float trigger{},squeeze{};
};
// Real OpenXR action states only. Controller poses never manufacture finger joints.
class ControllerInput {
    XrInstance instance_{};XrSession session_{};XrActionSet set_{};
    XrAction aim_{},grip_{},trigger_{},squeeze_{},select_{},recenter_{},haptic_{};
    std::array<XrPath,2> paths_{};
    std::array<XrSpace,2> aims_{},grips_{};
    static void require(XrResult result,char const* label){if(XR_FAILED(result))throw std::runtime_error(std::string(label)+" "+std::to_string(result));}
    XrPath path(std::string const& name){XrPath p{};require(xrStringToPath(instance_,name.c_str(),&p),"controller path");return p;}
    XrAction action(char const* name,char const* label,XrActionType type){XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};std::strcpy(info.actionName,name);std::strcpy(info.localizedActionName,label);info.actionType=type;info.countSubactionPaths=2;info.subactionPaths=paths_.data();XrAction a{};require(xrCreateAction(set_,&info,&a),"create controller action");return a;}
    static P convert(XrPosef const& p){return {{p.position.x,p.position.y,p.position.z},{p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}};}
    bool locate(XrAction action,XrSpace space,XrPath subaction,XrSpace base,XrTime time,P& out,std::uint64_t required=3){
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};get.action=action;get.subactionPath=subaction;XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
        if(XR_FAILED(xrGetActionStatePose(session_,&get,&state))||!state.isActive)return false;
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};if(XR_FAILED(xrLocateSpace(space,base,time,&location))||(location.locationFlags&required)!=required)return false;
        auto p=convert(location.pose);if(!valid(p))return false;out=p;return true;
    }
public:
    std::array<Controller,2> hands{};
    void initialize(XrInstance instance,XrSession session){
        instance_=instance;session_=session;paths_={path("/user/hand/left"),path("/user/hand/right")};
        XrActionSetCreateInfo info{XR_TYPE_ACTION_SET_CREATE_INFO};std::strcpy(info.actionSetName,"workbench");std::strcpy(info.localizedActionSetName,"Workbench controls");info.priority=0;require(xrCreateActionSet(instance_,&info,&set_),"create action set");
        aim_=action("aim","Menu pointer",XR_ACTION_TYPE_POSE_INPUT);grip_=action("grip","Controller grip",XR_ACTION_TYPE_POSE_INPUT);
        trigger_=action("trigger","Menu trigger",XR_ACTION_TYPE_FLOAT_INPUT);squeeze_=action("squeeze","Grab weapon",XR_ACTION_TYPE_FLOAT_INPUT);select_=action("select","Menu A or X",XR_ACTION_TYPE_BOOLEAN_INPUT);recenter_=action("recenter","Reset work area B or Y",XR_ACTION_TYPE_BOOLEAN_INPUT);
        haptic_=action("feedback","Workbench vibration",XR_ACTION_TYPE_VIBRATION_OUTPUT);
        std::vector<XrActionSuggestedBinding> bindings;
        for(unsigned side=0;side<2;++side){std::string root=side?"/user/hand/right/input/":"/user/hand/left/input/";
            bindings.push_back({aim_,path(root+"aim/pose")});bindings.push_back({grip_,path(root+"grip/pose")});bindings.push_back({trigger_,path(root+"trigger/value")});bindings.push_back({squeeze_,path(root+"squeeze/value")});bindings.push_back({select_,path(root+(side?"a/click":"x/click"))});bindings.push_back({recenter_,path(root+(side?"b/click":"y/click"))});bindings.push_back({haptic_,path(std::string(side?"/user/hand/right":"/user/hand/left")+"/output/haptic")});}
        XrInteractionProfileSuggestedBinding suggest{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};suggest.interactionProfile=path("/interaction_profiles/oculus/touch_controller");suggest.countSuggestedBindings=std::uint32_t(bindings.size());suggest.suggestedBindings=bindings.data();require(xrSuggestInteractionProfileBindings(instance_,&suggest),"Touch controller bindings");
        XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&set_;require(xrAttachSessionActionSets(session_,&attach),"attach controller actions");
        for(unsigned side=0;side<2;++side){XrActionSpaceCreateInfo space{XR_TYPE_ACTION_SPACE_CREATE_INFO};space.subactionPath=paths_[side];space.poseInActionSpace.orientation.w=1;space.action=aim_;require(xrCreateActionSpace(session_,&space,&aims_[side]),"aim space");space.action=grip_;require(xrCreateActionSpace(session_,&space,&grips_[side]),"grip space");}
    }
    XrResult sync(XrSpace base,XrTime time,bool focused){
        hands={};if(!focused||!session_)return XR_SUCCESS;
        XrActiveActionSet active{set_,XR_NULL_PATH};XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;auto result=xrSyncActions(session_,&sync);if(XR_FAILED(result))return result;
        for(unsigned side=0;side<2;++side){auto& h=hands[side];h.aim_valid=locate(aim_,aims_[side],paths_[side],base,time,h.aim,15);h.grip_valid=locate(grip_,grips_[side],paths_[side],base,time,h.grip,15);
            XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};get.subactionPath=paths_[side];
            XrActionStateFloat value{XR_TYPE_ACTION_STATE_FLOAT};get.action=trigger_;if(XR_SUCCEEDED(xrGetActionStateFloat(session_,&get,&value))&&value.isActive)h.trigger=std::clamp(value.currentState,0.f,1.f);
            value={XR_TYPE_ACTION_STATE_FLOAT};get.action=squeeze_;if(XR_SUCCEEDED(xrGetActionStateFloat(session_,&get,&value))&&value.isActive)h.squeeze=std::clamp(value.currentState,0.f,1.f);
            XrActionStateBoolean click{XR_TYPE_ACTION_STATE_BOOLEAN};get.action=select_;if(XR_SUCCEEDED(xrGetActionStateBoolean(session_,&get,&click))&&click.isActive){h.select_active=true;h.select=click.currentState;}
            click={XR_TYPE_ACTION_STATE_BOOLEAN};get.action=recenter_;if(XR_SUCCEEDED(xrGetActionStateBoolean(session_,&get,&click))&&click.isActive){h.recenter_active=true;h.recenter=click.currentState;}
        }return result;
    }
    std::string profile(unsigned side){XrInteractionProfileState state{XR_TYPE_INTERACTION_PROFILE_STATE};if(side>1||XR_FAILED(xrGetCurrentInteractionProfile(session_,paths_[side],&state))||state.interactionProfile==XR_NULL_PATH)return "none";char text[256]{};std::uint32_t size{};if(XR_FAILED(xrPathToString(instance_,state.interactionProfile,sizeof text,&size,text)))return "unknown";return text;}
    void vibrate(unsigned side,float amplitude){if(side>1||!session_||amplitude<=0)return;XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};info.action=haptic_;info.subactionPath=paths_[side];XrHapticVibration v{XR_TYPE_HAPTIC_VIBRATION};v.amplitude=std::clamp(amplitude,0.f,1.f);v.frequency=XR_FREQUENCY_UNSPECIFIED;v.duration=25'000'000;xrApplyHapticFeedback(session_,&info,reinterpret_cast<XrHapticBaseHeader*>(&v));}
    void stop_feedback(){if(!session_)return;for(auto p:paths_){XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};info.action=haptic_;info.subactionPath=p;xrStopHapticFeedback(session_,&info);}}
    void shutdown(){for(auto p:aims_)if(p)xrDestroySpace(p);for(auto p:grips_)if(p)xrDestroySpace(p);if(set_)xrDestroyActionSet(set_);set_={};session_={};hands={};}
};
}
