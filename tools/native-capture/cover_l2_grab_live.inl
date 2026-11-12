// Original-L2 world reference collector, included after cover_input_live.
// Caller holds stereo_mutex and the current Scene GT admission.
namespace cover_l2_grab_live {
namespace input=cover_input_live;
struct Diagnostics {
    uint64_t attempts{},ready{},rejected{},hand_missing{};
    uint64_t point_frame{};int64_t point_sample_ns{};
    gears_vr::CoverEpochs previous{};
    gears_cover_l2_points::Result points{};
    gears_vr::CoverEdgeObservation selected{};
} diagnostics;
bool same_cover(gears_vr::CoverEpochs const& a,gears_vr::CoverEpochs const& b){
    return a.world==b.world&&a.pawn==b.pawn&&a.cover==b.cover;
}
bool same_grip(gears_vr::CoverEpochs const& a,gears_vr::CoverEpochs const& b){
    return same_cover(a,b)&&a.slot==b.slot&&a.session==b.session&&a.space==b.space&&a.reference==b.reference;
}
bool prepare(gears_anchor::Receipt const* scene,cover_neutral_eye_live::EyeSample const& eye,
    gears_vr::CoverInputOwnerFrame& frame){
    ++diagnostics.attempts;diagnostics.selected={};frame.cover.edge={};frame.cover.range={};
    cover_geometry_live::Geometry geometry{};gears_cover_l2_points::Result points{};
    if(!frame.cover.cover_valid||!frame.cover.interaction_enabled||!gears_vr::cover_view_basis(frame.anchor)||
       !std::isfinite(frame.units_per_metre)||frame.units_per_metre<10||frame.units_per_metre>500||
       !cover_geometry_live::sample_l2_with(scene,eye,geometry,points)){
        ++diagnostics.rejected;return false;
    }
    gears_vr::CoverL2GrabPoints references{};references.enabled=true;references.cover_type=points.type;
    references.lean_left=geometry.selected.permissions[1].value;
    references.lean_right=geometry.selected.permissions[2].value;
    references.can_popup=geometry.selected.permissions[3].value;
    gears_vr::CoverRange range{};
    for(unsigned i=0;i<3;++i){
        const auto p=points.points[i];references.points[i]={{p.x,p.y,p.z},points.available[i]};
        if(!gears_vr::cover_l2_allowed(references,i))continue;
        const double right=((p.x-points.neutral.x)*frame.anchor.right_world.x+
            (p.y-points.neutral.y)*frame.anchor.right_world.y)/frame.units_per_metre;
        const double up=(p.z-points.neutral.z)/frame.units_per_metre;
        // Original positions define displacement limits. These are not
        // surface/collision bounds and do not restrict physical HMD motion.
        if(i==0&&right<0)range.min_right=right;
        else if(i==1&&right>0)range.max_right=right;
        else if(i==2&&up>0)range.max_up=up;
        else references.points[i].available=false;
    }
    frame.cover.range=range;
    const auto& previous=input::diagnostics.latest.interaction;
    const auto offset=same_cover(diagnostics.previous,frame.cover.epochs)?previous.offset:gears_vr::CoverOffset{};
    const bool latched=same_grip(diagnostics.previous,frame.cover.epochs)&&
        (previous.phase==gears_vr::CoverPhase::grabbing||previous.phase==gears_vr::CoverPhase::requesting);
    diagnostics.previous=frame.cover.epochs;diagnostics.points=points;
    diagnostics.point_frame=frame.frame;diagnostics.point_sample_ns=frame.now_ns;++diagnostics.ready;
    if(!frame.controllers){++diagnostics.hand_missing;return true;}
    const auto& hand=frame.controllers->hands[0];gears_vr::CoverViewResult view{};gears_vr::Vector world{};
    if(!gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)||
       !gears_vr::prepare_cover_view(frame.anchor,offset,frame.units_per_metre,view)||
       !gears_vr::cover_tracking_point(frame.anchor,view,frame.stereo.reference,hand.grip.position,frame.units_per_metre,world)){
        ++diagnostics.hand_missing;return true;
    }
    frame.cover.edge=gears_vr::cover_l2_select(references,world,frame.units_per_metre,.18,latched?previous.edge_id:0);
    diagnostics.selected=frame.cover.edge;return true;
}
void summary(FILE* file){
    if(!file)return;const auto& p=diagnostics.points;
    fprintf(file,"cover_l2_reference_calls=%llu,%llu,%llu,%llu cover_l2_reference_type=%u cover_l2_reference_neutral=%.9g,%.9g,%.9g ",
        diagnostics.attempts,diagnostics.ready,diagnostics.rejected,diagnostics.hand_missing,unsigned(p.type),p.neutral.x,p.neutral.y,p.neutral.z);
    fprintf(file,"cover_l2_reference_last_sample=%llu,%lld,%llu,%llu,%llu,%llu ",
        diagnostics.point_frame,diagnostics.point_sample_ns,diagnostics.previous.world,diagnostics.previous.pawn,
        diagnostics.previous.cover,diagnostics.previous.slot);
    for(unsigned i=0;i<3;++i)fprintf(file,"cover_l2_reference_point_%u=%u,%.9g,%.9g,%.9g ",i,
        unsigned(p.available[i]),p.points[i].x,p.points[i].y,p.points[i].z);
}
struct Registration {Registration(){input::grab_points_provider=prepare;input::grab_point_summary=summary;}} registration;
} // namespace cover_l2_grab_live
