#pragma once
#include "ngx_stereo_layout.h"
#include <cmath>
namespace gears_ngx_stereo {
// Scalar receipt from the authenticated game GetOptimal helper. This carries
// no allocation, producer, feature ownership or GPU completion evidence.
struct NativeOptimalSettings {
    Extent output{},optimal{},maximum{},minimum{};uint32_t quality{};float sharpness{};
    uint32_t result{0xbad00007u};bool called{};
    bool supports(Extent input)const{
        return called&&(result&0xfff00000u)!=0xbad00000u&&bounded(output)&&bounded(optimal)&&bounded(maximum)&&bounded(minimum)&&
            std::isfinite(sharpness)&&minimum.width<=optimal.width&&minimum.height<=optimal.height&&
            optimal.width<=maximum.width&&optimal.height<=maximum.height&&bounded(input)&&
            input.width>=minimum.width&&input.height>=minimum.height&&input.width<=maximum.width&&input.height<=maximum.height&&
            input.width<=output.width&&input.height<=output.height;
    }
};
inline bool same_optimal(NativeOptimalSettings const& a,NativeOptimalSettings const& b){
    return same(a.output,b.output)&&same(a.optimal,b.optimal)&&same(a.maximum,b.maximum)&&same(a.minimum,b.minimum)&&
        a.quality==b.quality&&a.sharpness==b.sharpness&&a.result==b.result&&a.called==b.called;
}
}
