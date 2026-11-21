    void initialize_thumbnails(){
        thumbnails_.clear();thumbnails_.resize(parts_.size());
        for(unsigned part=0;part<parts_.size();++part){auto& thumb=thumbnails_[part];thumb.hands=renderer_.load_hands();
            if(generic_model()){Mesh const* source=&part_mesh(part);thumb.context.vertices=source->vertices;thumb.context.indices=source->indices;if(!thumb.context.indices.empty())renderer_.upload(thumb.context);continue;}if(model_!="Lancer.glb"||part>=3)continue;
            auto center=parts_[part].volume.local.position;
            auto append=[&](Mesh const& source){for(std::size_t k=0;k<source.indices.size();k+=3){V mean{};for(unsigned j=0;j<3;++j){auto const& v=source.vertices[source.indices[k+j]];mean=add(mean,{v.x/3.,v.y/3.,v.z/3.});}if(length(sub(mean,center))>.16)continue;for(unsigned j=0;j<3;++j){thumb.context.indices.push_back(unsigned(thumb.context.vertices.size()));thumb.context.vertices.push_back(source.vertices[source.indices[k+j]]);}}};
            append(gun_);if(part==2)append(components_[2]);if(!thumb.context.vertices.empty())renderer_.upload(thumb.context);
        }
    }
    void destroy_thumbnails(){for(auto& t:thumbnails_){for(auto& h:t.hands)renderer_.destroy(h.mesh);renderer_.destroy(t.context);t.context={};t.known={};} }
    void refresh_thumbnails(){
        if(page_!=4)return;
        for(unsigned part=0;part<parts_.size();++part){auto const& p=preview_mode_&&part_==part?preview_part_:parts_[part];auto& t=thumbnails_[part];
            for(unsigned side=0;side<2;++side){if(!p.known[side]){t.known[side]=false;continue;}
                auto local=p.poses[side].local;for(unsigned j=0;j<15;++j)local[j]=v::normalized(v::multiply(local[j],correction(p.finger_degrees[side][j])));
                bool changed=!t.known[side];for(unsigned j=0;j<15;++j){auto a=local[j],b=t.last[side][j];if(std::abs(a.x-b.x)+std::abs(a.y-b.y)+std::abs(a.z-b.z)+std::abs(a.w-b.w)>1e-9)changed=true;}
                if(changed){renderer_.skin(t.hands[side],local);t.last[side]=local;}
                t.known[side]=true;t.wrists[side]=generic_model()&&p.poses[side].part_delta_known?compose(current_part_delta(part,shown_),relative(p.poses[side].part_delta,p.poses[side].wrist_in_object)):p.poses[side].wrist_in_object;t.wrists[side].orientation=v::normalized(v::multiply(t.wrists[side].orientation,correction(p.wrist_degrees[side])));t.wrists[side].position=add(t.wrists[side].position,rotate(t.wrists[side].orientation,p.wrist_offset[side]));
            }
        }
    }
    void preview_current(unsigned part){
        
        if(preview_mode_&&part_==part){inspector_open_=advanced_open_;inspector_tab_=2;return;}
        select_part(part);auto const& current=parts_[part];unsigned side=current.known[part==0?1:0]?(part==0?1:0):(current.known[0]?0:1);
        if(!current.known[side]){tab(0);message_="此部件尚未采集，摆好手后点采集";return;}
        preview_part_=current;preview_mode_=true;preview_dirty_=false;capture_side_=edit_side_=side;paused_=true;playing_=false;frozen_=live_;frozen_.hands={};
        for(unsigned s=0;s<2;++s)if(current.known[s]){auto const& pose=current.poses[s];auto& h=frozen_.hands[s];h.active=true;h.data_source=pose.data_source;for(unsigned j=0;j<26;++j){h.joints[j]=pose.raw_in_object[j];h.joints[j].pose=compose(frozen_.object,h.joints[j].pose);}}
        frozen_known_=true;inspector_tab_=2;message_="正在预览"+label(part)+"，修改后点另存修正姿势";
    }
    void render_thumbnails(M4 vp){
        if(page_!=4)return;
        for(unsigned part=0;part<parts_.size();++part){if(part/5!=part_page_)continue;auto const& p=preview_mode_&&part_==part?preview_part_:parts_[part];auto const& t=thumbnails_[part];unsigned slot=part%5;double x=slot%2?.15:-.15,y=-.16-double(slot/2)*.14;
            P card=compose(panel_,{{x,y,0},{}});renderer_.text(label(part),compose(card,{{-.005,.048,.015},{}}),.021,part_color(part),vp);
            renderer_.text(std::string("左 ")+(p.known[0]?(p.poses[0].authored_default?"游戏默认":"已采"):"未采"),compose(card,{{-.005,.013,.015},{}}),.014,p.known[0]?Color{.5f,1,.76f,1}:Color{.45f,.54f,.62f,1},vp);
            renderer_.text(std::string("右 ")+(p.known[1]?(p.poses[1].authored_default?"游戏默认":"已采"):"未采"),compose(card,{{-.005,-.012,.015},{}}),.014,p.known[1]?Color{.5f,1,.76f,1}:Color{.45f,.54f,.62f,1},vp);
            renderer_.text("点开微调",compose(card,{{-.005,-.039,.015},{}}),.014,{.65f,.78f,.88f,1},vp);
            Q view_rotation=v::multiply(axis({1,0,0},-.25),axis({0,1,0},part==0?-1.1:1.1));
            V lo{1e9,1e9,1e9},hi{-1e9,-1e9,-1e9};auto bound=[&](V a){a=rotate(view_rotation,a);lo={std::min(lo.x,a.x),std::min(lo.y,a.y),std::min(lo.z,a.z)};hi={std::max(hi.x,a.x),std::max(hi.y,a.y),std::max(hi.z,a.z)};};
            unsigned active=p.known[part==0?1:0]?(part==0?1:0):p.known[0]?0:1;Frame state=shown_;if(p.known[active])captured_geometry(part,p.poses[active],state);
            Mesh const* context=part>=3&&model_=="Lancer.glb"?&components_[part==4?0:1]:&t.context;P component=model_=="Lancer.glb"&&part>=3?component_transform(part==4?0:1,state):generic_model()?current_part_delta(part,state):P{};
            for(auto const& a:context->vertices)bound(compose(component,{{a.x,a.y,a.z},{}}).position);
            if(model_=="Lancer.glb"&&part==2)for(auto const& a:components_[3].vertices)bound(compose(state.magazine_delta,{{a.x,a.y,a.z},{}}).position);
            for(unsigned side=0;side<2;++side)if(t.known[side])for(auto const& a:t.hands[side].mesh.vertices)bound(compose(t.wrists[side],{{a.x,a.y,a.z},{}}).position);
            if(lo.x>hi.x)continue;V extent=sub(hi,lo),center=mul(add(lo,hi),.5);double scale=std::min({.125/std::max(extent.x,.001),.095/std::max(extent.y,.001),.04/std::max(extent.z,.001)});
            P group=compose(card,{{-.073,0,.042},view_rotation});group.position=sub(group.position,rotate(card.orientation,mul(center,scale)));
            auto scaled=[&](P local){return P{add(group.position,rotate(group.orientation,mul(local.position,scale))),v::normalized(v::multiply(group.orientation,local.orientation))};};
            if(!context->indices.empty())renderer_.mesh(*context,scaled(component),{.31f,.4f,.46f,1},vp,scale);
            if(model_=="Lancer.glb"&&part==2)renderer_.mesh(components_[3],scaled(state.magazine_delta),{.31f,.4f,.46f,1},vp,scale);
            for(unsigned side=0;side<2;++side)if(t.known[side])renderer_.mesh(t.hands[side].mesh,scaled(t.wrists[side]),part_color(part),vp,scale);
        }
    }
