#include "weapon_boltok_object02_input.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

namespace input=gears_weapon::original_part_input;
namespace geom=gears_weapon::part_geometry;

std::vector<unsigned char> file(char const* path) {
    std::ifstream stream(path,std::ios::binary);
    assert(stream);
    return {std::istreambuf_iterator<char>(stream),{}};
}

int main(int argc,char** argv){
    assert(argc==2);
    auto vertices=file((std::string(argv[1])+"/boltok-object02.vtx60").c_str());
    auto indices=file((std::string(argv[1])+"/boltok-object02.idx16").c_str());
    assert(vertices.size()==input::boltok_object02.vertices*sizeof(geom::Vertex));
    assert(indices.size()==input::boltok_object02.indices*sizeof(std::uint16_t));
    geom::Input raw{vertices.data(),vertices.size(),indices.data(),indices.size(),
        input::boltok_object02.sections,input::boltok_object02.section_count};
    geom::Conversion identity{{{{1,0,0},{0,1,0},{0,0,1}}},100,1,true};
    geom::Prepared prepared{};
    assert(geom::prepare(raw,identity,prepared)==geom::Result::ready);
    assert(prepared.vertices.size()==122&&prepared.indices.size()==348);
    assert(prepared.sections.size()==1&&prepared.sections[0].material==0);
    // UV0 is byte-for-byte in the prepared 60-byte record at offset24.
    for(unsigned i=0;i<prepared.vertices.size();++i){
        auto& vertex=prepared.vertices[i];
        auto const* raw_uv=reinterpret_cast<unsigned char const*>(&vertex)+24;
        std::array<unsigned char,32> expected{};
        for(unsigned j=0;j<expected.size();++j)expected[j]=vertices[i*60+24+j];
        for(unsigned j=0;j<expected.size();++j)assert(raw_uv[j]==expected[j]);
        for(float uv:vertex.uv[0])assert(std::isfinite(uv));
    }
    // Identity-basis Object02 is centered at its source mesh reference pivot's
    // feed-frame offset; no authored 180-degree/+63mm conversion is applied.
    double min_y=1e9,max_y=-1e9;
    for(auto const& vertex:prepared.vertices){
        min_y=std::fmin(min_y,vertex.position[1]);
        max_y=std::fmax(max_y,vertex.position[1]);
    }
    assert(std::isfinite(min_y)&&std::isfinite(max_y)&&max_y>min_y);
    assert(prepared.bounds.radius>0&&std::isfinite(prepared.bounds.radius));
    std::printf("Boltok Object02 private cache: 122 vertices, 348 indices, material0, UV0 preserved, identity 100/m feed frame.\n");
}
