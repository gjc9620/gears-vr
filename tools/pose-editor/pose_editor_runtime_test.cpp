#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <cassert>
#include <cstring>
#include "../native-capture/atomic_status_file.h"
bool safe_copy(void* out,void const* in,std::size_t n){std::memcpy(out,in,n);return true;}
#include "../native-capture/pose_editor_live.inl"
void write(gears_pose_editor::Config c){
    const auto wire=gears_pose_editor::encode(c);FILE* f{};
    assert(!_wfopen_s(&f,(pose_editor_live::channel+L"\\preview.bin").c_str(),L"wb"));
    assert(fwrite(wire.data(),1,wire.size(),f)==wire.size());fclose(f);
    pose_editor_live::next_poll=0;pose_editor_live::worker_tick();
}
int wmain(int argc,wchar_t** argv){
    assert(argc==2);CreateDirectoryW(argv[1],nullptr);
    pose_editor_live::enabled=true;pose_editor_live::channel=argv[1];
    gears_pose_editor::Config c{},out{};assert(!pose_editor_live::snapshot(out));
    c.revision=10;c.enabled=1;write(c);assert(pose_editor_live::snapshot(out)&&out.revision==10&&out.enabled);
    c.revision=9;write(c);assert(pose_editor_live::snapshot(out)&&out.revision==10);
    c.revision=10;c.palm.position.x=.001;write(c);assert(pose_editor_live::snapshot(out)&&out.palm.position.x==0);
    c.revision=11;c.palm.position.x=1.;write(c);assert(pose_editor_live::snapshot(out)&&out.revision==10);
    c.revision=12;c.palm={};c.enabled=0;write(c);assert(pose_editor_live::snapshot(out)&&out.revision==12&&!out.enabled);
    c.revision=13;c.enabled=1;c.reserved=1|768;c.palm.position.y=.01;write(c);
    assert(pose_editor_live::snapshot(out,1)&&out.revision==13&&out.palm.position.y==.01);
    assert(pose_editor_live::snapshot(out,0)&&out.revision==12&&!out.enabled);
    c.revision=14;c.reserved=4|768;c.palm.position.y=.02;write(c);
    assert(pose_editor_live::snapshot(out,4)&&out.revision==14&&out.palm.position.y==.02);
    assert(pose_editor_live::snapshot(out,1)&&out.revision==13&&out.palm.position.y==.01);
    c.revision=15;c.enabled=0;write(c);
    for(unsigned part=0;part<5;++part)assert(pose_editor_live::snapshot(out,part)&&out.revision==15&&!out.enabled);
    assert(!pose_editor_live::finger_revision.load()&&!pose_editor_live::mount_revision.load());
    pose_editor_live::receipt(12,1,77);pose_editor_live::receipt(12,2,88);
    assert(pose_editor_live::finger_revision.load()==12&&pose_editor_live::mount_revision.load()==12);
    pose_editor_live::next_poll=0;pose_editor_live::worker_tick(true);
    assert(pose_editor_live::rejected==3);
    std::puts("Real worker file reload, invalid/replayed revision retention, restoration, independent receipts and stop status passed (host fixture, not a game)");
}
