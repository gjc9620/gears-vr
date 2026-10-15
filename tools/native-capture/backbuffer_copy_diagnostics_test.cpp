#include "backbuffer_copy_diagnostics.h"
#include <cassert>
#include <cstdio>
using namespace gears_backbuffer_copy;
int main(){
    // Parser accepts copied POD only; these sentinels are never dereferenced.
    D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=reinterpret_cast<ID3D12Resource*>(1);
    dst.pResource=reinterpret_cast<ID3D12Resource*>(2);src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=3;
    dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint.Offset=512;
    dst.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,1920,1081,1,7680};
    D3D12_BOX bounds{1,2,0,100,101,1};Key key{};
    assert(decode(src,dst,4,5,6,&bounds,key));assert(key.src.type==0&&key.src.subresource==3&&key.dst.type==1&&key.dst.subresource==UINT32_MAX);
    assert(key.dst.offset==512&&key.dst.footprint[2]==1081&&key.dst.footprint[4]==7680&&key.has_box&&key.box[4]==101&&key.xyz[2]==6);
    Key full{};assert(decode(src,dst,0,0,0,nullptr,full)&&!full.has_box&&full.box[0]==0);
    auto bad=src;bad.Type=static_cast<D3D12_TEXTURE_COPY_TYPE>(99);assert(!decode(bad,dst,0,0,0,nullptr,full));
    bad=src;bad.pResource=nullptr;assert(!decode(bad,dst,0,0,0,nullptr,full));
    uintptr_t rejected[]{17,0,19};assert(match_identity(17,false,rejected,3)==2&&match_identity(17,true,rejected,3)==3&&
        match_identity(20,true,rejected,3)==1&&match_identity(20,false,rejected,3)==0&&match_identity(0,true,rejected,3)==0);
    struct Payload{unsigned desc_queries{};};Recorder<Payload> recorder;Epoch epoch{1,2,3};key.source=17;key.destination=20;key.source_match=2;key.xyz[0]=0;
    auto* row=recorder.admit(epoch,key,7,10);assert(row);row->payload.desc_queries=2;
    // Repeated copies retain their first snapshot despite newer candidate context.
    assert(!recorder.admit(epoch,key,8,20));assert(recorder.size==1&&row->count==2&&row->first_qpc==10&&row->last_qpc==20&&
        row->last_candidate_ticket==8&&row->payload.desc_queries==2);
    for(unsigned i=1;i<8;++i){key.xyz[0]=i;assert(recorder.admit(epoch,key,0,30));}
    key.xyz[0]=99;assert(!recorder.admit(epoch,key,0,40)&&recorder.dropped==1);
    for(unsigned e=3;e<=5;++e){epoch.inventory=e;for(unsigned i=0;i<8;++i){key.xyz[0]=i;assert(recorder.admit(epoch,key,0,50));}}
    epoch.world=2;assert(!recorder.admit(epoch,key,0,60)&&recorder.size==32&&recorder.dropped==2);
    auto saved=recorder.rows[0];assert(!recorder.admit(saved.epoch,saved.key,9,70)&&recorder.rows[0].count==3&&recorder.dropped==2);
    Recorder<Payload> differences;epoch={1,1,1};assert(differences.admit(epoch,key,0,1));
    key.src.subresource++;assert(differences.admit(epoch,key,0,2));key.dst.footprint[4]++;assert(differences.admit(epoch,key,0,3));
    key.has_box=false;assert(differences.admit(epoch,key,0,4));key.destination++;assert(differences.admit(epoch,key,0,5));
    std::puts("backbuffer copy parser, identity matching, coalescing and epoch bounds passed");return 0;
}
