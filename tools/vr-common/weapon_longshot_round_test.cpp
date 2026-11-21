#include "weapon_part_input_catalog.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
namespace geometry=gears_weapon::part_geometry;
std::vector<std::uint8_t> read_bytes(std::filesystem::path const& path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return {std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv){
    assert(argc==2);const std::filesystem::path directory=argv[1];unsigned lods{};
    for(auto const& asset:gears_weapon::part_input_catalog::assets){
        if(std::string(asset.id).find("longshot-round-")!=0)continue;
        const auto vertices=read_bytes(directory/asset.vertex_file),indices=read_bytes(directory/asset.index_file);
        assert(vertices.size()==asset.vertices*60&&indices.size()==asset.indices*2);
        geometry::Prepared baseline{},engine{};
        const geometry::Input input{vertices.data(),vertices.size(),indices.data(),indices.size(),asset.sections,asset.section_count};
        const geometry::Conversion original{{{{1,0,0},{0,1,0},{0,0,1}}},100.,42,true};
        const geometry::Conversion converted{{{{1,0,0},{0,0,1},{0,-1,0}}},100.,42,true};
        assert(geometry::prepare(input,original,baseline)==geometry::Result::ready);
        assert(geometry::prepare(input,converted,engine)==geometry::Result::ready);
        assert(engine.sections.size()==3&&engine.vertices.size()==baseline.vertices.size());
        assert(engine.indices==baseline.indices&&!engine.winding_reversed);
        for(std::size_t i=0;i<engine.vertices.size();++i){
            auto const& a=baseline.vertices[i].position;auto const& b=engine.vertices[i].position;
            assert(std::abs(b[0]-a[0])<1e-5);
            assert(std::abs(b[1]-a[2])<1e-5);
            assert(std::abs(b[2]+a[1])<1e-5);
        }
        assert(engine.bounds.extent[1]>engine.bounds.extent[0]);
        assert(engine.bounds.extent[1]>engine.bounds.extent[2]);
        for(auto const& section:engine.sections)assert(section.material<3);
        std::printf("%s: %zu vertices, %zu triangles, authored +Z to engine +Y verified\n",
            asset.id,engine.vertices.size(),engine.indices.size()/3);++lods;
    }
    assert(lods==2);
}
