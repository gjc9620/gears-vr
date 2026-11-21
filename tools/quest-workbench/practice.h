#pragma once
#include "practice_geometry.h"
#include "interaction.h"
#include "game_rules/body_slots.h"
#include "game_rules/weapon_two_hand_hold.h"
#include "game_rules/weapon_lancer_front_handle.h"
#include "game_rules/weapon_lancer_chainsaw.h"
#include "game_rules/weapon_lancer_insertion.h"
#include "game_rules/weapon_lancer_insertion_assist.h"
#include "game_rules/weapon_reload_model.h"
#include "game_rules/weapon_lancer_reload_policy.h"

namespace workbench {
struct PracticeSettings {
    bool grab_pose{true},snap_enabled{true},two_hand{true},insertion_assist{true},haptics{true},audio{true},show_regions{},sight_guide{};
    double snap_seconds{.12},fire_interval{.09},recoil_degrees{.6},vibration{.5},audio_gain{.15};
    V shoulder_adjust{};double shoulder_scale{1};
    Json json()const{return {{"schema",1},{"grab_pose",grab_pose},{"snap_enabled",snap_enabled},{"two_hand",two_hand},{"insertion_assist",insertion_assist},{"haptics",haptics},{"audio",audio},{"show_regions",show_regions},{"sight_guide",sight_guide},{"snap_seconds",snap_seconds},{"preview_fire_interval",fire_interval},{"preview_recoil_degrees",recoil_degrees},{"vibration",vibration},{"audio_gain",audio_gain},{"shoulder_adjust_metres",workbench::json(shoulder_adjust)},{"shoulder_scale",shoulder_scale},{"shooting_feedback_source","procedural_preview_not_original_engine"}};}
    static PracticeSettings parse(Json const& j){PracticeSettings s;
        s.grab_pose=j.value("grab_pose",true);s.snap_enabled=j.value("snap_enabled",true);s.two_hand=j.value("two_hand",true);s.insertion_assist=j.value("insertion_assist",true);s.haptics=j.value("haptics",true);s.audio=j.value("audio",true);s.show_regions=j.value("show_regions",false);s.sight_guide=j.value("sight_guide",false);
        s.snap_seconds=j.value("snap_seconds",.12);s.fire_interval=j.value("preview_fire_interval",.09);s.recoil_degrees=j.value("preview_recoil_degrees",.6);s.vibration=j.value("vibration",.5);s.audio_gain=j.value("audio_gain",.15);s.shoulder_scale=j.value("shoulder_scale",1.);if(j.contains("shoulder_adjust_metres"))s.shoulder_adjust=vector(j.at("shoulder_adjust_metres"));
        if(!std::isfinite(s.snap_seconds)||s.snap_seconds<0||s.snap_seconds>.5||!std::isfinite(s.fire_interval)||s.fire_interval<.03||s.fire_interval>.5||!std::isfinite(s.recoil_degrees)||s.recoil_degrees<0||s.recoil_degrees>5||!std::isfinite(s.vibration)||s.vibration<0||s.vibration>1||!std::isfinite(s.audio_gain)||s.audio_gain<0||s.audio_gain>1||!std::isfinite(s.shoulder_scale)||s.shoulder_scale<.5||s.shoulder_scale>2||length(s.shoulder_adjust)>.5)throw std::runtime_error("Invalid practice settings");return s;
    }
};
struct PracticeController {P grip{};bool valid{};double squeeze{},trigger{};P aim{};bool aim_valid{};};
struct PracticeFrame {std::int64_t time_ns{};std::uint64_t epoch{1};P head{},eye{};bool focused{true},suppress_actions{};std::array<PracticeController,2> controllers;};
struct PracticeFeedback {std::array<float,2> vibration{};bool shot{},click{},seat{};};
struct PracticeImpact {V point{};double time{};};

// Standalone scene adapter. IDs below identify THIS sandbox only; they never
// masquerade as original engine actors, XR tickets or game animation receipts.
class Practice {
    static constexpr std::uint64_t scene_weapon=1,scene_item=2;
    std::array<ButtonEdge,2> squeeze_,trigger_;
    std::int64_t previous_ns_{};std::uint64_t epoch_{1},sequence_{},token_{1},render_serial_{};
    bool mag_visible_{true};unsigned ejected_loaded_{},ejected_reserve_{};bool detached_rebased_{};
    bool main_{},main_aim_{},off_new_{},original_melee_{},mag_old_{true},mag_seated_{true},mag_missing_{},inspection_{},chamber_{true},need_cycle_{},fire_ready_{};
    unsigned off_control_{};int off_{-1};P main_mount_{},mag_wrist_{},mag_world_{},mag_in_hand_{},body_{};V previous_head_{};
    double now_{},next_shot_{},shot_time_{-100},reload_start_{},tail_until_{},motor_pulse_{},grab_started_{};P grab_origin_{};
    double c_angle_{},c_pull_{},e_distance_{},observed_c_angle_{},observed_c_pull_{},observed_e_{};
    gears_weapon::ReloadModel reload_;gears_weapon::lancer_front::Stroke charge_;
    gears_weapon::LancerChainsawInteraction saw_;gears_weapon::LancerMagazineExtractionStroke extraction_;
    gears_weapon::LancerMagazineInsertionStroke insertion_;gears_weapon::LancerMagazineInsertionAssist assist_;
    PracticeSettings settings_;
    std::array<P,2> wrists_{};std::array<bool,2> tracked_{};std::array<int,2> hand_part_{{-1,-1}};
    std::array<P,2> visible_wrists_{};PracticeFeedback feedback_{};std::string status_{"右手握持键拿枪；左手操作部件"};
    P gun_{},target_board_{};std::vector<PracticeImpact> impacts_;unsigned shots_{},hits_{},cycles_{};
    gears_weapon::ReloadPolicy policy(bool empty=false)const{return {gears_weapon::LoadUnit::magazine,60,60,empty,false,gears_weapon::LancerReloadPolicy{}.deadline(empty,4000),gears_weapon::MechanismAction::bolt,gears_weapon::MechanismAction::none};}
    Frame geometry()const{Frame f{};f.object=gun_;f.starter_angle=c_angle_;f.starter_pull=c_pull_;f.charging_distance=e_distance_;f.magazine_delta=relative(gun_,compose(mag_world_,inverse(magazine_seat())));return f;}
    P wrist_local(Part const& part,unsigned side)const{return effective_wrist(part,side,geometry());}
    Volume volume(unsigned index,std::vector<Part> const& parts)const{auto a=parts[index].volume;a.local=compose(part_delta(index,geometry()),a.local);return a;}
    P desired_wrist(unsigned part,unsigned side,std::vector<Part> const& parts)const{return compose(gun_,wrist_local(parts[part],side));}
    bool candidate(unsigned part,unsigned side,PracticeFrame const& input,std::vector<Part> const& parts)const{
        if((part==2&&!mag_visible_)||!tracked_[side]||!parts[part].known[side]||!contains(volume(part,parts),gun_,input.controllers[side].grip.position))return false;
        if(!settings_.snap_enabled)return true;auto target=desired_wrist(part,side,parts);auto const& a=parts[part].volume;
        return length(sub(target.position,wrists_[side].position))<=a.snap_distance&&rotation_error(target.orientation,wrists_[side].orientation)<=a.snap_angle;
    }
    bool down(unsigned side,unsigned control)const{return control?trigger_[side].down:squeeze_[side].down;}
    void pulse(unsigned side,float amount){feedback_.vibration[side]=std::max(feedback_.vibration[side],amount);}
    gears_weapon::LancerInsertionSample magazine_sample()const{
        gears_weapon::LancerInsertionSample s{};s.weapon=scene_weapon;s.component=scene_item;s.world=s.session=s.space_epoch=s.input_epoch=s.world_generation=epoch_;s.sequence=sequence_;s.created_ms=std::max<std::uint64_t>(1,std::uint64_t(now_*1000));s.tracked=tracked_[0];s.grip_down=off_==2&&down(0,off_control_);s.units_per_metre=100;
        s.gun_mesh={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};auto local=relative(gun_,mag_world_);auto r=v::multiply(v::multiply(v::transpose(asset_to_xr),v::rotation_matrix(local.orientation)),asset_to_xr);v::Matrix rest{{{0,1,0},{1,0,0},{0,0,-1}}};s.magazine={to_raw(local.position),v::ue_angles(v::multiply(r,rest))};
        // Preserve the exact closed endpoint across the reflected-metre round trip.
        if(length(sub(s.magazine.position,gears_weapon::lancer_magazine_upper_anchor_raw))<1e-8)s.magazine.position=gears_weapon::lancer_magazine_upper_anchor_raw;
        return s;
    }
    void accept_magazine_pose(gears_vr::Viewpoint const& raw){v::Matrix rest{{{0,1,0},{1,0,0},{0,0,-1}}};auto delta=v::multiply(v::ue_matrix(raw.rotation),v::transpose(rest));auto xr=v::multiply(v::multiply(asset_to_xr,delta),v::transpose(asset_to_xr));mag_world_=compose(gun_,{from_raw(raw.position),matrix_quaternion(xr)});}
    void begin_magazine(bool replacement,std::vector<Part> const& parts,bool loose=false){
        off_=2;mag_visible_=true;mag_old_=!replacement;++token_;auto authored=relative(magazine_seat(),effective_wrist(parts[2],0,Frame{}));mag_wrist_=authored;
        if(replacement){if(reload_.grab(scene_weapon)!=gears_weapon::ReloadEvent::grabbed){off_=-1;status_="肩部没有可取的新弹匣";return;}if(!loose)mag_world_=compose(wrists_[0],inverse(authored));mag_in_hand_=relative(wrists_[0],mag_world_);mag_seated_=false;status_="新弹匣已取出，对准弹匣井插入";pulse(0,.4f);}
        else{if(!loose)mag_world_=compose(gun_,magazine_seat());mag_in_hand_=relative(wrists_[0],mag_world_);status_=loose?"已重新握住原弹匣":"向下拔出弹匣";}
        extraction_.reset();insertion_.reset();assist_.reset();auto s=magazine_sample();if(replacement||loose)insertion_.arm(s);else extraction_.begin(s,true);
    }
    void release_off(bool interrupted=false){
        if(off_==3){if(interrupted)charge_.cancel(now_);else charge_.release(now_);}
        if(off_==4){if(original_melee_&&!interrupted)tail_until_=now_+3;else{tail_until_=0;original_melee_=false;}saw_.reset();}
        if(off_==2){if(mag_seated_)mag_world_=compose(gun_,magazine_seat());else{mag_visible_=false;if(!mag_old_)reload_.release(scene_weapon);}extraction_.reset();insertion_.reset();assist_.reset();}
        off_=-1;
    }
    void update_magazine(std::vector<Part> const& parts){
        if(off_!=2){if(mag_seated_)mag_world_=compose(gun_,magazine_seat());return;}
        mag_world_=compose(wrists_[0],mag_in_hand_);auto s=magazine_sample();
        if(mag_seated_){
            gears_vr::Viewpoint visual{};if(gears_weapon::lancer_extraction_display_target(s,visual))accept_magazine_pose(visual);
            if(extraction_.observe(s)){
                ejected_loaded_=reload_.loaded();ejected_reserve_=reload_.reserve();detached_rebased_=false;bool empty=ejected_loaded_==0;auto result=reload_.begin(scene_weapon,policy(empty),ejected_loaded_,ejected_reserve_,std::int64_t(now_*1000));
                if(result==gears_weapon::ReloadEvent::full||result==gears_weapon::ReloadEvent::no_ammo){reload_.reset();reload_.prime(scene_weapon,policy(empty),0,ejected_loaded_+ejected_reserve_);reload_.begin(scene_weapon,policy(empty),0,ejected_loaded_+ejected_reserve_,std::int64_t(now_*1000));detached_rebased_=true;}
                inspection_=result==gears_weapon::ReloadEvent::full;mag_seated_=false;mag_missing_=true;need_cycle_=empty;reload_start_=now_;insertion_.arm(s);status_=inspection_?"满匣检查：可把原匣装回":"原匣已拔出；松开后到左肩取新匣";pulse(0,.35f);
            }
        }else{
            if(settings_.snap_enabled&&settings_.insertion_assist)assist_.apply(s);
            auto result=insertion_.observe(s);accept_magazine_pose(s.magazine);
            if(result==gears_weapon::LancerInsertResult::inserted){
                auto event=gears_weapon::ReloadEvent::rejected;
                if(mag_old_&&detached_rebased_){reload_.reset();reload_.prime(scene_weapon,policy(),ejected_loaded_,ejected_reserve_);event=gears_weapon::ReloadEvent::cancelled;}
                else event=mag_old_?reload_.restore_ejected_magazine(scene_weapon):reload_.insert(scene_weapon,std::int64_t(now_*1000));
                if(event!=gears_weapon::ReloadEvent::rejected){mag_seated_=true;mag_missing_=false;mag_world_=compose(gun_,magazine_seat());
                    if(mag_old_){status_="原弹匣已装回";}else{chamber_=!need_cycle_;status_=need_cycle_?"新弹匣已入位，拉栓上膛":event==gears_weapon::ReloadEvent::perfect?"完美换弹":"换弹完成";}
                    off_=-1;feedback_.seat=true;pulse(0,.7f);
                }
            }
        }
        (void)parts;
    }
    gears_weapon::HammerburstLatchSample charge_sample(PracticeFrame const& input)const{V p=relative(gun_,input.controllers[0].grip).position;return {mul(to_raw(p),.01),sequence_,std::max<std::uint64_t>(1,std::uint64_t(now_*1000)),token_,tracked_[0],off_==3&&down(0,off_control_)};}
    void update_charge(PracticeFrame const& input){
        using Phase=gears_weapon::lancer_front::Stroke::Phase;
        if(off_==3&&!off_new_){if(!charge_.move(charge_sample(input))){charge_.cancel(now_);off_=-1;status_="拉栓握持中断，请松开再试";}}
        if(charge_.phase()==Phase::returning)charge_.advance_return(now_);
        e_distance_=charge_.wanted_distance();
        if(charge_.phase()==Phase::complete){reload_.cycle(scene_weapon,std::int64_t(now_*1000),gears_weapon::MechanismAction::bolt);chamber_=mag_seated_&&reload_.loaded()>0;if(chamber_)need_cycle_=false;++cycles_;charge_.reset();e_distance_=0;status_=chamber_?"拉栓完成回位，可以射击":"拉栓已回位，仍为空仓";pulse(0,.5f);feedback_.click=true;}
    }
    void update_saw(PracticeFrame const& input,double dt){
        // The shared motor kernel does not animate an unowned scene part.
        // Finish the sandbox's released tail, then spring the rail home before folding.
        if(tail_until_>0&&now_>=tail_until_){tail_until_=0;original_melee_=false;}
        if(!main_||!tracked_[1]){original_melee_=false;tail_until_=0;saw_.reset();}
        if(off_!=4&&!motor())c_pull_=std::max(0.,c_pull_-dt*.32);
        auto g=geometry();auto closed=parts_closed_c_;bool nearby=tracked_[0]&&main_&&contains(closed,gun_,input.controllers[0].grip.position);
        bool deploy=nearby||off_==4||c_pull_>.001||motor();double target=deploy?1.5707963267948966:0;c_angle_+=std::clamp(target-c_angle_,-dt*8,dt*8);
        if(!main_||!tracked_[1]){original_melee_=false;tail_until_=0;saw_.reset();return;}
        gears_weapon::LancerSawSample s{};s.world=s.pawn=s.weapon=s.session=s.space_epoch=s.input_epoch=s.world_generation=epoch_;s.sequence=sequence_;s.created_ms=std::max<std::uint64_t>(1,std::uint64_t(now_*1000));s.lancer_active=s.right_owns_weapon=s.right_tracked=s.game_can_melee=s.original_ready=true;
        s.left_tracked=tracked_[0];s.left_grip_down=off_==4&&down(0,off_control_);s.left_palm_m=input.controllers[0].grip.position;s.starter_anchor_m=compose(gun_,compose(starter_delta(c_angle_,c_pull_),closed.local)).position;s.starter_grasp_m=s.starter_anchor_m;
        s.starter_pivot_m=compose(gun_,{from_raw(gears_lancer_owner_grabposes::starter_pivot),{}}).position;s.pull_axis_m=rotate(gun_.orientation,{0,0,-1});s.original_handle=s.original_slide=s.local_rail=s.require_full_slide=true;
        s.rail_hand_m=mul(to_raw(relative(gun_,input.controllers[0].grip).position),.01);s.starter_actual_angle=observed_c_angle_;s.starter_actual_pull_m=observed_c_pull_;s.starter_travel_m=.08;s.original_hold=s.original_button_held=original_melee_&&now_>=tail_until_;
        auto d=saw_.observe(s,off_new_&&off_==4);if(off_==4)c_pull_=saw_.pull_metres();
        if(d.event==gears_weapon::LancerSawEvent::request_original_hold){original_melee_=true;tail_until_=0;status_="电锯已启动：握住可持续运行，回拉可停机";pulse(0,.7f);feedback_.click=true;}
        if(d.event==gears_weapon::LancerSawEvent::request_original_release){original_melee_=false;tail_until_=0;status_="电锯已回拉停机";}
        if(d.event==gears_weapon::LancerSawEvent::interrupted&&off_==4){off_=-1;original_melee_=false;status_="电锯操作中断，松开后重新握持";}
        if(motor()&&now_>=motor_pulse_){motor_pulse_=now_+.07;pulse(0,.15f);pulse(1,.08f);}
        (void)g;
    }
    Volume parts_closed_c_{};
public:
    void reset(P gun,P head,bool empty=false){*this=Practice{};gun_=gun;body_={head.position,axis({0,1,0},std::atan2(-rotate(head.orientation,{0,0,-1}).x,-rotate(head.orientation,{0,0,-1}).z))};previous_head_=head.position;mag_world_=compose(gun_,magazine_seat());target_board_=compose(body_,{{0,-.1,-10},{}});reload_.prime(scene_weapon,policy(),empty?0:30,360);chamber_=!empty;need_cycle_=empty;}
    void resume(PracticeSettings const& settings,std::uint64_t epoch){settings_=settings;epoch_=epoch?epoch:1;previous_ns_=0;main_=false;release_off(true);for(auto& e:squeeze_)e={};for(auto& e:trigger_)e={};status_="右手握持键拿枪；左手到左肩取新弹匣";}
    void interrupt(){main_=false;release_off(true);original_melee_=false;tail_until_=0;for(auto& e:squeeze_)e={};for(auto& e:trigger_)e={};}
    void settings(PracticeSettings const& s){settings_=s;}
    void reposition(P gun){gun_=gun;if(mag_seated_)mag_world_=compose(gun_,magazine_seat());}
    void body(P p){body_=p;previous_head_=p.position;}
    V shoulder_adjust_for(V point)const{V center{};gears_weapon::shoulder_ammo_center(gears_weapon::default_body_slot_layout(),body_,center);return rotate(v::inverse(body_.orientation),sub(point,center));}
    P gun()const{return gun_;}Frame frame()const{return geometry();}P magazine()const{return mag_world_;}
    P board()const{return target_board_;}bool main_held()const{return main_;}int off_part()const{return off_;}
    bool mag_visible()const{return mag_visible_;}bool mag_seated()const{return mag_seated_;}bool motor()const{return original_melee_&&(tail_until_<=0||now_<tail_until_);}
    bool chambered()const{return chamber_;}double charge_progress()const{return e_distance_/gears_weapon::lancer_front::travel_raw*100;}
    double starter_progress()const{return c_pull_/.08;}unsigned loaded()const{return reload_.loaded();}unsigned reserve()const{return reload_.reserve();}
    unsigned shots()const{return shots_;}unsigned hits()const{return hits_;}unsigned cycles()const{return cycles_;}std::string const& status()const{return status_;}
    std::array<P,2> const& hand_wrists()const{return visible_wrists_;}std::array<int,2> const& hand_parts()const{return hand_part_;}
    std::vector<PracticeImpact> const& impacts()const{return impacts_;}PracticeFeedback const& feedback()const{return feedback_;}
    P shoulder()const{V p{};gears_weapon::shoulder_ammo_center(gears_weapon::default_body_slot_layout(),body_,p);return {add(p,rotate(body_.orientation,settings_.shoulder_adjust)),body_.orientation};}
    bool shoulder_contains(V point)const{P shifted=body_;shifted.position=add(shifted.position,rotate(body_.orientation,settings_.shoulder_adjust));if(settings_.shoulder_scale==1)return gears_weapon::body_slot(gears_weapon::default_body_slot_layout(),shifted,point,0)==gears_weapon::BodySlot::shoulder_ammo;auto p=relative(shoulder(),{point,{}}).position;return p.x*p.x/std::pow(.2045*settings_.shoulder_scale,2)+(p.y*p.y+p.z*p.z)/std::pow(.24*settings_.shoulder_scale,2)<=1;}
    void acknowledge_render(std::uint64_t serial){if(serial<=render_serial_)return;render_serial_=serial;observed_c_angle_=c_angle_;observed_c_pull_=c_pull_;observed_e_=e_distance_;charge_.acknowledge_visible_part(e_distance_,serial,now_);}
    void step(PracticeFrame const& input,std::vector<Part> const& parts){
        feedback_={};if(parts.size()!=5||!input.focused||input.time_ns<=0||!valid(input.head)){interrupt();return;}
        if(previous_ns_&&(input.time_ns<=previous_ns_||input.time_ns-previous_ns_>100'000'000||input.epoch!=epoch_)){interrupt();previous_ns_=0;}
        double dt=previous_ns_?double(input.time_ns-previous_ns_)/1e9:0;previous_ns_=input.time_ns;epoch_=input.epoch;now_=double(input.time_ns)/1e9;++sequence_;parts_closed_c_=parts[4].volume;
        body_.position=add(body_.position,sub(input.head.position,previous_head_));previous_head_=input.head.position;hand_part_={-1,-1};off_new_=false;
        std::array<bool,2> grip_press{},trigger_press{};
        for(unsigned side=0;side<2;++side){tracked_[side]=input.controllers[side].valid&&valid(input.controllers[side].grip);auto mount=controller_wrist_mount(side,side==1&&main_);wrists_[side]=compose(input.controllers[side].grip,mount);visible_wrists_[side]=wrists_[side];grip_press[side]=squeeze_[side].update(tracked_[side]?1:0,input.controllers[side].squeeze,v::controller_analog_press_threshold,v::controller_analog_release_threshold);trigger_press[side]=trigger_[side].update(tracked_[side]?1:0,input.controllers[side].trigger,v::controller_analog_press_threshold,v::controller_analog_release_threshold);}
        if(main_&&(!tracked_[1]||!squeeze_[1].down||(main_aim_&&(!input.controllers[1].aim_valid||!valid(input.controllers[1].aim))))){main_=false;release_off(!tracked_[1]);original_melee_=false;tail_until_=0;}
        if(!main_&&!input.suppress_actions&&grip_press[1]&&candidate(0,1,input,parts)){main_=true;main_aim_=settings_.grab_pose&&input.controllers[1].aim_valid;grab_started_=now_;grab_origin_=gun_;fire_ready_=!trigger_[1].down;wrists_[1]=compose(input.controllers[1].grip,controller_wrist_mount(1,true));main_mount_=settings_.grab_pose?wrist_local(parts[0],1):relative(gun_,wrists_[1]);status_="已拿起：右扳机射击，左手操作部件";pulse(1,.3f);}
        if(main_){if(settings_.grab_pose&&input.controllers[1].aim_valid&&valid(input.controllers[1].aim))wrists_[1]=quest_main_wrist(input.controllers[1].grip,input.controllers[1].aim,main_mount_);auto goal=compose(wrists_[1],inverse(main_mount_));if(off_==1&&settings_.two_hand&&tracked_[0]){gears_weapon_two_hand::Solution solved{};auto fore=relative(main_mount_,wrist_local(parts[1],0));P r=wrists_[1],l=wrists_[0];r.position=mul(r.position,100);l.position=mul(l.position,100);if(gears_weapon_two_hand::solve(r,l,fore,100,solved)){r={mul(solved.right.position,.01),solved.right.orientation};goal=compose(r,inverse(main_mount_));}}
            double alpha=settings_.snap_enabled&&settings_.snap_seconds>0?std::clamp((now_-grab_started_)/settings_.snap_seconds,0.,1.):1;alpha=alpha*alpha*(3-2*alpha);gun_=alpha<1?mix(grab_origin_,goal,alpha):goal;hand_part_[1]=0;visible_wrists_[1]=compose(gun_,main_mount_);}
        if(off_>=0&&(!tracked_[0]||!down(0,off_control_)))release_off(!tracked_[0]);
        if(off_==1&&settings_.snap_enabled){auto target=desired_wrist(1,0,parts);auto const& a=parts[1].volume;if(length(sub(target.position,wrists_[0].position))>std::max(a.release_distance,a.snap_distance)||rotation_error(target.orientation,wrists_[0].orientation)>std::min(3.141592653589793,a.snap_angle+.35)){release_off();status_="左手已离开护木吸附范围";}}
        if(main_&&!input.suppress_actions&&off_<0&&(grip_press[0]||trigger_press[0])){
            off_control_=trigger_press[0]?1:0;
            if(mag_missing_&&reload_.stage()==gears_weapon::ReloadStage::feed_port&&shoulder_contains(input.controllers[0].grip.position))begin_magazine(true,parts);
            else{int selected=-1;double best=1e9;for(unsigned part=1;part<5;++part){if(!candidate(part,0,input,parts))continue;double d=length(sub(compose(gun_,volume(part,parts).local).position,input.controllers[0].grip.position));if(d<best){best=d;selected=int(part);}}
                if(selected==2)begin_magazine(!mag_seated_&&!mag_old_,parts,!mag_seated_);else if(selected==3){gears_weapon::SnubSlideGeometry g{};gears_weapon::lancer_front::geometry(100,g);off_=3;++token_;if(charge_.phase()!=gears_weapon::lancer_front::Stroke::Phase::idle)charge_.reset();if(charge_.begin(g,charge_sample(input),e_distance_)){off_new_=true;pulse(0,.25f);status_="握住拉栓后向后拉，松开让它回位";}else off_=-1;}
                else if(selected>=0){off_=selected;off_new_=true;++token_;pulse(0,.2f);status_=selected==4?"握住握柄，向前推拉电锯滑柄":"双手握持";}
            }
        }
        update_magazine(parts);update_charge(input);update_saw(input,dt);
        if(off_>=0){hand_part_[0]=off_;visible_wrists_[0]=off_==2?compose(mag_world_,mag_wrist_):desired_wrist(unsigned(off_),0,parts);if(!settings_.grab_pose||!settings_.snap_enabled)visible_wrists_[0]=wrists_[0];}
        if(!trigger_[1].down)fire_ready_=true;
        if(input.suppress_actions&&trigger_[1].down)fire_ready_=false;
        bool firing=!input.suppress_actions&&main_&&tracked_[1]&&trigger_[1].down&&trigger_[1].armed&&fire_ready_;
        if(firing&&now_>=next_shot_){next_shot_=now_+settings_.fire_interval;if(mag_seated_&&chamber_&&reload_.can_fire()&&off_!=3&&!motor()){
                auto after=reload_.loaded()-1,reserve=reload_.reserve();reload_.reset();reload_.prime(scene_weapon,policy(),after,reserve);if(!after)chamber_=false;++shots_;shot_time_=now_;feedback_.shot=true;pulse(1,1);
                auto ray=shot_ray();auto plane=target_board_;V normal=rotate(plane.orientation,{0,0,1}),d=rotate(ray.orientation,{0,0,-1});double den=dot(d,normal);if(std::abs(den)>1e-7){double t=dot(sub(plane.position,ray.position),normal)/den;auto point=add(ray.position,mul(d,t));auto at=relative(plane,{point,{}}).position;if(t>0&&std::abs(at.x)<1.5&&std::abs(at.y)<1.5){++hits_;impacts_.push_back({point,now_});if(impacts_.size()>80)impacts_.erase(impacts_.begin());}}
            }else if(trigger_press[1]){feedback_.click=true;pulse(1,.15f);status_=!mag_seated_?"先装入弹匣":!chamber_?"空仓：换匣后拉栓上膛":"当前动作阻止射击";}}
        if(!firing)next_shot_=now_;
    }
    P shot_ray()const{gears_weapon_lancer_sight::Result s{};V muzzle{.00685,72.66864,11.24774};V direction{0,0,-1};if(gears_weapon_lancer_sight::solve(v::ue_matrix({}),{},1,100,muzzle,{0,1,0},s))direction=unit(from_raw(s.direction));P ray=compose(gun_,{from_raw(muzzle),arc({0,0,-1},direction)});return ray;}
    P visual_gun()const{double recoil=settings_.recoil_degrees*std::exp(-std::max(0.,now_-shot_time_)*25)*3.141592653589793/180;return compose(gun_,{{0,0,recoil*.12},axis({1,0,0},recoil)});}
};
}
