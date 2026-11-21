#include "weapon_boltok_proposal_projection.h"
#include "left_hand_interaction_owner.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace proj = gears_boltok_proposal_projection;
namespace hand = gears_weapon_hand;
namespace vr = gears_vr;

static unsigned checks{};
static unsigned groups{};
static void check(bool value, char const* text, int line) {
    ++checks;
    if (!value) {
        std::fprintf(stderr, "FAIL line %d: %s\n", line, text);
        std::exit(1);
    }
}
#define C(x) check(bool(x), #x, __LINE__)

static hand::Frame frame() {
    hand::Frame f{};
    f.key = {1, 2, 3, 4, 5, 6, 7, 8};
    f.controllers.sequence = 10;
    f.controllers.profile_generation = 1;
    f.controllers.sync_ns = 2000000000;
    for (auto& h : f.controllers.hands) {
        h.grip_flags = 15;
        h.grip_active = 1;
        h.grip.position = {0, 0, 0};
        h.grip.orientation = {0, 0, 0, 1};
    }
    return f;
}

static vr::Quaternion half_yaw() {
    return {0, 0, -std::sqrt(0.5), std::sqrt(0.5)};
}

static vr::Vector ue_to_raw(vr::Vector ue) {
    return {ue.y, ue.z, -ue.x};
}

int main() {
    const vr::Vector contact{.0570243594, .138853699, -0.0354825922};
    const vr::Vector volume{.053165798, .0897294392, .0801367544};
    const double radius = .13;

    ++groups;
    {
        auto f = frame();
        proj::Hint hint{};
        vr::Pose actual_root{
            {-.157041393, .0694981914, -.0235984415}, half_yaw()};
        C(hint.capture(f, actual_root));

        const auto raw_left = ue_to_raw(gears_weapon::sum(
            actual_root.position,
            gears_weapon::rotate(actual_root.orientation, contact)));
        f.controllers.hands[0].grip.position = raw_left;

        vr::Vector projected{};
        C(hint.project(f, f.controllers.sync_ns + 1000000, projected));
        const double error = std::sqrt(
            (projected.x - contact.x) * (projected.x - contact.x) +
            (projected.y - contact.y) * (projected.y - contact.y) +
            (projected.z - contact.z) * (projected.z - contact.z));
        C(error < 1e-9);
        const double correct_distance = std::sqrt(
            (projected.x - volume.x) * (projected.x - volume.x) +
            (projected.y - volume.y) * (projected.y - volume.y) +
            (projected.z - volume.z) * (projected.z - volume.z));
        C(correct_distance < radius);

        proj::Hint old_hint{};
        const vr::Pose old_root{
            {-.0732623438, .0711982856, -.00825079968}, half_yaw()};
        C(old_hint.capture(f, old_root));
        vr::Vector old_projected{};
        C(old_hint.project(f, f.controllers.sync_ns + 1000000, old_projected));
        const double old_distance = std::sqrt(
            (old_projected.x - volume.x) * (old_projected.x - volume.x) +
            (old_projected.y - volume.y) * (old_projected.y - volume.y) +
            (old_projected.z - volume.z) * (old_projected.z - volume.z));
        C(old_distance > radius);
    }

    ++groups;
    {
        auto f = frame();
        proj::Hint hint{};
        C(hint.capture(f, vr::Pose{{0, 0, 0}, half_yaw()}));
        vr::Vector out{};

        f.controllers.sync_ns = 2100000001;
        C(!hint.project(f, f.controllers.sync_ns, out));

        auto changed_key = frame();
        changed_key.key.lifecycle += 1;
        C(!hint.project(changed_key, changed_key.controllers.sync_ns, out));

        auto changed_profile = frame();
        changed_profile.controllers.profile_generation += 1;
        C(!hint.project(changed_profile,
            changed_profile.controllers.sync_ns, out));

        auto regressed = frame();
        regressed.controllers.sequence = 9;
        C(!hint.project(regressed, regressed.controllers.sync_ns, out));
    }

    ++groups;
    {
        auto f = frame();
        proj::Hint hint{};
        C(hint.capture(f, vr::Pose{{0, 0, 0}, half_yaw()}));
        f.controllers.hands[0].grip_active = 0;
        vr::Vector out{};
        C(!hint.project(f, f.controllers.sync_ns + 1000000, out));
        hint.clear();
        C(!hint.known);
        C(!hint.project(f, f.controllers.sync_ns + 1000000, out));
    }

    ++groups;
    {
        vr::LeftHandInteractionOwner owner;
        vr::LeftHandInteractionFrame f{};
        f.lifecycle=4;f.frame=1;f.now_ns=2000000000;
        f.context_valid=f.tracked=true;
        C(owner.update(f).status==vr::LeftHandInteractionStatus::released);
        ++f.frame;f.now_ns+=60000000;f.press_token=1;f.grip_down=true;
        f.requests.magazine_observation_pending=true;
        const auto waiting=owner.update(f);
        C(waiting.status==vr::LeftHandInteractionStatus::no_candidate&&!waiting.new_grant);
        ++f.frame;f.now_ns+=60000000;
        f.requests.magazine_observation_pending=false;f.requests.magazine=true;
        const auto acquired=owner.update(f);
        C(acquired.status==vr::LeftHandInteractionStatus::granted&&acquired.new_grant&&acquired.press_token==1);
    }
    std::printf("BoltokProposalProjectionTest: %u groups / %u checks passed\n",
        groups, checks);
    return 0;
}

