#include "cover_body_query_catalog.h"
#define main PriorOverlapMain
#include "cover_body_overlap_test.cpp"
#undef main
int main(int argc,char** argv){if(argc!=2)return 2;
 auto f=std::make_unique<Fixture>();C(f->init(argv[1]));
 namespace c=gears_cover_body_query_catalog;auto work=std::make_unique<c::Workspace>();auto out=std::make_unique<c::Catalog>();
 C(c::prepare(f->mapping,f->inventory.data(),8,f->geometry,*work,*out,.01)==c::Code::prepared);
 C(out->body_count==8&&out->count==53);
 for(unsigned i=0;i<out->count;++i){const auto& s=out->rows[i].shape;
  C(s.body_index==f->geometry.rows[i].primitive.body_index&&s.bone_index==f->geometry.rows[i].primitive.bone_index);
  C(s.radius_expansion>=0&&s.radius_expansion<=.01);
  for(double v:s.box_half_expansion)C(v>=0&&v<=.01);
 }
 const auto saved=std::make_unique<c::Catalog>(*out);f->inventory[7].count[3]=1;
 C(c::prepare(f->mapping,f->inventory.data(),8,f->geometry,*work,*out,.01)==c::Code::unsupported&&!memcmp(saved.get(),out.get(),sizeof *out));
 printf("BodyCatalogC0 checks=%u failures=%u retained C0 geometry only; no current world/permission\n",checks,failures);return failures?1:0;
}
