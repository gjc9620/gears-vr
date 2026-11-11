#define GEARS_POINT_SCALE_TEST_MAIN PriorInventoryScaleMain
#include "cover_point_scale_live_test.cpp"
#undef GEARS_POINT_SCALE_TEST_MAIN
#include "cover_authored_inventory_live.inl"
static uintptr_t inventory_payload[2][3]{};
static uintptr_t inventory_base(unsigned n){return setup_members[n]+0x6c;}
static void inventory_rows(unsigned n,unsigned k,unsigned count){namespace a=gears_cover_authored_array;
 const auto payload=arena+0xc0000+n*0x5000+k*0x1800;inventory_payload[n][k]=payload;
 put(inventory_base(n)+k*16,a::s::pose::ArrayHeader{count?payload:0,int(count),int(count)});
 for(unsigned i=0;i<count;++i){std::array<unsigned char,80> raw{};float tm[16]{};tm[0]=tm[5]=tm[10]=tm[15]=1;tm[12]=float(i);
  memcpy(raw.data(),tm,64);float dimensions[]{2,3,4};memcpy(raw.data()+64,dimensions,(k==0?1:k==1?3:2)*4);
  const unsigned disabled=(n==1&&k==2&&i==1)?1:0;memcpy(raw.data()+64+(k==0?1:k==1?3:2)*4,&disabled,4);
  memcpy(reinterpret_cast<void*>(payload+i*80),raw.data(),80);}
}
static void inventory_fixture(){scale_fixture(0);put(get<uintptr_t>(schema_owner+0x80)+0x94,0x6cu);
 uintptr_t fields[4]{};auto child=get<uintptr_t>(array_aggregate+0x80);
 for(unsigned k=0;k<4;++k){fields[k]=child;child=get<uintptr_t>(child+0x68);}
 const auto render=make_field(array_aggregate,L"RenderInfo",structure_kind,8,64,pointer_type);put(fields[3]+0x68,render);
 for(unsigned k=1;k<3;++k){const auto element=arena+0xe7000+k*0x400,element_inner=arena+0xe5000+k*0x200;
  object(element,schema_index++,script_kind,schema_owner,k==1?L"KBoxElem":L"KSphylElem");put(element+0x88,k==1?80u:76u);
  typed_property(element_inner,schema_index++,fields[k],L"Inner",structure_kind,80,0,element);put(fields[k]+0xd0,element_inner);
  auto last=make_field(element,L"TM",structure_kind,64,0,schema_matrix);put(element+0x80,last);
  const wchar_t* box[]={L"X",L"Y",L"Z"};const wchar_t* sphyl[]={L"Radius",L"Length"};const unsigned dimensions=k==1?3:2;
  for(unsigned d=0;d<dimensions;++d){const auto field=make_field(element,k==1?box[d]:sphyl[d],schema_float,4,64+d*4);put(last+0x68,field);last=field;}
  const auto flag=make_field(element,L"bNoRBCollision",schema_bool,4,64+dimensions*4);put(last+0x68,flag);put(flag+0xd0,1u);
 }
 const auto convex=arena+0xe7c00,convex_inner=arena+0xe5c00;
 object(convex,schema_index++,script_kind,schema_owner,L"KConvexElem");put(convex+0x88,40u);
 typed_property(convex_inner,schema_index++,fields[3],L"Inner",structure_kind,40,0,convex);put(fields[3]+0xd0,convex_inner);
 for(unsigned n=0;n<2;++n){put(inventory_base(n)+48,roles::pose::ArrayHeader{});
  for(unsigned k=0;k<3;++k)inventory_rows(n,k,n==0?(k==1?2:1):(k==0?0:k));}
}
__declspec(noinline) static int inventory_cases(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 namespace live=cover_authored_inventory_live;namespace a=gears_cover_authored_array;using Counts=std::array<unsigned,4>;
 auto work=std::make_unique<live::Workspace>();auto output=std::make_unique<live::Snapshot>();
 inventory_fixture();roles::Snapshot typed{};if(!observe(typed))return 98;
 unsigned outer_calls{};bool outside_lock=true;
 auto owner=[&](roles::Snapshot const& expected){++outer_calls;
  if(!h::mutex.try_lock()){outside_lock=false;return false;}h::mutex.unlock();roles::Snapshot fresh{};
  return roles::sample(actor,physics_expected,guard,workspace,fresh)==roles::Code::current&&roles::same(fresh,expected);};
 C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::captured&&output->count==7&&output->membership.count==2&&outside_lock&&outer_calls==2);
 C(output->inventory[0].count==Counts({1,2,1,0})&&output->inventory[1].count==Counts({0,1,2,0}));
 unsigned at{},disabled{};for(unsigned n=0;n<2;++n)for(unsigned k=0;k<3;++k)for(unsigned i=0;i<output->inventory[n].count[k];++i){const auto& row=output->rows[at++];
  C(row.shape.authored.body_index==n&&unsigned(row.shape.authored.kind)==k&&row.shape.array_index==i&&row.shape.authored.receiver_scale[0]==6);
  disabled+=row.collision_disabled?1:0;}
 C(disabled==1);auto saved=std::make_unique<live::Snapshot>(*output);
 a::Source packed{};packed.header=inventory_base(0);packed.bone=output->membership.rows[0].bone;
 packed.reflected_extent=72;packed.collision_mask=1;packed.receiver_scale=output->scaling.value;
 auto certified_header=[](a::Source const&){return true;};
 C((packed.header&7)==4&&a::capture(packed,safe_copy,certified_header,work->array_work,work->array_result)==a::Code::captured);
 ++packed.header;C(a::capture(packed,safe_copy,certified_header,work->array_work,work->array_result)==a::Code::request);
 for(unsigned mutation=0;mutation<3;++mutation){bool fired=false;
  auto read=[&](void* dst,void const* ptr,size_t size){if(!fired&&reinterpret_cast<uintptr_t>(ptr)==inventory_payload[1][2]){fired=true;
    if(mutation==0)put(inventory_payload[0][0]+64,3.f);if(mutation==1)put(schema_last+0x94,8u);if(mutation==2)put(setup_list,setup_members[1]);}
    return safe_copy(dst,ptr,size);};
  const auto verdict=live::capture(typed,read,owner,*work,*output);
  C(verdict!=live::Code::captured&&fired&&!memcmp(saved.get(),output.get(),sizeof *output)&&!work->busy);
  if(mutation==0)C(verdict==live::Code::changed);
  put(inventory_payload[0][0]+64,2.f);put(schema_last+0x94,12u);put(setup_list,setup_members[0]);
 }
 put(inventory_base(1)+48,roles::pose::ArrayHeader{arena+0xe9000,1,1});
 C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::unsupported&&work->failed_body==1&&work->failed_kind==3&&!memcmp(saved.get(),output.get(),sizeof *output));
 put(inventory_base(1)+48,roles::pose::ArrayHeader{});
 auto convex_field=get<uintptr_t>(array_aggregate+0x80);for(unsigned n=0;n<3;++n)convex_field=get<uintptr_t>(convex_field+0x68);
 const auto convex_inner=get<uintptr_t>(convex_field+0xd0),render_field=get<uintptr_t>(convex_field+0x68);
 for(unsigned malformed=0;malformed<3;++malformed){
  if(malformed==0)put(convex_inner+0x74,36u);if(malformed==1)put(render_field+0x74,16u);
  if(malformed==2){const auto extra=make_field(array_aggregate,L"UnknownGeometry",schema_bool,4,68);put(render_field+0x68,extra);}
  C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::layout&&!memcmp(saved.get(),output.get(),sizeof *output));
  put(convex_inner+0x74,40u);put(render_field+0x74,8u);put(render_field+0x68,uintptr_t{});
 }
 unsigned reads_raw{};auto counted=[&](void* dst,void const* ptr,size_t size){++reads_raw;return safe_copy(dst,ptr,size);};
 auto reject_owner=[](roles::Snapshot const&){return false;};
 C(live::capture(typed,counted,reject_owner,*work,*output)==live::Code::guard&&!reads_raw&&!memcmp(saved.get(),output.get(),sizeof *output));
 unsigned final_calls{};auto reject_final=[&](roles::Snapshot const& expected){return ++final_calls==1&&owner(expected);};
 C(live::capture(typed,safe_copy,reject_final,*work,*output)==live::Code::guard&&final_calls==2&&!memcmp(saved.get(),output.get(),sizeof *output));
 auto throw_owner=[](roles::Snapshot const&)->bool{throw 1;};
 C(live::capture(typed,safe_copy,throw_owner,*work,*output)==live::Code::exception&&!work->busy&&!memcmp(saved.get(),output.get(),sizeof *output));
 auto exception_reader=[](void*,void const*,size_t)->bool{throw 1;};
 C(live::capture(typed,exception_reader,owner,*work,*output)==live::Code::array&&!work->busy&&!memcmp(saved.get(),output.get(),sizeof *output));
 C(live::capture(typed,safe_copy,owner,*work,work->first)==live::Code::alias);
 bool entered=false;live::Code reentry{};
 auto reentrant=[&](void* dst,void const* ptr,size_t size){if(!entered){entered=true;reentry=live::capture(typed,safe_copy,owner,*work,*output);}return safe_copy(dst,ptr,size);};
 C(live::capture(typed,reentrant,owner,*work,*output)==live::Code::captured&&entered&&reentry==live::Code::busy);
 for(unsigned n=0;n<2;++n)for(unsigned k=0;k<3;++k)inventory_rows(n,k,64);
 C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::capacity&&!memcmp(saved.get(),output.get(),sizeof *output));
 inventory_rows(1,1,0);inventory_rows(1,2,0);
 C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::captured&&output->count==256);*saved=*output;
 inventory_rows(1,1,1);
 C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::capacity&&!memcmp(saved.get(),output.get(),sizeof *output));
 for(unsigned n=0;n<2;++n)for(unsigned k=0;k<3;++k)inventory_rows(n,k,0);
 C(live::capture(typed,safe_copy,owner,*work,*output)==live::Code::captured&&output->count==0&&output->membership.count==2);
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("AuthoredInventory checks=%u failures=%u synthetic complete authored arrays; no active/world permission\n",checks,failures);return failures?1:0;
}
int main(){if(PriorInventoryScaleMain())return 1;return inventory_cases();}
