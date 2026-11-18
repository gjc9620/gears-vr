#pragma once
#include "hand_authored_mount_profile.h"

namespace gears_weapon_hammerburst_foregrip {
// Generated from the exact archived owner LEFT capture and the HB rig conversion.
// This is a provisional measured support start, not a fitted or game-accepted pose.
inline constexpr char source_capture_id[]="1794486500346-1";
inline constexpr char source_capture_sha256[]="8d31686662067aef8d93a4416630c4d82bb29d962c86e74b95e9e96c7ef0344c";
inline constexpr char source_model_sha256[]="eec7ca09e7055b03f2a1b96b65baa6bc90c8dda1f505e5677926bcf6bf8dd3be";
inline constexpr char source_model_fnv64[]="8329009210090488606";
inline constexpr char source_rig_signature[]="3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193";
inline constexpr char original_label[]="main_grip / LEFT / 1794486500346-1";
// Captured wrist in raw HB object coordinates, after the documented
// wrist correction and wrist offset. Units are original HB raw units.
inline constexpr gears_vr::Vector captured_wrist_raw={4.4744514397284272,34.362673625776353,-0.38733435871831368};
inline constexpr gears_vr::Quaternion captured_wrist_raw_rotation={0.36613350067968076,0.64233758569869159,-0.6055984935658284,-0.29427733563701064};
// Captured left finger locals after q->[x,-y,-z,w], in the existing
// Marcus115 reference-joint order. All 15 are retained; no right-hand use.
inline constexpr gears_vr::Quaternion captured_finger_locals[15]={
    {0.024238713505074286,0.084602855323973353,0.3150119782544587,-0.94499856888508682},
    {-0.019649441978031978,-0.034492323730761727,0.45066680657036401,-0.89180917717260255},
    {-0.0070666190370978557,-0.014027907397736602,0.18053744755923659,-0.98344268299592374},
    {-0.054432392050033335,0.048916615667366811,0.27682049958328808,-0.95813083157668066},
    {-0.068673831183660397,-0.011323321749870283,0.49781043088500471,-0.86448855527253399},
    {-0.043217234659534219,-0.021008438509381618,0.19988095411625859,-0.97864116014060765},
    {-0.14534570895812204,0.070103129571895864,0.37706771668549355,-0.91201979865860117},
    {-0.050183297433582158,0.0087544299907060894,0.43661036274267179,-0.89820731891915861},
    {-0.02252399753927023,0.00050970581787371195,0.22402182744760804,-0.97432367853904922},
    {-0.18566295005301392,-0.06659266724250873,0.48674513485190457,-0.85098405351988926},
    {-0.052404820912372863,-0.02339382938513139,0.21078479960184493,-0.97584641811539774},
    {-0.036482103461937698,-0.025562677437877209,0.19414207414298243,-0.97996145878124219},
    {0.79611502497079345,-0.37553619001846705,-0.04161740602927104,-0.47269591548643786},
    {0.002539604511765253,0.0036276798495609745,-0.11252875598601444,-0.99363860101350376},
    {-0.068537543388762293,-0.032128450033634862,-0.10445109632372876,-0.99164526738216274}
};

// Captured wrist rotation in mesh coordinates, for the existing geometry
// helper. The grip anchor is computed at runtime from the current Marcus
// left calibration, so this header does not duplicate calibration values.
inline gears_vr::Quaternion wrist_in_mesh() noexcept {return {0.36613350067968076,0.64233758569869159,-0.6055984935658284,-0.29427733563701064};}
inline bool grip_in_mesh(gears_vr::Vector& out) noexcept {
    gears_hand_authored_mount::Mount left{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,1,100,left))return false;
    const auto marker=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(left.calibration.grip_to_wrist_rotation)),
        {-100*left.calibration.grip_to_wrist_metres.x,-100*left.calibration.grip_to_wrist_metres.y,-100*left.calibration.grip_to_wrist_metres.z});
    const auto axes=gears_vr::rotation_matrix(wrist_in_mesh());
    const auto shift=gears_vr::transform(axes,marker);
    out={captured_wrist_raw.x+shift.x,captured_wrist_raw.y+shift.y,captured_wrist_raw.z+shift.z};
    return gears_vr::finite(out);
}
}
