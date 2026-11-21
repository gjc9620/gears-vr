    void render_workshop_ammo(M4 vp){
        if(!generic_model())return;auto const& cfg=generic_info_.at("practice");std::string kind=cfg.at("kind");
        auto render_item=[&](P item,unsigned){if(ammo_mesh_.indices.empty())return;renderer_.mesh(ammo_mesh_,item,kind=="revolver"?Color{.45f,.52f,.58f,1}:kind=="lever"?Color{.76f,.15f,.07f,1}:Color{.76f,.58f,.25f,1},vp);};
        if(testing_){
            if(practice_.ammo_held()&&(!practice_.mag_seated()||(kind!="magazine"&&kind!="pistol")))render_item(practice_.magazine(),kind=="revolver"?practice_.held_quantity():1);
            if(generic_info_.at("practice").at("kind")=="bow"&&practice_.loaded()&&!practice_.ammo_held())render_item(compose(practice_.gun(),{vector(cfg.at("feed")),{}}),1);
            if(practice_.beam_active()){auto ray=practice_.shot_ray();renderer_.line(ray.position,add(ray.position,rotate(ray.orientation,{0,0,-10})),.005,{.35f,.9f,.45f,.8f},vp);}
            if(practice_.armory().is_grenade())for(auto const& item:practice_.armory().throws())renderer_.mesh(gun_,item.pose,{.45f,.51f,.55f,1},vp);
        }else if(part_<parts_.size()&&parts_[part_].id=="ammo"){auto pose=compose(shown_.object,compose(current_part_delta(part_,shown_),{vector(cfg.at("feed")),{}}));render_item(pose,1);}
    }
    void render_yellow_guide(M4 vp){
        if(!yellow_guides_||!testing_)return;
        unsigned index=0;bool shoulder=false,insert=false;std::string text="右手握持拿枪";
        if(practice_.generic()){auto const& sim=practice_.armory();auto stage=sim.guide_stage();index=stage;shoulder=stage==4;insert=stage==2&&sim.ammo_held();text=sim.guide_text();}
        else if(!practice_.main_held()){index=0;}
        else if(!practice_.mag_seated()){if(practice_.off_part()==2){insert=true;text="送入弹匣井";}else{shoulder=true;text="左肩 · 按握持取新匣";}}
        else if(!practice_.chambered()&&practice_.loaded()){index=3;text="抓住拉栓，向后拉并松手";}
        else if(!practice_.loaded()){index=2;text="拔出旧匣";}
        else if(practice_.off_part()==4){index=4;text=practice_.motor()?"电锯已启动 · 回拉停机":"向前推满滑柄";}
        else{index=1;text="左手握护木 · 右扳机射击";}
        Volume box{};box.shape="box";box.half={.065,.065,.065};P world{};
        if(shoulder){world=practice_.shoulder();box.half={.13,.16,.16};}
        else if(insert){P feed=practice_.generic()?P{vector(generic_info_.at("practice").at("feed")),{}}:magazine_seat();if(practice_.generic()&&practice_.armory().kind()=="revolver")feed=compose(practice_.part_delta("mechanism"),feed);world=compose(practice_.gun(),feed);}
        else{index=std::min(index,unsigned(parts_.size()-1));auto actual=displayed_volume(index,shown_);box=actual;world=compose(shown_.object,actual.local);box.local={};}
        box.local=world;renderer_.volume(box,P{},{1,.80f,.10f,.95f},vp);renderer_.text(text,facing(add(world.position,{-.10,.115,0})),.023,{1,.87f,.3f,1},vp);
        if(practice_.generic()&&generic_info_.at("practice").at("kind")=="lever"&&practice_.off_part()==1){auto start=controllers_.hands[1].grip.position;auto direction=rotate(practice_.gun().orientation,{0,practice_.armory().lever_returning()?.16:-.16,0});auto end=add(start,direction);renderer_.line(start,end,.004,{1,.82f,.15f,1},vp);renderer_.ball(end,.011,{1,.82f,.15f,1},vp);}
    }
