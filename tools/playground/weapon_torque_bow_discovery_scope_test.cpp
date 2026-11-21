#include <cstdio>
#include <vector>
FILE* log_file{}; // Current offline client diagnostics have no output sink here.
#include "weapon_torque_bow_fields_fixture.h"
namespace {
struct Values {std::vector<std::uint64_t> words;};
bool observe(Fixture& f,bool reuse,Values& out,std::function<void()> after_discovery={}){
 out={};Batch batch;batch.r.reuse_initial_identities=reuse;
 if(!batch.b.begin(batch.r,f.m->weapon))return false;
 if(after_discovery)after_discovery();
 Values candidate;
 for(unsigned i=0;i<11;++i){
  weaponhand::BoolField p{};
  if(!batch.b.binding(batch.r,static_cast<B::Index>(i),p))return false;
  assert(batch.schema.identities[p.property.identity].address==f.properties[i]);
  if(i==0){unsigned mask{},bits{};
   if(!batch.r.at(f.properties[i],0xd0,mask)||!mask||(mask&(mask-1))||!batch.r.at(f.m->weapon,p.property.offset,bits))return false;
   candidate.words.push_back(mask);candidate.words.push_back(bits&mask);
  }else if(i==2){unsigned char value{};if(!batch.r.at(f.m->weapon,p.property.offset,value))return false;candidate.words.push_back(value);}
  else {uptr value{};if(!batch.r.at(f.m->weapon,p.property.offset,value))return false;candidate.words.push_back(value);}
 }
 batch.r.reuse_initial_identities=false; // Same boundary as read_once, before timer/original queries.
 if(!batch.b.finish(batch.r)||batch.r.reuse_initial_identities)return false;
 out=std::move(candidate);return true;
}
}
int main(){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 {Fixture f;Values fresh,reused;std::uint64_t a{},b{};
  read_observer=[&](uptr,std::size_t){++a;};assert(observe(f,false,fresh));
  read_observer=[&](uptr,std::size_t){++b;};assert(observe(f,true,reused));read_observer={};
  assert(fresh.words==reused.words&&b<a);
  std::printf("Bow initial-discovery owned read calls: fresh=%llu, query-local reuse+fresh closure=%llu; not game FPS evidence.\n",a,b);}
 {Fixture f;Values value;assert(observe(f,true,value,[&]{Memory::put(f.m->weapon,0x138,f.m->pc);}));
  assert(value.words[2]==f.m->pc);Values next;
  assert(observe(f,true,next,[&]{Memory::put(f.m->weapon,0x138,f.m->pawn);}));
  assert(next.words[2]==f.m->pawn&&next.words!=value.words);} // Values and another query are never cached.
 {Fixture f;Values out;assert(!observe(f,true,out,[&]{Memory::put(f.m->weapon,0x50,f.m->weapon_class);}));assert(out.words.empty());}
 {Fixture f;Values out;assert(!observe(f,true,out,[&]{Memory::put(f.properties[4],0x94,0x1d0u);}));assert(out.words.empty());}
 {Fixture f;Values out;assert(!observe(f,true,out,[&]{f.m->property(f.m->concrete,L"AimingSound",f.m->object_type,0x1d0,8);f.m->tables();}));assert(out.words.empty());}
 {Fixture f;Batch b;b.r.reuse_initial_identities=true;assert(b.b.begin(b.r,f.m->weapon));
  Memory::put(f.m->weapon,0x50,f.m->weapon_class);
  assert(!b.b.finish(b.r)&&!b.r.reuse_initial_identities);} // finish itself refuses a cached closing check.
 for(unsigned mask:{0u,3u}){Fixture f;Values out;
  assert(!observe(f,true,out,[&]{Memory::put(f.properties[0],0xd0,mask);}));assert(out.words.empty());}
 {Fixture f;Values first,second;Memory::put(f.m->weapon,0x130,0u);assert(observe(f,true,first));
  Memory::put(f.m->weapon,0x130,2u);assert(observe(f,true,second));assert(first.words!=second.words);}
 {Fixture f;Values out;fail_read=f.m->weapon+0x138;assert(!observe(f,true,out));assert(out.words.empty());fail_read=0;}
 std::puts("Bow discovery scope: value equivalence/freshness, fresh closing selector, incarnation/property/shadow/mask/read failure rejection; no action or game backend.");
}
