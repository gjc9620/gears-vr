#pragma once
#include "hand_authored_grasp.h"
#include "hand_marcus115_reference.h"
// Boomshot skin authoring. The ammo relation is the selected Boomshot.glb
// left capture 1794499171798-4; support is 1794486222346-2. The right main
// pose is the Boomshot project's authored fallback because this readback has no
// adopted Boomshot right capture. Gnasher/Boltock captures are not used.
// Finger quaternions use transfer.py correction [xyz degrees] then reflection
// [x,-y,-z,w]. Original Marcus115 reference/bone lengths stay fixed.
namespace gears_boomshot_grasp_pose {
namespace api=gears_hand_authored_grasp;namespace fingers=gears_hand_fingers;
namespace vr=gears_vr;
enum class Part:unsigned {main_grip,support,mechanism_open,mechanism_close,carrier,carrier_seating};
struct Angles {double fingers[4][3],thumb[3],opposition;};
inline constexpr api::Joints main_captured{{
    {0.057780791099126573,-0.00095128692682741427,-0.035535884051518349,-0.99769618430553242},
    {-8.1593929569921321e-09,-6.2577476777845097e-08,0.50195461374720729,-0.86489396213517877},
    {-2.1557096658474059e-08,-2.2546294112390519e-08,0.40712325135066463,-0.91337323050857078},
    {0.0020962905627092223,-0.10780639932397548,0.29183191592360253,-0.9503723052988422},
    {-3.6257611554214386e-17,7.4978281008544408e-17,0.54462831462097494,-0.83867752975331122},
    {-2.5362207374912302e-17,1.788081058179858e-17,0.52620758243325672,-0.85035614902801004},
    {0.03520303820944385,-0.088174191846720582,0.46251297067017877,-0.8815144978699152},
    {1.0766956274463849e-09,-5.4648312932148505e-09,0.45686388950477669,-0.8895366133367234},
    {5.0931257554539766e-09,-5.3746671819939142e-09,0.38246657543370027,-0.92396932777880547},
    {0.071875140529480561,-0.14430815828179286,0.44107492001897952,-0.88287147114256403},
    {1.2983560307991764e-08,-2.9364437413229412e-08,0.3888598611412083,-0.92129691652215961},
    {-3.3873735236683169e-09,-4.245811813899024e-09,0.31168522106235674,-0.95018541504872078},
    {-0.85209156951362341,0.50219105868814218,-0.14464159291624951,0.028686361500597061},
    {-5.0174495130841152e-08,-9.5513526886661812e-08,0.16757823776202294,-0.98585878006363736},
    {1.0066369521468221e-08,9.0806071600951025e-09,0.18409203867757637,-0.98290900967258077},
}};
inline constexpr api::Joints support_captured{{
    {0.0615548412514778,-0.0691899365210426,-0.044987166332658,-0.994685834355792},
    {0.0193626001866063,0.0113820004033025,-0.231402727332229,-0.972598744377165},
    {0.00911404952148915,0.00851375681170911,-0.166352277223804,-0.985987510016595},
    {-0.00868096680464739,-0.0389863250909746,-0.131974986004535,-0.99044803515396},
    {-0.00585005123443027,0.00227856548974921,-0.223363619772851,-0.974714972903275},
    {-0.0104362289116676,0.0117192553677532,-0.125892600359632,-0.99191975348534},
    {-0.109961199650545,-0.0547853197309904,-0.250324006980851,-0.960335876057141},
    {-0.00818134063402644,-0.00388657781723083,-0.202487172750737,-0.979243026551486},
    {-0.00376661235767004,-0.00107126261097128,-0.124552595478271,-0.992205279157187},
    {-0.183061822052701,-0.0741735970987142,-0.316926662140422,-0.927655182503021},
    {-0.0315521681297117,0.023538708229319,-0.20162580016068,-0.978671255637344},
    {-0.00675368934779419,0.00806279868223927,-0.0746869223496554,-0.997151564501375},
    {0.800195617481669,0.444612568532232,-0.0145435145543711,-0.402237645988922},
    {-0.0364638417482385,0.0660661079326513,0.0151118343236817,-0.997034247200649},
    {-0.0388723720896091,0.0223380605060425,0.133338562260605,-0.990055946679394},
}};
inline constexpr api::Joints carrier_captured{{
    {0.051336862109755049,0.075424228931782342,-0.081086272905290746,-0.99252240711487161},
    {0.036134846001512504,-0.021241188662769382,0.37742618740600858,-0.92509056738721762},
    {0.019249865228738346,-0.017982168350514017,0.26818012222643384,-0.96300857023845032},
    {-0.0075233996009868419,0.054450274664026334,0.066086984926079229,-0.99629868838156976},
    {-0.011065853504717173,-0.0043100096830168272,0.37448322153251024,-0.92715763896619952},
    {-0.021656403937524056,-0.024318605512436451,0.22809183163425359,-0.97309491928390868},
    {-0.10591529026593491,0.059589091309803693,0.17028347976190514,-0.97787250089412558},
    {-0.0127290738433718,0.0060468540121232216,0.29275395122058651,-0.95608395566518145},
    {-0.0063444377551285288,0.0018046233265628141,0.18276298811067429,-0.9831348745828713},
    {-0.14332458765399322,0.042864071625474724,0.20841980146971911,-0.96653086877389582},
    {-0.033568390103937869,-0.02504283780801958,0.21224480915301197,-0.97631867771130576},
    {-0.0084984147787140699,-0.010145706868881556,0.093981010384404123,-0.99548601761426381},
    {0.8096726636718381,-0.470334815722107,0.094069614587422634,-0.33818078956909453},
    {-0.052022478456998436,-0.094255925125021775,0.02497126187728603,-0.99387419646264763},
    {-0.050404043895606807,-0.028964849195275765,-0.11350163833141656,-0.99183559523045395},
}};
inline constexpr std::array<Angles,6> angles{{
    {{{18,27,9},{54,65,25},{61,68,29},{67,65,27}},{8,27,18},31},
    {{{12,42,17},{18,53,23},{15,61,25},{20,57,22}},{10,24,15},18},
    {{{36,51,20},{43,60,27},{47,64,28},{52,60,26}},{12,35,23},30},
    {{{22,40,17},{30,49,22},{33,55,25},{38,51,23}},{9,28,19},25},
    {{{16,38,18},{19,46,20},{22,53,23},{25,49,21}},{17,35,25},44},
    {{{12,33,15},{16,42,18},{18,48,21},{22,45,19}},{14,31,22},39},
}};
inline constexpr std::uint64_t profile_id(Part part) noexcept {return 0x424f4f4d00000100ULL+unsigned(part);}
inline api::object::Kind kind(Part part) noexcept {
    return part==Part::main_grip?api::object::Kind::none:part==Part::support?api::object::Kind::support:
        (part==Part::mechanism_open||part==Part::mechanism_close)?api::object::Kind::starter:api::object::Kind::magazine;
}
inline bool request(Part part,api::Signature const& actual_geometry,api::Joints const& model_reference,
    gears_weapon_hand::Frame const& source,vr::Pose const& current_part_grip,bool proximity,api::Request& out) noexcept {
    if(unsigned(part)>=angles.size()||actual_geometry!=gears_hand_authored_mount::marcus115_signature)return false;
    for(auto q:model_reference)if(!api::unit(q))return false;
    const bool main=part==Part::main_grip;auto const& value=angles[unsigned(part)];constexpr double rad=.017453292519943295;
    api::Request q{};q.source=source;q.reference=model_reference;q.geometry_signature=actual_geometry;q.profile_id=profile_id(part);
    q.side=main?api::Side::right:api::Side::left;q.mode=main?api::Mode::right_main_fingers:api::Mode::left_part;
    q.kind=kind(part);q.grip=main?vr::Pose{}:current_part_grip;q.proximity=main?0u:unsigned(proximity);
    if(part==Part::main_grip)q.closed=main_captured;
    else if(part==Part::support)q.closed=support_captured;
    else if(part==Part::carrier||part==Part::carrier_seating)q.closed=carrier_captured;
    else{
        for(unsigned i=0;i<12;++i)q.closed[i]=fingers::product(model_reference[i],fingers::turn(2,-rad*value.fingers[i/3][i%3]));
        q.closed[12]=fingers::product(fingers::turn(2,-rad*value.thumb[0]),
            fingers::product(fingers::turn(1,(main?1:-1)*rad*value.opposition),model_reference[12]));
        for(unsigned i=13;i<15;++i)q.closed[i]=fingers::product(model_reference[i],fingers::turn(2,-rad*value.thumb[i-12]));
    }
    // The captured main grip already holds the index on the trigger. Keep the
    // same captured endpoint instead of adding a second 40/52/22-degree bend.
    if(main){q.has_trigger_endpoint=1;q.trigger_closed=q.closed;}
    if(!api::structural(q))return false;out=q;return true;
}
inline bool marcus115_request(Part part,gears_weapon_hand::Frame const& source,
    vr::Pose const& current_part_grip,bool proximity,api::Request& out) noexcept {
    api::Joints reference{};const auto side=part==Part::main_grip?api::Side::right:api::Side::left;
    const auto& expected=gears_hand_authored_mount::marcus115_signature;
    return gears_hand_marcus115_reference::read(expected,side,reference)&&
        request(part,expected,reference,source,current_part_grip,proximity,out);
}
}
