#include "weapon_troika_handle_geometry.h"
#include "weapon_troika_interaction.h"
#include <cstdio>
#include <cstdlib>
namespace g=gears_weapon::troika_handle_geometry;
namespace {unsigned checks{};void check(bool x,int line){++checks;if(!x){std::fprintf(stderr,"line %d\n",line);std::exit(1);}}
#define CHECK(x) check(bool(x),__LINE__)
}
int main(){
    std::array<float,16> m{1,0,0,0,0,1,0,0,0,0,1,0,0,47.27169418f,111.43261719f,1};
    g::Contact c{};
    // Actual LOD0 nearest vertex to handle_L origin from saved export.
    const g::V vertex{16.13383,24.20097,109.1738};
    CHECK(g::contact(m,vertex,0,100,c)&&c.touched);
    CHECK(g::contact(m,{-vertex.x,vertex.y,vertex.z},1,100,c)&&c.touched);
    CHECK(g::contact(m,vertex,1,100,c)&&!c.touched); // wrong handle
    CHECK(g::contact(m,{13.46,27.8,109.1738},0,100,c)&&!c.touched); // not the surface
    CHECK(g::contact(m,{20.04242513,19.81964493,109.48483276},0,100,c)&&!c.touched); // old bone marker
    CHECK(g::contact(m,{16,24,125},0,100,c)&&!c.touched);
    const auto before=c;auto bad=m;bad[4]=.1f;CHECK(!g::contact(bad,vertex,0,100,c));
    CHECK(c.touched==before.touched&&c.distance_metres==before.distance_metres);
    // Same physical contact with translated/rotated and uniformly scaled mesh.
    std::array<float,16> rotated{0,2,0,0,-2,0,0,0,0,0,2,0,10,20,30,1};
    const g::V p{10-2*(vertex.y-47.27169418),20+2*vertex.x,30+2*(vertex.z-111.43261719)};
    CHECK(g::contact(rotated,p,0,200,c)&&c.touched);
    gears_weapon::TroikaEntryContact gate;
    CHECK(!gate.observe(1,3,true,true,true));CHECK(!gate.observe(0,3,true,true,true));
    CHECK(gate.observe(1,3,true,true,true));CHECK(!gate.observe(1,3,true,true,true));
    CHECK(!gate.observe(0,3,true,true,true));CHECK(!gate.observe(1,0,true,true,true));
    CHECK(!gate.observe(1,3,true,true,true)); // releasing held item inside is no new touch
    CHECK(!gate.observe(0,3,true,true,true));CHECK(!gate.observe(1,3,false,true,true));
    CHECK(!gate.observe(1,3,true,true,true)); // no synthetic edge on recovery
    std::printf("Troika actual-mesh contact and entry gate: %u checks passed; live fit pending.\n",checks);
}
