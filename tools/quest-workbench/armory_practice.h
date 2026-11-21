#pragma once
#include "practice.h"
#include "game_rules/gnasher_lever.h"

namespace workbench {
// Local workshop simulation. Inventory follows C-L-S, never original engine calls.
// Measured controller motion is separate from snapped/rendered hand transforms.
class ArmoryPractice {
    Json config_;std::string kind_;PracticeSettings settings_;Practice slots_;
    std::array<ButtonEdge,2> grip_,trigger_;std::array<P,2> wrist_{},visible_{};std::array<int,2> part_{{-1,-1}};
    std::array<bool,2> tracked_{};bool main_{},main_aim_{},ready_trigger_{},mag_seated_{true},mag_visible_{true},chamber_{true},cycle_needed_{},opened_{},old_ammo_{},fully_open_{},arrived_{};
    bool pistol_waiting_{};Q mechanism_home_q_{};
    int off_{-1};unsigned off_control_{},capacity_{},loaded_{},reserve_{},old_loaded_{},held_qty_{},shots_{},hits_{},cycles_{},inserted_rounds_{};
    std::uint64_t epoch_{1},sequence_{},render_serial_{};std::int64_t previous_ns_{};
    double now_{},dt_{},next_shot_{},shot_at_{-100},charge_at_{},mech_{},bolt_lift_{},observed_mech_{},return_from_{},return_at_{-1},insert_start_{-1},best_approach_{},reload_at_{-1},auto_reload_at_{-1};
    P grab_origin_{};double grab_at_{};P gun_{},home_{},board_{},main_mount_{},support_mount_{},ammo_{},ammo_in_wrist_{},mechanism_start_{},mechanism_gun_{},support_start_{},right_home_{},body_{},ammo_wrist_{};
    V last_right_{};bool right_history_{};V throw_velocity_{};gears_weapon::GnasherLever lever_;
    PracticeFeedback feedback_;std::string status_;std::vector<PracticeImpact> impacts_;
    struct Throw {P pose;V velocity;double born;};std::vector<Throw> throws_;
    std::array<bool,6> chambers_{};
    bool is(std::string const& k)const{return kind_==k;}
    bool magazine_kind()const{return is("magazine")||is("pistol");}
    bool grenade()const{return is("frag")||is("smoke");}
    bool down(unsigned side,unsigned control)const{return control?trigger_[side].down:grip_[side].down;}
    V axis_value(char const* name,V fallback)const{return config_.contains(name)?vector(config_.at(name)):fallback;}
    P feed()const{return {vector(config_.at("feed")),{}};}
    unsigned find(std::vector<Part> const& parts,char const* id)const{for(unsigned i=0;i<parts.size();++i)if(parts[i].id==id)return i;return unsigned(parts.size());}
    P wrist_local(Part const& p,unsigned side)const{auto const& q=p.poses[side];auto d=delta(p.id);P w=compose(d,q.part_delta_known?relative(q.part_delta,q.wrist_in_object):q.wrist_in_object);w.orientation=v::normalized(v::multiply(w.orientation,degrees(p.wrist_degrees[side])));w.position=add(w.position,rotate(w.orientation,p.wrist_offset[side]));return w;}
    Volume volume(unsigned i,std::vector<Part> const& parts)const{auto a=parts[i].volume;a.local=compose(delta(parts[i].id),a.local);return a;}
    bool candidate(unsigned i,unsigned side,PracticeFrame const& f,std::vector<Part> const& parts)const{
        if(i>=parts.size()||!tracked_[side]||!parts[i].known[side])return false;
        auto a=volume(i,parts);if(!contains(a,gun_,f.controllers[side].grip.position))return false;
        auto w=compose(gun_,wrist_local(parts[i],side));return !settings_.snap_enabled||(length(sub(w.position,wrist_[side].position))<=a.snap_distance&&(parts[i].poses[side].authored_default||rotation_error(w.orientation,wrist_[side].orientation)<=a.snap_angle));
    }
    void pulse(unsigned side,float amount){feedback_.vibration[side]=std::max(feedback_.vibration[side],amount);}
    P mechanism_delta(double amount)const{
        V pivot=vector(config_.at("mechanism_pivot"));
        if(is("revolver")||is("lever")||is("magazine")){auto q=axis(axis_value("hinge_axis",{1,0,0}),amount*(is("revolver")?1.3962634:is("lever")?.65:1.2));return {sub(pivot,rotate(q,pivot)),q};}
        auto q=is("bolt")?axis({0,0,-1},bolt_lift_):Q{};return {add({0,0,amount*config_.value("travel",.1)},sub(pivot,rotate(q,pivot))),q};
    }
    void release_off(bool interrupted=false){if(off_<0)return;
        if(off_==2){if(!mag_seated_){mag_visible_=false;held_qty_=0;}insert_start_=-1;arrived_=false;}
        if(off_==3&&is("pistol")){if(interrupted){fully_open_=false;pistol_waiting_=false;}if(!interrupted&&mech_>.95&&!fully_open_&&observed_mech_<.9)pistol_waiting_=true;else{return_from_=mech_;return_at_=now_;}}
        off_=-1;
    }
    void start_reload(){if(reload_at_<0){reload_at_=now_;inserted_rounds_=0;status_="到左肩取弹，再对准装填口";}}
    void close_mechanism(){if(is("bolt")&&!chamber_&&loaded_==0){status_="未装弹，闭栓后仍为空仓";}opened_=false;fully_open_=false;mech_=0;bolt_lift_=0;chamber_=loaded_>0;cycle_needed_=false;reload_at_=-1;inserted_rounds_=0;off_=-1;pulse(0,.55f);feedback_.seat=true;status_="机构已闭锁";}
    bool can_supply()const{
        if(!reserve_||is("beam")||is("turret")||grenade()||(loaded_>=capacity_&&!magazine_kind()))return false;
        if(magazine_kind())return !mag_seated_;
        if(is("bolt")||is("revolver")||is("breech"))return opened_&&fully_open_;
        if(is("lever"))return inserted_rounds_<std::min(3u,capacity_);
        return true;
    }
    void take_ammo(std::vector<Part> const& parts){
        if(!can_supply()){status_="当前不能取弹：检查机构或剩余弹药";return;}start_reload();old_ammo_=false;
        held_qty_=is("revolver")||magazine_kind()?std::min(capacity_-loaded_,reserve_):1;
        // Magazine pool includes retained old rounds; replacement seats only the deficit.
        off_=2;mag_visible_=true;auto w=relative(delta(parts[2].id),wrist_local(parts[2],0));P local=relative(feed(),w);ammo_wrist_=local;ammo_in_wrist_=inverse(local);ammo_=compose(wrist_[0],ammo_in_wrist_);best_approach_=0;insert_start_=-1;arrived_=false;status_=is("revolver")?"装填弹巢已取出，对准支架送入":"已取弹，对准装填口送入";pulse(0,.3f);
    }
    void update_insertion(){
        if(off_!=2||!tracked_[0])return;ammo_=compose(wrist_[0],ammo_in_wrist_);
        if(magazine_kind()&&mag_seated_){
            auto in=relative(gun_,ammo_);auto d=sub(in.position,feed().position);double travel=dot(d,{0,-1,0});
            if(travel>=.045&&std::hypot(d.x,d.z)<.12){mag_seated_=false;old_loaded_=loaded_;start_reload();chamber_=is("pistol")?loaded_>0:true;status_="旧匣已拔出，松手后到左肩取新匣";pulse(0,.35f);}return;
        }
        if(old_ammo_&&!magazine_kind())return;
        P target=compose(gun_,compose(is("revolver")?mechanism_delta(mech_):P{},feed()));auto raw=relative(target,ammo_);V direction=unit(axis_value("insertion_axis",{0,0,-1}));double depth=-dot(raw.position,direction),lateral=length(sub(raw.position,mul(direction,dot(raw.position,direction))));double angle=rotation_error(raw.orientation,Q{});
        // Real approach is mandatory even when interpolation helps the final contact.
        double distance=length(raw.position);if(distance<.25&&angle<1.5)best_approach_=std::max(best_approach_,distance);
        bool contact=lateral<.07&&angle<1.48&&depth>=-.035&&depth<.065;
        if(contact&&best_approach_-distance>=.02){if(insert_start_<0)insert_start_=now_;double blend=std::clamp((now_-insert_start_)/.18,0.,1.);if(settings_.snap_enabled&&settings_.insertion_assist)ammo_=mix(ammo_,target,blend);if(blend>=1&&(settings_.insertion_assist||distance<.025))arrived_=true;}else insert_start_=-1;
        if(!arrived_)return;
        if(old_ammo_){loaded_=old_loaded_;mag_seated_=true;reload_at_=-1;status_="原匣已装回，保留原弹量";}
        else{unsigned q=std::min({held_qty_,capacity_-loaded_,reserve_});loaded_+=q;reserve_-=q;inserted_rounds_+=q;
            if(is("revolver")){for(unsigned i=0;i<6&&q;++i)if(!chambers_[i]){chambers_[i]=true;--q;}}
            mag_seated_=true;status_=is("revolver")?"已装弹，左手掌靠近弹巢按一下Grip合上":is("lever")?"已装一发；继续取弹或握护木下压上弹":"装填到位";
            if(magazine_kind()){cycle_needed_=is("pistol")&&!chamber_;if(!cycle_needed_){chamber_=loaded_>0;reload_at_=-1;}}
            else if(is("bow")){chamber_=true;reload_at_=-1;}
            else if(is("lever")){cycle_needed_=true;chamber_=false;}
        }
        off_=-1;mag_visible_=magazine_kind();held_qty_=0;insert_start_=-1;arrived_=false;feedback_.seat=true;pulse(0,.7f);
    }
    void update_mechanism(PracticeFrame const& f,std::vector<Part> const& parts){
        if(return_at_>=0){double t=std::clamp((now_-return_at_)/.15,0.,1.);mech_=return_from_*(1-t);if(t==1){return_at_=-1;if(fully_open_){cycle_needed_=false;chamber_=loaded_>0;fully_open_=false;++cycles_;status_="套筒已回位，可以射击";}fully_open_=false;}}
        if(off_!=3)return;
        if(is("lever"))return;
        auto local=relative(mechanism_gun_,f.controllers[0].grip);auto start=relative(mechanism_gun_,mechanism_start_);double amount=dot(sub(local.position,start.position),axis_value("open_axis",{0,0,1}))/std::max(.03,config_.value("travel",.1));
        if(is("bolt")){
            // Ordinary lift -> pull -> feed -> push -> lock, measured in gun-local axes.
            V up=rotate(local.orientation,{0,1,0}),home_up=rotate(start.orientation,{0,1,0});double lifted=std::acos(std::clamp(dot(unit(up),unit(home_up)),-1.,1.));
            if(!opened_){bolt_lift_=std::max(bolt_lift_,lifted);if(bolt_lift_<.35){status_="先向上转动栓柄，再向后拉";return;}}
            mech_=std::clamp(amount,0.,1.);if(mech_>.95&&observed_mech_>.9){opened_=fully_open_=true;start_reload();status_="枪膛已打开，到左肩取一发实弹";}
            if(opened_&&loaded_&&mech_<.10){if(lifted<.20)close_mechanism();else status_="向下转动栓柄，完成闭锁";}return;
        }
        if(is("pistol")){mech_=std::clamp(amount,0.,1.);if(mech_>.95&&observed_mech_>.9)fully_open_=true;return;}
        mech_=std::clamp(amount,0.,1.);if(mech_>.95&&observed_mech_>.9){opened_=fully_open_=true;start_reload();status_=is("revolver")?"弹巢已打开；松手，到左肩取装填弹巢":"装填机构已打开，到左肩取装填组件";}
        if(is("magazine")){opened_=mech_>.9;if(loaded_&&mag_seated_&&mech_<.1){reload_at_=-1;status_="锁扣已关，无需拉栓";}}
        if(is("breech")&&opened_&&loaded_&&mech_<.1)close_mechanism();
        (void)parts;
    }
    void shoot(){
        bool gate=loaded_&&mag_seated_&&chamber_&&!cycle_needed_&&!opened_;
        if(is("beam"))gate=true;if(is("turret"))gate=loaded_&&off_==1;
        if(!gate){feedback_.click=true;status_=cycle_needed_?"先完成上膛和回位":opened_?"先合上机构":"缺弹或弹匣未到位";pulse(1,.15f);return;}
        if(!is("beam")){--loaded_;if(is("revolver"))for(auto& slot:chambers_)if(slot){slot=false;break;}}
        ++shots_;shot_at_=now_;feedback_.shot=true;pulse(1,1);
        auto ray=shot_ray();V normal=rotate(board_.orientation,{0,0,1}),d=rotate(ray.orientation,{0,0,-1});double den=dot(d,normal);if(std::abs(den)>1e-7){double t=dot(sub(board_.position,ray.position),normal)/den;auto p=add(ray.position,mul(d,t));auto at=relative(board_,{p,{}}).position;if(t>0&&std::abs(at.x)<1.5&&std::abs(at.y)<1.5){++hits_;impacts_.push_back({p,now_});if(impacts_.size()>80)impacts_.erase(impacts_.begin());}}
        if(is("lever")){cycle_needed_=true;chamber_=false;lever_.reset();status_="握住护木，右手下压并回位上弹";}
        else if(!loaded_&&!is("beam")){chamber_=false;if(is("turret"))auto_reload_at_=now_+2.5;}
    }
public:
    void configure(Json config){config_=std::move(config);kind_=config_.at("kind");capacity_=config_.at("capacity");}
    void reset(P gun,P head,bool empty=false){auto config=config_;*this=ArmoryPractice{};configure(config);gun_=home_=gun;loaded_=empty?0:config_.value("initial_loaded",capacity_);reserve_=config_.value("reserve",0u);chamber_=loaded_>0;cycle_needed_=is("lever")&&empty;body_={head.position,axis({0,1,0},std::atan2(-rotate(head.orientation,{0,0,-1}).x,-rotate(head.orientation,{0,0,-1}).z))};slots_.reset(gun,head);board_=compose(body_,{{0,-.1,-10},{}});for(unsigned i=0;i<6;++i)chambers_[i]=i<loaded_;status_="右手握持拿枪；左手操作，左肩取弹";}
    void resume(PracticeSettings const& s,std::uint64_t epoch){settings_=s;epoch_=epoch;previous_ns_=0;interrupt();slots_.settings(s);}
    void settings(PracticeSettings const& s){settings_=s;slots_.settings(s);}
    void body(P p){body_=p;slots_.body(p);}void reposition(P p){gun_=p;home_=p;}
    void interrupt(){main_=false;release_off(true);for(auto& b:grip_)b={};for(auto& b:trigger_)b={};ready_trigger_=false;charge_at_=0;right_history_=false;lever_.reset();}
    P delta(std::string const& id)const{if(id=="mechanism")return mechanism_delta(mech_);if(id=="ammo"){if(off_==2)return compose(relative(gun_,ammo_),inverse(feed()));if(is("revolver"))return mechanism_delta(mech_);}return {};}
    P bone_delta(unsigned bone)const{if(int(bone)==config_.value("magazine_bone",-1)&&off_==2&&mag_seated_)return delta("ammo");P d{};for(auto const& b:config_.at("mechanism_bones"))if(b.get<unsigned>()==bone)d=mechanism_delta(mech_);return d;}
    bool bone_visible(unsigned bone)const{if(is("revolver")&&int(bone)==config_.value("cylinder_bone",-1)&&off_==2)return false;if(int(bone)==config_.value("magazine_bone",-1)){if(is("breech"))return loaded_>0&&off_!=2;return mag_seated_;}return true;}
    P gun()const{return gun_;}P board()const{return board_;}P magazine()const{return ammo_;}bool main_held()const{return main_;}int off_part()const{return off_;}
    bool mag_seated()const{return mag_seated_;}bool mag_visible()const{return mag_visible_;}bool motor()const{return false;}bool chambered()const{return chamber_&&!cycle_needed_&&!opened_;}
    unsigned loaded()const{return loaded_;}unsigned reserve()const{return reserve_;}unsigned shots()const{return shots_;}unsigned hits()const{return hits_;}unsigned cycles()const{return cycles_;}
    double charge_progress()const{return mech_;}double starter_progress()const{return 0;}bool ammo_held()const{return off_==2;}unsigned held_quantity()const{return held_qty_;}
    bool beam_active()const{return is("beam")&&main_&&trigger_[1].down&&ready_trigger_;}bool is_grenade()const{return grenade();}
    std::string const& status()const{return status_;}std::array<P,2> const& hand_wrists()const{return visible_;}std::array<int,2> const& hand_parts()const{return part_;}
    PracticeFeedback const& feedback()const{return feedback_;}std::vector<PracticeImpact> const& impacts()const{return impacts_;}auto const& throws()const{return throws_;}
    auto const& chambers()const{return chambers_;}
    std::string const& kind()const{return kind_;}
    bool lever_returning()const{return lever_.phase()==gears_weapon::LeverPhase::return_home;}
    unsigned guide_stage()const{if(!main_)return 0;if(off_==2)return 2;if(is("turret"))return off_==1?0:1;if(grenade()||is("beam"))return 0;if(is("lever")&&cycle_needed_)return off_==1?3:1;if(magazine_kind()){if(!mag_seated_)return 4;if(is("pistol")&&cycle_needed_)return 3;if(is("magazine")&&opened_&&inserted_rounds_)return 3;if(!loaded_)return is("magazine")&&!opened_?3:2;}if(opened_){if(inserted_rounds_||!reserve_)return 3;return 4;}if(!loaded_)return is("bow")||is("lever")?4:3;return 0;}
    std::string guide_text()const{unsigned stage=guide_stage();if(stage==4)return is("revolver")?"左肩 · 按握持取弹巢":"左肩 · 按握持取弹";if(stage==2)return off_==2?"对准黄框，送入弹药":"抓住弹匣，向下拔出";if(stage==1)return is("turret")?"左手握住机枪柄":"左手握住护木";if(stage==3){if(is("revolver"))return opened_?"左手掌靠近，按一下 Grip 合上":"抓住弹巢，向左拉出";if(is("lever"))return lever_returning()?"抬回原位，完成上膛":"右手大幅下压";if(is("bolt"))return opened_&&loaded_?"推栓向前，再转下锁定":"提起栓柄，再向后拉";if(is("pistol"))return "拉套筒到底，松手回位";return opened_&&loaded_?"关闭机构":"抓住机构并打开";}return !main_?"右手握持拿起":grenade()?"真实松 Grip 投出":is("beam")?"右扳机持续照射":"右扳机射击";}
    P shoulder()const{return slots_.shoulder();}bool shoulder_contains(V p)const{return slots_.shoulder_contains(p);}V shoulder_adjust_for(V p)const{return slots_.shoulder_adjust_for(p);}
    Frame frame()const{Frame f{};f.object=gun_;return f;}
    P visual_gun()const{double r=settings_.recoil_degrees*std::exp(-std::max(0.,now_-shot_at_)*25)*3.141592653589793/180;return compose(gun_,{{0,0,r*.10},axis({1,0,0},r)});}
    P shot_ray()const{return compose(gun_,{axis_value("muzzle",{0,.07,-.7}),{}});}
    void acknowledge_render(std::uint64_t serial){if(serial>render_serial_){render_serial_=serial;observed_mech_=mech_;if(pistol_waiting_&&observed_mech_>.95){fully_open_=true;pistol_waiting_=false;return_from_=mech_;return_at_=now_;}}}
    void step(PracticeFrame const& f,std::vector<Part> const& parts){
        feedback_={};if(parts.empty()||!f.focused||f.time_ns<=0||!valid(f.head)){interrupt();return;}
        if(previous_ns_&&(f.time_ns<=previous_ns_||f.time_ns-previous_ns_>100'000'000||f.epoch!=epoch_)){interrupt();previous_ns_=0;}
        dt_=previous_ns_?double(f.time_ns-previous_ns_)/1e9:0;previous_ns_=f.time_ns;epoch_=f.epoch;now_=double(f.time_ns)/1e9;++sequence_;
        // Stable torso heading plus actual head translation, as in the Lancer workbench.
        V shift=sub(f.head.position,body_.position);body_.position=f.head.position;slots_.body(body_);slots_.settings(settings_);(void)shift;
        std::array<bool,2> gp{},tp{};part_={-1,-1};
        for(unsigned s=0;s<2;++s){tracked_[s]=f.controllers[s].valid&&valid(f.controllers[s].grip);wrist_[s]=compose(f.controllers[s].grip,controller_wrist_mount(s,s==1&&main_));visible_[s]=wrist_[s];gp[s]=grip_[s].update(tracked_[s]?1:0,f.controllers[s].squeeze,.65,.35);tp[s]=trigger_[s].update(tracked_[s]?1:0,f.controllers[s].trigger,.65,.35);}
        bool released=main_&&tracked_[1]&&!grip_[1].down;
        if(main_&&(!tracked_[1]||!grip_[1].down||(main_aim_&&!f.controllers[1].aim_valid))){if(grenade()&&released&&!f.suppress_actions&&right_history_&&loaded_){throws_.push_back({gun_,throw_velocity_,now_});--loaded_;++shots_;auto_reload_at_=now_+.7;feedback_.shot=true;}main_=false;release_off(true);charge_at_=0;}
        if(!main_&&gp[1]&&!f.suppress_actions&&(!grenade()||loaded_)&&candidate(0,1,f,parts)){grab_origin_=gun_;grab_at_=now_;main_=true;main_aim_=f.controllers[1].aim_valid;main_mount_=wrist_local(parts[0],1);ready_trigger_=!trigger_[1].down;wrist_[1]=quest_main_wrist(f.controllers[1].grip,main_aim_?f.controllers[1].aim:f.controllers[1].grip,main_mount_);status_=is("turret")?"左右手分别握住两只机枪柄":"已握住，右扳机射击，左肩取弹";pulse(1,.3f);}
        if(main_){
            if(main_aim_)wrist_[1]=quest_main_wrist(f.controllers[1].grip,f.controllers[1].aim,main_mount_);
            P target=compose(wrist_[1],inverse(main_mount_));
            if(is("turret")){gun_=home_;if(off_==1&&tracked_[0]){V line=sub(f.controllers[1].grip.position,f.controllers[0].grip.position);V initial=rotate(home_.orientation,{1,0,0});Q q=arc(initial,unit(line));gun_.orientation=v::normalized(v::multiply(q,home_.orientation));}}
            else if(is("lever")&&off_==1&&tracked_[0])gun_=compose(f.controllers[0].grip,support_mount_);
            else if(off_==1&&config_.value("shared_grip",false))gun_=target;
            else if(off_==1&&settings_.two_hand&&tracked_[0]){auto fore=relative(main_mount_,wrist_local(parts[1],0));P r=wrist_[1],l=wrist_[0];r.position=mul(r.position,100);l.position=mul(l.position,100);gears_weapon_two_hand::Solution solution{};if(gears_weapon_two_hand::solve(r,l,fore,100,solution))target=compose({mul(solution.right.position,.01),solution.right.orientation},inverse(main_mount_));gun_=target;}
            else{double a=settings_.snap_enabled&&settings_.snap_seconds>0?std::clamp((now_-grab_at_)/settings_.snap_seconds,0.,1.):1.;a=a*a*(3-2*a);gun_=mix(grab_origin_,target,a);}
            part_[1]=0;visible_[1]=compose(gun_,main_mount_);
        }
        if(off_>=0&&(!main_||!tracked_[0]||!down(0,off_control_)))release_off(!main_||!tracked_[0]);
        if(main_&&off_<0&&!f.suppress_actions&&(gp[0]||tp[0])){
            off_control_=gp[0]?0:1;
            if(shoulder_contains(f.controllers[0].grip.position)&&parts.size()>2)take_ammo(parts);
            else{
                int selected=-1;double best=1e9;for(unsigned i=1;i<parts.size();++i){if(i==2&&(!magazine_kind()||!mag_visible_||!mag_seated_))continue;if(!candidate(i,0,f,parts))continue;double d=length(sub(compose(gun_,volume(i,parts).local).position,f.controllers[0].grip.position));if(d<best){selected=int(i);best=d;}}
                if(selected==3&&is("revolver")&&opened_&&gp[0]){close_mechanism();}
                else if(selected==2){if(is("magazine")&&!opened_){status_="先拉开弹匣锁扣";}else{off_=2;old_ammo_=true;held_qty_=0;ammo_=compose(gun_,feed());ammo_in_wrist_=relative(wrist_[0],ammo_);ammo_wrist_=relative(feed(),relative(delta(parts[2].id),wrist_local(parts[2],0)));status_="抓住旧匣，向下拔出";}}
                else if(selected>0){off_=selected;mechanism_start_=f.controllers[0].grip;mechanism_gun_=gun_;if(off_==3){if(!opened_)mechanism_home_q_=relative(gun_,mechanism_start_).orientation;else mechanism_start_.orientation=compose(gun_,{{},mechanism_home_q_}).orientation;}if(off_==3&&opened_){mechanism_start_.position=sub(mechanism_start_.position,rotate(gun_.orientation,mul(axis_value("open_axis",{0,0,1}),config_.value("travel",.1)*mech_)));}if(off_==1){support_mount_=relative(f.controllers[0].grip,gun_);support_start_=f.controllers[0].grip;right_home_=relative(gun_,f.controllers[1].grip);lever_.reset();lever_.arm(1,1,epoch_);}pulse(0,.25f);}
            }
        }
        update_insertion();update_mechanism(f,parts);
        if(main_&&is("lever")&&off_==1&&tracked_[0]){
            auto relative_right=relative(gun_,f.controllers[1].grip);double press=right_home_.position.y-relative_right.position.y;double angle=std::clamp(press/.16*.65,0.,.70);mech_=angle/.65;part_[1]=parts.size()>3?3:0;visible_[1]=wrist_[1];
            gears_weapon::LeverPoseSample sample{1,1,epoch_,sequence_,f.time_ns,angle,true,true,true};if(press>.30||std::abs(relative_right.position.x-right_home_.position.x)>.20){lever_.reset();}else if(lever_.phase()==gears_weapon::LeverPhase::unarmed)lever_.arm(1,1,epoch_);else if(lever_.observe(sample,gears_weapon::original_gnasher_lever_travel)){++cycles_;cycle_needed_=false;chamber_=loaded_>0;reload_at_=-1;inserted_rounds_=0;mech_=0;status_="杠杆回位，上膛完成";pulse(1,.65f);feedback_.click=true;}
        }
        if(off_>=0&&unsigned(off_)<parts.size()){part_[0]=off_;visible_[0]=off_==2?compose(ammo_,ammo_wrist_):compose(gun_,wrist_local(parts[off_],0));if(!settings_.grab_pose)visible_[0]=wrist_[0];}
        if(tracked_[1]){if(right_history_&&dt_>0&&dt_<.10)throw_velocity_=mul(sub(f.controllers[1].grip.position,last_right_),1/dt_);last_right_=f.controllers[1].grip.position;right_history_=true;}else right_history_=false;
        for(auto& t:throws_){t.velocity.y-=9.81*dt_;t.pose.position=add(t.pose.position,mul(t.velocity,dt_));if(t.pose.position.y<.1){t.pose.position.y=.1;t.velocity={};}}throws_.erase(std::remove_if(throws_.begin(),throws_.end(),[&](auto const& t){return now_-t.born>5;}),throws_.end());
        if(auto_reload_at_>=0&&now_>=auto_reload_at_){unsigned q=std::min(capacity_-loaded_,reserve_);loaded_+=q;reserve_-=q;chamber_=loaded_>0;auto_reload_at_=-1;gun_=home_;ready_trigger_=false;}
        if(!trigger_[1].down)ready_trigger_=true;if(f.suppress_actions){ready_trigger_=false;charge_at_=0;}
        bool firing=main_&&tracked_[1]&&ready_trigger_&&!f.suppress_actions&&!grenade();
        if(is("bow")){if(firing&&tp[1])charge_at_=now_;if(charge_at_>0&&!trigger_[1].down&&firing){shoot();charge_at_=0;}if(charge_at_>0&&now_-charge_at_>6.5){charge_at_=0;ready_trigger_=false;status_="蓄力取消，请松扳机重新开始";}}
        else if(firing&&trigger_[1].down&&now_>=next_shot_){next_shot_=now_+config_.value("fire_interval",.1)*settings_.fire_interval/.09;shoot();}
    }
};
}
