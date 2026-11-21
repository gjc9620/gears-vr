#define BOOMSHOT_POSE_FIXTURE_ONLY 1
#include "weapon_boomshot_pose_metadata_test.cpp"
namespace {
unsigned original_calls{},source_current_calls{},boundary_failure{};
PoseFixture* current_fixture{};
namespace lancerreload_inventory {
struct Identity {
 abi::ObjectIdentity weapon{};std::uint64_t hold{};
 bool valid()const noexcept{return abi::identity_valid(weapon)&&hold;}
 bool operator==(Identity const& b)const noexcept{return abi::same(weapon,b.weapon)&&hold==b.hold;}
};
Identity identity(abi::Frame const& f) noexcept{return {f.weapon,vrequip::model.hold_generation()};}
}
namespace nativeequipment {
bool current(void*,abi::Frame const&)noexcept {++source_current_calls;return boundary_failure!=1;}
}
namespace nativelancerfollow {
bool bound_key_known{};
struct Bound {abi::ObjectIdentity weapon{},component{},asset{};std::uint64_t hold_generation{};}bound_key;
struct Mount {bool bound()const noexcept{return boundary_failure!=2;}}mount;
}
namespace hammerburst_animation {struct Parts {abi::ObjectIdentity component{},asset{};std::array<pose::Matrix,3> world{};};}
namespace boomshot_parts {
bool read(abi::Frame const&,hammerburst_animation::Parts& out)noexcept {
 ++original_calls;if(boundary_failure==3)return false;
 pose::Source source{};if(!current_fixture->inspect(source))return false;
 out={source.component,source.asset,source.world};
 if(boundary_failure==4)out.world[2][12]+=.003f;
 if(boundary_failure==5)++out.component.registry_index;
 if(boundary_failure==6)current_fixture->records[2][60]=1;
 if(boundary_failure==10)out.world[2][0]+=.00002f;
 return true;
}
}
#include "weapon_boomshot_pose_read.inl"
void setup(PoseFixture& f){current_fixture=&f;original_calls=source_current_calls=boundary_failure=0;
 pose::certification={};nativelancerfollow::bound_key_known=true;
 nativelancerfollow::bound_key={f.frame.weapon,f.evidence.component,f.evidence.asset,vrequip::model.hold_generation()};}
}
int main(){unsigned count{};
 {auto f=std::make_unique<PoseFixture>();setup(*f);assert(pose::certify(f->frame));
  for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)f->bones[1][row*4+col]=0;
  pose::Source current{};source_current_calls=0;assert(pose::ready(f->frame,current)&&current.magazine_collapsed&&source_current_calls==0&&original_calls==1);
  setup(*f);assert(pose::certify(f->frame)&&pose::certification.source.magazine_collapsed);++count;}
 {auto f=std::make_unique<PoseFixture>();setup(*f);pose::Source current{};
  assert(!pose::ready(f->frame,current)&&original_calls==0&&source_current_calls==0);
  assert(pose::certify(f->frame)&&original_calls==1);
  const auto old_calls=source_current_calls;assert(pose::certify(f->frame)&&original_calls==1&&source_current_calls==old_calls+1);
  source_current_calls=0;assert(pose::ready(f->frame,current)&&original_calls==1&&source_current_calls==0);
  assert(current.world==pose::certification.source.world);++count;}
 {auto f=std::make_unique<PoseFixture>();setup(*f);assert(pose::certify(f->frame));
  const auto old=pose::certification.source.world[2][12];f->component_matrix[12]+=7;
  Memory::put(f->component,0x180,f->component_matrix);++f->frame.key.frame;
  pose::Source current{};source_current_calls=0;
  assert(pose::ready(f->frame,current)&&current.world[2][12]==old+7&&original_calls==1&&source_current_calls==0);++count;}
 {auto f=std::make_unique<PoseFixture>();setup(*f);assert(pose::certify(f->frame));
  f->records[1][60]=1;pose::Source current{};source_current_calls=0;
  assert(!pose::ready(f->frame,current)&&!pose::certification.known&&original_calls==1&&source_current_calls==0);
  assert(pose::certify(f->frame)&&original_calls==2);++count;}
 for(unsigned reason=1;reason<=10;++reason){auto f=std::make_unique<PoseFixture>();setup(*f);boundary_failure=reason;
  if(reason==7)nativelancerfollow::bound_key_known=false;
  if(reason==8)++nativelancerfollow::bound_key.asset.registry_index;
  if(reason==9)Memory::put(f->component,0x168,f->mesh);
  assert(!pose::certify(f->frame)&&!pose::certification.known);++count;}
 {auto f=std::make_unique<PoseFixture>();setup(*f);assert(pose::certify(f->frame));
  game_thread.store(0);pose::Source current{};assert(!pose::ready(f->frame,current)&&!pose::certify(f->frame)&&original_calls==1);
  game_thread.store(GetCurrentThreadId());++count;}
 {auto f=std::make_unique<PoseFixture>();setup(*f);assert(pose::certify(f->frame));
  ++nativelancerfollow::bound_key.hold_generation;pose::Source current{};source_current_calls=0;
  assert(!pose::ready(f->frame,current)&&original_calls==1&&source_current_calls==0);++count;}
 std::printf("Boomshot current pose reader: %u boundary cases passed; actual Reader, original-getter comparison stub, fresh callback matrices, no callback client/current/engine calls; no live-game claim\n",count);return 0;
}
