#include "weapon_boomshot_carrier_geometry.h"
#include "weapon_boomshot_part_input.h"
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>
namespace geom=gears_weapon::part_geometry;namespace carrier=gears_weapon::boomshot_carrier_geometry;
std::vector<std::uint8_t> bytes(char const* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
 assert(argc==3);auto vertices=bytes(argv[1]),indices=bytes(argv[2]);auto const& input=gears_weapon::original_part_input::boomshot_carrier;
 geom::Input source{vertices.data(),vertices.size(),indices.data(),indices.size(),input.sections,input.section_count};
 geom::Conversion conversion{};for(unsigned i=0;i<3;++i)conversion.axes[i][i]=1;
 conversion.units_per_metre=100;conversion.calibration_generation=1;conversion.current_basis_verified=true;
 geom::Prepared original{};assert(geom::prepare(source,conversion,original)==geom::Result::ready);
 auto player=original,boomer=original;assert(carrier::apply_original_scale(player,false)&&carrier::apply_original_scale(boomer,true));
 assert(player.calibration_generation==original.calibration_generation&&player.indices==original.indices);
 assert(boomer.vertices.size()==original.vertices.size()&&!std::memcmp(boomer.vertices.data(),original.vertices.data(),108*sizeof(geom::Vertex)));
 for(unsigned i=0;i<108;++i){const auto before=original.vertices[i];auto after=player.vertices[i];
    for(unsigned axis=0;axis<3;++axis){assert(after.position[axis]==before.position[axis]*.75f);after.position[axis]=before.position[axis];}
    assert(!std::memcmp(&before,&after,sizeof before));}
 for(unsigned axis=0;axis<3;++axis){assert(player.bounds.centre[axis]==original.bounds.centre[axis]*.75f);assert(player.bounds.extent[axis]==original.bounds.extent[axis]*.75f);}
 assert(player.bounds.radius==original.bounds.radius*.75f);
 for(unsigned reason=0;reason<5;++reason){auto invalid=original;
    switch(reason){case 0:invalid.calibration_generation=0;break;case 1:invalid.vertices.pop_back();break;case 2:invalid.indices.pop_back();break;
      case 3:invalid.bounds.radius=std::numeric_limits<float>::quiet_NaN();break;case 4:invalid.vertices[7].position[1]=std::numeric_limits<float>::infinity();break;}
    const auto before=invalid.vertices[0];assert(!carrier::apply_original_scale(invalid,false));assert(!std::memcmp(&before,&invalid.vertices[0],sizeof before));}
 std::puts("Boomshot carrier: original player0.75/Geist1 scale, real100units calibration preserved,108positions/bounds scaled, tangents/UV/color/indices unchanged, invalid source rejected; CPU only");
}
