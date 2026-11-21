// Application member functions. Kept separate from OpenXR lifecycle/input plumbing.
    Color part_color(unsigned i,float alpha=1)const{
        static constexpr Color colors[]={{.2f,.65f,1,1},{.2f,.86f,.55f,1},{.76f,.43f,1,1},{1,.8f,.18f,1},{1,.43f,.16f,1}};
        auto c=colors[i%5];if(generic_model()&&i>=5){double h=std::fmod(i*.61803398875,1.)*6;float x=float(.35+.55*(1-std::abs(std::fmod(h,2.)-1)));switch(int(h)){case 0:c={.9f,x,.35f,1};break;case 1:c={x,.9f,.35f,1};break;case 2:c={.35f,.9f,x,1};break;case 3:c={.35f,x,.9f,1};break;case 4:c={x,.35f,.9f,1};break;default:c={.9f,.35f,x,1};}}c.a=alpha;return c;
    }
    std::string label(unsigned i)const{if(generic_model()&&i<generic_info_.at("parts").size()){std::string text=generic_info_.at("parts")[i].at("label");auto at=text.find(" · ");return text.substr(0,at);}static char const* names[]={"主握把","副手握持","弹匣","拉栓 E","电锯 C"};return names[i%5];}
    P facing(V position)const{
        auto z=unit(sub(live_.head.position,position)),x=unit(cross({0,1,0},z)),y=cross(z,x);
        if(length(x)<.5)return {position,panel_.orientation};
        v::Matrix m{{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}}};return {position,matrix_quaternion(m)};
    }
    void go_live(){if(preview_mode_&&preview_dirty_){capture_side_=edit_side_;archive_current_pose();}paused_=playing_=frozen_known_=preview_mode_=false;snapped_={};active_capture_.clear();}
    void select_part(unsigned i){if(i!=2)finish_ammo_stage();if(preview_mode_)go_live();part_=i;inspector_open_=advanced_open_;preview_mode_=false;rotation_drag_={};ui_drag_pointer_=-1;edit_side_=capture_side_=i==0?1:0;if(generic_model()){std::string name=generic_info_.at("parts")[i].at("label");if(generic_info_.at("parts")[i].value("side",0u)==1||name.find("右")!=std::string::npos||name.find("_R")!=std::string::npos)edit_side_=capture_side_=1;}library_page_=0;message_=generic_model()?generic_info_.at("parts")[i].at("label").get<std::string>():"已选择"+label(i)+"，控件在部件旁";}
    void tab(unsigned i){page_=i;ui_drag_pointer_=-1;rotation_drag_={};}
    void ui(std::string text,P origin,double x,double y,double w,double h,std::function<void()> action,Color color={.14f,.23f,.31f,1},double font=.022){
        Button b{std::move(text),compose(origin,{{x,y,.006},{}}),w,h,color,std::move(action)};b.font_size=font;buttons_.push_back(std::move(b));
    }
    void slider(std::string text,P origin,double y,double value,std::function<void(double)> change,double width=.30){
        Button b{std::move(text),compose(origin,{{0,y,.007},{}}),width,.052,{.10f,.18f,.24f,1},{}};b.value=std::clamp(value,0.,1.);b.change=std::move(change);b.font_size=.017;buttons_.push_back(std::move(b));
    }
    void immediate_capture(){
        if(!paused_&&capture_side_!=2&&operator_side_==capture_side_)throw std::runtime_error("请用另一只手点击采集，让这只手保持姿势");
        if(capture_side_==2&&!paused_){capture_due_=live_.time_ns+3'000'000'000ll;message_="三秒后采集双手";return;}
        capture_pose();
    }
    std::vector<unsigned> visible_captures()const{std::vector<unsigned> out;for(std::size_t i=captures_.size();i>0;--i)if(captures_[i-1].part==part_)out.push_back(unsigned(i-1));return out;}
    void preview_capture(unsigned index){if(index>=captures_.size()||!captures_[index].ready)throw std::runtime_error("姿势正在保存，请稍后");auto const c=captures_[index];part_=c.part;capture_side_=c.side;edit_side_=c.side==2?0:c.side;load_project(data_+"/captures/"+c.id+".json",true);preview_dirty_=false;active_capture_=c.id;inspector_open_=true;inspector_tab_=2;}
    void set_mechanical(unsigned i,double fraction){if(model_!="Lancer.glb")throw std::runtime_error("该模型没有原版机械预览");go_live();if(i==4)live_.starter_angle=std::clamp(fraction,0.,1.)*3.141592653589793/2;else if(i==3)live_.charging_distance=std::clamp(fraction,0.,1.)*.23041412353515625;}
    void rotate_volume(double radians){if(preview_mode_)preview_dirty_=true;auto& a=selected().volume.local.orientation;a=v::normalized(v::multiply(a,axis(axis_vector(axis_),radians)));}
    void move_volume(double amount){if(preview_mode_)preview_dirty_=true;auto& p=selected().volume.local.position;if(axis_==0)p.x=std::clamp(p.x+amount,-2.,2.);else if(axis_==1)p.y=std::clamp(p.y+amount,-2.,2.);else p.z=std::clamp(p.z+amount,-2.,2.);}
    void build_inspector(){
        if(!inspector_open_)return;
        auto anchor=compose(shown_.object,displayed_volume(part_,shown_).local).position;
        // The card stays attached to the selected part; only its facing tracks the viewer.
        if(!inspector_placed_||(!inspector_pinned_&&ui_drag_pointer_<0&&!rotation_drag_.active)){auto local=relative(workspace_,{anchor,{}}).position;V target{local.x+inspector_distance_,local.y+.30,local.z-.25};inspector_=facing(compose(workspace_,{target,{}}).position);inspector_placed_=true;}
        ui(inspector_pinned_?"已固定":"跟随",inspector_,.015,.065,.09,.035,[this]{inspector_pinned_=!inspector_pinned_;},{.13f,.23f,.28f,1},.015);ui("收起",inspector_,.12,.065,.085,.035,[this]{inspector_open_=false;advanced_open_=false;rotation_drag_={};}, {.17f,.22f,.26f,1},.016);
        static char const* modes[]={"部件","区域","手型"};for(unsigned i=0;i<3;++i)ui(modes[i],inspector_,-.106+.106*i,0,.10,.04,[this,i]{inspector_tab_=i;rotation_drag_={};ui_drag_pointer_=-1;},inspector_tab_==i?Color{.2f,.4f,.48f,1}:Color{.09f,.16f,.2f,1},.018);
        if(inspector_tab_==0){
            if(generic_model()){generic_controls();return;}
            if(part_==4){slider("电锯握柄 "+number(shown_.starter_angle*180/3.14159265,0)+" 度",inspector_,-.07,shown_.starter_angle/(3.141592653589793/2),[this](double x){set_mechanical(4,x);});
                for(unsigned i=0;i<3;++i)ui(std::to_string(i*45)+" 度",inspector_,-.106+.106*i,-.14,.1,.043,[this,i]{set_mechanical(4,i*.5);}, {.17f,.25f,.29f,1},.018);
            }else if(part_==3){slider("拉栓行程 "+number(shown_.charging_distance*1000,0)+" mm",inspector_,-.07,shown_.charging_distance/.23041412353515625,[this](double x){set_mechanical(3,x);});ui("前端",inspector_,-.08,-.14,.14,.043,[this]{set_mechanical(3,0);});ui("后端",inspector_,.08,-.14,.14,.043,[this]{set_mechanical(3,1);});}
            ui("编辑此处吸附区域",inspector_,0,-.22,.30,.048,[this]{inspector_tab_=1;}, {.15f,.27f,.31f,1},.019);ui("查看此处已采集姿势",inspector_,0,-.285,.30,.048,[this]{tab(2);}, {.15f,.27f,.31f,1},.018);
        }else if(inspector_tab_==1){
            for(unsigned i=0;i<3;++i)ui(i==0?"X":i==1?"Y":"Z",inspector_,-.106+.106*i,-.065,.1,.04,[this,i]{axis_=i;},axis_==i?Color{.24f,.45f,.51f,1}:Color{.12f,.20f,.26f,1},.018);
            auto& a=selected().volume;
            if(!volume_detail_){
                ui("转 -5度",inspector_,-.08,-.12,.14,.042,[this]{rotate_volume(-3.141592653589793/36);}, {.13f,.25f,.30f,1},.018);ui("转 +5度",inspector_,.08,-.12,.14,.042,[this]{rotate_volume(3.141592653589793/36);}, {.13f,.25f,.30f,1},.018);
                ui("移 -5mm",inspector_,-.08,-.175,.14,.042,[this]{move_volume(-.005);}, {.13f,.25f,.30f,1},.018);ui("移 +5mm",inspector_,.08,-.175,.14,.042,[this]{move_volume(.005);}, {.13f,.25f,.30f,1},.018);
                ui("区域移到左手",inspector_,0,-.235,.3,.044,[this]{P hand{};if(controllers_.hands[0].grip_valid)hand=controllers_.hands[0].grip;else if(valid(live_.hands[0]))hand=live_.hands[0].joints[0].pose;else throw std::runtime_error("请伸出左手或左手柄，再用右手点击");selected().volume.local.position=relative(compose(shown_.object,current_part_delta(part_,shown_)),hand).position;if(preview_mode_)preview_dirty_=true;},{.13f,.25f,.30f,1},.018);
                ui("形状 "+a.shape,inspector_,0,-.29,.3,.044,[this]{if(preview_mode_)preview_dirty_=true;auto& s=selected().volume.shape;s=s=="sphere"?"capsule":s=="capsule"?"box":"sphere";}, {.13f,.25f,.30f,1},.018);
                ui("大小与吸附参数",inspector_,0,-.345,.30,.044,[this]{volume_detail_=true;});
                ui("保存区域",inspector_,0,-.4,.30,.044,[this]{save_project();}, {.12f,.37f,.29f,1},.019);
            }else{
                double size=a.shape=="box"?(axis_==0?a.half.x:axis_==1?a.half.y:a.half.z):a.radius;
                slider("半尺寸 "+number(size*1000,0)+" mm",inspector_,-.13,(size-.005)/.245,[this](double x){if(preview_mode_)preview_dirty_=true;auto& v=selected().volume;double r=.005+x*.245;if(v.shape!="box")v.radius=r;else if(axis_==0)v.half.x=r;else if(axis_==1)v.half.y=r;else v.half.z=r;});
                if(a.shape=="capsule")slider("胶囊半长 "+number(a.half_length*1000,0)+" mm",inspector_,-.20,a.half_length/.5,[this](double x){if(preview_mode_)preview_dirty_=true;selected().volume.half_length=x*.5;});
                slider("Snap距离 "+number(a.snap_distance*1000,0)+" mm",inspector_,-.27,(a.snap_distance-.005)/.295,[this](double x){if(preview_mode_)preview_dirty_=true;selected().volume.snap_distance=.005+x*.295;});
                slider("Snap角度 "+number(a.snap_angle*180/3.14159265,0)+" 度",inspector_,-.34,a.snap_angle/3.141592653589793,[this](double x){if(preview_mode_)preview_dirty_=true;selected().volume.snap_angle=x*3.141592653589793;});
                slider("释放距离 "+number(a.release_distance*1000,0)+" mm",inspector_,-.41,(a.release_distance-.005)/.495,[this](double x){if(preview_mode_)preview_dirty_=true;selected().volume.release_distance=.005+x*.495;});
                ui("位置旋转",inspector_,-.08,-.485,.14,.044,[this]{volume_detail_=false;}, {.13f,.25f,.30f,1},.018);ui("保存",inspector_,.08,-.485,.14,.044,[this]{save_project();}, {.12f,.37f,.29f,1},.019);
            }
        }else{
            ui(edit_side_?"编辑右手":"编辑左手",inspector_,0,-.065,.3,.042,[this]{edit_side_=1-edit_side_;});
            ui("手指",inspector_,-.08,-.12,.14,.04,[this]{hand_tool_=0;},hand_tool_==0?Color{.22f,.43f,.48f,1}:Color{.11f,.19f,.24f,1},.019);
            ui("手腕",inspector_,.08,-.12,.14,.04,[this]{hand_tool_=1;},hand_tool_==1?Color{.22f,.43f,.48f,1}:Color{.11f,.19f,.24f,1},.019);
            if(hand_tool_==0){
                static constexpr unsigned fingers[]={4,0,1,2,3};static char const* finger_names[]={"拇","食","中","无","小"};
                for(unsigned f=0;f<5;++f){unsigned finger=fingers[f];ui(finger_names[f],inspector_,-.128+.064*f,-.18,.058,.04,[this,finger]{joint_=finger*3+joint_%3;},joint_/3==finger?Color{.22f,.43f,.48f,1}:Color{.11f,.19f,.24f,1},.019);}
                static char const* joints[]={"根节","中节","末节"};for(unsigned j=0;j<3;++j)ui(joints[j],inspector_,-.106+.106*j,-.235,.1,.04,[this,j]{joint_=joint_/3*3+j;},joint_%3==j?Color{.22f,.43f,.48f,1}:Color{.11f,.19f,.24f,1},.018);
                for(unsigned i=0;i<3;++i)ui(i==0?"X":i==1?"Y":"Z",inspector_,-.106+.106*i,-.29,.1,.04,[this,i]{axis_=i;},axis_==i?Color{.24f,.45f,.51f,1}:Color{.12f,.20f,.26f,1},.018);
                auto d=selected().finger_degrees[edit_side_][joint_];double degrees=axis_==0?d.x:axis_==1?d.y:d.z;
                slider("关节旋转 "+number(degrees,0)+" 度",inspector_,-.355,(degrees+90)/180,[this](double x){if(!selected().known[edit_side_])throw std::runtime_error("请先采集这只手");if(preview_mode_)preview_dirty_=true;auto& d=selected().finger_degrees[edit_side_][joint_];double angle=x*180-90;if(axis_==0)d.x=angle;else if(axis_==1)d.y=angle;else d.z=angle;});
                ui("-5度",inspector_,-.08,-.425,.14,.044,[this]{edit_joint(-5);});ui("+5度",inspector_,.08,-.425,.14,.044,[this]{edit_joint(5);});
            }else{
                ui("平移",inspector_,-.08,-.18,.14,.04,[this]{rotate_wrist_=false;},!rotate_wrist_?Color{.22f,.43f,.48f,1}:Color{.11f,.19f,.24f,1},.019);ui("旋转",inspector_,.08,-.18,.14,.04,[this]{rotate_wrist_=true;},rotate_wrist_?Color{.22f,.43f,.48f,1}:Color{.11f,.19f,.24f,1},.019);
                for(unsigned i=0;i<3;++i)ui(i==0?"X":i==1?"Y":"Z",inspector_,-.106+.106*i,-.235,.1,.04,[this,i]{axis_=i;},axis_==i?Color{.24f,.45f,.51f,1}:Color{.12f,.20f,.26f,1},.018);
                auto d=rotate_wrist_?selected().wrist_degrees[edit_side_]:selected().wrist_offset[edit_side_];double value=axis_==0?d.x:axis_==1?d.y:d.z;
                slider(std::string(rotate_wrist_?"旋转 ":"平移 ")+number(rotate_wrist_?value:value*1000,0)+(rotate_wrist_?" 度":" mm"),inspector_,-.31,rotate_wrist_?(value+90)/180:(value+.15)/.3,[this](double x){if(!selected().known[edit_side_])throw std::runtime_error("请先采集这只手");if(preview_mode_)preview_dirty_=true;auto& d=rotate_wrist_?selected().wrist_degrees[edit_side_]:selected().wrist_offset[edit_side_];double value=rotate_wrist_?x*180-90:x*.3-.15;if(axis_==0)d.x=value;else if(axis_==1)d.y=value;else d.z=value;});
                ui(rotate_wrist_?"-5度":"-2mm",inspector_,-.08,-.385,.14,.044,[this]{offset(-.002);});ui(rotate_wrist_?"+5度":"+2mm",inspector_,.08,-.385,.14,.044,[this]{offset(.002);});
            }
            ui("另存修正",inspector_,-.08,-.50,.145,.05,[this]{if(!selected().known[edit_side_])throw std::runtime_error("先采集这只手");capture_side_=edit_side_;archive_current_pose();}, {.10f,.37f,.29f,1},.018);
            ui(generic_model()?"采用此姿势":"采用并测试",inspector_,.08,-.50,.145,.05,[this]{if(preview_mode_){parts_[part_]=preview_part_;preview_dirty_=false;}enter_practice();}, {.10f,.37f,.29f,1},.018);
        }
    }
