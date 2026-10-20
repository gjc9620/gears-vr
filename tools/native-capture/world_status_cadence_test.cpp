#include "world_status_cadence.h"
#include <cassert>
int main(){
    gears_world::StatusCadence cadence;
    gears_world::StatusProgress value{1,0,0,0,0};
    assert(cadence.changed(value));assert(!cadence.changed(value));
    value.requests=1;value.candidate=2;value.phase=1;
    assert(cadence.changed(value));assert(!cadence.changed(value));
    value.phase=2;assert(cadence.changed(value));assert(!cadence.changed(value));
    value.generation=2;value.candidate=0;value.phase=3;
    assert(cadence.changed(value));assert(!cadence.changed(value));
    value.requests=2;value.rejected=1;value.phase=4;
    assert(cadence.changed(value));assert(!cadence.changed(value));
    // Rejection count alone matters even if the generation did not advance.
    ++value.rejected;assert(cadence.changed(value));assert(!cadence.changed(value));
}
