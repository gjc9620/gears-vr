#include "ngx_typed_copy.h"
#include <cstdio>
#include <stdexcept>
using namespace gears_ngx_stereo;
static unsigned checks{};
static void check(bool ok){++checks;if(!ok)throw std::runtime_error("native Copy preservation CPU check failed");}
int main(){try{
 const uintptr_t output=0x400,color=0x100;
 D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=reinterpret_cast<ID3D12Resource*>(output);dst.pResource=reinterpret_cast<ID3D12Resource*>(color);
 src.Type=dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 auto accept=[&](bool attested,uintptr_t caller,UINT x,UINT y,UINT z,D3D12_BOX const* box){return preserves_native_copy(attested,caller,dst,x,y,z,src,box,output,color);};
 check(accept(true,0x1c6093,0,0,0,nullptr));check(!accept(false,0x1c6093,0,0,0,nullptr));
 check(!accept(true,0x1401c6093,0,0,0,nullptr));check(!accept(true,0x1c6092,0,0,0,nullptr));
 check(!accept(true,0,0,0,0,nullptr));
 for(UINT n:{1u,~0u}){check(!accept(true,0x1c6093,n,0,0,nullptr));check(!accept(true,0x1c6093,0,n,0,nullptr));check(!accept(true,0x1c6093,0,0,n,nullptr));}
 D3D12_BOX box{0,0,0,3840,2160,1};check(!accept(true,0x1c6093,0,0,0,&box));
 src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;check(!accept(true,0x1c6093,0,0,0,nullptr));src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;check(!accept(true,0x1c6093,0,0,0,nullptr));dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 src.SubresourceIndex=1;check(!accept(true,0x1c6093,0,0,0,nullptr));src.SubresourceIndex=0;
 dst.SubresourceIndex=1;check(!accept(true,0x1c6093,0,0,0,nullptr));dst.SubresourceIndex=0;
 src.pResource=reinterpret_cast<ID3D12Resource*>(0x401);check(!accept(true,0x1c6093,0,0,0,nullptr));src.pResource=reinterpret_cast<ID3D12Resource*>(output);
 dst.pResource=reinterpret_cast<ID3D12Resource*>(0x101);check(!accept(true,0x1c6093,0,0,0,nullptr));dst.pResource=reinterpret_cast<ID3D12Resource*>(color);
 check(!preserves_native_copy(true,0x1c6093,dst,0,0,0,src,nullptr,0,color));
 check(!preserves_native_copy(true,0x1c6093,dst,0,0,0,src,nullptr,output,0));
 check(!preserves_native_copy(true,0x1c6093,dst,0,0,0,src,nullptr,output,output));
 // The preserved native shape feeds the existing copy planner. It changes only
 // the rectangle and destination, keeping canonical source/destination IDs.
 Layout l;for(auto& e:l.physical)e={3840,2160};l.full_output={3840,2160};l.input={1280,1440};l.eye_output={1920,2160};l.stride=1920;l.input_base[1]={1920,0};
 Identity id{1,2,3,4,5};CopyFacts f;f.identity=id;f.ticket=7;f.caller=0x1c6093;f.list=8;f.generation=9;f.list_device=3;
 f.direct=true;f.copy_source=f.copy_dest=accept(true,0x1c6093,0,0,0,nullptr);f.input_base=l.input_base;f.feature_output=l.eye_output;
 for(unsigned i=0;i<4;++i)f.resources[i]={10+i,3,{3840,2160},26,0,1,0};
 CopySequence sequence(l,id);CopyPlan plan;check(sequence.next(f,0,plan));check(plan.source==13&&plan.target==10&&plan.list==8&&plan.generation==9&&plan.ticket==7);
 check((plan.source_box==std::array<uint32_t,6>{0,0,0,1920,2160,1})&&(plan.destination==std::array<uint32_t,3>{0,0,0}));
 f.evaluated_ordinal=1;f.list=18;f.generation=19;check(sequence.next(f,1,plan));check(plan.source==13&&plan.target==10&&plan.destination[0]==1920&&sequence.complete());
 check(!sequence.next(f,1,plan));
 for(unsigned bad=0;bad<5;++bad){CopySequence q(l,id);CopyFacts g=f;g.evaluated_ordinal=0;
  switch(bad){case 0:g.identity.world++;break;case 1:g.list=0;break;case 2:g.generation=0;break;case 3:g.ticket=0;break;case 4:g.evaluated_ordinal=1;break;}
  check(!q.next(g,0,plan));}
 std::printf("native Copy preservation CPU: %u checks passed; no COM calls/GPU work\n",checks);return 0;
 }catch(std::exception const& e){std::fprintf(stderr,"check %u: %s\n",checks,e.what());return 1;}}
