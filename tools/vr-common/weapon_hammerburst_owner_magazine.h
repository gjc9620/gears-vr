#pragma once
#include "hand_authored_mount_profile.h"

namespace gears_weapon_hammerburst_owner_magazine {
// Captured owner pose plus a separately derived authoring variant.
// This is not a new capture or a recovered original attachment.
inline constexpr char source_capture_id[]="1794499115840-2";
inline constexpr char source_capture_sha256[]="932cf42cad6afbc56b5a3bd914261ea48274c4b5cdd62096e39060eed77de940";
inline constexpr char source_model_sha256[]="eec7ca09e7055b03f2a1b96b65baa6bc90c8dda1f505e5677926bcf6bf8dd3be";
inline constexpr char source_rig_signature[]="3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193";
inline constexpr char static_mesh_sha256[]="b7e5f647b1aa290e22a3a22658e22a27ad3f845ac1cc50f3773a9dfe3f3e078a";
inline constexpr char skeletal_mesh_sha256[]="4b6bd78407481887e6efcd80532fd64b7108dce2ee6c05d259e19e8b44c0152b";
inline constexpr char registration_authority[]="derived authoring registration";
inline constexpr char authored_variant_id[]="owner-ammo-static-fit-stage0";
inline constexpr char authored_variant_sha256[]="03284ade261745ab3f5d025c055b25c709b39e9fd78ced14310ec770398ed091";
inline constexpr char authored_variant_authority[]="derived authoring variant; not a new measured capture";
inline constexpr double registration_rms_raw=0.8006123285606352;
inline constexpr double registration_max_raw=2.921689884298664;
inline constexpr double source_units_per_raw=0.01;
inline constexpr double source_part_scale=1.0;

// Original captured wrist, unchanged.
inline constexpr gears_vr::Pose wrist_in_weapon_raw={{8.3502681976420465,24.003369325068384,-12.918992103433114},{0.4110567988604179,0.8426789837007499,0.24494209575217438,-0.2468355895494613}};
inline constexpr gears_vr::Pose wrist_in_static={{-1.8073731059149765,8.2749750807698437,14.217499616833772},{-0.003963720394389701,0.35184336959648244,-0.3037386548753776,0.88540011391806606}};
inline constexpr gears_vr::Pose static_in_bone={{0.1193668837873344,-0.14844923355440939,0.48147760844932541},{0.99997581466865226,0.0028042397889082335,-0.0056459369784999502,-0.0029376372489252501}};
inline constexpr gears_vr::Pose bone_in_weapon={{2.01507701033423e-08,25.805870056152337,0.8443783521652295},{0,-2.0030705624396564e-16,0.70710678118654746,0.70710678118654768}};
inline constexpr gears_vr::Pose static_in_weapon={{0.14844925370517936,25.925236939939669,1.325855960614555},{0.70510678260393633,0.70907257654555655,-0.0060695035430300311,0.0019150571042678857}};

// Frozen authored correction in StaticMesh coordinates.
inline constexpr gears_vr::Pose authored_wrist_in_static={{-1.8573731059149765,8.7749750807698437,14.367499616833772},{0.0020557229945072182,0.36396881935761516,-0.25568203897124336,0.89562780633804639}};
inline constexpr double authored_inside_skin_vertices=0;
inline constexpr double authored_closest_surface_max_raw=0.1409649015502322;

inline constexpr gears_vr::Quaternion captured_finger_locals[15]={
    {0.063017091157246588,0.059802891644741361,-0.064829857494564005,-0.99410741368840083},
    {-0.020776558750199844,-0.036470649193655481,0.47267936438587554,-0.88023431246325401},
    {-0.017328389847494376,-0.034398448602314578,0.3335209460384051,-0.94195554682410276},
    {0.0052331342172500508,0.024158428203739989,-0.036333540832478591,-0.99903396261766975},
    {-0.06728825898962297,-0.011094860857732541,0.48895051764795172,-0.86964164203221828},
    {-0.068361460581323974,-0.033231369659812771,0.29795427889102477,-0.95154907097230157},
    {-0.087539348258789632,0.045284814494803059,-0.012680527713016638,-0.99505042701338364},
    {-0.050702962162941713,0.0088451641179475848,0.44068691300949014,-0.89618414257439072},
    {-0.028438721430314118,0.00064350415675875554,0.27230687488027355,-0.96178988917473451},
    {-0.095282864976165824,-0.011200485678881062,0.007470191917597911,-0.99535919194791445},
    {-0.094558505457942185,-0.042211564970836536,0.35134939510859042,-0.93050012110966462},
    {-0.038510203801325082,-0.026983755987723568,0.20493539682914894,-0.97764529571949299},
    {0.81823789372352451,-0.44187269326550149,0.078830751691311304,-0.35919491200317016},
    {-0.053436032083556288,-0.076330219061063673,-0.010247826898655213,-0.99559694162703727},
    {-0.10134012144952129,-0.047505362588727985,-0.057959778492358303,-0.99202524382549184}
};
inline constexpr gears_vr::Quaternion authored_finger_locals[15]={
    {0.065065793667734087,0.057567196573927913,-0.099484213302972552,-0.99123930091401014},
    {-0.020776558750199844,-0.036470649193655481,0.47267936438587554,-0.88023431246325401},
    {-0.017328389847494376,-0.034398448602314578,0.3335209460384051,-0.94195554682410276},
    {0.0035351798026197105,0.024464624476459119,0.033444052229700885,-0.99913486578311639},
    {-0.06728825898962297,-0.011094860857732541,0.48895051764795172,-0.86964164203221828},
    {-0.068361460581323974,-0.033231369659812771,0.29795427889102477,-0.95154907097230157},
    {-0.090485015779457792,0.039068066714308437,0.056761570401210944,-0.99351108308271319},
    {-0.050702962162941713,0.0088451641179475848,0.44068691300949014,-0.89618414257439072},
    {-0.028438721430314118,0.00064350415675875554,0.27230687488027355,-0.96178988917473451},
    {-0.095780248084865724,0.0055153707319311011,-0.16548560688884412,-0.98153463447558575},
    {-0.094558505457942185,-0.042211564970836536,0.35134939510859042,-0.93050012110966462},
    {-0.038510203801325082,-0.026983755987723568,0.20493539682914894,-0.97764529571949299},
    {0.80231831070517157,-0.47015960703363024,0.066247008480306818,-0.36172725372660613},
    {-0.053436032083556288,-0.076330219061063673,-0.010247826898655213,-0.99559694162703727},
    {-0.10134012144952129,-0.047505362588727985,-0.057959778492358303,-0.99202524382549184}
};

inline bool valid_pose(gears_vr::Pose const& p) noexcept {
    if(!gears_vr::finite(p.position)||!std::isfinite(p.orientation.x)||
       !std::isfinite(p.orientation.y)||!std::isfinite(p.orientation.z)||!std::isfinite(p.orientation.w))return false;
    const double n=p.orientation.x*p.orientation.x+p.orientation.y*p.orientation.y+
        p.orientation.z*p.orientation.z+p.orientation.w*p.orientation.w;
    return std::isfinite(n)&&n>.98&&n<1.02;
}
inline bool compose(gears_vr::Pose const& world,gears_vr::Pose const& local,gears_vr::Pose& out) noexcept {
    if(!valid_pose(world)||!valid_pose(local))return false;
    const auto shift=gears_vr::transform(gears_vr::rotation_matrix(world.orientation),local.position);
    gears_vr::Pose candidate{{world.position.x+shift.x,world.position.y+shift.y,world.position.z+shift.z},
        gears_vr::normalized(gears_vr::multiply(world.orientation,local.orientation))};
    if(!valid_pose(candidate))return false;out=candidate;return true;
}
inline bool wrist_world_from_static(gears_vr::Pose const& world,double scale,gears_vr::Pose& out) noexcept {
    if(!valid_pose(world)||!std::isfinite(scale)||scale<=0||scale>3)return false;
    gears_vr::Pose local=wrist_in_static;
    local.position={local.position.x*scale,local.position.y*scale,local.position.z*scale};
    return valid_pose(local)&&compose(world,local,out);
}
inline bool authored_wrist_world_from_static(gears_vr::Pose const& world,double scale,gears_vr::Pose& out) noexcept {
    if(!valid_pose(world)||!std::isfinite(scale)||scale<=0||scale>3)return false;
    gears_vr::Pose local=authored_wrist_in_static;
    local.position={local.position.x*scale,local.position.y*scale,local.position.z*scale};
    return valid_pose(local)&&compose(world,local,out);
}
inline bool static_world_from_bone(gears_vr::Pose const& bone,double scale,gears_vr::Pose& out) noexcept {
    if(!valid_pose(bone)||!std::isfinite(scale)||scale<=0||scale>3)return false;
    const auto offset=gears_vr::transform(gears_vr::rotation_matrix(bone.orientation),static_in_bone.position);
    gears_vr::Pose candidate{{bone.position.x+offset.x*scale,bone.position.y+offset.y*scale,bone.position.z+offset.z*scale},
        gears_vr::normalized(gears_vr::multiply(bone.orientation,static_in_bone.orientation))};
    if(!valid_pose(candidate))return false;out=candidate;return true;
}
inline bool grip_from_wrist(gears_vr::Pose const& wrist,double model_scale,double units,gears_vr::Pose& out) noexcept {
    if(!valid_pose(wrist))return false;
    gears_hand_authored_mount::Mount left{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,model_scale,units,left))return false;
    const auto rotation=gears_vr::normalized(gears_vr::multiply(wrist.orientation,gears_vr::inverse(left.calibration.grip_to_wrist_rotation)));
    const auto delta=gears_vr::transform(gears_vr::rotation_matrix(rotation),left.calibration.grip_to_wrist_metres);
    gears_vr::Pose candidate{{wrist.position.x-delta.x*units,wrist.position.y-delta.y*units,wrist.position.z-delta.z*units},rotation};
    if(!valid_pose(candidate))return false;out=candidate;return true;
}
} // namespace gears_weapon_hammerburst_owner_magazine
