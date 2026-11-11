#define GEARS_AUTHORED_INVENTORY_TEST_MAIN PriorGeometryInventoryMain
#include "cover_authored_inventory_live_test.cpp"
#undef GEARS_AUTHORED_INVENTORY_TEST_MAIN
#include "cover_authored_geometry_live.inl"
#include "cover_body_query_catalog.h"
#include <bcrypt.h>
#include <fstream>
#include <string>
#pragma push_macro("C")
#undef C
namespace oracle {
#define main PriorGeometryOracleMain
#include "cover_primitive_world_test.cpp"
#undef main
}
#pragma pop_macro("C")
__declspec(noinline) static int geometry_cases(std::string const& path){
 namespace geo=cover_authored_geometry_live;namespace live=cover_authored_inventory_live;namespace a=gears_cover_authored_array;
 auto fixture=std::make_unique<oracle::Fixture>();C(fixture->init(path));if(failures)return 97;
 // The reusable oracle's default component belongs to its older fixture.
 // Derive only the oracle pose from THIS retained planar component matrix;
 // leave every input frame byte unchanged and use independent quaternion FK.
 const auto& component=fixture->frame.component_to_world.values;
 C(component[2]==0&&component[6]==0&&component[8]==0&&component[9]==0&&component[10]==1);
 const double yaw=std::atan2(component[1],component[0]);
 fixture->component={{component[12],component[13],component[14]},{0,0,std::sin(yaw*.5),std::cos(yaw*.5)}};
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 inventory_fixture();const unsigned selected[]={11,8};
 for(unsigned n=0;n<2;++n)put(setup_members[n]+0xb4,oracle::names::detail::name(fixture->frame.reference[selected[n]]));
 roles::Snapshot typed{};if(!observe(typed))return 98;
 auto read_work=std::make_unique<live::Workspace>();auto inventory=std::make_unique<live::Snapshot>();
 auto owner=[&](roles::Snapshot const& expected){roles::Snapshot fresh{};
  return roles::sample(actor,physics_expected,guard,workspace,fresh)==roles::Code::current&&roles::same(fresh,expected);};
 C(live::capture(typed,safe_copy,owner,*read_work,*inventory)==live::Code::captured&&inventory->count==7);if(failures)return 96;
 auto work=std::make_unique<geo::Workspace>();auto output=std::make_unique<geo::Result>();
 C(geo::convert(typed,*inventory,fixture->model,fixture->frame,*work,*output)==geo::Code::converted);
 C(output->mapping.count==2&&output->mapping.rows[0].bone_index==11&&output->mapping.rows[1].bone_index==8&&output->mapping.root_bone_index==8);
 C(output->geometry.count==7&&output->no_rb_collision[6]);
 const auto frame_before=std::make_unique<a::g::body::Frame>(fixture->frame);
 namespace catalog=gears_cover_body_query_catalog;
 auto query_work=std::make_unique<catalog::Workspace>();auto queries=std::make_unique<catalog::Catalog>();
 C(catalog::prepare(output->mapping,inventory->inventory.data(),2,output->geometry,*query_work,*queries,.002)==catalog::Code::prepared&&queries->count==7);
 for(unsigned i=0;i<7;++i){const auto& actual=output->geometry.rows[i].primitive;const auto& raw=inventory->rows[i];
  fixture->input.body_index=actual.body_index?4:1;fixture->input.shape_in_bone={{double(raw.shape.array_index)*6,0,0},{0,0,0,1}};
  const auto expected=fixture->expected();C(oracle::close(actual.shape_world.position,expected.position)&&oracle::same_q(actual.shape_world.orientation,expected.orientation));
  if(actual.dimensions.kind==a::g::metrics::Kind::box)C((actual.dimensions.box_half_extents==std::array<float,3>{6,9,12}));
  else C(actual.dimensions.radius==12);
  if(actual.dimensions.kind==a::g::metrics::Kind::sphyl){const auto axis=oracle::rotate(expected.orientation,{0,0,1});
   C(actual.dimensions.half_segment==9&&oracle::close(actual.segment_axis_world,axis,1e-6));
   C(oracle::close(actual.segment_start,oracle::add(expected.position,oracle::mul(axis,-9)))&&oracle::close(actual.segment_end,oracle::add(expected.position,oracle::mul(axis,9))));}
 }
 const auto saved=std::make_unique<geo::Result>(*output);const auto original=std::make_unique<live::Snapshot>(*inventory);
 for(unsigned bad=0;bad<10;++bad){
  if(bad==0)inventory->rows[0].shape.authored.raw_dimensions[0]=99;
  if(bad==1)inventory->rows[6].collision_disabled=false;
  if(bad==2)++inventory->inventory[1].count[2];if(bad==3)inventory->collision_masks[1][2]=0;
  if(bad==4)inventory->rows[0].shape.authored.bone.number++;
  if(bad==5)inventory->scaling.value={1,1,1};
  if(bad==6)inventory->membership.mesh.address+=8;
  if(bad==7)inventory->rows[0].raw[0]^=1;
  if(bad==8)inventory->inventory[0].count[3]=1;
  if(bad==9)inventory->count=0;
  C(geo::convert(typed,*inventory,fixture->model,fixture->frame,*work,*output)!=geo::Code::converted&&!memcmp(saved.get(),output.get(),sizeof *output));
  *inventory=*original;
 }
 auto wrong_frame=std::make_unique<a::g::body::Frame>(fixture->frame);wrong_frame->local[11].scale=2;
 C(geo::convert(typed,*inventory,fixture->model,*wrong_frame,*work,*output)==geo::Code::geometry&&!memcmp(saved.get(),output.get(),sizeof *output));
 C(geo::convert(typed,*inventory,fixture->model,fixture->frame,*work,work->candidate)==geo::Code::alias);
 for(unsigned mismatch=0;mismatch<2;++mismatch){auto wrong=inventory->membership.rows[1].bone;
  if(!mismatch)++wrong.number;else wrong=inventory->membership.rows[0].bone;
  inventory->membership.rows[1].bone=wrong;
  for(unsigned i=0;i<inventory->count;++i)if(inventory->rows[i].shape.authored.body_index==1)inventory->rows[i].shape.authored.bone=wrong;
  C(geo::convert(typed,*inventory,fixture->model,fixture->frame,*work,*output)==geo::Code::mapping&&!memcmp(saved.get(),output.get(),sizeof *output));
  *inventory=*original;
 }
 // This flag remains attached to its row; changing it consistently does not
 // silently remove the shape from the complete authored geometry.
 inventory->rows[6].collision_disabled=false;inventory->rows[6].collision_word=0;
 const unsigned word_offset=72;memset(inventory->rows[6].raw.data()+word_offset,0,4);
 C(geo::convert(typed,*inventory,fixture->model,fixture->frame,*work,*output)==geo::Code::converted&&output->geometry.count==7&&!output->no_rb_collision[6]&&
   !memcmp(&saved->geometry,&output->geometry,sizeof output->geometry));
 C(!memcmp(frame_before.get(),&fixture->frame,sizeof fixture->frame));
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
 printf("AuthoredGeometry checks=%u failures=%u actual saved C0 + synthetic source shapes; no active physics permission\n",checks,failures);return failures?1:0;
}
int main(int argc,char** argv){if(argc!=2)return 2;if(PriorGeometryInventoryMain())return 1;return geometry_cases(argv[1]);}
