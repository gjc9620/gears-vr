#include "world_handoff_display.h"
#include <cassert>
int main(){
    WorldHandoffDisplay p;
    p.observe({false,0,false});assert(!p.eligible());
    p.observe({true,1,false});assert(!p.eligible()); // First valid read is baseline only.
    p.observe({true,1,true});assert(p.eligible());
    assert(!p.projection_completed(true,1)); // Old-world projection cannot end quiescing.
    p.observe({false,2,false});assert(!p.eligible());
    p.observe({true,1,false});assert(p.eligible());
    assert(!p.projection_completed(true,1)); // Even an unquiesced old generation cannot clear.
    p.observe({true,2,false});assert(p.eligible());
    assert(!p.projection_completed(false,2));
    assert(!p.projection_completed(true,1));
    assert(p.projection_completed(true,2)&&!p.eligible());
    p.observe({true,2,false});assert(!p.eligible()); // No generic waiting-image fallback.
    p.observe({false,0,false});assert(!p.eligible());
    p.observe({true,3,false});assert(p.eligible()); // Missed quiescing, observed real advance.
    p.observe({true,2,false});assert(!p.eligible()); // Regressing metadata cannot authorize.
    p.observe({true,3,false});assert(p.eligible());
    assert(p.projection_completed(true,3));
    p.observe({true,1,true});assert(!p.eligible());
    p.observe({true,3,false});assert(!p.eligible()); // Regression cannot manufacture a wait.
    WorldHandoffDisplay first_quiescing;
    first_quiescing.observe({true,7,true});assert(first_quiescing.eligible());
    first_quiescing.observe({true,8,false});assert(first_quiescing.projection_completed(true,8));
    WorldHandoffDisplay invalid_only;
    invalid_only.observe({false,99,true});invalid_only.observe({true,100,false});assert(!invalid_only.eligible());
}
