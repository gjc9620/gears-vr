#pragma once
#include "hand_authored_grasp.h"
#include "hand_marcus115_reference.h"

// Boltok production owner profiles derived from task04 accepted native data.
// Wrist: A.T * p_object * 100; rotation: A.T * R(q_object) * S.
// Fingers: [x,-y,-z,w]. Volumes: A.T * centre * 100; radii/snap stay in metres.
// This is static data, not a lease, input, contact check or live mapping.
namespace gears_boltok_owner_profiles {
namespace api=gears_hand_authored_grasp;
inline constexpr char source_sha256[]="a68a9b053bfde2f7133cca16e0dd2413f44ab90e779ec1a5016a90bc704fb4f2";
inline constexpr double authoring_raw_per_metre=100.;
struct Profile {
    const char* capture_id{};std::uint64_t source_time_ns{},profile_id{};
    gears_vr::Pose wrist_raw_object{},captured_delta_xr_object{};
    api::Joints fingers_native{};
    gears_vr::Vector volume_center_raw{};
    double volume_radius_metres{},snap_distance_metres{},snap_angle_radians{},release_distance_metres{};
};
inline constexpr Profile make_support() noexcept {
    Profile p{};
    p.capture_id="1794485865263-1";
    p.source_time_ns=622151298154ull;
    p.profile_id=0x424f4c544f4b0011ull;
    p.wrist_raw_object={{4.1445193001664133,3.1801881974416633,-2.7943037862158793},{0.64883461961704181,0.75230867477311925,-0.030568965090831495,-0.11004922816993029}};
    p.captured_delta_xr_object={{0,0,0},{0,0,0,1}};
    p.fingers_native[0]={0.046771019450684223,0.15177275525942566,0.30198685752750959,-0.93999012781048186};
    p.fingers_native[1]={0.062110297996126024,-0.036510373979543299,0.60021819583639535,-0.79658484850115552};
    p.fingers_native[2]={0.024520983944268052,-0.022906098349518646,0.32080089174551313,-0.94655206928097235};
    p.fingers_native[3]={-0.035835524955743561,0.063335507631000737,0.37926915394316951,-0.92242036918722803};
    p.fingers_native[4]={-0.017610636887618197,-0.0068591568833398066,0.56198716788262726,-0.82693000947512241};
    p.fingers_native[5]={-0.029595147102819876,-0.033233304463337887,0.30019056414933687,-0.95284064771509014};
    p.fingers_native[6]={-0.12086893924032488,0.051365368487884346,0.47773313003425272,-0.86863303812064308};
    p.fingers_native[7]={-0.023044485206201851,0.010947035110599522,0.49609668636435877,-0.86789238497763743};
    p.fingers_native[8]={-0.011528630627705009,0.0032792754551743348,0.29940050935885648,-0.95405223757611746};
    p.fingers_native[9]={-0.15122511288367566,-0.086427652789053053,0.58308313297183778,-0.79352081643170558};
    p.fingers_native[10]={-0.04481717368455055,-0.03343456119437372,0.2714050432765604,-0.96083966068471416};
    p.fingers_native[11]={-0.027463242823149867,-0.032787198071739611,0.3037107713641789,-0.951803833409814};
    p.fingers_native[12]={0.79263647873704512,-0.25637091298790937,-0.084903808217739282,-0.54661934735170969};
    p.fingers_native[13]={-0.05902548045813602,-0.1069440482571513,0.043107865187482157,-0.99157484596879319};
    p.fingers_native[14]={-0.11091107868937949,-0.063735543679719397,-0.0075000674626788952,-0.99175615051476451};
    p.volume_center_raw={-0,14.000000000000002,3};
    p.volume_radius_metres=0.13;
    p.snap_distance_metres=0.10000000000000001;
    p.snap_angle_radians=1.48;
    p.release_distance_metres=0.17999999999999999;
    return p;
}
inline constexpr Profile support=make_support();
inline constexpr Profile make_ammo() noexcept {
    Profile p{};
    p.capture_id="1794500066511-3";
    p.source_time_ns=14823400072382ull;
    p.profile_id=0x424f4c544f4b0012ull;
    p.wrist_raw_object={{12.138463761210424,17.966399686308296,2.4223265051841745},{0.46281154176815142,0.53689096740388298,0.67521465250443291,-0.20405572515768147}};
    p.captured_delta_xr_object={{0,0,0},{0,0,0,1}};
    p.fingers_native[0]={0.070627673993030285,0.044413624131155606,0.097756880764764603,-0.99170698995268458};
    p.fingers_native[1]={0.049513477789448826,-0.029105608624021145,0.49274894410419512,-0.86827401040686447};
    p.fingers_native[2]={0.022814340687643226,-0.02131184221795494,0.30378914347030889,-0.95222763431326718};
    p.fingers_native[3]={-0.0064556865522560772,0.03399907843925376,0.11145996753741944,-0.99316618066312434};
    p.fingers_native[4]={-0.01153813147483496,-0.0044939195797996309,0.38810037959365978,-0.92153392317816341};
    p.fingers_native[5]={-0.017366296426620829,-0.019501286704809408,0.18905611114308588,-0.98161901897056403};
    p.fingers_native[6]={-0.10145357670618563,0.048373793082130086,0.14729782871130612,-0.98268535023846992};
    p.fingers_native[7]={-0.015263681288943311,0.0072508915556412082,0.34290635539272829,-0.9392175871630779};
    p.fingers_native[8]={-0.0057286382756228157,0.0016294585547322593,0.16886787548257715,-0.98562070199358442};
    p.fingers_native[9]={-0.12706962573919078,0.04688335699310086,0.13438575217625179,-0.98162912072920006};
    p.fingers_native[10]={-0.052534814724289947,-0.03919201965734298,0.31191146657941493,-0.94784772819970486};
    p.fingers_native[11]={-0.010627244172123455,-0.012687275368849119,0.11752378366939684,-0.99293215024846981};
    p.fingers_native[12]={0.83337953253600283,-0.40020198558072129,0.10186370477215763,-0.36734277063447446};
    p.fingers_native[13]={-0.071390677313952253,-0.1293474672548337,0.075278827120720093,-0.98615703723836168};
    p.fingers_native[14]={-0.088772490325044251,-0.051013526462799996,-0.046663721461374159,-0.99364961740982649};
    p.volume_center_raw={-0,17.672944259643554,8.9682669639587385};
    p.volume_radius_metres=0.10000000000000001;
    p.snap_distance_metres=0.10000000000000001;
    p.snap_angle_radians=1.48;
    p.release_distance_metres=0.17999999999999999;
    return p;
}
inline constexpr Profile ammo=make_ammo();
inline constexpr Profile make_mechanism() noexcept {
    Profile p{};
    p.capture_id="1794486092117-4";
    p.source_time_ns=849008135048ull;
    p.profile_id=0x424f4c544f4b0013ull;
    p.wrist_raw_object={{13.847748150025044,21.69615478799939,2.5278592901669228},{0.40907646932748853,0.59340782794134828,0.6381600310041835,-0.27069423120788383}};
    p.captured_delta_xr_object={{0,0,0},{0,0,0,1}};
    p.fingers_native[0]={0.077723303577153888,0.039272652719403466,0.15164175679011119,-0.98459205990452892};
    p.fingers_native[1]={0.0433744808892449,-0.025496759876494357,0.43996974686302764,-0.89660202514196474};
    p.fingers_native[2]={0.021499067049419365,-0.020083143757527171,0.29066151326132683,-0.95637353694121763};
    p.fingers_native[3]={-0.0012588039652512318,0.0079542169672567574,0.12606706360532402,-0.99198903286223172};
    p.fingers_native[4]={-0.012014050428792966,-0.0046793170536554643,0.40180823847889635,-0.91563306300869918};
    p.fingers_native[5]={-0.020357719241779711,-0.02286038276373821,0.2162804297734702,-0.97585129085541067};
    p.fingers_native[6]={-0.1005810007672447,0.040980021818297692,0.16381277904566363,-0.98049450458315568};
    p.fingers_native[7]={-0.016814294992813044,0.007987511749535544,0.37353059724092391,-0.92743106054562596};
    p.fingers_native[8]={-0.0066740687252943298,0.0018983054029542922,0.1901931984521695,-0.98172216054532502};
    p.fingers_native[9]={-0.12035246898027456,0.052691901029782419,0.10036952417060224,-0.98623770227769458};
    p.fingers_native[10]={-0.052064754176124502,-0.038841401577414385,0.30944676463185317,-0.94869558170941237};
    p.fingers_native[11]={-0.011438771132169673,-0.01365623247961258,0.12649866365034021,-0.99180681078732602};
    p.fingers_native[12]={0.84347634648660264,-0.3632212934552484,0.096888942319335952,-0.3837062388788452};
    p.fingers_native[13]={-0.047921264838177408,-0.086825092413027036,0.014377245504953357,-0.99496645698006758};
    p.fingers_native[14]={-0.086896647282284398,-0.049935539001256711,-0.049961782736406141,-0.99370983435887994};
    p.volume_center_raw={-0,17.672944259643554,8.9682669639587385};
    p.volume_radius_metres=0.13;
    p.snap_distance_metres=0.10000000000000001;
    p.snap_angle_radians=1.48;
    p.release_distance_metres=0.17999999999999999;
    return p;
}
inline constexpr Profile mechanism=make_mechanism();
inline constexpr Profile const* profile(api::object::Kind kind) noexcept {
    return kind==api::object::Kind::magazine?&mechanism:kind==api::object::Kind::loading?&ammo:
        kind==api::object::Kind::support?&support:nullptr;
}
inline bool native_fingers(Profile const& p,api::Binding const& binding,api::Joints& out) noexcept {
    if(binding.geometry_signature!=gears_hand_authored_mount::marcus115_signature)return false;
    for(unsigned i=0;i<15;++i)if(!api::unit(p.fingers_native[i]))return false;
    out=p.fingers_native;return true;
}
inline bool source_object_wrist(Profile const& p,gears_vr::Pose const& current_delta_xr_object,
    gears_vr::Pose const& captured_wrist_in_object,gears_vr::Pose& out) noexcept {
    if(!gears_vr::stereo_pose_valid(current_delta_xr_object)||
       !gears_vr::stereo_pose_valid(captured_wrist_in_object))return false;
    const auto q=gears_vr::normalized(gears_vr::multiply(current_delta_xr_object.orientation,
        gears_vr::inverse(p.captured_delta_xr_object.orientation)));
    const auto r=gears_vr::rotation_matrix(q);
    auto rotate=[&](gears_vr::Vector v){return gears_vr::transform(r,v);};
    const auto removed=rotate(p.captured_delta_xr_object.position);
    const auto delta=gears_vr::Vector{current_delta_xr_object.position.x-removed.x,
        current_delta_xr_object.position.y-removed.y,current_delta_xr_object.position.z-removed.z};
    const auto rotated=rotate(captured_wrist_in_object.position);
    out.position={delta.x+rotated.x,delta.y+rotated.y,delta.z+rotated.z};
    out.orientation=gears_vr::normalized(gears_vr::multiply(q,captured_wrist_in_object.orientation));
    return gears_vr::stereo_pose_valid(out);
}
// Live mapping remains undecided. in_root is part-relative ROOT metres, while
// this wrist is object raw. No exact inverse captured-delta/object-world
// relation is present in the accepted data, so no world grip is fabricated.
}
