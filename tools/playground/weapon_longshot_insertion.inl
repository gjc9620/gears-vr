// Raw physical axial travel plus the same current alignment used by the
// displayed round/hand. Cosmetic interpolation never supplies axial motion.
bool inserted(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(last_feed_ns&&!gears_weapon_longshot_round_presentation::same_context(feed_source,f)){
        feed_sequence=0;last_feed_ns=0;approached=false;
    }
    if(f.controllers.sequence<=feed_sequence)return false;
    round::presentation::Sample sample{};P root{},part{};double size{};
    if(!round::presentation_sample(f,d,sample)||!bolt::bones(f,root,part,size)||
       !gears_weapon_longshot_round_presentation::same_pose(root,sample.root)||size!=sample.scale){
        approached=false;last_feed_ns=0;return false;
    }
    const double units=nativelancerfollow::bound_units;
    if(units!=sample.units||sample.port.x!=port_local.x||sample.port.y!=port_local.y||sample.port.z!=port_local.z){
        approached=false;last_feed_ns=0;return false;
    }
    // This is the UNASSISTED tip in physical metres relative to the chamber.
    const auto tip=bolt::mul(bolt::sub(sample.measurement.raw_tip_local_m,port_local),size);
    const double depth=sample.measurement.raw_depth_m;
    const auto axis_world=gears_weapon::rotate(sample.display.orientation,{0,1,0});
    const auto axis=gears_weapon::rotate(gears_vr::inverse(root.orientation),axis_world);
    const double angle=std::acos((std::clamp)(axis.x,-1.,1.));
    const auto displayed_tip=bolt::sub(bolt::local(root,bolt::point(sample.display,{0,.15*units,0}),units),bolt::mul(port_local,size));
    const double lateral=std::hypot(displayed_tip.y,displayed_tip.z);
    const auto now=f.controllers.sync_ns;const auto old_ns=last_feed_ns;const auto previous=last_tip;
    feed_sequence=f.controllers.sequence;last_feed_ns=now;last_tip=tip;feed_source=f;
    if(!old_ns||now<=old_ns||now-old_ns>100000000||snub_visual::length(bolt::sub(tip,previous))>3.*double(now-old_ns)/1e9){approached=false;return false;}
    if(!sample.inside||std::abs(-displayed_tip.x-depth)>1e-8){approached=false;return false;}
    if(depth>=.025){approached=true;return false;}
    return approached&&depth<=.006&&depth>=-.006&&lateral<=.015&&angle<=3.14159265358979323846/18;
}
