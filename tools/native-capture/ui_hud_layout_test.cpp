#include "ui_hud_layout.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace gears_ui_layout;
static void near(double a,double b,double tolerance=.0003){assert(std::abs(a-b)<tolerance);}
static gears_viewport::DrawState state(){return {true,{0,0,1920,1080},{0,0,1920,1080},1,1,true,.2f,.8f};}
static gears_vr::StereoSample sample(){gears_vr::StereoSample s{};s.head.orientation={};
    constexpr double radians=3.14159265358979323846/180;
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position={i?.032:-.032,0,0};
        s.eyes[i].fov={ (i?-40.:-54.)*radians,(i?54.:40.)*radians,44*radians,-54*radians};}return s;}
static Matrix transform(){Matrix m{};m[0]=2.f/1920;m[5]=-2.f/1080;m[10]=1;m[15]=1;
    // Nonzero source origin plus a half-pixel raster bias.
    m[12]=-1-30*m[0]+.5f*2/1920;m[13]=1-20*m[5]-.5f*2/1080;return m;}
static void projection_tests(){
    assert(identity(Parameters{}));assert(valid(Parameters{1,1})&&valid(Parameters{2,0}));
    for(Parameters p: {Parameters{.9,1},{2.1,1},{1,-.1},{1,1.1},{std::numeric_limits<double>::infinity(),1}})assert(!valid(p));
    double parsed=1;assert(gears_ui_hud::parse_bounded_decimal(L"1.3",1,2,parsed)&&parsed==1.3);
    for(auto value:{L"nan",L" 1",L"1x",L"0x1",L"2.01"})assert(!gears_ui_hud::parse_bounded_decimal(value,1,2,parsed)&&parsed==1.3);
    const auto m=transform();Bounds b{};
    assert(project(b,m,30+1536,20+108)&&project(b,m,30+1728,20+216));
    // Independent pixel oracle, not a reuse of row-vector projection code.
    near(b.left,(1536+.5)/1920);near(b.right,(1728+.5)/1920);
    near(b.top,(108+.5)/1080);near(b.bottom,(216+.5)/1080);
    const auto saved=state();const auto s=sample();gears_viewport::OverlayPlan panel{},result{};
    assert(gears_ui_hud::plan(saved,1920,1080,s,panel));
    assert(compose(saved,1920,1080,panel,b,{},result));assert(result.viewports==panel.viewports&&result.scissors==panel.scissors);
    const Parameters p{1.3,.65};assert(compose(saved,1920,1080,panel,b,p,result));
    const double cx=(b.left+b.right)/2,cy=(b.top+b.bottom)/2;
    for(unsigned eye=0;eye<2;++eye)for(double x:{b.left,cx,b.right})for(double y:{b.top,cy,b.bottom}){
        const double nx=.5+p.center_scale*(cx-.5)+p.element_scale*(x-cx);
        const double ny=.5+p.center_scale*(cy-.5)+p.element_scale*(y-cy);
        // Direct physical panel -> asymmetric eye frustum interpolation.
        const double half=2*std::tan(65*3.14159265358979323846/360);
        const double ex=((2*nx-1)*half-s.eyes[eye].pose.position.x)/2;
        const double ey=((1-2*ny)*half*1080/1920)/2;
        const auto f=s.eyes[eye].fov;const auto v=result.viewports[eye];
        near(v.x+v.width*x,eye*960+960*(ex-std::tan(f.left))/(std::tan(f.right)-std::tan(f.left)));
        near(v.y+v.height*y,1080*(std::tan(f.up)-ey)/(std::tan(f.up)-std::tan(f.down)));
    }
    near((result.viewports[0].width*(b.right-b.left))/(panel.viewports[0].width*(b.right-b.left)),1.3);
    assert(std::abs(.5+p.center_scale*(cx-.5)-.5)<std::abs(cx-.5));
    assert(std::abs(.5+p.center_scale*(cy-.5)-.5)<std::abs(cy-.5));
    // Native mirrored Material quads keep their vertex order; bbox is agnostic.
    std::array<std::array<unsigned char,32>,4> quad{};
    const float positions[4][3]{{1800,100,0},{1600,100,0},{1800,200,0},{1600,200,0}};
    for(unsigned i=0;i<4;++i)std::memcpy(quad[i].data(),positions[i],sizeof(positions[i]));
    const auto before=quad;Bounds qb{};assert(quad_bounds(quad,m,qb)&&quad==before);near(qb.right-qb.left,200./1920);
    auto bad=m;bad[15]=0;Bounds untouched{};assert(!project(untouched,bad,0,0)&&!untouched.valid);
    bad=m;bad[0]=std::numeric_limits<float>::quiet_NaN();assert(!project(untouched,bad,0,0));
    const auto sentinel=result;assert(!compose(saved,1920,1080,panel,{},p,result)&&result.viewports==sentinel.viewports);
    auto clipped=saved;clipped.scissor={200,100,1200,600};
    assert(gears_ui_hud::plan(clipped,1920,1080,s,panel));assert(compose(clipped,1920,1080,panel,b,p,result));
    for(unsigned eye=0;eye<2;++eye){auto v=result.viewports[eye],c=result.scissors[eye];
        const double l=(std::max)(eye*960.,v.x+double(v.width)*200/1920);
        const double r=(std::min)((eye+1)*960.,v.x+double(v.width)*1400/1920);
        near(c.x,std::floor(l));near(c.x+c.width,std::ceil(r));
        near(c.y,std::floor((std::max)(0.,v.y+double(v.height)*100/1080)));
        near(c.y+c.height,std::ceil((std::min)(1080.,v.y+double(v.height)*700/1080)));
        assert(c.x>=eye*960&&c.x+c.width<=(eye+1)*960&&c.y>=0&&c.y+c.height<=1080);
    }
    // Non-identity HUD policy can produce an identity affine for centered
    // bounds. It must still recompute the saved source clip, as before.
    const auto full_panel=gears_viewport::overlay_plan(saved,1920,1080);
    const Bounds centered{.4,.4,.6,.6,true};
    assert(compose(clipped,1920,1080,full_panel,centered,{1,.8},result));
    assert((result.scissors[0]==gears_viewport::Rect{100,100,600,600}));
    assert((result.scissors[1]==gears_viewport::Rect{1060,100,600,600}));
    assert(result.viewports==full_panel.viewports);
    assert(compose(clipped,1920,1080,full_panel,centered,{},result));
    assert(result.scissors==full_panel.scissors); // Actual identity policy preserves panel.
    clipped.scissor={0,0,0,1080};assert(!compose(clipped,1920,1080,panel,b,p,result));
}
struct Fixture {
    gears_ui_receipts::Claim claim{};gears_ui_indices::Snapshot snapshot{};
    Fixture(){gears_ui_receipts::Store store;
        const float xy[16][2]{{1600,100},{1700,100},{1600,200},{1700,200},
            {200,800},{300,800},{200,900},{300,900},
            {1000,500},{1020,500},{1000,530},{1020,530},{1040,500},{1060,500},{1040,530},{1060,530}};
        std::array<gears_ui_batch::Vertex,16> vertices{};
        for(unsigned i=0;i<16;++i){const float p[4]{xy[i][0],xy[i][1],0,1};std::memcpy(vertices[i].data(),p,sizeof(p));}
        for(unsigned entry=0;entry<3;++entry){const unsigned begin=entry*4,end=entry==2?16:begin+4;
            gears_ui_receipts::RangeMetadata r{11,22,33,44,0,begin,end};
            assert(store.publish_range(r,vertices.data()+begin,end-begin,entry==0?2:3,100+entry,
                entry==2?gears_ui_receipts::Source::CanvasText:gears_ui_receipts::Source::Canvas,entry==2));}
        assert(store.claim(44,claim));snapshot.payload=44;snapshot.generation=claim.generation;snapshot.vertex_count=16;snapshot.segment_count=1;
        const unsigned indices[]{0,1,2,1,3,2, 4,5,6,5,7,6, 0,1,2,1,3,2, 8,9,10,9,11,10,12,13,14,13,15,14};
        snapshot.total_indices=30;auto& seg=snapshot.segments[0];seg.minimum=0;seg.maximum=15;seg.index_count=30;
        seg.kind_mask=6;seg.entry_mask=7;seg.known={0,12,18};seg.expected={30,1,0,0,0};
        for(unsigned i=0;i<30;++i)snapshot.indices[i]=snapshot.global_vertices[i]=indices[i];
        assert(gears_ui_indices::detail::bounded_snapshot(snapshot));
    }
};
static void grouping_tests(){
    Fixture f;Groups groups{};const auto matrix=transform();assert(capture(f.claim,f.snapshot,matrix,groups));
    assert(groups.count==4&&groups.segments[0].valid);
    for(unsigned i=0;i<4;++i){assert(groups.runs[i].first==i*6);assert(groups.runs[i].count==(i==3?12u:6u));}
    assert(groups.runs[0].entry==0&&groups.runs[1].entry==1&&groups.runs[2].entry==0&&groups.runs[3].entry==2);
    near(groups.bounds[2].right-groups.bounds[2].left,60./1920); // Both glyphs, including gap.
    auto saved=state();gears_viewport::OverlayPlan panel{};assert(gears_ui_hud::plan(saved,1920,1080,sample(),panel));
    Replay plans{};assert(!prepare(groups,0,saved,1920,1080,panel,{},plans)&&!plans.count);
    assert(prepare(groups,0,saved,1920,1080,panel,{1.3,.65},plans)&&plans.count==4);
    for(unsigned eye=0;eye<2;++eye)near(plans.runs[3].plan.viewports[eye].width*(40./1920),panel.viewports[eye].width*(40./1920)*1.3);
    gears_viewport::Rect last_view{},last_scissor{};float min_depth{},max_depth{};std::vector<unsigned> starts,counts;
    const auto viewport=[&](auto r,float a,float b){last_view=r;min_depth=a;max_depth=b;};const auto scissor=[&](auto r){last_scissor=r;};
    unsigned submitted=0;assert(replay(saved,plans,viewport,scissor,[&](unsigned first,unsigned count){starts.push_back(first);counts.push_back(count);submitted+=count;})==3);
    assert((starts==std::vector<unsigned>{0,6,12,18,0,6,12,18}));assert(submitted==2*f.snapshot.total_indices&&counts.size()==8);
    assert(last_view==saved.viewport&&last_scissor==saved.scissor&&min_depth==saved.min_depth&&max_depth==saved.max_depth);
    try {replay(saved,plans,viewport,scissor,[&](unsigned,unsigned){throw std::runtime_error("injected");});assert(false);}catch(std::runtime_error const&){}
    assert(last_view==saved.viewport&&last_scissor==saved.scissor&&min_depth==saved.min_depth&&max_depth==saved.max_depth);
    // Mixed triangle, subtitle, reticle, incomplete text, malformed coordinate:
    // reject this segment as a unit so caller can replay its old layout once.
    for(unsigned failure=0;failure<6;++failure){Fixture bad;
        if(failure==0){bad.snapshot.global_vertices[1]=bad.snapshot.indices[1]=4;}
        if(failure==1){bad.claim.entries[1].source=gears_ui_receipts::Source::NativeSubtitle;bad.claim.canvas=0;
            for(auto& e:bad.claim.entries)e.receipt.canvas=0;}
        if(failure==2){bad.claim.entries[0].kind=1;bad.claim.kind_mask=5;bad.snapshot.segments[0].kind_mask=5;bad.snapshot.segments[0].known={12,0,18};}
        if(failure==3)bad.claim.entries[2].whole_text_group=false;
        if(failure==5)bad.claim.entries[2].source=gears_ui_receipts::Source::Canvas; // Unproven multi-quad group.
        auto m=matrix;if(failure==4)m[15]=-1;
        Groups rejected{};const bool captured=capture(bad.claim,bad.snapshot,m,rejected);
        assert(!captured||!rejected.segments[0].valid);
        Replay unchanged{};unchanged.count=77;assert(!prepare(rejected,0,saved,1920,1080,panel,{1.3,.65},unchanged)&&unchanged.count==77);
    }
    // Unknown vertex references retain fallback; invalid generation never lends
    // grouping authority to an otherwise identical vertex/index copy.
    auto bad=f.snapshot;bad.generation++;Groups sentinel{};sentinel.count=17;
    assert(!capture(f.claim,bad,matrix,sentinel)&&sentinel.count==17);
    bad=f.snapshot;bad.vertex_count=17;bad.segments[0].maximum=16;bad.global_vertices[0]=bad.indices[0]=16;
    --bad.segments[0].known[1];++bad.segments[0].unknown;
    assert(capture(f.claim,bad,matrix,groups)&&!groups.segments[0].valid);
    // Many alternating receipt runs hit a hard storage bound, never truncate.
    bad=f.snapshot;bad.total_indices=bad.segments[0].index_count=3*(run_capacity+1);bad.segments[0].expected.index_count=bad.total_indices;
    bad.segments[0].known={0,0,0};bad.segments[0].entry_mask=3;
    for(unsigned i=0;i<bad.total_indices;++i){const auto owner=(i/3)%2;bad.indices[i]=bad.global_vertices[i]=owner*4+i%3;++bad.segments[0].known[owner?2:1];}
    assert(capture(f.claim,bad,matrix,groups)&&!groups.segments[0].valid&&groups.count==0);
}
static void subtitle_tests(){
    Fixture f;
    // Direct native adapter has no UCanvas. Exercise the legacy complete-range
    // proof with one root; separate producer roots now require separate segments.
    f.claim.canvas=0;f.claim.kind_mask=4;
    for(auto& e:f.claim.entries){e.receipt.canvas=0;e.kind=3;
        e.source=gears_ui_receipts::Source::NativeSubtitle;e.whole_text_group=true;e.root_invocation=777;}
    auto& segment=f.snapshot.segments[0];segment.kind_mask=4;segment.known={0,0,30};
    Groups groups{};assert(capture(f.claim,f.snapshot,transform(),groups)&&groups.segments[0].valid&&groups.count==4);
    const auto original=*f.claim.vertices;const auto saved=state();gears_viewport::OverlayPlan panel{};
    assert(gears_ui_hud::plan(saved,1920,1080,sample(),panel));
    Replay plans{};assert(prepare(groups,0,saved,1920,1080,panel,{1.3,.65},plans));
    // Entire string shares one transform: width, glyph width and inter-glyph gap
    // all grow together, while its center moves toward panel center.
    auto const bounds=groups.bounds[2];const double cx=(bounds.left+bounds.right)/2,cy=(bounds.top+bounds.bottom)/2;
    for(unsigned eye=0;eye<2;++eye){auto const v=plans.runs[3].plan.viewports[eye],base=panel.viewports[eye];
        const auto x=[&](double source){return v.x+v.width*source;};
        near(x(bounds.right)-x(bounds.left),base.width*60/1920*1.3);
        near(x(bounds.left+20./1920)-x(bounds.left),base.width*20/1920*1.3); // Glyph.
        near(x(bounds.left+40./1920)-x(bounds.left+20./1920),base.width*20/1920*1.3); // Gap.
        near(x(cx),base.x+base.width*(.5+.65*(cx-.5)));
        near(v.y+v.height*cy,base.y+base.height*(.5+.65*(cy-.5)));
    }
    assert(f.claim.vertices->data==original.data);
    // A fragmented subtitle cannot claim a whole-string center. Even a single
    // quad without producer proof remains on the previous replay path.
    for(unsigned entry:{0u,2u}){auto fragmented=f.claim;fragmented.entries[entry].whole_text_group=false;
        assert(capture(fragmented,f.snapshot,transform(),groups)&&!groups.segments[0].valid);
        Replay sentinel{};sentinel.count=73;
        assert(!prepare(groups,0,saved,1920,1080,panel,{1.3,.65},sentinel)&&sentinel.count==73);}
    // Isolate another segment: one unsupported call cannot discard the valid
    // complete subtitle call sharing the same immutable payload claim.
    f.snapshot.segment_count=2;segment.index_count=18;segment.maximum=7;segment.entry_mask=3;
    segment.known={0,0,18};segment.expected.index_count=18;
    auto& last=f.snapshot.segments[1];last=segment;last.minimum=8;last.maximum=15;
    last.index_offset=18;last.index_count=12;last.entry_mask=4;last.known={0,0,12};last.expected.index_count=12;
    for(unsigned i=18;i<30;++i)f.snapshot.indices[i]-=8;
    f.claim.entries[0].whole_text_group=false;
    assert(capture(f.claim,f.snapshot,transform(),groups)&&!groups.segments[0].valid&&groups.segments[1].valid);
    assert(prepare(groups,1,saved,1920,1080,panel,{1.3,.65},plans)&&plans.count==1&&plans.runs[0].count==12);
    // Wrong semantic kind never grants subtitle layout authority.
    f.claim.entries[2].kind=2;f.claim.kind_mask=6;
    assert(!capture(f.claim,f.snapshot,transform(),groups));
}
struct TextMatrices {
    Matrix first=transform(),second=transform();unsigned reads{};bool mutate{},unreadable{};
    bool read(void* out,void const* address,std::size_t size){
        if(unreadable||size!=sizeof(Matrix))return false;
        const auto source=reinterpret_cast<std::uintptr_t>(address);Matrix value{};
        if(source==0x10080)value=first;else if(source==0x20080)value=second;else return false;
        if(mutate&&reads>=2)value[12]+=.125f;
        ++reads;std::memcpy(out,value.data(),size);return true;
    }
};
static gears_ui_text_batch::Delta text_delta(){
    gears_ui_text_batch::Delta d{};d.fcanvas=22;d.sort_key=-7;
    d.ranges={{33,0x10000,0,4,0},{44,0x20000,0,8,4}};d.vertices.resize(12);
    // Background spans both separate atlas glyphs and their inter-glyph gap.
    const float xy[12][2]{{1000,800},{1200,800},{1000,840},{1200,840},
        {1020,810},{1040,810},{1020,830},{1040,830},{1080,810},{1100,810},{1080,830},{1100,830}};
    for(unsigned i=0;i<12;++i){const float p[4]{xy[i][0],xy[i][1],0,1};std::memcpy(d.vertices[i].data(),p,sizeof(p));}
    return d;
}
static gears_ui_indices::Snapshot text_snapshot(gears_ui_receipts::Claim const& claim,bool separate=false){
    gears_ui_indices::Snapshot s{};s.payload=claim.payload;s.generation=claim.generation;s.vertex_count=claim.observed_end;
    s.segment_count=separate?unsigned(claim.count):1;
    for(unsigned n=0;n<claim.count;++n){auto const& e=claim.entries[n];auto& seg=s.segments[separate?n:0];
        seg.minimum=0;seg.maximum=int(claim.observed_end-1);if(separate)seg.index_offset=s.total_indices;
        for(unsigned q=e.receipt.begin;q<e.receipt.end;q+=4){const unsigned indices[]{q,q+1,q+2,q+1,q+3,q+2};
            for(unsigned v:indices){s.indices[s.total_indices]=s.global_vertices[s.total_indices]=v;++s.total_indices;++seg.index_count;}}
        seg.kind_mask=4;seg.known={0,0,seg.index_count};seg.entry_mask|=1u<<n;seg.expected={seg.index_count,1,0,0,0};
    }
    assert(gears_ui_indices::detail::bounded_snapshot(s));return s;
}
static void multirange_subtitle_tests(){
    using namespace gears_ui_receipts;
    auto delta=text_delta();const auto original=delta.vertices;TextMatrices memory;
    auto read=[&](void* out,void const* address,std::size_t size){return memory.read(out,address,size);};
    auto group=gears_ui_text_group::capture(delta,900,read);assert(group&&memory.reads==4);
    assert(group->vertex_count==12&&group->range_count==2&&delta.vertices==original);
    auto store=std::make_unique<Store>();Claim background{},letters{};
    for(auto const& range:delta.ranges){RangeMetadata r{0,delta.fcanvas,range.item,range.payload,delta.sort_key,range.begin,range.end};
        assert(store->publish_range(r,delta.vertices.data()+range.vertex_offset,range.end-range.begin,3,900,
            Source::NativeSubtitle,false,group,range.vertex_offset));}
    assert(store->claim(0x10000,background)&&store->claim(0x20000,letters));
    assert(!background.entries[0].whole_text_group&&!letters.entries[0].whole_text_group);
    assert(background.entries[0].text_group==letters.entries[0].text_group);
    const auto bg_snapshot=text_snapshot(background),glyph_snapshot=text_snapshot(letters);
    Groups bg{},glyph{};assert(capture(background,bg_snapshot,transform(),bg)&&bg.segments[0].valid);
    assert(capture(letters,glyph_snapshot,transform(),glyph)&&glyph.segments[0].valid);
    near(bg.bounds[0].left,glyph.bounds[0].left);near(bg.bounds[0].right,glyph.bounds[0].right);
    near(bg.bounds[0].right-bg.bounds[0].left,200./1920);
    const auto saved=state();gears_viewport::OverlayPlan panel{};assert(gears_ui_hud::plan(saved,1920,1080,sample(),panel));
    Replay bg_plan{},glyph_plan{};assert(prepare(bg,0,saved,1920,1080,panel,{1.2,.8},bg_plan));
    assert(prepare(glyph,0,saved,1920,1080,panel,{1.2,.8},glyph_plan));
    assert(bg_plan.runs[0].plan.viewports==glyph_plan.runs[0].plan.viewports&&bg_plan.runs[0].plan.scissors==glyph_plan.runs[0].plan.scissors);
    for(unsigned eye=0;eye<2;++eye){const auto v=glyph_plan.runs[0].plan.viewports[eye];
        near(v.width*20/1920,panel.viewports[eye].width*20/1920*1.2); // Glyph width.
        near(v.width*40/1920,panel.viewports[eye].width*40/1920*1.2); // Gap, including separate atlas geometry.
        const double cx=(bg.bounds[0].left+bg.bounds[0].right)/2;
        near(v.x+v.width*cx,panel.viewports[eye].x+panel.viewports[eye].width*(.5+.8*(cx-.5)));}
    // Geometry survives detached payload claims and retirement, independently of
    // the original Delta and the caller's group reference.
    auto weak=std::weak_ptr<const TextGroupGeometry>(group);group.reset();store->reset();delta.vertices.clear();
    assert(!weak.expired()&&capture(letters,glyph_snapshot,transform(),glyph)&&glyph.segments[0].valid);
    for(unsigned failure=0;failure<8;++failure){auto bad=letters;auto matrix=transform();
        auto mutable_group=std::make_shared<TextGroupGeometry>(*bad.entries[0].text_group);
        if(failure==0)bad.entries[0].text_group.reset();
        if(failure==1)++bad.entries[0].root_invocation;
        if(failure==2)++bad.entries[0].text_group_offset;
        if(failure==3){++mutable_group->fcanvas;bad.entries[0].text_group=mutable_group;}
        if(failure==4){mutable_group->positions[4][0]+=1;bad.entries[0].text_group=mutable_group;}
        if(failure==5)matrix[12]+=.1f;
        if(failure==6){mutable_group->positions[0][0]=std::numeric_limits<float>::quiet_NaN();bad.entries[0].text_group=mutable_group;}
        if(failure==7)bad.entries[0].source=Source::CanvasText;
        Groups rejected{};assert(!capture(bad,glyph_snapshot,matrix,rejected)||!rejected.segments[0].valid);
        Replay untouched{};untouched.count=77;assert(!prepare(rejected,0,saved,1920,1080,panel,{1.2,.8},untouched)&&untouched.count==77);}
    // Unequal/unreadable/changing matrices never produce a partial group.
    for(unsigned failure=0;failure<12;++failure){auto bad=text_delta();memory={};
        if(failure==0)memory.second[12]+=.1f;
        if(failure==1)memory.mutate=true;
        if(failure==2)memory.unreadable=true;
        if(failure==3)memory.first[0]=std::numeric_limits<float>::infinity();
        if(failure==4)bad.ranges[1].vertex_offset=3;
        if(failure==5)bad.ranges[1].payload=bad.ranges[0].payload;
        if(failure==6)bad.ranges[1].item=bad.ranges[0].item;
        if(failure==7)bad.canvas=12;
        if(failure==8)bad.vertices.pop_back();
        if(failure==9)bad.vertices.resize(vertex_capacity+1);
        if(failure==10){float z=1;std::memcpy(bad.vertices[0].data()+8,&z,sizeof(z));}
        if(failure==11)bad.ranges[1].end=7;
        const auto unchanged=bad.vertices;assert(!gears_ui_text_group::capture(bad,900,read)&&bad.vertices==unchanged);}
    // Project actual saved positions, including an arbitrary projective matrix;
    // bounding-box corner projection would include vertices never produced.
    auto projected=text_delta();memory={};memory.first={};memory.first[0]=.2f;memory.first[5]=.3f;
    memory.first[3]=.1f;memory.first[7]=.05f;memory.first[15]=1;memory.second=memory.first;
    const float diamond[4][2]{{0,2},{1,0},{2,1},{1,3}};Bounds expected{};
    for(unsigned i=0;i<projected.vertices.size();++i){const float p[4]{diamond[i%4][0],diamond[i%4][1],0,1};
        std::memcpy(projected.vertices[i].data(),p,sizeof(p));assert(project(expected,memory.first,p[0],p[1]));}
    auto project_group=gears_ui_text_group::capture(projected,901,read);assert(project_group);
    auto const& range=projected.ranges[1];RangeMetadata r{0,22,range.item,range.payload,-7,range.begin,range.end};
    assert(store->publish_range(r,projected.vertices.data()+4,8,3,901,Source::NativeSubtitle,false,project_group,4));
    Claim project_claim{};assert(store->claim(range.payload,project_claim));
    assert(capture(project_claim,text_snapshot(project_claim),memory.first,glyph)&&glyph.segments[0].valid);
    near(glyph.bounds[0].left,expected.left);near(glyph.bounds[0].right,expected.right);
    near(glyph.bounds[0].top,expected.top);near(glyph.bounds[0].bottom,expected.bottom);
    // Combining native roots, or distinct group objects with the same root,
    // falls back per segment. Independent segments retain their own plan.
    for(unsigned same_root=0;same_root<2;++same_root){auto mixed=std::make_unique<Store>();auto a=text_delta(),b=text_delta();memory={};
        b.ranges[0].begin=4;b.ranges[0].end=8;b.ranges[1].begin=8;b.ranges[1].end=16;
        auto ga=gears_ui_text_group::capture(a,910,read),gb=gears_ui_text_group::capture(b,same_root?910:911,read);assert(ga&&gb);
        for(auto const* d:{&a,&b}){auto const& part=d->ranges[0];const auto g=d==&a?ga:gb;
            RangeMetadata metadata{0,22,part.item,part.payload,-7,part.begin,part.end};
            assert(mixed->publish_range(metadata,d->vertices.data(),4,3,g->root_invocation,Source::NativeSubtitle,false,g,0));}
        Claim claim{};assert(mixed->claim(0x10000,claim));Groups result{};
        assert(capture(claim,text_snapshot(claim),transform(),result)&&!result.segments[0].valid);
        assert(capture(claim,text_snapshot(claim,true),transform(),result)&&result.segments[0].valid&&result.segments[1].valid);
        auto partial=text_snapshot(claim);partial.segment_count=2;
        partial.segments[1]=partial.segments[0];auto& independent=partial.segments[1];
        independent.index_offset=partial.total_indices;independent.index_count=6;independent.entry_mask=1;
        independent.known={0,0,6};independent.expected.index_count=6;
        for(unsigned i=0;i<6;++i){partial.indices[partial.total_indices]=partial.indices[i];
            partial.global_vertices[partial.total_indices]=partial.global_vertices[i];++partial.total_indices;}
        assert(capture(claim,partial,transform(),result)&&!result.segments[0].valid&&result.segments[1].valid);
        Replay kept{};assert(prepare(result,1,saved,1920,1080,panel,{1.2,.8},kept)&&kept.count==1);
    }
    assert(background.vertices->data[0]==original[0]&&letters.vertices->data[0]==original[4]);
}
int main(){projection_tests();grouping_tests();subtitle_tests();multirange_subtitle_tests();std::cout<<"UiHudLayoutTest passed\n";}
