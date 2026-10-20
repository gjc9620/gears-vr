#pragma once
#include "ui_canvas_batch.h"
#include <cmath>

// Read-only native candidates and an explicit z=0 Canvas projection check.
// No GPU IA, ticket or allocator epoch proof. Native originals run once.
namespace gears_ui_material {
struct Config {std::uintptr_t base{};};
namespace d=gears_ui_batch::detail;
inline constexpr unsigned item_bound=128;
struct Before {std::uintptr_t canvas{},fcanvas{};std::int32_t key{};unsigned count{};std::array<std::uintptr_t,item_bound> items{};};
struct Receipt {std::uintptr_t canvas{},fcanvas{},item{},payload{},proxy{};std::int32_t key{};unsigned kind{};std::array<unsigned char,80> transform{};std::array<unsigned char,36> record{};};
struct Consumer {Receipt receipt{};std::uintptr_t command{},view{},family{},target{};};
struct ViewPlane {float source_width{},source_height{},aspect{};};
enum class ViewPlaneReject : unsigned { none,read,extent,copy,nonfinite,identity,xy_w,scale,offset,changed };
// UE row-vector convention. Certifies only the copied native view's z=0 XY/W
// mapping; no IA, shader, OM target, ticket, or GPU visibility proof is supplied.
// Call while the native view is live, before original consumer destruction.
template<class Read> bool material_view_plane(Read read,Consumer const& s,ViewPlane& out,ViewPlaneReject* rejection=nullptr){
 d::Reader<Read> r{read};std::array<float,4> rect{},rect_after{};
 std::array<float,16> view{},projection{},view_after{},projection_after{};
 auto fail=[&](ViewPlaneReject reason){if(rejection)*rejection=reason;return false;};
 if(!s.view||!r.field(rect,s.view,0x48)||!r.field(view,s.view,0x70)||!r.field(projection,s.view,0xb0))return fail(ViewPlaneReject::read);
 for(float f:rect)if(!std::isfinite(f))return fail(ViewPlaneReject::extent);
 const double x=rect[0],y=rect[1],width=rect[2],height=rect[3];
 if(width<=0||height<=0||width>16384||height>16384||std::abs(x)>16384||std::abs(y)>16384||
    !std::isfinite(rect[2]/rect[3])||rect[2]/rect[3]<=0)return fail(ViewPlaneReject::extent);
 if(std::memcmp(projection.data(),s.receipt.transform.data(),64))return fail(ViewPlaneReject::copy);
 for(unsigned i=0;i<16;++i){
  if(!std::isfinite(view[i])||!std::isfinite(projection[i]))return fail(ViewPlaneReject::nonfinite);
  if(view[i]!=(i%5==0?1.0f:0.0f))return fail(ViewPlaneReject::identity);
 }
 // At z=0, row 2 cannot influence XY/W. Finite Z coefficients remain native.
 if(projection[1]!=0||projection[4]!=0||projection[3]!=0||projection[7]!=0||projection[15]!=1)return fail(ViewPlaneReject::xy_w);
 // Tiny arithmetic tolerance only, never a one-pixel scale/rotation allowance.
 if(std::abs(double(projection[0])*width-2)>1e-5||std::abs(double(projection[5])*height+2)>1e-5)return fail(ViewPlaneReject::scale);
 const double left=x*projection[0]+projection[12],right=(x+width)*projection[0]+projection[12];
 const double top=y*projection[5]+projection[13],bottom=(y+height)*projection[5]+projection[13];
 // Both ends must be within one source pixel, including half-pixel bias.
 const double pixel_limit=1.0+2e-7*(width+height); // Float matrix rounding only.
 if(std::abs(left+1)*width/2>pixel_limit||std::abs(right-1)*width/2>pixel_limit||
    std::abs(top-1)*height/2>pixel_limit||std::abs(bottom+1)*height/2>pixel_limit)return fail(ViewPlaneReject::offset);
 if(!r.field(rect_after,s.view,0x48)||!r.field(view_after,s.view,0x70)||!r.field(projection_after,s.view,0xb0)||
    std::memcmp(rect.data(),rect_after.data(),sizeof(rect))||std::memcmp(view.data(),view_after.data(),sizeof(view))||
    std::memcmp(projection.data(),projection_after.data(),sizeof(projection)))return fail(ViewPlaneReject::changed);
 out={rect[2],rect[3],rect[2]/rect[3]};if(rejection)*rejection=ViewPlaneReject::none;return true;
}
struct Quad {std::uintptr_t source{},index_data{};std::array<std::array<unsigned char,32>,4> vertices{};};
struct QuadDiagnostic {
 unsigned step{},stride{},first{},primitives{},minimum{},maximum{};std::uint16_t flags{};
 std::uintptr_t proxy{},index_data{},source{};
};
struct DrawArgs {bool indexed{};unsigned count{},instances{},start{},first_instance{};int base_vertex{};};
struct Candidate {std::uintptr_t payload{},view{},target{};unsigned kind{};bool indexed{};bool mutation_allowed{};};
inline constexpr std::array<unsigned char,32> execute_bytes{0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x68,0x18,0x48,0x89,0x70,0x20,0x57,0x48,0x83,0xec,0x70,0x48,0x8b,0xd9,0x48,0x8d,0x48,0x08,0xe8,0x08,0xfe,0xff,0xff};
inline constexpr std::array<unsigned char,32> factory_bytes{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x20,0x57,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0xc5,0xf8,0x29,0x70,0xe8,0xc5,0xf8,0x29,0x78,0xd8,0xc5,0x78,0x29};
inline constexpr std::array<unsigned char,5> mesh_flag_and_bytes{0x25,0x9d,0xf5,0xff,0xff},mesh_flag_or_bytes{0x0d,0x11,0x04,0x00,0x00};
// Native init computes (old & 0xfffff59d) | 0x411. Only these bits are fixed;
// the others survive initialization and are not a permission to replay a draw.
inline constexpr std::uint16_t mesh_fixed_flag_mask=0x0e73,mesh_fixed_flag_value=0x0411;
template<class Read> bool validate_runtime_binding(Config c,Read read){d::Reader<Read> r{read};std::array<unsigned char,32> b{};std::array<unsigned char,5> mask{},bits{};std::array<std::uintptr_t,3> table{};
 return c.base&&r.field(b,c.base,0x159fcc)&&b==factory_bytes&&
  r.field(mask,c.base,0x15a3ca)&&mask==mesh_flag_and_bytes&&r.field(bits,c.base,0x15a3d6)&&bits==mesh_flag_or_bytes&&
  r.field(table,c.base,0x136c5f0)&&table==std::array<std::uintptr_t,3>{c.base+0x3a514c,c.base+0x1ea9e0,c.base+0x1a44e0};}
template<class Read> bool validate_binding(Config c,Read read){d::Reader<Read> r{read};std::array<unsigned char,32> a{};return validate_runtime_binding(c,read)&&r.field(a,c.base,0x15a420)&&a==execute_bytes;}
template<class Read> bool snapshot_before(Config c,Read read,std::uintptr_t canvas,Before& out){
 d::Reader<Read> r{read};Before b{};b.canvas=canvas;d::Array keys{},elements{},items{};std::uintptr_t element{};
 if(!validate_runtime_binding(c,read)||!r.field(b.fcanvas,canvas,0x9c)||!r.field(keys,b.fcanvas,0x28)||!d::valid(keys,64,4)||!keys.count||!r.field(b.key,keys.data,std::size_t(keys.count-1)*4)||!r.field(elements,b.fcanvas,0x48)||!d::valid(elements,128,20))return false;
 for(int i=0;i<elements.count;++i){std::int32_t key{};std::uintptr_t at{};if(!d::add(elements.data,std::size_t(i)*20,at)||!r.field(key,at,0))return false;if(key==b.key){if(element||!r.field(items,at,4)||!d::valid(items,item_bound,8))return false;element=at;}}
 b.count=unsigned(items.count);for(unsigned i=0;i<b.count;++i)if(!r.field(b.items[i],items.data,i*8)||!b.items[i])return false;
 d::Array checked{};std::int32_t key{};std::uintptr_t fc{};
 if(!r.field(fc,canvas,0x9c)||fc!=b.fcanvas||!r.field(checked,b.fcanvas,0x28)||!d::same(checked,keys)||!r.field(key,keys.data,std::size_t(keys.count-1)*4)||key!=b.key||!r.field(checked,b.fcanvas,0x48)||!d::same(checked,elements))return false;
 if(element){if(!r.field(key,element,0)||key!=b.key||!r.field(checked,element,4)||!d::same(checked,items))return false;for(unsigned i=0;i<b.count;++i){std::uintptr_t p{};if(!r.field(p,items.data,i*8)||p!=b.items[i])return false;}}
 out=b;return true;
}
template<class Read> bool read_payload(Read read,std::uintptr_t payload,Receipt& s){d::Reader<Read> r{read};std::array<unsigned char,112> raw{},after{};d::Array records{};
 if(!r.bytes(raw.data(),payload,raw.size()))return false;std::memcpy(&records,raw.data()+0x60,16);
 if(!d::valid(records,128,36)||records.count!=1||!r.bytes(s.record.data(),records.data,36)||!r.bytes(after.data(),payload,after.size())||raw!=after)return false;
 std::array<unsigned char,36> checked{};if(!r.bytes(checked.data(),records.data,36)||checked!=s.record)return false;
 std::memcpy(&s.proxy,raw.data(),8);std::memcpy(s.transform.data(),raw.data()+0x10,80);if(!s.proxy)return false;
 for(unsigned i=0;i<8;++i){float f{};std::memcpy(&f,s.record.data()+i*4,4);if(!std::isfinite(f))return false;}return true;}
template<class Read> bool snapshot_after(Config c,Read read,Before const& old,unsigned pure_screen_kind,Receipt& out){
 if(pure_screen_kind>3)return false;Before now{};if(!snapshot_before(c,read,old.canvas,now)||now.fcanvas!=old.fcanvas||now.key!=old.key||old.count>=item_bound||now.count!=old.count+1)return false;
 for(unsigned i=0;i<old.count;++i)if(now.items[i]!=old.items[i]||now.items[i]==now.items[old.count])return false;
 Receipt s{};s.canvas=old.canvas;s.fcanvas=old.fcanvas;s.key=old.key;s.kind=pure_screen_kind;s.item=now.items[old.count];d::Reader<Read> r{read};std::array<unsigned char,20> item{},checked{};std::uintptr_t table{};unsigned flags{};
 if(!r.bytes(item.data(),s.item,item.size()))return false;std::memcpy(&table,item.data(),8);std::memcpy(&s.payload,item.data()+8,8);std::memcpy(&flags,item.data()+16,4);
 if(table!=c.base+0x136c5f0||flags||!read_payload(read,s.payload,s)||!r.bytes(checked.data(),s.item,20)||checked!=item)return false;
 Before final{};if(!snapshot_before(c,read,old.canvas,final)||final.canvas!=now.canvas||final.fcanvas!=now.fcanvas||final.key!=now.key||final.count!=now.count||final.items!=now.items)return false;out=s;return true;
}
template<class Read> bool begin_consumer(Config c,Read read,Receipt const& receipt,std::uintptr_t command,Consumer& out){
 if(!validate_runtime_binding(c,read)||receipt.kind>3)return false;d::Reader<Read> r{read};std::array<unsigned char,36> header{},after{};std::uintptr_t table{},payload{};unsigned mode{},flags{};Consumer s{};s.receipt=receipt;s.command=command;
 if(!r.bytes(header.data(),command,36))return false;std::memcpy(&table,header.data(),8);std::memcpy(&s.view,header.data()+8,8);std::memcpy(&payload,header.data()+16,8);std::memcpy(&mode,header.data()+24,4);std::memcpy(&flags,header.data()+32,4);
 Receipt live=receipt;if(table!=c.base+0x136c5d8||payload!=receipt.payload||mode||flags!=8||!read_payload(read,payload,live)||live.proxy!=receipt.proxy||live.transform!=receipt.transform||live.record!=receipt.record||!r.field(s.family,s.view,0)||!r.field(s.target,s.family,0x10)||!s.target||!r.bytes(after.data(),command,36)||after!=header)return false;
 std::uintptr_t family{},target{};if(!r.field(family,s.view,0)||family!=s.family||!r.field(target,s.family,0x10)||target!=s.target)return false;out=s;return true;
}
template<class Read> bool capture_quad(Config c,Read read,Consumer const& s,Quad& out,QuadDiagnostic* diagnostic=nullptr){
 d::Reader<Read> r{read};const auto mesh=c.base+0x1ad6980;std::array<unsigned char,0xec> raw{},after{};Quad q{};unsigned stride{},first{},primitives{},minimum{},maximum{};std::uint16_t flags{};std::uintptr_t proxy{};
 QuadDiagnostic local{};auto& diag=diagnostic?*diagnostic:local;diag={};diag.step=1;
 if(!s.command||!r.bytes(raw.data(),mesh,raw.size()))return false;
 std::memcpy(&q.source,raw.data()+0x10,8);std::memcpy(&stride,raw.data()+0x18,4);std::memcpy(&q.index_data,raw.data()+0x1c,8);std::memcpy(&proxy,raw.data()+0x28,8);std::memcpy(&first,raw.data()+0xd0,4);std::memcpy(&primitives,raw.data()+0xd4,4);std::memcpy(&minimum,raw.data()+0xd8,4);std::memcpy(&maximum,raw.data()+0xdc,4);std::memcpy(&flags,raw.data()+0xe8,2);
 diag={2,stride,first,primitives,minimum,maximum,flags,proxy,q.index_data,q.source};
 if(stride!=32||first||primitives!=2||minimum||maximum!=3||(flags&mesh_fixed_flag_mask)!=mesh_fixed_flag_value||proxy!=s.receipt.proxy)return false;
 diag.step=3;if(!r.bytes(q.vertices.data(),q.source,128))return false;
 float f[8]{};std::memcpy(f,s.receipt.record.data(),32);const float xyuv[4][4]={{f[0]+f[2],f[1],f[4]+f[6],f[5]},{f[0],f[1],f[4],f[5]},{f[0]+f[2],f[1]+f[3],f[4]+f[6],f[5]+f[7]},{f[0],f[1]+f[3],f[4],f[5]+f[7]}};
 diag.step=4;for(unsigned i=0;i<4;++i){float v[8]{};std::memcpy(v,q.vertices[i].data(),32);if(v[0]!=xyuv[i][0]||v[1]!=xyuv[i][1]||v[2]!=0||v[6]!=xyuv[i][2]||v[7]!=xyuv[i][3])return false;}
 std::array<std::array<unsigned char,32>,4> checked{};diag.step=5;if(!r.bytes(after.data(),mesh,raw.size())||raw!=after)return false;
 diag.step=6;if(!r.bytes(checked.data(),q.source,128)||checked!=q.vertices)return false;out=q;diag.step=0;return true;
}
// Native candidate only: indexed GPU IA contents remain an external proof.
inline bool classify_draw(Config c,Consumer const& s,Quad const& q,std::uintptr_t caller,DrawArgs a,Candidate& out){
 if(!s.command||!q.source||a.count!=4||a.instances!=1||a.start||a.first_instance||a.base_vertex||a.indexed!=bool(q.index_data)||caller!=c.base+(a.indexed?0x177c4a:0x1af345))return false;
 out={s.receipt.payload,s.view,s.target,s.receipt.kind,a.indexed};return true;
}
inline bool finish_consumer(Consumer const& s,std::uint32_t result){return s.command&&result==36;}
}
