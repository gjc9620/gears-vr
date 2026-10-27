#pragma once
#include "hand_authored_mount_profile.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

// Data and model proportions only: no tracking, asset authentication, Native
// permission or write authority. Unknown calibration is a caller-owned choice
// to use default scales of one, never a successful derive().
namespace gears_hand_personal {
namespace mount=gears_hand_authored_mount;
enum class Provenance:std::uint32_t {unknown,controller_span_model_estimate};
enum class Status {okay,alias,invalid_measurement,invalid_model,invalid_geometry,invalid_profile,numerical_failure};
struct Measurement {
    std::uint32_t version{1};
    double armspan_metres{},hands_below_head_metres{};
    bool valid() const noexcept {
        return version==1&&std::isfinite(armspan_metres)&&armspan_metres>=.9&&armspan_metres<=2.4&&
            std::isfinite(hands_below_head_metres)&&hands_below_head_metres>=-.35&&hands_below_head_metres<=.1;
    }
};
struct ModelKey {
    mount::Model model{mount::Model::unknown};
    mount::Signature geometry_signature{};
    std::uint32_t bone_count{};
    double model_to_world_scale{},units_per_metre{};
};
struct Dimensions {
    ModelKey key{};
    double shoulder_span_metres{};
    std::array<double,2> upper_metres{},lower_metres{},authored_controller_extension_metres{};
};
struct Profile {
    Measurement measurement{};
    Dimensions model{};
    double scale{};
    std::array<double,2> upper_metres{},lower_metres{};
    Provenance provenance{Provenance::unknown};
};
namespace detail {
inline bool positive(double value) noexcept {return std::isfinite(value)&&value>0;}
inline bool disjoint(void const* a,std::size_t a_size,void const* b,std::size_t b_size) noexcept {
    const auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
    constexpr auto limit=(std::numeric_limits<std::uintptr_t>::max)();
    if(!x||!y||!a_size||!b_size||a_size-1>limit-x||b_size-1>limit-y)return false;
    return x>y+b_size-1||y>x+a_size-1;
}
inline bool model_valid(ModelKey const& key) noexcept {
    return key.model==mount::Model::marcus115&&key.geometry_signature==mount::marcus115_signature&&
        key.bone_count==115&&positive(key.model_to_world_scale)&&std::isfinite(key.units_per_metre)&&
        key.units_per_metre>=10&&key.units_per_metre<=500;
}
inline Status lengths(Measurement const& measurement,Dimensions const& model,double& scale,
    std::array<double,2>& upper,std::array<double,2>& lower) noexcept {
    if(!measurement.valid())return Status::invalid_measurement;
    if(!model_valid(model.key))return Status::invalid_model;
    if(!positive(model.shoulder_span_metres))return Status::invalid_geometry;
    double total{},extension{};
    for(unsigned i=0;i<2;++i){
        if(!positive(model.upper_metres[i])||!positive(model.lower_metres[i])||
           !std::isfinite(model.authored_controller_extension_metres[i])||
           model.authored_controller_extension_metres[i]<0)return Status::invalid_geometry;
        total+=model.upper_metres[i]+model.lower_metres[i];
        extension+=model.authored_controller_extension_metres[i];
    }
    if(!positive(total)||!std::isfinite(extension))return Status::numerical_failure;
    const double budget=measurement.armspan_metres-model.shoulder_span_metres-extension;
    if(!std::isfinite(budget))return Status::numerical_failure;
    if(budget<=0)return Status::invalid_geometry;
    scale=budget/total;
    if(!positive(scale))return Status::numerical_failure;
    for(unsigned i=0;i<2;++i){
        upper[i]=scale*model.upper_metres[i];lower[i]=scale*model.lower_metres[i];
        if(!positive(upper[i])||!positive(lower[i]))return Status::numerical_failure;
    }
    return Status::okay;
}
} // namespace detail

// v1 is an authored collinear estimate, NOT measured anatomical arm lengths.
// A = measured grip span, D = model shoulder span, H = both mount offset norms.
// Treating H as maximum outward extension is an authoring assumption that can
// underestimate arm length. s=(A-D-H)/(UL+LL+UR+LR); no clamping, target-distance
// feedback, shoulder/body-slot estimate or change to the full wrist mapping.
// The caller binds the measurement, model identity, mount version and scale.
// All failures preserve the entire output representation.
inline Status derive(Measurement const& measurement,Dimensions const& model,Profile& out) noexcept {
    if(!detail::disjoint(&measurement,sizeof(measurement),&out,sizeof(out))||
       !detail::disjoint(&model,sizeof(model),&out,sizeof(out)))return Status::alias;
    Profile candidate{};
    const auto status=detail::lengths(measurement,model,candidate.scale,candidate.upper_metres,candidate.lower_metres);
    if(status!=Status::okay)return status;
    candidate.measurement=measurement;candidate.model=model;
    candidate.provenance=Provenance::controller_span_model_estimate;
    out=candidate;return Status::okay;
}

// Fixed absolute target lengths divided by this call's ORIGINAL segment
// lengths. Never supply previous modified output as the original baseline.
// The caller, not this data helper, certifies the current model/owner/stage.
// No profile means the caller explicitly supplies its normal default of one.
inline Status scales(Profile const& profile,mount::Side side,double current_upper_metres,
    double current_lower_metres,double& upper_scale,double& lower_scale) noexcept {
    if(!detail::disjoint(&profile,sizeof(profile),&upper_scale,sizeof(upper_scale))||
       !detail::disjoint(&profile,sizeof(profile),&lower_scale,sizeof(lower_scale))||
       !detail::disjoint(&upper_scale,sizeof(upper_scale),&lower_scale,sizeof(lower_scale)))return Status::alias;
    if(!mount::valid_side(side))return Status::invalid_model;
    if(profile.provenance!=Provenance::controller_span_model_estimate)return Status::invalid_profile;
    double expected_scale{};std::array<double,2> upper{},lower{};
    const auto status=detail::lengths(profile.measurement,profile.model,expected_scale,upper,lower);
    if(status!=Status::okay)return status;
    if(profile.scale!=expected_scale||profile.upper_metres!=upper||profile.lower_metres!=lower)
        return Status::invalid_profile;
    if(!detail::positive(current_upper_metres)||!detail::positive(current_lower_metres))return Status::invalid_geometry;
    const auto i=side==mount::Side::left?0u:1u;
    const double u=profile.upper_metres[i]/current_upper_metres,l=profile.lower_metres[i]/current_lower_metres;
    if(!detail::positive(u)||!detail::positive(l))return Status::numerical_failure;
    upper_scale=u;lower_scale=l;return Status::okay;
}
} // namespace gears_hand_personal
