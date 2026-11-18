// Snub adapter for the SAME original animation owner. Reading original parts
// creates no action permission. The caller must own the native reload/item
// before StopAnim or SetPosition, and restore through that owner on exit.
namespace snub_animation {
namespace hand=gears_weapon_hand;namespace anim=hammerburst_animation;namespace meta=gripdamage_metadata;
struct Parts {
    hand::ObjectIdentity component{},asset{};hand::FrameKey key{};
    std::array<std::array<float,16>,5> world{};
};
gears_equipment_names::Lookup first_names({L"b_HG_Root",L"b_HG_Slide",L"b_HG_Wing_L"});
gears_equipment_names::Lookup last_names({L"b_HG_Wing_R",L"b_HG_Magazine",L"b_HG_Root"});
bool read_parts(hand::Frame const& source,Parts& out) noexcept {
    out={};anim::Parts a{},b{};
    if(!anim::read_part_group(source,a,lancermain_asset::HeldAsset::pistol,first_names,
           {L"b_HG_Root",L"b_HG_Slide",L"b_HG_Wing_L"},{0,1,2})||
       !anim::read_part_group(source,b,lancermain_asset::HeldAsset::pistol,last_names,
           {L"b_HG_Wing_R",L"b_HG_Magazine",L"b_HG_Root"},{3,4,0})||
       !hand::same(a.component,b.component)||!hand::same(a.asset,b.asset)||!hand::same(a.key,b.key)||
       a.world[0]!=b.world[2]||!nativeequipment::current(nullptr,source))return false;
    out.component=a.component;out.asset=a.asset;out.key=a.key;
    out.world={a.world[0],a.world[1],a.world[2],b.world[0],b.world[1]};return true;
}
struct Curve {
    hand::ObjectIdentity sequence{};std::uint64_t content{};
    gears_weapon::SnubSlideCurve slide;
    double units{};
};
bool original_set(meta::Reader& r,hand::ObjectIdentity const& sequence) noexcept {
    if(!weaponhand::object(r,sequence)||!weaponhand::derives(r,sequence.address,L"AnimSequence",L"Engine"))return false;
    const auto owner=get<uptr>(sequence.address+0x40),package=owner?get<uptr>(owner+0x40):0;
    hand::ObjectIdentity set{},outer{};
    return lancermain_asset::identify(r,owner,set)&&lancermain_asset::identify(r,package,outer)&&
        weaponhand::derives(r,owner,L"AnimSet",L"Engine")&&playground::named(owner,L"COG_Pistol")&&
        playground::named(package,L"COG_Pistol")&&get<uptr>(package+0x40)==0;
}
// Decode the exact original5-track Float96 stream from the currently selected
// instance. No proprietary key table, guessed offset or clip duration is in
// the package. Uniform layout is checked; native sampling remains separate.
unsigned curve_failure{};
unsigned acquire_gate{};
unsigned observe_failure{};
bool curve_rejected(unsigned line) noexcept {curve_failure=line;return false;}
bool read_curve(hand::Frame const& source,anim::Observation const& expected,Curve& out) noexcept try {
    out={};curve_failure=0;anim::Observation before{};
    if(!anim::observe_kind(source,before,lancermain_asset::HeldAsset::pistol,&observe_failure)||
       !hand::same(before.node,expected.node)||!hand::same(before.sequence,expected.sequence)||
       before.reload_start!=expected.reload_start||before.length!=expected.length)return curve_rejected(__LINE__);
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!original_set(r,before.sequence))return curve_rejected(__LINE__);
    weaponhand::BoolField frames{},format{},offsets{},stream{},duration{};
    std::int32_t count{};unsigned char compression{};float length{};meta::Table table{},bytes{};
    if(!weaponhand::field(r,before.sequence.address,L"NumFrames",L"IntProperty",4,frames)||
       !weaponhand::field(r,before.sequence.address,L"RotationCompressionFormat",L"ByteProperty",1,format)||
       !weaponhand::field(r,before.sequence.address,L"CompressedTrackOffsets",L"ArrayProperty",16,offsets)||
       !weaponhand::field(r,before.sequence.address,L"CompressedByteStream",L"ArrayProperty",16,stream)||
       !weaponhand::field(r,before.sequence.address,L"SequenceLength",L"FloatProperty",4,duration)||
       !r.at(before.sequence.address,frames.property.offset,count)||count<4||count>256||
       !r.at(before.sequence.address,format.property.offset,compression)||compression!=1||
       !r.at(before.sequence.address,duration.property.offset,length)||length!=before.length||
       !r.at(before.sequence.address,offsets.property.offset,table)||
       !r.at(before.sequence.address,stream.property.offset,bytes))return curve_rejected(__LINE__);
    const auto inner=[&](weaponhand::BoolField const& field,bool byte_array) noexcept {
        const auto address=schema.identities[field.property.identity].address;uptr element{};meta::Property p{};
        return r.at(address,0xd0,element)&&r.property(element,p)&&schema.identities[p.identity].outer==address&&
            p.dimension==1&&p.element_bytes==(byte_array?1u:4u)&&
            (byte_array?weaponhand::core_type(r,p.type,L"ByteProperty"):weaponhand::core_type(r,p.type,L"IntProperty"));
    };
    if(!inner(offsets,false)||!inner(stream,true)||!table.data||table.count!=20||
       table.capacity<table.count||table.capacity>4096||!bytes.data||bytes.count<120||bytes.count>65536||
       bytes.capacity<bytes.count||bytes.capacity>65536)return curve_rejected(__LINE__);
    std::array<std::int32_t,20> tracks{};
    std::vector<unsigned char> content(std::size_t(bytes.count)),fresh(std::size_t(bytes.count));
    if(!copy(tracks.data(),table.data,sizeof(tracks))||!copy(content.data(),bytes.data,content.size()))return curve_rejected(__LINE__);
    struct Span {std::int32_t start{},end{};};std::array<Span,10> spans{};
    std::array<std::array<gears_vr::Vector,256>,5> positions{};
    for(unsigned bone=0;bone<5;++bone){
        const auto t=tracks[bone*4],tn=tracks[bone*4+1],q=tracks[bone*4+2],qn=tracks[bone*4+3];
        if(tn!=(bone?count:1)||qn!=(bone<2?1:count)||t<0||q<0)return curve_rejected(__LINE__);
        const auto tb=12*tn,qb=12*qn+(qn>1?24:0);
        if(t>bytes.count-tb||q>bytes.count-qb)return curve_rejected(__LINE__);
        spans[bone*2]={t,t+tb};spans[bone*2+1]={q,q+qb};
        for(int i=0;i<tn;++i){std::array<float,3> p{};std::memcpy(p.data(),content.data()+t+12*i,12);
            for(auto v:p)if(!std::isfinite(v))return curve_rejected(__LINE__);positions[bone][unsigned(i)]={p[0],p[1],p[2]};}
        std::array<float,6> range{};
        if(qn>1){std::memcpy(range.data(),content.data()+q,24);
            for(unsigned i=0;i<6;++i)if(!std::isfinite(range[i])||(i>=3&&range[i]<0))return curve_rejected(__LINE__);}
        for(int i=0;i<qn;++i){std::array<float,3> v{};std::memcpy(v.data(),content.data()+q+(qn>1?24:0)+12*i,12);
            double squared=0;for(unsigned j=0;j<3;++j){
                if(!std::isfinite(v[j])||(qn>1&&(v[j]<range[j]-1e-5f||v[j]>range[j]+range[j+3]+1e-5f)))return curve_rejected(__LINE__);
                squared+=double(v[j])*v[j];}
            if(squared>1.00001)return curve_rejected(__LINE__);
        }
    }
    std::sort(spans.begin(),spans.end(),[](Span a,Span b){return a.start<b.start;});
    if(spans.front().start||spans.back().end!=bytes.count)return curve_rejected(__LINE__);
    for(unsigned i=1;i<spans.size();++i)if(spans[i].start!=spans[i-1].end)return curve_rejected(__LINE__);
    const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500)return curve_rejected(__LINE__);
    for(int i=0;i<count;++i){auto& p=positions[1][unsigned(i)];p={p.x/units,p.y/units,p.z/units};}
    // Exact original reference profile, authenticated by named5-bone mesh and
    // private source hash. Live bone endpoints must also agree before finish.
    Curve value{};value.sequence=before.sequence;value.units=units;
    if(!value.slide.bind(positions[1].data(),std::size_t(count),before.length,
        {7.087201118469238/units,.014453720301389694/units,10.67758846282959/units},
        {-1,0,0},true))return curve_rejected(__LINE__);
    // The chosen tail keeps the original magazine seated. Full AR_Reload,
    // including its earlier magazine extraction, is never used for the pull.
    const auto end=positions[4][unsigned(count-1)];
    for(auto i=value.slide.open_key();i<std::size_t(count);++i){const auto p=positions[4][i];
        if(std::hypot(std::hypot(p.x-end.x,p.y-end.y),p.z-end.z)>.005)return curve_rejected(__LINE__);}
    meta::Table table_after{},bytes_after{};std::array<std::int32_t,20> tracks_after{};
    std::int32_t count_after{};unsigned char format_after{};float length_after{};
    if(!r.at(before.sequence.address,offsets.property.offset,table_after)||
       !r.at(before.sequence.address,stream.property.offset,bytes_after)||
       table.data!=table_after.data||table.count!=table_after.count||table.capacity!=table_after.capacity||
       bytes.data!=bytes_after.data||bytes.count!=bytes_after.count||bytes.capacity!=bytes_after.capacity||
       !copy(tracks_after.data(),table.data,sizeof(tracks_after))||tracks_after!=tracks||
       !copy(fresh.data(),bytes.data,fresh.size())||fresh!=content||
       !r.at(before.sequence.address,frames.property.offset,count_after)||count_after!=count||
       !r.at(before.sequence.address,format.property.offset,format_after)||format_after!=compression||
       !r.at(before.sequence.address,duration.property.offset,length_after)||length_after!=before.length||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,source))return curve_rejected(__LINE__);
    value.content=14695981039346656037ull;
    const auto hash=[&](void const* data,std::size_t size){auto p=static_cast<unsigned char const*>(data);
        for(std::size_t i=0;i<size;++i){value.content^=p[i];value.content*=1099511628211ull;}};
    hash(tracks.data(),sizeof(tracks));hash(content.data(),content.size());hash(&count,sizeof(count));hash(&length,sizeof(length));
    out=value;return true;
}catch(...){out={};return curve_rejected(__LINE__);}
anim::Acquire acquire(hand::Frame const& source,std::uint64_t serial,Curve& curve) noexcept {
    curve={};acquire_gate=0;curve_failure=0;anim::Observation before{};anim::Controls controls{};Curve actual{};
    if(!anim::observe_kind(source,before,lancermain_asset::HeldAsset::pistol,&observe_failure)){acquire_gate=1;return anim::Acquire::rejected;}
    if(before.looping){acquire_gate=2;return anim::Acquire::rejected;}
    if(!read_curve(source,before,actual)){acquire_gate=3;return anim::Acquire::rejected;}
    if(!anim::prepare_controls(source,before,controls,true)){acquire_gate=4;return anim::Acquire::rejected;}
    const auto result=anim::acquire_prepared(source,serial,before,controls);
    if(result==anim::Acquire::rejected)acquire_gate=5;
    if(result!=anim::Acquire::rejected)curve=actual;return result;
}
bool seek_distance(hand::Frame const& source,std::uint64_t serial,Curve const& initial,double distance,
    gears_weapon::SnubSlideCurve::Sampling law,bool settling=false) noexcept {
    if(!anim::ownership.active||!anim::ownership.stopped||serial!=anim::ownership.serial)return false;
    Curve current{};float time{};
    if(!read_curve(source,anim::ownership.last,current)||!hand::same(current.sequence,initial.sequence)||
       current.content!=initial.content||current.units!=initial.units||!current.slide.time_for_distance(distance,time,law))return false;
    return settling?anim::seek_settling(source,serial,time):anim::seek(source,serial,time);
}
} // namespace snub_animation
