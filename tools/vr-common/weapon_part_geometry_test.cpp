#include "weapon_part_input_catalog.h"
#include "weapon_lancer_starter_mount.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace geometry=gears_weapon::part_geometry;
std::vector<std::uint8_t> bytes(std::filesystem::path const& path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return {std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv){
    using namespace geometry;
    assert(argc==2);
    const std::filesystem::path directory=argv[1];
    std::array<Prepared,3> parts{};unsigned count{};
    const Conversion conversion{{{{1,0,0},{0,1,0},{0,0,1}}},100.,42,true};
    for(auto const& asset:gears_weapon::part_input_catalog::assets){
        if(std::string(asset.id).find("lancer-starter-")!=0)continue;
        assert(count<parts.size());
        const auto vertices=bytes(directory/asset.vertex_file),indices=bytes(directory/asset.index_file);
        assert(prepare({vertices.data(),vertices.size(),indices.data(),indices.size(),asset.sections,asset.section_count},
                       conversion,parts[count++])==Result::ready);
    }
    assert(count==3);
    Prepared joined{};assert(merge_rigid(parts,joined)==Result::ready);
    std::size_t vb{},ib{},sb{};
    for(auto const& part:parts){
        assert(std::memcmp(joined.vertices.data()+vb,part.vertices.data(),part.vertices.size()*sizeof(Vertex))==0);
        for(std::size_t i=0;i<part.indices.size();++i)assert(joined.indices[ib+i]==part.indices[i]+vb);
        for(std::size_t i=0;i<part.sections.size();++i){const auto& old=part.sections[i];const auto& now=joined.sections[sb+i];
            assert(now.material==old.material&&now.triangles==old.triangles&&now.first_index==old.first_index+ib&&
                   now.min_vertex==old.min_vertex+vb&&now.max_vertex==old.max_vertex+vb);}
        vb+=part.vertices.size();ib+=part.indices.size();sb+=part.sections.size();
    }
    assert(joined.vertices.size()==vb&&joined.indices.size()==ib&&joined.sections.size()==sb&&
           joined.calibration_generation==42&&!joined.winding_reversed);
    for(auto const& vertex:joined.vertices){double squared{};
        for(unsigned i=0;i<3;++i){const double delta=double(vertex.position[i])-joined.bounds.centre[i];
            assert(std::abs(delta)<=joined.bounds.extent[i]);squared+=delta*delta;}
        assert(std::sqrt(squared)<=joined.bounds.radius);
    }
    const auto rejects=[&](std::array<Prepared,3> const& bad,Result expected){
        auto unchanged=joined;assert(merge_rigid(bad,unchanged)==expected);
        assert(unchanged.vertices.size()==joined.vertices.size()&&
               std::memcmp(unchanged.vertices.data(),joined.vertices.data(),vb*sizeof(Vertex))==0&&
               unchanged.indices==joined.indices);
    };
    auto bad=parts;bad[1].calibration_generation++;rejects(bad,Result::invalid_conversion);
    bad=parts;bad[1].winding_reversed=true;rejects(bad,Result::invalid_conversion);
    bad=parts;bad[1].indices[0]=65535;rejects(bad,Result::invalid_indices);
    bad=parts;bad[1].sections[0].first_index++;rejects(bad,Result::invalid_sections);
    bad=parts;bad[1].sections[0].max_vertex--;rejects(bad,Result::invalid_sections);
    bad=parts;bad[1].vertices[0].position[0]=NAN;rejects(bad,Result::invalid_vertex);
    bad=parts;bad[1].vertices.resize(32767);rejects(bad,Result::invalid_input);
    bad=parts;bad[1].indices.resize(196602);rejects(bad,Result::invalid_input);
    bad=parts;bad[1].sections.resize(32);rejects(bad,Result::invalid_input);
    assert(merge_rigid(parts,parts[0])==Result::invalid_conversion);
    // The rigid representation is exact only at rest, including rotations,
    // scale and all supported unit calibrations; moving parts stay separate.
    for(double scale:{.5,1.,2.})for(double units:{80.,100.,150.}){
        const std::array<float,16> gun{0,float(scale),0,0,float(-scale),0,0,0,0,0,float(scale),0,123,456,789,1};
        gears_weapon::LancerStarterParts pose{};
        assert(gears_weapon::lancer_starter_parts(gun,0,units,pose));
        assert(pose.socket==pose.grip&&pose.socket==pose.cord);
        assert(gears_weapon::lancer_starter_parts(gun,.001,units,pose));
        assert(pose.socket!=pose.grip&&pose.socket!=pose.cord);
    }
    std::printf("Rigid starter: %zu byte-identical vertices, %zu rebased indices; bounds, calibration, rejection and rest/moving geometry passed\n",vb,ib);
}
