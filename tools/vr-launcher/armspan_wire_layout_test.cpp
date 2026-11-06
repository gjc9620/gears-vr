#include "../vr-common/controller_input_bridge.h"
#include "../vr-common/stereo_bridge.h"
#include <cstddef>
#include <cstdio>

using namespace gears_vr;
// These are the x64 ABI offsets copied by armspan_wire.py, not a new protocol.
static_assert(sizeof(void*) == 8);
static_assert(sizeof(Vector) == 24 && sizeof(Quaternion) == 32 && sizeof(Pose) == 56);
static_assert(sizeof(StereoSample) == 312 && sizeof(StereoBridgePacket) == 336);
static_assert(offsetof(StereoBridgePacket, sample) == 24);
static_assert(offsetof(StereoSample, session) == 8 && offsetof(StereoSample, head) == 64);
static_assert(offsetof(StereoSample, eyes) == 120 && offsetof(StereoSample, space_epoch) == 296);
static_assert(sizeof(ControllerHand) == 312 && sizeof(ControllerInputFrame) == 720);
static_assert(offsetof(ControllerHand, pressed) == 172 && offsetof(ControllerHand, last_squeeze_press_grip) == 240);
static_assert(offsetof(ControllerInputFrame, hands) == 80);
static_assert(sizeof(ControllerInputPacket) == 5816 && offsetof(ControllerInputPacket, frames) == 56);

int main() {
    std::puts("PASS: x64 Stereo-v4/GVC4 calibration reader ABI matches existing C++ publishers");
}
