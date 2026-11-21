#ifdef NDEBUG
#undef NDEBUG
#endif
#include "weapon_boltok_visual_geometry.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
int main(){
    for(double units:{50.,100.,200.})for(double scale:{.3,1.,2.}){
        BoltokVisualGeometry g{scale,units,1};assert(g.valid());
        const Pose bone{{11,22,33},{0,0,std::sin(.35),std::cos(.35)}};Pose face{};
        assert(g.cylinder(bone,face));
        const auto local=rotate(gears_vr::inverse(bone.orientation),difference(face.position,bone.position));
        assert(length(difference(local,scaled(boltok_loading_face_in_bone_raw,scale)))<1e-10);
        // Original y-min minus source pivot, not the cylinder's centre.
        assert(std::abs(local.y/scale-(17.534860610961914-21.472944259643555))<1e-10);
        BoltokChamberGeometry ports{};assert(g.ports(ports));
        for(unsigned i=0;i<6;++i){Pose round{};assert(g.round(face,i,round));
            const auto base=scaled(rotate(gears_vr::inverse(face.orientation),difference(round.position,face.position)),1/units);
            const auto tip=sum(base,ports.ports[i].item_in_left_grip.position);
            assert(length(difference(tip,ports.ports[i].port_in_weapon.position))<1e-10);
            assert(std::abs(ports.ports[i].lateral_tolerance_metres-.006*g.physical_scale())<1e-12);
        }
        Pose invalid{};assert(!g.round(face,6,invalid));
    }
    BoltokVisualGeometry invalid{0,100,1};assert(!invalid.valid());
    FeedPortMount port{};port.lateral_tolerance_metres=.0001;assert(!port.valid());
    std::puts("Boltok visual geometry: source loading face, transformed six slots, scaled disjoint insertion ports passed (offline only).");
}
