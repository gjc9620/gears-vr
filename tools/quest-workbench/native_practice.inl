    unsigned practice_options_{};
    PracticeFrame practice_input_frame(){PracticeFrame f{};f.time_ns=live_.time_ns;f.epoch=live_.epoch;f.head=live_.head;f.eye=practice_eye_;f.focused=state_==XR_SESSION_STATE_FOCUSED;for(unsigned i=0;i<2;++i){auto const& c=controllers_.hands[i];f.controllers[i]={c.grip,c.grip_valid,c.squeeze,c.trigger,c.aim,c.aim_valid};}return f;}
    void enter_practice(bool empty=false,bool reset=false){
        if(!catalog_model())throw std::runtime_error("自定义模型可编辑姿势；需配置专用机制后才能测试");
        practice_.configure(generic_model()?generic_info_.at("practice"):Json{});go_live();finish_ammo_stage();grab_={};capture_due_=calibration_due_=pickup_due_=0;
        if(!practice_known_||reset){P gun=live_.object;if(controllers_.hands[1].grip_valid){auto wrist=compose(controllers_.hands[1].grip,controller_wrist_mount(1,true));auto local=effective_wrist(parts_[0],1,Frame{});if(controllers_.hands[1].aim_valid)wrist=quest_main_wrist(controllers_.hands[1].grip,controllers_.hands[1].aim,local);gun=compose(wrist,inverse(local));}practice_.reset(gun,live_.head,empty);practice_.body({live_.head.position,workspace_.orientation});practice_known_=true;}
        else practice_.reposition(live_.object);if(generic_model()){std::fill(generic_offset_.begin(),generic_offset_.end(),V{});std::fill(generic_angles_.begin(),generic_angles_.end(),V{});update_generic();}
        practice_.resume(practice_settings_,live_.epoch);test_menu_edges_={};click_edges_={};testing_=true;inspector_open_=false;message_="右握持拿枪；左肩取弹；A/X 回编辑";log("practice_enter source=standalone_quest_controllers");
    }
    void leave_practice(){if(!testing_)return;auto state=practice_.frame();practice_.interrupt();live_.object=practice_.gun();live_.starter_angle=state.starter_angle;live_.starter_pull=state.starter_pull;live_.charging_distance=state.charging_distance;live_.magazine_delta=state.magazine_delta;testing_=false;audio_.update(false,0,false,false,false);controllers_.stop_feedback();click_edges_={};page_=0;practice_cache_[model_]=practice_;message_="已回编辑，改好设置再进入测试";log("practice_exit");}
    void update_practice(){
        for(unsigned side=0;side<2;++side){auto const& c=controllers_.hands[side];if(test_menu_edges_[side].update(c.select_active?1:0,c.select?1:0)){leave_practice();return;}}
        auto input=practice_input_frame();for(auto const& c:controllers_.hands)if(c.aim_valid)for(auto const& b:buttons_)if(ray_rectangle(c.aim,b.pose,b.width,b.height)>=0)input.suppress_actions=true;
        practice_.settings(practice_settings_);practice_.step(input,parts_);if(generic_model())update_generic();auto state=practice_.frame();live_.object=practice_.gun();live_.starter_angle=state.starter_angle;live_.starter_pull=state.starter_pull;live_.charging_distance=state.charging_distance;live_.magazine_delta=state.magazine_delta;
        auto const& event=practice_.feedback();for(unsigned side=0;side<2;++side)if(practice_settings_.haptics)controllers_.vibrate(side,event.vibration[side]*float(practice_settings_.vibration));audio_.update(practice_settings_.audio,float(practice_settings_.audio_gain),practice_.motor(),event.shot,event.click||event.seat);message_=practice_.status();
    }
    void apply_practice_hands(){
        for(unsigned side=0;side<2;++side){hand_mesh_known_[side]=controllers_.hands[side].grip_valid;if(!hand_mesh_known_[side])continue;hand_pose_[side]=practice_.hand_wrists()[side];int part=practice_.hand_parts()[side];
            if(part>=0&&practice_settings_.grab_pose&&parts_[part].known[side]){hand_local_[side]=parts_[part].poses[side].local;for(unsigned j=0;j<15;++j){if(part==0&&side==1&&parts_[part].poses[side].authored_default)hand_local_[side][j]=mix(from_raw_joint(gears_lancer_main_hand::rest[j]),from_raw_joint(gears_lancer_main_hand::trigger[j]),controllers_.hands[side].trigger);hand_local_[side][j]=v::normalized(v::multiply(hand_local_[side][j],correction(parts_[part].finger_degrees[side][j])));}}
            else for(unsigned j=0;j<15;++j)hand_local_[side][j]=hands_[side].rig.reference[j+1].orientation;
            if(part>=0&&!(part==2&&!practice_.mag_seated()))hand_pose_[side]=compose(compose(practice_.visual_gun(),inverse(practice_.gun())),hand_pose_[side]);renderer_.skin(hands_[side],hand_local_[side]);
        }
    }
    P practice_hud()const{return compose(workspace_,{{-.48,-.10,-1.5},axis({0,1,0},.30)});}
    void build_practice_ui(){auto origin=practice_hud();ui("回编辑 A/X",origin,-.11,-.12,.20,.055,[this]{leave_practice();},{.10f,.32f,.34f,1},.020);ui(practice_settings_.show_regions?"隐藏区域":"显示区域",origin,.11,-.12,.20,.055,[this]{practice_settings_.show_regions=!practice_settings_.show_regions;},{.13f,.23f,.28f,1},.019);}
    void practice_input(){hover_.fill(-1);pointer_valid_={};for(unsigned side=0;side<2;++side){auto const& c=controllers_.hands[side];if(recenter_edges_[side].update(c.recenter_active?2:0,c.recenter?1:0)){reset_workspace();return;}unsigned id=side*2+1;bool press=click_edges_[id].update(c.aim_valid?2:0,std::max(double(c.trigger),c.select?1.:0.));if(!c.aim_valid)continue;pointers_[id]=c.aim;pointer_valid_[id]=true;for(unsigned i=0;i<buttons_.size();++i)if(ray_rectangle(c.aim,buttons_[i].pose,buttons_[i].width,buttons_[i].height)>=0){hover_[id]=int(i);if(press&&buttons_[i].action){buttons_[i].action();if(!testing_)return;}break;}}}
    void build_practice_settings(){practice_.configure(generic_model()?generic_info_.at("practice"):Json{});
        static char const* tabs[]={"流程","反馈","左肩补给"};for(unsigned i=0;i<3;++i)ui(tabs[i],panel_,-.20+.20*i,-.125,.185,.045,[this,i]{practice_options_=i;},{.12f,i==practice_options_?.4f:.22f,.32f,1},.020);
        ui("进入手柄测试 · A/X返回",panel_,0,-.215,.58,.075,[this]{enter_practice();},{.08f,.43f,.33f,1},.027);
        if(practice_options_==0){
            ui("默认弹量重置",panel_,-.15,-.31,.28,.05,[this]{enter_practice(false,true);},{.13f,.25f,.34f,1},.020);ui("空仓重置",panel_,.15,-.31,.28,.05,[this]{enter_practice(true,true);},{.13f,.25f,.34f,1},.020);
            ui(practice_settings_.snap_enabled?"吸附：开":"吸附：关",panel_,-.15,-.385,.28,.05,[this]{practice_settings_.snap_enabled=!practice_settings_.snap_enabled;});ui(practice_settings_.grab_pose?"Grab Pose：开":"Grab Pose：关",panel_,.15,-.385,.28,.05,[this]{practice_settings_.grab_pose=!practice_settings_.grab_pose;}, {.13f,.25f,.34f,1},.019);
            ui(practice_settings_.two_hand?"双手持枪：开":"双手持枪：关",panel_,-.15,-.46,.28,.05,[this]{practice_settings_.two_hand=!practice_settings_.two_hand;}, {.13f,.25f,.34f,1},.019);ui(practice_settings_.insertion_assist?"插匣辅助：开":"插匣辅助：关",panel_,.15,-.46,.28,.05,[this]{practice_settings_.insertion_assist=!practice_settings_.insertion_assist;}, {.13f,.25f,.34f,1},.019);
            ui(practice_settings_.show_regions?"区域提示：开":"区域提示：关",panel_,-.15,-.535,.28,.05,[this]{practice_settings_.show_regions=!practice_settings_.show_regions;}, {.13f,.25f,.34f,1},.019);ui(practice_settings_.sight_guide?"瞄准辅助线：开":"瞄准辅助线：关",panel_,.15,-.535,.28,.05,[this]{practice_settings_.sight_guide=!practice_settings_.sight_guide;}, {.13f,.25f,.34f,1},.017);
        }else if(practice_options_==1){
            slider("吸附过渡 "+number(practice_settings_.snap_seconds*1000,0)+" ms",panel_,-.29,practice_settings_.snap_seconds/.4,[this](double x){practice_settings_.snap_seconds=x*.4;},.58);
            slider("预览射速 "+number(60/(practice_settings_.fire_interval*(generic_model()?generic_info_.at("practice").value("fire_interval",.1)/.09:1)),0)+" RPM",panel_,-.355,(practice_settings_.fire_interval-.04)/.16,[this](double x){practice_settings_.fire_interval=.04+x*.16;},.58);
            slider("后坐力预览 "+number(practice_settings_.recoil_degrees,1)+" 度",panel_,-.42,practice_settings_.recoil_degrees/3,[this](double x){practice_settings_.recoil_degrees=x*3;},.58);
            slider("手柄振动 "+number(practice_settings_.vibration*100,0)+" %",panel_,-.485,practice_settings_.vibration,[this](double x){practice_settings_.vibration=x;},.58);
            slider("合成声音预览 "+number(practice_settings_.audio_gain*100,0)+" %",panel_,-.55,practice_settings_.audio_gain,[this](double x){practice_settings_.audio_gain=x;},.58);
        }else{
            if(!practice_known_){practice_.reset(live_.object,live_.head);practice_known_=true;}practice_.body({live_.head.position,workspace_.orientation});practice_.settings(practice_settings_);
            ui("用左手柄设置补给中心",panel_,-.15,-.31,.28,.05,[this]{if(!controllers_.hands[0].grip_valid)throw std::runtime_error("把左手柄放在左肩，再用右手点击");auto offset=practice_.shoulder_adjust_for(controllers_.hands[0].grip.position);if(length(offset)>.5)throw std::runtime_error("位置离左肩过远，请重新放置");practice_settings_.shoulder_adjust=offset;message_="左肩补给区已移动到左手柄处";},{.13f,.25f,.34f,1},.017);
            ui("恢复默认肩区",panel_,.15,-.31,.28,.05,[this]{practice_settings_.shoulder_adjust={};practice_settings_.shoulder_scale=1;}, {.13f,.25f,.34f,1},.019);
            slider("肩区大小 "+number(practice_settings_.shoulder_scale,2)+" 倍",panel_,-.385,(practice_settings_.shoulder_scale-.5)/1.5,[this](double x){practice_settings_.shoulder_scale=.5+x*1.5;},.58);
            for(unsigned a=0;a<3;++a)ui(a==0?"X 左右":a==1?"Y 高低":"Z 前后",panel_,-.20+.20*a,-.46,.185,.045,[this,a]{axis_=a;},{.13f,a==axis_?.4f:.22f,.32f,1},.019);
            auto p=practice_settings_.shoulder_adjust;double value=axis_==0?p.x:axis_==1?p.y:p.z;slider("肩区偏移 "+number(value*1000,0)+" mm",panel_,-.535,(value+.25)/.5,[this](double x){auto& p=practice_settings_.shoulder_adjust;double v=x*.5-.25;if(axis_==0)p.x=v;else if(axis_==1)p.y=v;else p.z=v;},.58);
        }
    }
    void render_shoulder(M4 vp){auto center=practice_.shoulder();auto radii=V{.2045*practice_settings_.shoulder_scale,.24*practice_settings_.shoulder_scale,.24*practice_settings_.shoulder_scale};for(unsigned plane=0;plane<3;++plane)for(unsigned i=0;i<64;++i){auto point=[&](double t){V p{};double a=std::cos(t),b=std::sin(t);if(plane==0)p={0,radii.y*a,radii.z*b};else if(plane==1)p={radii.x*a,0,radii.z*b};else p={radii.x*a,radii.y*b,0};return compose(center,{p,{}}).position;};renderer_.line(point(i*6.283185307179586/64),point((i+1)*6.283185307179586/64),.002,{.5f,.9f,1,.65f},vp);}renderer_.ball(center.position,.012,{.6f,.9f,1,1},vp);renderer_.text("左肩 · 弹药补给",facing(add(center.position,{0,.28,0})),.027,{.6f,.9f,1,1},vp);}
    void render_practice(M4 vp){
        render_yellow_guide(vp);
        auto hud=practice_hud();renderer_.box(compose(hud,{{0,-.035,-.012},{}}),{.46,.30,.012},{.04f,.075f,.09f,1},vp);renderer_.text("手柄手感测试",compose(hud,{{-.21,.10,.01},{}}),.025,{.8f,.95f,1,1},vp);
        renderer_.text(std::to_string(practice_.loaded())+" / "+std::to_string(practice_.reserve())+"  "+(practice_.chambered()?"已上膛":"需上膛"),compose(hud,{{-.21,.055,.01},{}}),.026,{1,.9f,.5f,1},vp);
        renderer_.text(practice_.generic()?practice_.armory().guide_text():practice_.motor()?"电锯运行中":"左肩取弹 · 拉栓 · 电锯",compose(hud,{{-.21,.01,.01},{}}),.019,{.5f,.9f,.75f,1},vp);
        for(unsigned i=0;i<buttons_.size();++i)renderer_.button(buttons_[i],vp,std::find(hover_.begin(),hover_.end(),int(i))!=hover_.end());
        for(unsigned side=0;side<2;++side){unsigned id=side*2+1;if(pointer_valid_[id]&&hover_[id]>=0){auto ray=pointers_[id];auto const& b=buttons_[hover_[id]];double t=ray_rectangle(ray,b.pose,b.width,b.height);if(t>=0){auto end=add(ray.position,rotate(ray.orientation,{0,0,-t}));renderer_.line(ray.position,end,.0013,{1,.8f,.2f,1},vp);renderer_.ball(end,.004,{1,.8f,.2f,1},vp);}}}
        P board=practice_.board();renderer_.box(compose(board,{{0,0,-.025},{}}),{3,3,.04},{.12f,.17f,.21f,1},vp);
        for(double radius:{.15,.35,.65,1.0,1.4})for(unsigned k=0;k<64;++k){double a=k*6.283185307179586/64,b=(k+1)*6.283185307179586/64;renderer_.line(compose(board,{{radius*std::cos(a),radius*std::sin(a),.005},{}}).position,compose(board,{{radius*std::cos(b),radius*std::sin(b),.005},{}}).position,.006,{.7f,.78f,.83f,1},vp);}
        renderer_.text("10m  "+std::to_string(practice_.hits())+" / "+std::to_string(practice_.shots()),compose(board,{{-1.35,1.4,.02},{}}),.13,{1,.8f,.3f,1},vp);
        for(auto const& hit:practice_.impacts())renderer_.ball(add(hit.point,rotate(board.orientation,{0,0,.016})),.025,{1,.3f,.13f,1},vp);
        auto feedback=practice_.feedback();if(feedback.shot&&!(practice_.generic()&&practice_.armory().is_grenade())){auto ray=practice_.shot_ray();renderer_.line(ray.position,add(ray.position,rotate(ray.orientation,{0,0,-15})),.003,{1,.8f,.25f,1},vp);renderer_.ball(ray.position,.023,{1,.7f,.2f,.8f},vp);}
        if(practice_settings_.sight_guide){auto f=practice_.shot_ray();renderer_.line(f.position,add(f.position,rotate(f.orientation,{0,0,-10})),.0012,{.25f,.9f,.65f,.65f},vp);}
        if(practice_settings_.show_regions)render_shoulder(vp);
        renderer_.text(practice_.status(),compose(hud,{{-.21,-.19,.01},{}}),.018,{1,.8f,.4f,1},vp);
    }
