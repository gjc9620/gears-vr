// Private geometry/observed-endpoint cache only. No mechanical/fire/inventory
// reset, source refresh, new hold/grant or original-object mutation occurs here.
// Route availability only, never input or action authority. Keep a changed
// profile out of re-reads of the Frame that was committed before it existed.
bool profile_route_ready(hand::Frame const& f) noexcept {
    return profile.known&&!hand::same(route_deferred_key,f.key);
}
bool profile_actor(Profile const& value,hand::Frame const& f) noexcept {
    return value.known&&inv::same_instance_across_hold(value.identity,inv::identity(f));
}
bool profile_cohort(Profile const& value,hand::Frame const& f) noexcept {
    const auto& s=value.source;
    return profile_actor(value,f)&&hand::same(s.controller,f.controller)&&
        s.stereo.session==f.stereo.session&&s.stereo.space_epoch==f.stereo.space_epoch&&
        s.controllers.input_epoch==f.controllers.input_epoch&&s.controllers.profile_generation==f.controllers.profile_generation&&
        s.reference_generation==f.reference_generation;
}
bool select_profile(hand::Frame const& f) noexcept {
    if(profile_actor(profile,f))return profile_cohort(profile,f);
    Profile target{};
    for(auto const& saved:saved_profiles)if(profile_actor(saved,f)){
        if(!profile_cohort(saved,f))return false;
        target=saved;break;
    }
    if(profile.known){
        Profile* destination{};
        for(auto& saved:saved_profiles)if(saved.known&&
            (inv::same_instance_across_hold(saved.identity,profile.identity)||
             inv::same_instance_across_hold(profile.identity,saved.identity))){destination=&saved;break;}
        if(!destination)for(auto& saved:saved_profiles)if(!saved.known){destination=&saved;break;}
        if(!destination)return false; // Never evict another instance's record.
        auto retained=profile;retained.retained_shot=shot;
        retained.retained_down=shot&&down_seen;
        *destination=retained;
    }
    profile=target;route_deferred_key=f.key;
    // These are derived from a current source/rig/animation, not reusable hand
    // permits. Rebind the curve and basis through the existing live readers.
    proposal=publication={};main={};decision_source={};committed_decision={};decision_known=false;
    main_required=basis_known=curve_known=false;observed_geometry={};main_in_component={};curve={};closed={};
    animation_serial=0;last_sequence=last_frame=0;
    shot=target.retained_shot;down_seen=target.retained_shot&&target.retained_down;
    return true;
}
