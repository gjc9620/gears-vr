// Included after mounted Scene tracking. Called on the admitted Scene GT while
// the existing stereo lock is held. No camera/controller/actor memory writer.
namespace troika_view_live {
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
bool prepare(gears_anchor::Receipt const* scene,gears_vr::StereoResult const& stereo,
    gears_vr::StereoMatrix& output) noexcept {
    namespace source=troika_scene_tracking;
    try{
        if(!scene||scene->thread!=GetCurrentThreadId()||!stereo)return false;
        gears_troika_geometry::Source expected{};
        {
            std::lock_guard<std::mutex> lock(source::mutex);
            if(source::stopped.load()||!source::active||!source::basis_retained||!source::mapping_ready||
               std::memcmp(scene,&source::receipt,sizeof *scene)||
               std::memcmp(&stereo.sample,&source::value.stereo,sizeof stereo.sample)||
               stereo.reference_generation!=source::value.reference_generation||
               !gears_vr::stereo_same_pose(stereo.reference,source::value.reference))return false;
            expected.frame=source::value;expected.entry_binding=source::retained_basis.owner.binding;
            expected.entry_token=source::retained_basis.owner.token;
        }
        // Includes current original possession, Driver, both meshes, weapon,
        // identities, reference/session and the unchanged source age checks.
        // Viewing does not require Grip, a geometry contact, or a fire lease.
        if(!source::geometry_current(expected))return false;
        std::lock_guard<std::mutex> lock(source::mutex);
        if(source::stopped.load()||!source::active||!source::basis_retained||!source::mapping_ready||
           std::memcmp(&expected.frame,&source::value,sizeof expected.frame)||
           expected.entry_binding!=source::retained_basis.owner.binding||
           expected.entry_token!=source::retained_basis.owner.token||
           source::mapped_value.units_per_metre!=stereo_units_per_metre)return false;
        gears_vr::StereoMatrix candidate{};
        if(!source::entry_space.render_base(source::retained_basis.owner,expected.frame,
            rig_tracking_live::clock_ns(),candidate))return false;
        output=candidate;return true;
    }catch(...){return false;}
}
#endif
} // namespace troika_view_live
