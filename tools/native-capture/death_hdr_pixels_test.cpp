#include "death_hdr_pixels.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace gears_death_hdr_pixels;
    Texture src{10,3840,2160,26,3,1,1,1,0},dst=src;dst.canonical=20;
    gears_death_hdr::Event e{};e.quad_caller=0x2ab6d9;e.view.eye_valid=true;e.target_count=1;
    e.target={1,3840,2160,26,0};
    e.rectangles.tracked=true;e.rectangles.viewport_count=1;e.rectangles.viewport={0,0,1920,2160};
    e.args={0,0,1920,2160,0,0,1920,2160};e.dimensions={1920,2160,3840,2160};
    Trigger trigger;trigger.sample(7,1,3,1,e,src);assert(!trigger.eyes);
    trigger.sample(7,1,49,166,e,src);assert(trigger.eyes==1);
    assert(!trigger.copy(7,1,49,167,0x1c6093,src,dst,true)&&!trigger.copies);
    e.view.eye=1;e.rectangles.viewport.x=1920;e.args[4]=1920;trigger.sample(7,1,49,168,e,src);assert(trigger.eyes==3);
    assert(!trigger.copy(7,1,49,184,0x1c6093,src,dst,false)&&!trigger.copies);
    assert(!trigger.copy(7,1,49,167,0x1c6093,src,dst,true)&&!trigger.copies);
    auto wrong=src;wrong.canonical=11;assert(!trigger.copy(7,1,49,184,0x1c6093,wrong,dst,true));
    assert(!trigger.copy(7,1,49,184,0x1c6092,src,dst,true));
    assert(!trigger.copy(7,1,49,184,0x1c6093,src,dst,true)&&trigger.copies==1);
    assert(!trigger.copy(7,1,49,184,0x1c6093,src,dst,true)&&trigger.copies==1);
    assert(trigger.copy(7,1,49,201,0x1c6093,src,dst,true)&&trigger.fired);
    assert(!trigger.copy(7,1,49,202,0x1c6093,src,dst,true));
    Trigger mismatch;mismatch.sample(7,1,49,1,e,src);assert(mismatch.invalid); // Right before left.
    e.view.eye=0;e.rectangles.viewport.x=0;e.args[4]=0;
    Trigger shapes;auto bad=e;bad.args[0]=1920;shapes.sample(7,1,49,1,bad,src);assert(!shapes.eyes);
    bad=e;bad.rectangles.viewport.x=1920;shapes.sample(7,1,49,2,bad,src);assert(!shapes.eyes);
    shapes.sample(7,1,49,3,e,src);shapes.sample(7,1,49,4,e,src);assert(shapes.invalid);
    Trigger targets;targets.sample(7,1,49,1,e,src);e.view.eye=1;e.rectangles.viewport.x=1920;e.args[4]=1920;
    auto other=src;other.canonical=11;targets.sample(7,1,49,2,e,other);assert(targets.invalid);
    for(unsigned order=0;order<2;++order){Lifetime life;life.appended=true;assert(life.before(true,1,90,true));life.after(true);
        if(!order){life.completion(1);assert(!life.may_map());life.reset(true,true);}else{life.reset(true,true);assert(!life.may_map());life.completion(1);}
        assert(life.may_map());life.saved=true;assert(!life.may_map());}
    for(unsigned reason=0;reason<5;++reason){Lifetime life;life.appended=true;
        if(reason==0){assert(life.before(true,1,90,true));life.after(true);assert(!life.before(true,1,90,true));}
        if(reason==1)assert(!life.before(false,0,0,false));
        if(reason==2){assert(life.before(true,1,90,true));life.after(false);}
        if(reason==3){assert(life.before(true,1,90,true));life.after(true);assert(!life.before(true,1,91,true));}
        if(reason==4)life.stop();
        life.reset(true,true);life.completion(1);assert(life.failure&&!life.may_map());}
    Lifetime early_reset;early_reset.appended=true;early_reset.reset(true,true);assert(early_reset.failure==reset_before_execute&&!early_reset.may_map());
    Lifetime removed;removed.appended=true;assert(removed.before(true,1,90,true));removed.after(true);removed.reset(true,true);
    removed.completion(UINT64_MAX);assert(removed.failure==device_removed&&!removed.may_map());
    Lifetime unrelated;unrelated.appended=true;assert(!unrelated.before(true,0,0,false)&&!unrelated.failure);
    assert(!unrelated.before(true,2,90,true)&&unrelated.failure==duplicate_execution);
    assert(footprint_valid(src,33177600,0,15360,2160,15360,3840,2160,1,26));
    assert(!footprint_valid(src,33177599,0,15360,2160,15360,3840,2160,1,26));
    assert(!footprint_valid(src,33177600,1,15360,2160,15360,3840,2160,1,26));
    assert(!footprint_valid(src,33177600,0,15361,2160,15360,3840,2160,1,26));
    assert(!footprint_valid(src,UINT64_MAX,0,15360,2160,15360,3840,2160,1,26));
    std::puts("DeathHdrPixelsTest passed");
}
