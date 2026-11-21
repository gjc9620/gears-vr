    bool advanced_open_{},yellow_guides_{true};
    std::string concise(std::string text,unsigned count=28)const{unsigned n=0;std::size_t i=0;while(i<text.size()&&n<count){unsigned char c=text[i];i+=c<128?1:c<224?2:c<240?3:4;++n;}return i<text.size()?text.substr(0,i)+"…":text;}
    std::string short_weapon(std::string const& model)const{static char const* ids[]={"Lancer.glb","Hammerburst.glb","Gnasher.glb","COG_Pistol.glb","Boltock.glb","Longshot.glb","Boomshot.glb","Torque_Bow.glb","Hammer_of_Dawn.glb","Troika_turret.glb","Frag_Grenade.glb","Smoke_Grenade.glb"};static char const* names[]={"电锯步枪","兽族步枪","杠杆霰弹枪","Snub 手枪","Boltok 左轮","栓动狙击枪","Boomshot","扭力弓","黎明之锤","Troika 机枪","破片手雷","烟雾手雷"};for(unsigned i=0;i<12;++i)if(model==ids[i])return names[i];return model;}
    void capture_button(unsigned side){capture_side_=edit_side_=side;if(operator_side_==side)throw std::runtime_error("请用另一只手点击采集");if(preview_mode_){go_live();capture_due_=live_.time_ns+20'000'000;return;}immediate_capture();}
    void build_ui(){
        buttons_.clear();if(testing_){build_practice_ui();return;}
        if(page_==5){
            unsigned pages=std::max(1u,unsigned((models_.size()+11)/12));if(weapon_page_>=pages)weapon_page_=0;for(unsigned i=0;i<12&&weapon_page_*12+i<models_.size();++i){auto model=models_[weapon_page_*12+i];ui(short_weapon(model),panel_,-.21+.21*(i%3),-.02-.09*(i/3),.197,.072,[this,model]{choose_model(model);},model==model_?Color{.08f,.38f,.42f,1}:Color{.09f,.15f,.22f,1},.019);}
            ui("测试手感",panel_,0,-.44,.61,.095,[this]{enter_practice();},{.06f,.43f,.40f,1},.031);if(pages>1){ui("上一页",panel_,-.16,-.55,.25,.045,[this]{if(weapon_page_)--weapon_page_;});ui("下一页",panel_,.16,-.55,.25,.045,[this,pages]{if(weapon_page_+1<pages)++weapon_page_;});}return;
        }
        ui("‹ 武器库",panel_,-.23,.115,.15,.044,[this]{go_live();page_=5;inspector_open_=false;},{.10f,.16f,.23f,1},.018);
        if(page_!=1)ui("保存",panel_,.245,.115,.12,.044,[this]{save_project();},{.10f,.23f,.28f,1},.019);
        std::vector<unsigned> entries;for(unsigned i=0;i<parts_.size();++i)if(!generic_model()||generic_info_.at("parts")[i].value("editor_visible",true))entries.push_back(i);for(unsigned slot=0;slot<entries.size();++slot){unsigned i=entries[slot];double w=.61/entries.size();ui(i==2&&(!generic_model()||generic_info_.at("practice").at("kind")!="revolver")?"子弹":label(i),panel_,-.305+w*(slot+.5),.015,w-.008,.057,[this,i]{preview_current(i);},i==part_?Color{.1f,.37f,.41f,1}:Color{.10f,.16f,.23f,1},.017);}
        if(page_==2){
            auto items=visible_captures();unsigned pages=unsigned((items.size()+3)/4);if(library_page_>=std::max(pages,1u))library_page_=0;
            for(unsigned row=0;row<4;++row){unsigned at=library_page_*4+row;if(at>=items.size())break;unsigned index=items[at];auto const& item=captures_[index];ui(item.name,panel_,0,-.10-.075*row,.6,.061,[this,index]{preview_capture(index);advanced_open_=true;}, {.10f,.18f,.25f,1},.020);}
            ui("上一页",panel_,-.20,-.42,.18,.046,[this]{if(library_page_)--library_page_;});ui("回到编辑",panel_,0,-.42,.18,.046,[this]{go_live();page_=0;});ui("下一页",panel_,.20,-.42,.18,.046,[this,pages]{if(library_page_+1<pages)++library_page_;});build_inspector();return;
        }
        if(page_==1){build_practice_settings();ui("‹ 返回",panel_,.245,.115,.12,.044,[this]{page_=0;});return;}
        ui("采集左手",panel_,-.155,-.13,.295,.095,[this]{capture_button(0);},{.12f,.29f,.41f,1},.030);
        ui("采集右手",panel_,.155,-.13,.295,.095,[this]{capture_button(1);},{.12f,.29f,.41f,1},.030);
        if(ammo_editing()){ui("移到操作手前",panel_,-.155,-.215,.295,.043,[this]{place_ammo_near_hand();},{.11f,.23f,.30f,1},.019);ui("恢复摆放",panel_,.155,-.215,.295,.043,[this]{restore_ammo_placement();},{.11f,.23f,.30f,1},.019);}
        ui("测试手感",panel_,0,-.29,.61,.103,[this]{enter_practice();},{.06f,.43f,.40f,1},.034);
        ui(advanced_open_?"高级设置  ▴":"高级设置  ▾",panel_,0,-.405,.61,.052,[this]{advanced_open_=!advanced_open_;inspector_open_=advanced_open_;}, {.075f,.12f,.18f,1},.019);
        if(advanced_open_){
            ui("校准左手",panel_,-.21,-.48,.19,.046,[this]{calibration_target_=0;calibration_due_=live_.time_ns+2'000'000'000ll;message_="展开左手，两秒后校准";},{.11f,.18f,.25f,1},.017);
            ui("校准右手",panel_,0,-.48,.19,.046,[this]{calibration_target_=1;calibration_due_=live_.time_ns+2'000'000'000ll;message_="展开右手，两秒后校准";},{.11f,.18f,.25f,1},.017);
            ui("姿势库",panel_,.21,-.48,.19,.046,[this]{page_=2;},{.11f,.18f,.25f,1},.017);
            ui("反馈 / 左肩",panel_,-.155,-.54,.295,.043,[this]{page_=1;},{.11f,.18f,.25f,1},.017);
            ui(yellow_guides_?"黄框引导：开":"黄框引导：关",panel_,.155,-.54,.295,.043,[this]{yellow_guides_=!yellow_guides_;},{.11f,.18f,.25f,1},.017);
            build_inspector();
        }
    }
    void render_editor(M4 vp){
        auto card=compose(panel_,{{0,-.225,-.018},{}});renderer_.box(card,{.68,.87,.018},{.027f,.046f,.071f,.98f},vp);
        renderer_.box(compose(panel_,{{-.332,-.225,0},{}}),{.004,.80,.006},{.12f,.68f,.64f,1},vp);
        renderer_.text(page_==5?"武器工作台":short_weapon(model_),compose(panel_,{{-.30,.19,.012},{}}),.032,{.9f,.95f,1,1},vp);
        renderer_.text(page_==5?"选择武器 · 位置与朝向保持不变":label(part_)+"  ·  "+(selected().known[edit_side_]?"姿势就绪":"等待采集"),compose(panel_,{{-.30,.075,.012},{}}),.018,{.43f,.68f,.76f,1},vp);
        if(page_==0&&!ammo_editing()){std::string hint=ammo_editing()?"子弹独立编辑 · 枪体已隐藏":(!calibration_[0].known&&!calibration_[1].known)?"首次使用：在高级设置中展开手掌校准":"一只手摆姿势，另一只手点击采集";renderer_.text(hint,compose(panel_,{{-.29,-.211,.012},{}}),.019,{.63f,.76f,.83f,1},vp);}
        if(page_==0&&ammo_editing())renderer_.text("侧握拿起 · 移动旋转 · 松手固定",compose(panel_,{{-.28,-.354,.012},{}}),.017,{.63f,.76f,.83f,1},vp);
        renderer_.text(concise(message_,30),compose(panel_,{{-.30,-.605,.012},{}}),.018,{.86f,.77f,.49f,1},vp);
        if(inspector_open_&&page_!=5){double bottom=inspector_tab_==0?-.32:inspector_tab_==1?(volume_detail_?-.535:-.45):-.55;renderer_.box(compose(inspector_,{{0,(.11+bottom)/2,-.016},{}}),{.36,.13-bottom,.014},{.035f,.06f,.085f,1},vp);renderer_.text(label(part_),compose(inspector_,{{-.16,.10,.012},{}}),.022,{.7f,.91f,.95f,1},vp);auto anchor=compose(shown_.object,displayed_volume(part_,shown_).local).position;renderer_.line(anchor,compose(inspector_,{{-.175,0,0},{}}).position,.0011,part_color(part_,.45f),vp);}
        for(unsigned i=0;i<buttons_.size();++i)renderer_.button(buttons_[i],vp,std::find(hover_.begin(),hover_.end(),int(i))!=hover_.end());
        for(unsigned id=0;id<4;++id)if(pointer_valid_[id]){auto p=pointers_[id];V end=add(p.position,rotate(p.orientation,{0,0,-1.2}));if(hover_[id]>=0&&unsigned(hover_[id])<buttons_.size()){auto const& b=buttons_[hover_[id]];double t=ray_rectangle(p,b.pose,b.width,b.height);if(t>=0)end=add(p.position,rotate(p.orientation,{0,0,-t}));}renderer_.line(p.position,end,.0011,hover_[id]>=0?Color{.55f,1,.85f,1}:Color{.3f,.65f,.8f,.55f},vp);renderer_.ball(end,.0035,{.6f,1,.85f,1},vp);}
    }
