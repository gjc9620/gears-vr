#pragma once
#include <cstdint>
namespace gears_world {
// Publish handshake progress immediately; ordinary telemetry keeps its cadence.
// This is used only by the session worker, outside the closed world gate.
struct StatusProgress {
    std::uint64_t generation{},requests{},rejected{},candidate{};
    unsigned phase{};
};
class StatusCadence {
    bool initialized_{};
    StatusProgress last_{};
public:
    bool changed(StatusProgress value) {
        const bool result=!initialized_||value.generation!=last_.generation||value.requests!=last_.requests||
            value.rejected!=last_.rejected||value.candidate!=last_.candidate||value.phase!=last_.phase;
        last_=value;initialized_=true;return result;
    }
};
}
