// Included after the real equipment backend, inside handstream. Private only.
// The mount is geometry; only the currently scoped hand Plan can grant a write.
namespace component {
namespace stow=vrequip::stow;
// Private name parsing only. No game metadata, success, or field address is
// cached. Every lookup follows the original live first-match traversal.
namespace lookup {
using ObjectKey=stow::ObjectKey;
std::size_t page_bytes(){
    static const std::size_t bytes=[](){SYSTEM_INFO info{};GetSystemInfo(&info);return std::size_t(info.dwPageSize);}();
    return bytes;
}
bool name(uptr object,wchar_t (&out)[128],std::size_t limit=128){
    if(!object||!limit||limit>128)return false;
    const auto n=get<Name>(object+0x48);const auto a=get<Array>(base+0x1c34800);
    if(!a.data||a.count<1000||a.count>a.capacity||a.capacity>4000000||n.index>=unsigned(a.count))return false;
    const auto entry=get<uptr>(a.data+n.index*8);
    if(!entry||get<unsigned>(entry)!=n.index)return false;
    const auto page=page_bytes();if(!page)return false;
    for(std::size_t at=0;at<limit;){
        const auto address=entry+20+at*sizeof(wchar_t);
        const auto room=page-std::size_t(address%page);
        auto chars=(std::min)(std::size_t(limit-at),room/sizeof(wchar_t));
        // An unaligned wchar may straddle the page. Preserve the original
        // single two-byte copy and its failure; never retry a guarded page.
        if(!chars)chars=1;
        if(!copy(out+at,address,chars*sizeof(wchar_t)))return false;
        for(std::size_t i=0;i<chars;++i)if(!out[at+i])return true;
        at+=chars;
    }
    out[limit-1]=0;return false;
}
std::size_t query_limit(const wchar_t* value){
    if(!value)return 0;
    std::size_t len{};while(len<127&&value[len])++len;
    return len<127?len+1:128;
}
bool named(uptr object,const wchar_t* value,std::size_t limit){
    wchar_t text[128]{};return value&&name(object,text,limit)&&!_wcsicmp(text,value);
}
bool named(uptr object,const wchar_t* value){return object&&named(object,value,query_limit(value));}
bool derives(uptr cls,const wchar_t* value){
    if(!cls)return false;
    const auto limit=query_limit(value);
    for(unsigned n=0;cls&&n<40;++n,cls=get<uptr>(cls+0x60))if(named(cls,value,limit))return true;
    return false;
}
bool instance(uptr object,const wchar_t* cls){
    if(!registered(object)||!derives(get<uptr>(object+0x50),cls))return false;
    wchar_t text[128]{};return name(object,text)&&wcsncmp(text,L"Default__",9);
}
uptr field(uptr cls,const wchar_t* value,const wchar_t* kind=nullptr){
    if(!cls)return 0;
    const auto value_limit=query_limit(value),kind_limit=kind?query_limit(kind):0;
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        auto f=get<uptr>(cls+0x80);
        for(unsigned n=0;f&&n<4096;++n,f=get<uptr>(f+0x68))
            if(named(f,value,value_limit)&&(!kind||named(get<uptr>(f+0x50),kind,kind_limit)))return f;
    }
    return 0;
}
uptr property(uptr object,const wchar_t* value){return registered(object)?field(get<uptr>(object+0x50),value):0;}
bool deleted(uptr object,uptr* resolved=nullptr,uptr* resolved_class=nullptr){
    if(!registered(object))return true;
    uptr p{};
    if(resolved_class){
        if(registered(object)){
            *resolved_class=get<uptr>(object+0x50);
            p=field(*resolved_class,L"bDeleteMe");
        }
    }else p=property(object,L"bDeleteMe");
    if(resolved)*resolved=p;
    return p&&(get<unsigned>(object+get<unsigned>(p+0x94))&get<unsigned>(p+0xd0));
}
bool deleted_at(uptr object,uptr p,uptr object_class){
    if(!registered(object))return true;
    return registered(object)&&(get<uptr>(object+0x50)!=object_class||
        (p&&(get<unsigned>(object+get<unsigned>(p+0x94))&get<unsigned>(p+0xd0))));
}
bool field_at(uptr object,uptr property,const wchar_t* kind,void* output,unsigned bytes){
    const auto cls=get<uptr>(object+0x50);
    if(!registered(property)||!registered(cls)||get<unsigned>(property+0x70)!=1||
       get<unsigned>(property+0x74)!=bytes||
       !lookup::named(get<uptr>(property+0x50),kind))return false;
    const auto offset=get<unsigned>(property+0x94),size=get<unsigned>(cls+0x88);
    return offset<=size&&bytes<=size-offset&&copy(output,object+offset,bytes);
}
bool field(uptr object,const wchar_t* label,const wchar_t* kind,void* output,unsigned bytes){
    return field_at(object,lookup::property(object,label),kind,output,bytes);
}
bool pointer(uptr object,const wchar_t* label,uptr& out,bool component=false){
    return field(object,label,component?L"ComponentProperty":L"ObjectProperty",&out,sizeof(out));
}
bool boolean(uptr object,const wchar_t* label,bool& out){
    const auto property=lookup::property(object,label);
    unsigned bits{};
    if(!field_at(object,property,L"BoolProperty",&bits,4))return false;
    const auto mask=get<unsigned>(property+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    out=(bits&mask)!=0;return true;
}
bool object_key(uptr object,ObjectKey& key){
    key={};
    uptr delete_property{},delete_class{};
    if(!registered(object)||lookup::deleted(object,&delete_property,&delete_class))return false;
    Name n{};
    key.address=object;
    if(!copy(&key.index,object+8,4)||!copy(&key.outer,object+0x40,8)||
       !copy(&n,object+0x48,8)||!copy(&key.object_class,object+0x50,8)||
       !registered(key.object_class)||!registered(key.outer))return false;
    if(delete_property&&key.object_class!=delete_class)return false;
    key.name_index=n.index;key.name_number=n.number;
    return registered(object)&&!(delete_property?lookup::deleted_at(object,delete_property,delete_class):lookup::deleted(object));
}
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
constexpr const wchar_t* pawn_link_labels[]={L"Controller",L"InvManager",L"Mesh",L"Weapon"};
// Four addresses live only within one non-invoking pawn-link observation.
// Keep lookup's stricter typed reader: dimension1, registered descriptors,
// exact ComponentProperty for Mesh, current extent and known value copy.
struct PawnLinkFields {
    uptr object{},cls{};std::array<uptr,4> addresses{};
    explicit PawnLinkFields(uptr owner):object(owner){}
    bool begin(){
        if(!registered(object))return false;
        cls=get<uptr>(object+0x50);
        return playground::fields(cls,pawn_link_labels,addresses)&&registered(object)&&get<uptr>(object+0x50)==cls;
    }
    bool pointer(unsigned index,uptr& value)const{
        return index<addresses.size()&&registered(object)&&get<uptr>(object+0x50)==cls&&
            lookup::field_at(object,addresses[index],index==2?L"ComponentProperty":L"ObjectProperty",&value,sizeof(value));
    }
    bool finish()const{
        std::array<uptr,4> current{};
        return registered(object)&&get<uptr>(object+0x50)==cls&&
            playground::fields(cls,pawn_link_labels,current)&&current==addresses&&
            registered(object)&&get<uptr>(object+0x50)==cls;
    }
};
bool pawn_links(uptr pawn,std::array<uptr,4> const& expected,bool fast){
    uptr value{};
    if(!fast||!component_names::enabled){
        const auto link=[fast](uptr object,const wchar_t* label,uptr& result,bool component=false){
            return fast?lookup::pointer(object,label,result,component):stow::pointer(object,label,result,component);};
        return link(pawn,L"Controller",value)&&value==expected[0]&&
            link(pawn,L"InvManager",value)&&value==expected[1]&&
            link(pawn,L"Mesh",value,true)&&value==expected[2]&&
            link(pawn,L"Weapon",value)&&value==expected[3];
    }
    PawnLinkFields fields(pawn);
    if(!fields.begin())return false;
    for(unsigned i=0;i<expected.size();++i)
        if(!fields.pointer(i,value)||value!=expected[i])return false;
    // No descriptor survives this return, the later weapon links or slot VMs.
    return fields.finish();
}
#endif
} // namespace lookup

using Value=gears_weapon::WeaponMeshComponentValue;
using Bytes=gears_weapon::MeshComponentBytes12;
using Absolute=gears_weapon::MeshComponentAbsolute;
struct Key {
    std::array<stow::ObjectKey,8> objects{};
    std::array<std::uint64_t,4> slots{};
    stow::ObjectKey asset{};
    bool operator==(Key const& other) const noexcept {
        return slots==other.slots&&!std::memcmp(objects.data(),other.objects.data(),sizeof(objects))&&
            !std::memcmp(&asset,&other.asset,sizeof(asset));
    }
};
using State=gears_weapon::WeaponMeshComponentState<Key>;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
gears_weapon::WeaponMeshComponentContinuousLease<Key> lease;
#else
gears_weapon::WeaponMeshComponentLease<Key> lease;
#endif
} // namespace component
namespace component_trace {
void masks(Row& row) noexcept {
    row.pending=component::lease.dirty_mask();row.owned=component::lease.owned_mask();row.revoked=component::lease.revoked_mask();
    row.snapshot=component::lease.has_snapshot();row.faulted=component::lease.faulted();
}
} // namespace component_trace
namespace component {
// First actual bind attempt only. Copies of existing locals are diagnostic;
// bind, solve and initial_read_after_bind are not one atomic observation.
struct FirstBindEvidence {
    Key key{};
    std::array<float,16> matrix{};
    gears_vr::Viewpoint right_grip{},solved_pose{};
    Value initial_read_after_bind{},quantized_wanted{};
    int pawn_mesh_tick{},hand_mesh_tick{};
    bool attempted{},dumped{},hand_mesh_tick_known{};
    bool bind_return_known{},bind_result{},solve_return_known{},solve_result{};
    bool initial_read_return_known{},initial_read_result{},initial_read_state_known{},quantized_wanted_known{};
};
FirstBindEvidence first_bind;
void reset_first_bind(){first_bind={};}
gears_weapon::WeaponMeshFollow mount;
Key fixed_key{};
bool key_known{},recorded{},terminal_dumped{},terminal_clean_dumped{};
std::uint64_t acquired{},restored{},setter_calls{},relinquished{};
unsigned hand_lease{};
std::uint64_t plan_sequence{},plan_revision{},action_epoch{};
gears_hand_continuity::Certificate certificate{}; // Evidence only, never read by authorization.
Value target{};
std::array<unsigned,3> baseline_absolute_storage{},target_absolute_storage{};

bool absolute_storage(Key const& key,std::array<unsigned,3>& bits,std::array<uptr,3>* addresses=nullptr,
                      std::array<unsigned,3>* masks=nullptr,bool fast=false){
    const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
    for(unsigned i=0;i<3;++i){
        const auto object=key.objects[7].address,property=fast?lookup::property(object,fields[i]):playground::property(object,fields[i]);
        if(!(fast?lookup::field_at(object,property,L"BoolProperty",&bits[i],4):stow::field(object,fields[i],L"BoolProperty",&bits[i],4)))return false;
        const auto mask=get<unsigned>(property+0xd0);
        if(!mask||(mask&(mask-1)))return false;
        if(addresses)(*addresses)[i]=object+get<unsigned>(property+0x94);
        if(masks)(*masks)[i]=mask;
    }
    return true;
}

// invoke_engine directly invokes the original trampoline/native wrapper. Only
// its initial Args -> invoke admission is exempt; every hooked recursive engine
// action remains an external boundary, including identical same-value setters.
struct SelfSetter {
    uptr object{},function{};
    bool armed{};
};
SelfSetter self;
struct SelfScope {
    SelfSetter saved;
    SelfScope(uptr object,uptr function):saved(self){self={object,function,true};}
    ~SelfScope(){self=saved;}
};
bool consume_self(uptr object,uptr function){
    if(!self.armed||self.object!=object||self.function!=function)return false;
    self.armed=false;return true;
}
bool current(Key const& expected,bool fast=false){
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    component_cost::AggregateSpan cost(component_cost::AggregateKind::current_total);
    const auto identify=[fast](uptr object,stow::ObjectKey& key){return fast?lookup::object_key(object,key):stow::object_key(object,key);};
    const auto link=[fast](uptr object,const wchar_t* label,uptr& value,bool component=false){return fast?lookup::pointer(object,label,value,component):stow::pointer(object,label,value,component);};
    const auto instance=[fast](uptr object,const wchar_t* label){return fast?lookup::instance(object,label):playground::instance(object,label);};
    for(auto const& old:expected.objects){
        stow::ObjectKey found{};
        if(!identify(old.address,found)||std::memcmp(&old,&found,sizeof(old)))return false;
    }
    stow::ObjectKey asset{};
    if(!identify(expected.asset.address,asset)||
       std::memcmp(&asset,&expected.asset,sizeof(asset)))return false;
    const auto address=[&](unsigned i){return expected.objects[i].address;};
    uptr pointer{};
    if(!link(address(2),L"WorldInfo",pointer)||pointer!=address(0)||
       !link(address(2),L"Pawn",pointer)||pointer!=address(3)||
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
       !lookup::pawn_links(address(3),std::array<uptr,4>{{address(2),address(4),address(5),address(6)}},fast)||
#else
       !link(address(3),L"Controller",pointer)||pointer!=address(2)||
       !link(address(3),L"InvManager",pointer)||pointer!=address(4)||
       !link(address(3),L"Mesh",pointer,true)||pointer!=address(5)||
       !link(address(3),L"Weapon",pointer)||pointer!=address(6)||
#endif
       !link(address(6),L"Instigator",pointer)||pointer!=address(3)||
       !link(address(6),L"Mesh",pointer,true)||pointer!=address(7)||
       !link(address(7),L"SkeletalMesh",pointer)||pointer!=expected.asset.address||
       get<uptr>(address(0)+0x40)!=address(1)||get<uptr>(address(2)+0x40)!=address(1)||
       get<uptr>(address(3)+0x40)!=address(1))return false;
    {
        component_cost::SlotOwner slot_cost;
        for(std::uint8_t i=0;i<4;++i){
            uptr slot{};if(!vrequip::read_slot(address(4),i,slot)||slot!=expected.slots[i])return false;
        }
    }
    return instance(address(6),L"Weap_AssaultRifle")&&
        instance(address(7),L"SkeletalMeshComponent")&&
        instance(expected.asset.address,L"SkeletalMesh")&&
        (fast?lookup::named(expected.asset.address,L"COGAssaultRifle"):playground::named(expected.asset.address,L"COGAssaultRifle"));
}
bool capture(Snapshot const& hand,Key& key){
    gears_weapon::StowState<stow::Key> state{};
    if(!stow::read_state(state)||!state.ready||state.attachment!=gears_weapon::StowAttachment::original_hand||
       state.key.objects[2].address!=hand.pc||state.key.objects[3].address!=hand.pawn||
       state.key.objects[5].address!=hand.mesh||state.key.objects[6].address!=hand.weapon)return false;
    key={};key.objects=state.key.objects;key.slots=state.key.slots;
    uptr asset{};
    return stow::pointer(key.objects[7].address,L"SkeletalMesh",asset)&&
        stow::object_key(asset,key.asset)&&current(key);
}
bool triple_schema(uptr property,bool rotation){
    if(!registered(property)||get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=12||
       !playground::named(get<uptr>(property+0x50),L"StructProperty"))return false;
    const auto structure=get<uptr>(property+0xd0);
    if(!registered(structure)||!playground::named(structure,rotation?L"Rotator":L"Vector")||
       get<unsigned>(structure+0x88)!=12)return false;
    const wchar_t* fields[2][3]={{L"X",L"Y",L"Z"},{L"Pitch",L"Yaw",L"Roll"}};
    for(unsigned i=0;i<3;++i){unsigned offset{};
        if(!handanim::structure_field(structure,fields[rotation?1:0][i],
            rotation?L"IntProperty":L"FloatProperty",4,12,offset)||offset!=i*4)return false;
    }
    return true;
}
// Geometry input only. These copies own no asset fields or component groups.
// Descriptors are resolved afresh inside one non-engine local read; retained
// identities/layouts are comparison evidence, never reusable property handles.
struct AssetSchema {
    stow::ObjectKey property{},structure{};
    std::array<unsigned,5> layout{}; // dimension, bytes, offset, class bytes, struct bytes
};
struct AssetInput {
    stow::ObjectKey mesh{},asset{};
    AssetSchema origin_schema{},rotation_schema{};
    Bytes origin{},rotation{};
    bool known{};
};
struct TargetGeometry {
    AssetInput input{};
    gears_vr::Viewpoint model{};
    gears_vr::Rotator control{};
    bool known{};
};
TargetGeometry target_geometry;
bool same_asset_schema(AssetSchema const& a,AssetSchema const& b){
    return !std::memcmp(&a.property,&b.property,sizeof(a.property))&&
        !std::memcmp(&a.structure,&b.structure,sizeof(a.structure))&&a.layout==b.layout;
}
bool same_asset_identity_schema(AssetInput const& a,AssetInput const& b){
    return a.known&&b.known&&!std::memcmp(&a.mesh,&b.mesh,sizeof(a.mesh))&&
        !std::memcmp(&a.asset,&b.asset,sizeof(a.asset))&&
        same_asset_schema(a.origin_schema,b.origin_schema)&&same_asset_schema(a.rotation_schema,b.rotation_schema);
}
bool asset_identity(Key const& key,stow::ObjectKey& mesh,stow::ObjectKey& asset){
    uptr pointer{};
    return lookup::object_key(key.objects[7].address,mesh)&&
        !std::memcmp(&mesh,&key.objects[7],sizeof(mesh))&&
        lookup::object_key(key.asset.address,asset)&&!std::memcmp(&asset,&key.asset,sizeof(asset))&&
        lookup::pointer(mesh.address,L"SkeletalMesh",pointer)&&pointer==asset.address;
}
bool asset_schema(uptr asset,uptr property,bool rotation,AssetSchema& out){
    out={};
    if(!triple_schema(property,rotation))return false;
    const auto structure=get<uptr>(property+0xd0),cls=get<uptr>(asset+0x50);
    if(!lookup::object_key(property,out.property)||!lookup::object_key(structure,out.structure))return false;
    out.layout={get<unsigned>(property+0x70),get<unsigned>(property+0x74),get<unsigned>(property+0x94),
        get<unsigned>(cls+0x88),get<unsigned>(structure+0x88)};
    return true;
}
bool asset_triple(uptr asset,const wchar_t* label,bool rotation,Bytes& bytes,AssetSchema& schema){
    const auto property=lookup::property(asset,label);
    if(!asset_schema(asset,property,rotation,schema)||
       !lookup::field_at(asset,property,L"StructProperty",bytes.data(),12))return false;
    AssetSchema after{};
    return lookup::property(asset,label)==property&&asset_schema(asset,property,rotation,after)&&
        same_asset_schema(schema,after);
}
// Caller has just passed its existing full current/read guard. This adds no
// engine call or full-current/getter traversal and is not an atomic snapshot.
bool read_asset_input(Key const& key,AssetInput& out){
    out={};AssetInput sample{};
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       !asset_identity(key,sample.mesh,sample.asset)||
       !asset_triple(key.asset.address,L"Origin",false,sample.origin,sample.origin_schema)||
       !asset_triple(key.asset.address,L"RotOrigin",true,sample.rotation,sample.rotation_schema))return false;
    std::array<float,3> origin{};std::memcpy(origin.data(),sample.origin.data(),12);
    for(float x:origin)if(!std::isfinite(x))return false;
    stow::ObjectKey mesh{},asset{};
    if(!asset_identity(key,mesh,asset))return false;
    sample.known=true;out=sample;return true;
}
bool zero_asset_origin(AssetInput const& input){
    if(!input.known)return false;
    std::array<float,3> origin{};std::memcpy(origin.data(),input.origin.data(),12);
    return std::all_of(origin.begin(),origin.end(),[](float x){return std::isfinite(x)&&x==0.f;});
}
bool asset_input_matches(Key const& key,AssetInput const& expected){
    AssetInput live{};
    return expected.known&&read_asset_input(key,live)&&zero_asset_origin(live)&&
        same_asset_identity_schema(expected,live)&&expected.origin==live.origin&&expected.rotation==live.rotation;
}
bool read(Key const& key,State& out,bool fast=false){
    component_cost::AggregateSpan cost(component_cost::AggregateKind::read_total);
    out={};
    if(!current(key,fast))return false;
    const auto mesh=key.objects[7].address;
    for(unsigned i=0;i<3;++i){
        const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
        if(!(fast?lookup::boolean(mesh,fields[i],out.value.absolute[i]):stow::boolean(mesh,fields[i],out.value.absolute[i])))return false;
    }
    if(fast){
        const auto translation=lookup::property(mesh,L"Translation");
        if(!triple_schema(translation,false))return false;
        const auto rotation=lookup::property(mesh,L"Rotation");
        if(!triple_schema(rotation,true)||
           !lookup::field_at(mesh,translation,L"StructProperty",out.value.translation.data(),12)||
           !lookup::field_at(mesh,rotation,L"StructProperty",out.value.rotation.data(),12))return false;
    }else if(!triple_schema(playground::property(mesh,L"Translation"),false)||
       !triple_schema(playground::property(mesh,L"Rotation"),true)||
       !stow::field(mesh,L"Translation",L"StructProperty",out.value.translation.data(),12)||
       !stow::field(mesh,L"Rotation",L"StructProperty",out.value.rotation.data(),12))return false;
    std::array<float,3> position{};std::memcpy(position.data(),out.value.translation.data(),12);
    for(float x:position)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    out.key=key;out.known=true;return current(key,fast);
}
bool world(Key const& key,std::array<float,16>& value,int& tick){
    return current(key)&&handik::structure(key.objects[7].address,L"LocalToWorld",L"Matrix",value.data(),64)&&
        stow::field(key.objects[5].address,L"TickTag",L"IntProperty",&tick,4);
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
// HOLD_EVIDENCE_BEGIN: copied bytes only; none of these fields grant a write.
namespace hold_detail {
struct Observation {
    State raw{};AssetInput asset{};std::array<float,16> matrix{};
    std::array<unsigned,3> packed{};
    std::uint64_t begin_ms{},end_ms{},qpc_begin{},qpc_end{},component_setters{};
    unsigned epoch{},hand_setters{},hand_writes{},pending{},owned{},revoked{};
    DWORD thread{};int mesh_tick{};
    bool attempted{},read_known{},world_known{},asset_known{},packed_known{},comparison_known{},exact{},asset_match{},valid{};
    bool hand_dirty{},component_fault{},stop{},shutdown{},ended{};
};
struct Pair {
    Observation before{},after{};std::uint64_t serial{},generation{};
    uptr token{},object{},function{};unsigned ordinal{};
    bool entered{},returned{},completed{};
};
struct HandWritten {
    uptr pc{},pawn{},mesh{},tree{},weapon{};std::array<uptr,8> controls{};
    std::array<unsigned char,1024> identity{};unsigned identity_bytes{};
    std::array<std::array<unsigned char,256>,8> fixed{};std::array<unsigned,8> fixed_bytes{};
    std::array<std::array<float,3>,8> strength{},effector{};std::array<Bytes,8> rotation{};
    std::array<float,16> mesh_world{},left{},right{};
    std::uint64_t ms{};float world_time{},last_render_time{};int mesh_tick{};
    bool control_tick_tag{},mesh_tick_known{},control_tag_known{},action_idle{},known{};
};
struct Evidence {
    Observation written{};Key key{};Value original{},owned{};TargetGeometry geometry{};
    gears_hand_continuity::Certificate certificate{};
    HandWritten hand_written{};
    State final_read{};bool final_read_attempted{},final_read_ok{};
    std::uint64_t sequence{},revision{};unsigned lease{},mask{},epoch{},dirty{},owned_mask{},revoked{};
    std::array<unsigned,3> baseline_packed{},target_packed{};
    std::array<Pair,component_hold::target_ticks> pairs{};
};
Evidence evidence;
} // namespace hold_detail
// HOLD_EVIDENCE_END
#endif

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
// No engine call. These rechecks do not make the preceding getter observations
// atomic, do not substitute raw slots, and do not exclude write-and-return ABA.
bool current_direct(Key const& expected){
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    for(auto const& old:expected.objects){
        stow::ObjectKey found{};
        if(!lookup::object_key(old.address,found)||std::memcmp(&old,&found,sizeof(old)))return false;
    }
    stow::ObjectKey asset{};
    if(!lookup::object_key(expected.asset.address,asset)||std::memcmp(&asset,&expected.asset,sizeof(asset)))return false;
    const auto address=[&](unsigned i){return expected.objects[i].address;};
    uptr pointer{};
    if(!lookup::pointer(address(2),L"WorldInfo",pointer)||pointer!=address(0)||
       !lookup::pointer(address(2),L"Pawn",pointer)||pointer!=address(3)||
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
       !lookup::pawn_links(address(3),std::array<uptr,4>{{address(2),address(4),address(5),address(6)}},true)||
#else
       !lookup::pointer(address(3),L"Controller",pointer)||pointer!=address(2)||
       !lookup::pointer(address(3),L"InvManager",pointer)||pointer!=address(4)||
       !lookup::pointer(address(3),L"Mesh",pointer,true)||pointer!=address(5)||
       !lookup::pointer(address(3),L"Weapon",pointer)||pointer!=address(6)||
#endif
       !lookup::pointer(address(6),L"Instigator",pointer)||pointer!=address(3)||
       !lookup::pointer(address(6),L"Mesh",pointer,true)||pointer!=address(7)||
       !lookup::pointer(address(7),L"SkeletalMesh",pointer)||pointer!=expected.asset.address)return false;
    for(unsigned i:{0u,2u,3u}){
        uptr outer{};if(!copy(&outer,address(i)+0x40,sizeof(outer))||outer!=address(1))return false;
    }
    return lookup::instance(address(6),L"Weap_AssaultRifle")&&
        lookup::instance(address(7),L"SkeletalMeshComponent")&&
        lookup::instance(expected.asset.address,L"SkeletalMesh")&&lookup::named(expected.asset.address,L"COGAssaultRifle");
}
bool read_direct(Key const& key,State& out){
    out={};if(!current_direct(key))return false;
    const auto mesh=key.objects[7].address;
    const wchar_t* fields[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale"};
    for(unsigned i=0;i<3;++i)if(!lookup::boolean(mesh,fields[i],out.value.absolute[i]))return false;
    const auto translation=lookup::property(mesh,L"Translation"),rotation=lookup::property(mesh,L"Rotation");
    if(!triple_schema(translation,false)||!triple_schema(rotation,true)||
       !lookup::field_at(mesh,translation,L"StructProperty",out.value.translation.data(),12)||
       !lookup::field_at(mesh,rotation,L"StructProperty",out.value.rotation.data(),12))return false;
    std::array<float,3> position{};std::memcpy(position.data(),out.value.translation.data(),12);
    for(float x:position)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    if(!current_direct(key))return false;
    out.key=key;out.known=true;return true;
}
// read_control/read_lists are the existing non-VM typed readers. The semantic
// getters and bone queries remain in the original full prepare snapshot; their
// sequential observations are not relabeled as newly sampled or atomic here.
bool hand_direct_matches(Snapshot const& sampled){
    if(!trial.anchored||sampled.pc!=trial.anchor.pc||sampled.pawn!=trial.anchor.pawn||
       sampled.mesh!=trial.anchor.mesh||sampled.weapon!=trial.anchor.weapon)return false;
    uptr asset{},tree{};
    if(!lookup::pointer(sampled.mesh,L"Animations",tree)||tree!=sampled.tree||
       !lookup::pointer(sampled.mesh,L"SkeletalMesh",asset))return false;
    std::size_t at{};
    const auto take=[&](void* to,std::size_t bytes){
        if(at>sampled.identity.size()||bytes>sampled.identity.size()-at)return false;
        std::memcpy(to,sampled.identity.data()+at,bytes);at+=bytes;return true;
    };
    for(uptr object:{uptr(trial.xr_start.objects[gears_hand_xr_trial::world_info].address),sampled.pc,
                    sampled.pawn,sampled.mesh,sampled.tree,sampled.weapon,asset}){
        stow::ObjectKey expected{},live{};Name n{};
        if(!take(&expected.address,sizeof(expected.address))||!take(&expected.index,sizeof(expected.index))||
           !take(&expected.outer,sizeof(expected.outer))||!take(&n,sizeof(n))||
           !take(&expected.object_class,sizeof(expected.object_class)))return false;
        expected.name_index=n.index;expected.name_number=n.number;
        if(expected.address!=object||!lookup::object_key(object,live)||std::memcmp(&live,&expected,sizeof(live)))return false;
    }
    int left{},right{};bool hidden{},live_hidden{};
    if(!take(&left,sizeof(left))||!take(&right,sizeof(right))||!take(&hidden,sizeof(hidden))||left!=11||right!=35||
       !lookup::boolean(sampled.mesh,L"bIgnoreControllersWhenNotRendered",live_hidden)||hidden!=live_hidden)return false;
    Snapshot layout{};layout.pc=sampled.pc;layout.pawn=sampled.pawn;layout.mesh=sampled.mesh;layout.tree=sampled.tree;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    // PAWN_CACHE_DIRECT_BEGIN
    {
        handik::PawnControlFields cached(sampled.pawn,sampled.mesh,sampled.tree);
        if(!cached.begin())return false;
        for(unsigned i=0;i<8;++i){
            auto& value=layout.controls[i];auto const& old=sampled.controls[i];
            if(!handik::read_control(sampled.pawn,sampled.mesh,sampled.tree,i,value,&cached.fields)||value.object!=old.object||value.fixed!=old.fixed||
               !gears_hand_ik::exact(value.strength,old.strength)||!gears_hand_ik::exact(value.effector,old.effector)||
               std::memcmp(&value.rotation,&old.rotation,sizeof(value.rotation)))return false;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
            if(!gears_hand_tail_translation::exact(value.translation.value,old.translation.value))return false;
#endif
        }
        if(!cached.finish())return false;
    }
    // PAWN_CACHE_DIRECT_END
#else
    for(unsigned i=0;i<8;++i){
        auto& value=layout.controls[i];auto const& old=sampled.controls[i];
        if(!handik::read_control(sampled.pawn,sampled.mesh,sampled.tree,i,value)||value.object!=old.object||value.fixed!=old.fixed||
           !gears_hand_ik::exact(value.strength,old.strength)||!gears_hand_ik::exact(value.effector,old.effector)||
           std::memcmp(&value.rotation,&old.rotation,sizeof(value.rotation)))return false;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        if(!gears_hand_tail_translation::exact(value.translation.value,old.translation.value))return false;
#endif
    }
#endif
    if(!handik::read_lists(layout)||layout.identity.size()!=sampled.identity.size()-at||
       !std::equal(layout.identity.begin(),layout.identity.end(),sampled.identity.begin()+at))return false;
    int health{},ignore{};std::uint8_t special{};
    if(!lookup::field(sampled.pawn,L"Health",L"IntProperty",&health,4)||health<=0||
       !lookup::field(sampled.mesh,L"bIgnoreControllers",L"IntProperty",&ignore,4)||ignore||
       !lookup::field(sampled.pawn,L"SpecialMove",L"ByteProperty",&special,1)||special)return false;
    for(const auto label:{L"bSwitchingWeapons",L"bDoingMeleeAttack",L"bDoingSpecialAttackMove",L"bIsMirrored",L"bWantsToMelee"}){
        bool active{};if(!lookup::boolean(sampled.pawn,label,active)||active)return false;
    }
    return true;
}
struct UpdateFrame {
    gears_hand_stream::Plan plan{};
    Key key{};AssetInput asset{};
    unsigned mask{},epoch{},hand_restored{},hand_setters{},hand_writes{},hand_leases{};
};
static_assert(std::is_trivially_copyable<UpdateFrame>::value,"Continuous update owns a plain immutable value frame");
bool same_update_frame(UpdateFrame const& a,UpdateFrame const& b){
    const auto context=[](gears_hand_stream::Context const& x,gears_hand_stream::Context const& y){
        return gears_hand_stream::same(x.game,y.game)&&x.expected_native_world_generation==y.expected_native_world_generation&&
            x.action_epoch==y.action_epoch&&x.observed_ms==y.observed_ms&&x.idle_ready==y.idle_ready&&x.clean==y.clean;
    };
    auto const& x=a.plan;auto const& y=b.plan;
    return context(x.context,y.context)&&gears_hand_stream::same(x.ticket.stream,y.ticket.stream)&&
        x.ticket.serial==y.ticket.serial&&x.ticket.sequence==y.ticket.sequence&&x.ticket.created_ms==y.ticket.created_ms&&
        x.ticket.controller==y.ticket.controller&&x.ticket.pawn==y.ticket.pawn&&x.ticket.weapon==y.ticket.weapon&&
        x.ticket.units_per_metre==y.ticket.units_per_metre&&x.ticket.valid_mask==y.ticket.valid_mask&&
        gears_hand_continuity::same_certificate(x.ticket.certificate,y.ticket.certificate)&&
        x.revision==y.revision&&x.observed_now==y.observed_now&&x.accept_mask==y.accept_mask&&x.bind_mask==y.bind_mask&&
        x.reset_binding_mask==y.reset_binding_mask&&x.restore_mask==y.restore_mask&&x.baseline_mask==y.baseline_mask&&
        x.has_ticket==y.has_ticket&&a.key==b.key&&same_asset_identity_schema(a.asset,b.asset)&&
        a.asset.origin==b.asset.origin&&a.asset.rotation==b.asset.rotation&&a.mask==b.mask&&a.epoch==b.epoch&&
        a.hand_restored==b.hand_restored&&a.hand_setters==b.hand_setters&&a.hand_writes==b.hand_writes&&a.hand_leases==b.hand_leases;
}
#endif

struct Forward {
    gears_hand_stream::Plan const& plan;
    unsigned mask;
    gears_hand_stream::Context context{};
    Key const& expected_key;
    AssetInput const& asset_input;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    Snapshot prepared_hand{};
#endif
    bool prepare(Key const& key){
        using component_trace::gate;using component_trace::Stage;
        Snapshot hand{};
        if(!gate(Stage::prepare_ended,!trial.ended)||!gate(Stage::prepare_phase,trial.xr_equipment_phase==1)||
           !gate(Stage::prepare_epoch,trial.epoch==plan.context.action_epoch)||!gate(Stage::prepare_tick,trial.tick_epoch==trial.epoch)||
           !gate(Stage::prepare_snapshot,component_cost::snapshot(component_trace::state.meta.group,
               component_trace::state.meta.forward,lease.faulted(),[&](){return handik::snapshot(key.objects[2].address,hand);}))||
           !gate(Stage::prepare_identity,xr_identity(hand))||!gate(Stage::prepare_idle,hand.action_idle)||
           !gate(Stage::prepare_baseline,handik::baseline_strengths(hand))||!gate(Stage::prepare_quiet,slots_quiet(hand))||
           !gate(Stage::prepare_equipment,xr_equipment_gate(hand,1,L"component_setter",EquipmentTiming::prewrite_phase)))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        context=xr_context(hand,trial.xr_ready);
        prepared_hand=hand;
        gate(Stage::prepare_context,true);return true;
#else
        context=xr_context(hand,trial.xr_ready);gate(Stage::prepare_context,true);return true;
#endif
    }
    bool authorize(){
        using component_trace::gate;using component_trace::Stage;
        return gate(Stage::authorize_ended,!trial.ended)&&gate(Stage::authorize_phase,trial.xr_equipment_phase==1)&&
            gate(Stage::authorize_tick,trial.tick_epoch==trial.epoch)&&gate(Stage::authorize_epoch,trial.epoch==plan.context.action_epoch)&&
            gate(Stage::authorize_hand,(mask&2)!=0)&&
            // acquire's internal failure cleanup reuses this backend after
            // setting faulted. Asset geometry cannot become a rollback gate.
            (lease.faulted()||asset_input_matches(expected_key,asset_input))&&
            gate(Stage::authorize,authorize_current_xr_plan(plan,context,GetTickCount64())==mask);
    }
};

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
struct ContinuousAuthorize {
    Forward& forward;
    UpdateFrame const& expected_frame;
    ComponentPreparedCall const* prepared{}; // Only bound during this synchronous callback; never TLS.
    bool active{};
    bool operator()(UpdateFrame const&,Key const&,Value const&,Value const&,unsigned);
};
// Only diagnostic/TLS values. No row, journal, result, context or clock is reset.
struct ForwardDiagnostics {
    unsigned depth{component_trace::state.depth};
    component_trace::Meta meta{component_trace::state.meta};
    bool names{component_names::enabled},paused{component_cost::invoke_paused};
    bool snapshot{component_cost::snapshot_owner},aggregate{component_cost::aggregate_owner},slot{component_cost::slot_owner};
    unsigned snapshot_group{component_cost::snapshot_group},aggregate_group{component_cost::aggregate_group};
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    unsigned callback_index{component_trace::state.callback_index};
#endif
    void restore()const noexcept {
        component_trace::state.depth=depth;component_trace::state.meta=meta;
        component_names::enabled=names;component_cost::invoke_paused=paused;
        component_cost::snapshot_owner=snapshot;component_cost::snapshot_group=snapshot_group;
        component_cost::aggregate_owner=aggregate;component_cost::aggregate_group=aggregate_group;component_cost::slot_owner=slot;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
        component_trace::state.callback_index=callback_index;
#endif
    }
};
static_assert(std::is_trivially_destructible<ForwardDiagnostics>::value,"SEH helper has no unwinding local");
template<class Permit>
bool component_callback_permit(Permit& permit,ContinuousAuthorize& authorize,ComponentPreparedCall const& view){
    const ForwardDiagnostics diagnostic_entry{};
    const auto saved_prepared=authorize.prepared;const bool saved_active=authorize.active;
    __try{
        authorize.prepared=&view;
        return permit();
    }__finally{
        // SEH continues to the selected invoke's original __except. Restore
        // before bridge_after; normal true has exactly the same scoped values.
        authorize.prepared=saved_prepared;authorize.active=saved_active;
        diagnostic_entry.restore();
    }
}
#endif

struct Backend {
    Key expected_key{};
    Forward* forward{}; // nullptr is rollback, authenticated by old Key/value only.
    bool restore_names{}; // Explicit null-forward restore only; names algorithm, no fast read or authority.
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    ContinuousAuthorize* continuous_authorize{};
#endif
    bool read(State& state){
        component_names::Scope names_scope(forward ? !lease.faulted() : restore_names);
        component_cost::Owner cost_owner(component_trace::state.meta.group,forward&&!lease.faulted());
        component_trace::Scope scope;
        const bool ok=component::read(expected_key,state,forward&&!lease.faulted());
        const auto saved=component_trace::state.meta;
        component_trace::state.meta.forward=forward!=nullptr;
        if(!component_trace::state.meta.group)component_trace::state.meta.expected=lease.owned_value();
        component_trace::Row row{};row.stage=component_trace::Stage::backend_read;row.passed=ok;
        row.read_executed=true;row.read_known=state.known;row.key_equal=state.key==expected_key;
        row.read_value=state.value;row.compare_known=lease.has_snapshot()&&state.known;
        if(row.compare_known)row.mismatch=component_trace::mismatch(state.value,component_trace::state.meta.expected);
        component_trace::add(row);
        // Only pure acquisition reads have no setter group, a forward guard,
        // a baseline snapshot and no earlier fault. Their expected full value
        // is the journal's current owned value. Rollback is deliberately excluded.
        if(ok&&state.known&&row.key_equal&&!component_trace::state.meta.group&&forward&&lease.has_snapshot()&&!lease.faulted()){
            row.stage=component_trace::Stage::pure_acquire_value;row.passed=row.mismatch==0;component_trace::add(row);
        }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
        if(!forward&&component_hold::state.closed){
            hold_detail::evidence.final_read_attempted=true;hold_detail::evidence.final_read_ok=ok;
            hold_detail::evidence.final_read=state; // Last actual rollback read, no new engine call.
        }
#endif
        component_trace::state.meta=saved;return ok;
    }
    bool schema(playground::Args& call,unsigned group){
        using component_trace::gate;using component_trace::Stage;
        if(!gate(Stage::schema_valid,call.valid)||!gate(Stage::schema_registered,registered(call.r.function))||
           !gate(Stage::schema_size,call.r.size==12)||
           !gate(Stage::schema_class,playground::named(get<uptr>(call.r.function+0x50),L"Function"))||
           !gate(Stage::schema_return,get<unsigned short>(call.r.function+0x108)==0xffff))return false;
        const auto native=get<uptr>(call.r.function+0x114);
        if(!gate(Stage::schema_native,native>=base+0x1000&&native<base+0x131b000))return false;
        if(group==1){
            if(!gate(Stage::schema_count,stow::parameters(call)==3))return false;
            const wchar_t* fields[]={L"NewAbsoluteTranslation",L"NewAbsoluteRotation",L"NewAbsoluteScale"};
            for(unsigned i=0;i<3;++i){
                if(!gate(Stage::schema_bool_argument,stow::argument(call,fields[i],L"BoolProperty",4,i*4,0x80),i))return false;
                const auto mask=get<unsigned>(call.prop(fields[i])+0xd0);
                if(!gate(Stage::schema_bool_mask,mask&&!(mask&(mask-1)),i,mask))return false;
            }
            return true;
        }
        const auto field=group==2?L"NewTranslation":L"NewRotation";
        return gate(Stage::schema_count,stow::parameters(call)==1)&&
            gate(Stage::schema_struct_argument,stow::argument(call,field,L"StructProperty",12,0,0x80))&&
            gate(Stage::schema_triple,triple_schema(call.prop(field),group==4));
    }
    bool set(Key const& key,Value const& expected,Value const& wanted,unsigned group){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        if(forward)return false; // Three arguments are exclusively original-byte handback.
#endif
        component_names::Scope names_scope(forward ? !lease.faulted() : restore_names);
        using component_trace::gate;using component_trace::Stage;
        component_trace::SetScope scope(group,forward!=nullptr,expected,wanted,
            (component_trace::mismatch(wanted,lease.original_value())&group)==0);
        if(!gate(Stage::set_key,key==expected_key))return false;
        const wchar_t* function=group==1?L"SetAbsolute":group==2?L"SetTranslation":L"SetRotation";
        playground::Args call(key.objects[7].address,function);
        if(!schema(call,group))return false;
        if(group==1){
            call.flag(L"NewAbsoluteTranslation",wanted.absolute[0]);
            call.flag(L"NewAbsoluteRotation",wanted.absolute[1]);call.flag(L"NewAbsoluteScale",wanted.absolute[2]);
        }else if(!gate(Stage::set_bytes,call.bytes(group==2?L"NewTranslation":L"NewRotation",
                  group==2?wanted.translation.data():wanted.rotation.data(),12)))return false;
        State before{};
        if(!gate(Stage::set_read_before,read(before))||
           !gate(Stage::set_expected_before,gears_weapon::mesh_component_exact(before.value,expected),0,
                 component_trace::mismatch(before.value,expected)))return false;
        if(forward&&!forward->prepare(key))return false;
        if(!gate(Stage::set_read_after,read(before))||
           !gate(Stage::set_expected_after,gears_weapon::mesh_component_exact(before.value,expected),0,
                 component_trace::mismatch(before.value,expected))||!schema(call,group)||
           (forward&&!forward->authorize()))return false;
        if(!gate(Stage::set_owned,(lease.owned_mask()&group)!=0))return false;
        SelfScope self_call(call.r.object,call.r.function);count_add(setter_calls);
        component_trace::Row attempt{};attempt.stage=Stage::call_begin;attempt.passed=true;attempt.call_attempted=true;component_trace::add(attempt);
        const bool called=call.run();
        component_trace::Row result{};result.stage=Stage::call_end;result.passed=called;
        result.call_attempted=result.call_return_known=true;result.call_returned=called;component_trace::add(result);
        return called;
    }

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    template<class Permit>bool set_forward(Key const& key,Value const& expected,Value const& wanted,unsigned group,Permit& permit){
        if(!forward||!continuous_authorize||component_continuous::state.closed||trial.ended)return false;
        component_names::Scope names_scope(!lease.faulted());
        using component_trace::gate;using component_trace::Stage;
        component_trace::SetScope scope(group,true,expected,wanted,
            (component_trace::mismatch(wanted,lease.original_value())&group)==0);
        if(!gate(Stage::set_key,key==expected_key))return false;
        const wchar_t* function=group==1?L"SetAbsolute":group==2?L"SetTranslation":L"SetRotation";
        playground::Args call(key.objects[7].address,function);
        if(!schema(call,group))return false;
        if(group==1){
            call.flag(L"NewAbsoluteTranslation",wanted.absolute[0]);
            call.flag(L"NewAbsoluteRotation",wanted.absolute[1]);call.flag(L"NewAbsoluteScale",wanted.absolute[2]);
        }else if(!gate(Stage::set_bytes,call.bytes(group==2?L"NewTranslation":L"NewRotation",
                  group==2?wanted.translation.data():wanted.rotation.data(),12)))return false;
        struct Call {
            Permit& permit;ContinuousAuthorize& authorize;
            static bool consume(void* opaque,ComponentPreparedCall const& view) noexcept {
                auto& binding_state=*static_cast<Call*>(opaque);
                if(binding_state.authorize.prepared||binding_state.authorize.active)return false;
                try{return component_callback_permit(binding_state.permit,binding_state.authorize,view);}
                catch(...){return false;}
            }
        } binding{permit,*continuous_authorize};
        ComponentForwardGate final_gate(call.r,&binding,&Call::consume);
        SelfScope self_call(call.r.object,call.r.function);count_add(setter_calls);
        component_trace::Row attempt{};attempt.stage=Stage::call_begin;attempt.passed=true;attempt.call_attempted=true;component_trace::add(attempt);
        const ForwardDiagnostics invoke_entry{};
        const bool called=invoke_component_forward(&call.r,final_gate);
        // The selected invoke may return false through SEH without unwinding
        // a callback-local C++ binding. Never retain a borrowed stack call-view.
        continuous_authorize->prepared=nullptr;continuous_authorize->active=false;
        invoke_entry.restore();
        component_trace::Row result{};result.stage=Stage::call_end;result.passed=called;
        result.call_attempted=result.call_return_known=true;result.call_returned=called;component_trace::add(result);
        return called;
    }
    template<class Permit>bool set_absolute(Key const& key,Value const& expected,Absolute const& wanted,Permit& permit){
        auto value=expected;value.absolute=wanted;return set_forward(key,expected,value,1,permit);
    }
    template<class Permit>bool set_translation(Key const& key,Value const& expected,Bytes const& wanted,Permit& permit){
        auto value=expected;value.translation=wanted;return set_forward(key,expected,value,2,permit);
    }
    template<class Permit>bool set_rotation(Key const& key,Value const& expected,Bytes const& wanted,Permit& permit){
        auto value=expected;value.rotation=wanted;return set_forward(key,expected,value,4,permit);
    }
#endif

    bool set_absolute(Key const& key,Value const& expected,Absolute const& wanted){
        auto value=expected;value.absolute=wanted;return set(key,expected,value,1);
    }
    bool set_translation(Key const& key,Value const& expected,Bytes const& wanted){
        auto value=expected;value.translation=wanted;return set(key,expected,value,2);
    }
    bool set_rotation(Key const& key,Value const& expected,Bytes const& wanted){
        auto value=expected;value.rotation=wanted;return set(key,expected,value,4);
    }
};

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
bool ContinuousAuthorize::operator()(UpdateFrame const& frame,Key const& key,Value const& previous,Value const& proposed,unsigned group){
    if(active||!component_forward_open()||trial.ended||lease.forward_stopped()||lease.faulted()||
       lease.relinquished()||!lease.has_snapshot())return false;
    struct Active {bool& flag;explicit Active(bool& value):flag(value){flag=true;}~Active(){flag=false;}} callback(active);
    // Frame is the generic owner's independent value copy. No mutable global
    // target/context is substituted for this Tick's source timestamps or keys.
    if(!same_update_frame(frame,expected_frame)||!(key==frame.key)||
       (group!=0&&group!=1&&group!=2&&group!=4))return false;
    if(group&&(!prepared||!prepared->request))return false;
    if(!group&&prepared)return false;
    component_names::Scope names_scope(!lease.faulted());
    Backend checks{key,&forward,false,this};
    using component_trace::gate;using component_trace::Stage;
    using component_trace::CallbackSite;
    component_trace::CallbackScope diagnostic(group);
    State before{};
    diagnostic.site(CallbackSite::read_before);
    if(!gate(Stage::set_read_before,diagnostic.measure(CallbackSite::read_before,[&](){return checks.read(before);}))||!before.known||!(before.key==key)||
       !gate(Stage::set_expected_before,gears_weapon::mesh_component_exact(before.value,previous),0,
             component_trace::mismatch(before.value,previous)))return diagnostic.finish(false);
    if(!diagnostic.measure(CallbackSite::prepare,[&](){return forward.prepare(key);}))return diagnostic.finish(false);
    diagnostic.site(CallbackSite::read_after);
    if(!gate(Stage::set_read_after,diagnostic.measure(CallbackSite::read_after,[&](){return checks.read(before);}))||!before.known||!(before.key==key)||
       !gate(Stage::set_expected_after,gears_weapon::mesh_component_exact(before.value,previous),0,
             component_trace::mismatch(before.value,previous)))return diagnostic.finish(false);
    // Both explicit spans close before the original authority clocks. The
    // prepare span includes its original VM calls; it is not pure reader time.
    State final_value{};diagnostic.site(CallbackSite::component_direct);
    if(!read_direct(key,final_value)||!final_value.known||!(final_value.key==key)||
       !gears_weapon::mesh_component_exact(final_value.value,previous))return diagnostic.finish(false);
    if(!diagnostic.measure(CallbackSite::hand_direct,[&](){return hand_direct_matches(forward.prepared_hand);}))
        return diagnostic.finish(false);
    diagnostic.site(CallbackSite::asset);
    if(!asset_input_matches(key,frame.asset))return diagnostic.finish(false);
    diagnostic.site(CallbackSite::schema);
    if(group){
        const auto& request=*prepared->request;
        if(request.object!=key.objects[7].address||request.size!=12){
            return diagnostic.finish(false);
        }
        const wchar_t* function=group==1?L"SetAbsolute":group==2?L"SetTranslation":L"SetRotation";
        playground::Args schema_call(key.objects[7].address,function);
        if(!checks.schema(schema_call,group)||schema_call.r.function!=request.function){
            return diagnostic.finish(false);
        }
        if(group==1){
            schema_call.flag(L"NewAbsoluteTranslation",proposed.absolute[0]);
            schema_call.flag(L"NewAbsoluteRotation",proposed.absolute[1]);schema_call.flag(L"NewAbsoluteScale",proposed.absolute[2]);
        }else if(!schema_call.bytes(group==2?L"NewTranslation":L"NewRotation",
                 group==2?proposed.translation.data():proposed.rotation.data(),12)){
            return diagnostic.finish(false);
        }
        if(!schema_call.valid||std::memcmp(schema_call.r.params,request.params,12)||!prepared->matches()){
            return diagnostic.finish(false);
        }
    }
    diagnostic.site(CallbackSite::state);
    if(!component_forward_open()||trial.ended||!active_visible.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||trial.xr_equipment_phase!=1||trial.phase!=2||
       trial.epoch!=frame.epoch||trial.tick_epoch!=frame.epoch||trial.epoch!=frame.plan.context.action_epoch||
       !trial.xr_ready||trial.lease.dirty()||trial.lease.faulted()||hand_writes_relinquished||
       trial.restored!=frame.hand_restored||trial.setter_calls!=frame.hand_setters||trial.target_writes!=frame.hand_writes||
       leases.load(std::memory_order_acquire)!=frame.hand_leases||!lease.has_snapshot()||!(lease.key()==key)||
       lease.forward_stopped()||lease.faulted()||lease.relinquished()||lease.revoked_mask()||
       !gears_weapon::mesh_component_exact(lease.owned_value(),previous)||
       (group&&!(lease.owned_mask()&group))||stop_requested.load(std::memory_order_acquire)||
       shutdown_requested.load(std::memory_order_acquire)){
        return diagnostic.finish(false);
    }
    // No VM/getter/hook after this original MS/NS decision. Generic Permit then
    // rechecks stopped/faulted and the explicit stack gate dispatches this call.
    diagnostic.site(CallbackSite::authorization);
    return diagnostic.finish(forward.authorize());
}
#else
bool ContinuousAuthorize::operator()(UpdateFrame const& frame,Key const& key,Value const& previous,Value const& proposed,unsigned group){
    if(active||!component_forward_open()||trial.ended||lease.forward_stopped()||lease.faulted()||
       lease.relinquished()||!lease.has_snapshot())return false;
    struct Active {bool& flag;explicit Active(bool& value):flag(value){flag=true;}~Active(){flag=false;}} callback(active);
    // Frame is the generic owner's independent value copy. No mutable global
    // target/context is substituted for this Tick's source timestamps or keys.
    if(!same_update_frame(frame,expected_frame)||!(key==frame.key)||
       (group!=0&&group!=1&&group!=2&&group!=4))return false;
    if(group&&(!prepared||!prepared->request))return false;
    if(!group&&prepared)return false;
    component_names::Scope names_scope(!lease.faulted());
    Backend checks{key,&forward,false,this};
    using component_trace::gate;using component_trace::Stage;
    State before{};
    if(!gate(Stage::set_read_before,checks.read(before))||!before.known||!(before.key==key)||
       !gate(Stage::set_expected_before,gears_weapon::mesh_component_exact(before.value,previous),0,
             component_trace::mismatch(before.value,previous))||!forward.prepare(key)||
       !gate(Stage::set_read_after,checks.read(before))||!before.known||!(before.key==key)||
       !gate(Stage::set_expected_after,gears_weapon::mesh_component_exact(before.value,previous),0,
             component_trace::mismatch(before.value,previous)))return false;
    // The preceding read's LAST current performs the original four slot VM
    // calls. Everything below is non-invoking, followed by the original clocks.
    State final_value{};
    if(!read_direct(key,final_value)||!final_value.known||!(final_value.key==key)||
       !gears_weapon::mesh_component_exact(final_value.value,previous)||
       !hand_direct_matches(forward.prepared_hand)||!asset_input_matches(key,frame.asset))return false;
    if(group){
        const auto& request=*prepared->request;
        if(request.object!=key.objects[7].address||request.size!=12)return false;
        const wchar_t* function=group==1?L"SetAbsolute":group==2?L"SetTranslation":L"SetRotation";
        playground::Args schema_call(key.objects[7].address,function);
        if(!checks.schema(schema_call,group)||schema_call.r.function!=request.function)return false;
        if(group==1){
            schema_call.flag(L"NewAbsoluteTranslation",proposed.absolute[0]);
            schema_call.flag(L"NewAbsoluteRotation",proposed.absolute[1]);schema_call.flag(L"NewAbsoluteScale",proposed.absolute[2]);
        }else if(!schema_call.bytes(group==2?L"NewTranslation":L"NewRotation",
                 group==2?proposed.translation.data():proposed.rotation.data(),12))return false;
        if(!schema_call.valid||std::memcmp(schema_call.r.params,request.params,12)||!prepared->matches())return false;
    }
    if(!component_forward_open()||trial.ended||!active_visible.load(std::memory_order_acquire)||
       GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||trial.xr_equipment_phase!=1||trial.phase!=2||
       trial.epoch!=frame.epoch||trial.tick_epoch!=frame.epoch||trial.epoch!=frame.plan.context.action_epoch||
       !trial.xr_ready||trial.lease.dirty()||trial.lease.faulted()||hand_writes_relinquished||
       trial.restored!=frame.hand_restored||trial.setter_calls!=frame.hand_setters||trial.target_writes!=frame.hand_writes||
       leases.load(std::memory_order_acquire)!=frame.hand_leases||!lease.has_snapshot()||!(lease.key()==key)||
       lease.forward_stopped()||lease.faulted()||lease.relinquished()||lease.revoked_mask()||
       !gears_weapon::mesh_component_exact(lease.owned_value(),previous)||
       (group&&!(lease.owned_mask()&group))||stop_requested.load(std::memory_order_acquire)||
       shutdown_requested.load(std::memory_order_acquire))return false;
    // No VM/getter/hook after this original MS/NS decision. Generic Permit then
    // rechecks stopped/faulted and the explicit stack gate dispatches this call.
    return forward.authorize();
}
#endif
#endif

void bytes(Bytes const& value){for(auto byte:value)fwprintf(log_file,L"%02x",unsigned(byte));}
void value(const wchar_t* label,Value const& value){
    fwprintf(log_file,L" %ls_absolute=%u%u%u %ls_translation=",label,
        unsigned(value.absolute[0]),unsigned(value.absolute[1]),unsigned(value.absolute[2]),label);
    bytes(value.translation);fwprintf(log_file,L" %ls_rotation=",label);bytes(value.rotation);
}
void asset_key_log(stow::ObjectKey const& key){
    fwprintf(log_file,L"%llx:%llx:%llx:%u:%u:%u:%u",key.address,key.object_class,key.outer,
        key.index,key.name_index,key.name_number,key.reserved);
}
void asset_schema_log(AssetSchema const& schema){
    asset_key_log(schema.property);fwprintf(log_file,L"@");asset_key_log(schema.structure);
    fwprintf(log_file,L"@%u:%u:%u:%u:%u",schema.layout[0],schema.layout[1],schema.layout[2],schema.layout[3],schema.layout[4]);
}
void asset_input_log(const wchar_t* label,AssetInput const& input,bool known){
    fwprintf(log_file,L" %ls_asset_key=",label);if(known)asset_key_log(input.asset);else fwprintf(log_file,L"unknown");
    fwprintf(log_file,L" %ls_weapon_mesh_key=",label);if(known)asset_key_log(input.mesh);else fwprintf(log_file,L"unknown");
    fwprintf(log_file,L" %ls_origin_hex=",label);if(known)bytes(input.origin);else fwprintf(log_file,L"unknown");
    fwprintf(log_file,L" %ls_rot_origin_hex=",label);if(known)bytes(input.rotation);else fwprintf(log_file,L"unknown");
    fwprintf(log_file,L" %ls_origin_schema=",label);if(known)asset_schema_log(input.origin_schema);else fwprintf(log_file,L"unknown");
    fwprintf(log_file,L" %ls_rot_origin_schema=",label);if(known)asset_schema_log(input.rotation_schema);else fwprintf(log_file,L"unknown");
}
void geometry_log(AssetInput const& asset_observation,bool observation_known){
    const bool known=target_geometry.known&&target_geometry.input.known;
    observation_known=observation_known&&asset_observation.known;
    fwprintf(log_file,L" geometry_target_known=%u geometry_non_atomic=1 asset_observation_known=%u "
        L"asset_identity_schema_match=%u asset_values_match=%u asset_observation_origin_zero=%u",
        unsigned(known),unsigned(observation_known),unsigned(known&&observation_known&&same_asset_identity_schema(target_geometry.input,asset_observation)),
        unsigned(known&&observation_known&&target_geometry.input.origin==asset_observation.origin&&target_geometry.input.rotation==asset_observation.rotation),
        unsigned(observation_known&&zero_asset_origin(asset_observation)));
    if(known){
        auto const& model=target_geometry.model;auto const& control=target_geometry.control;
        fwprintf(log_file,L" model_position=%.17g,%.17g,%.17g model_rotation_radians=%.17g,%.17g,%.17g "
            L"control_rotation_radians=%.17g,%.17g,%.17g",model.position.x,model.position.y,model.position.z,
            model.rotation.pitch,model.rotation.yaw,model.rotation.roll,control.pitch,control.yaw,control.roll);
    }else fwprintf(log_file,L" model_position=unknown model_rotation_radians=unknown control_rotation_radians=unknown");
    asset_input_log(L"frozen",target_geometry.input,known);asset_input_log(L"observed",asset_observation,observation_known);
}
void dump_first_bind(bool clean){
    if(!clean||dual_draining||component_trace::state.depth||!log_file||!first_bind.attempted||first_bind.dumped)return;
    first_bind.dumped=true;auto const& row=first_bind;
    fwprintf(log_file,L"hand_weapon_component_first_bind schema=1 bind_args_known=1 pawn_mesh_tick_known=1 "
        L"pawn_mesh_tick=%d hand_mesh_tick_known=%u hand_mesh_tick=%d bind_return_known=%u bind_result=%u "
        L"solve_return_known=%u solve_result=%u solve_pose_known=%u initial_read_after_bind_return_known=%u "
        L"initial_read_after_bind_result=%u initial_read_after_bind_state_known=%u quantized_wanted_known=%u "
        L"synchronized=0 grants_write_authority=0 initial_read_after_bind_not_atomic=1",
        row.pawn_mesh_tick,unsigned(row.hand_mesh_tick_known),row.hand_mesh_tick,unsigned(row.bind_return_known),unsigned(row.bind_result),
        unsigned(row.solve_return_known),unsigned(row.solve_result),unsigned(row.solve_return_known&&row.solve_result),
        unsigned(row.initial_read_return_known),unsigned(row.initial_read_result),unsigned(row.initial_read_state_known),unsigned(row.quantized_wanted_known));
    for(unsigned i=0;i<9;++i){const auto& object=i<8?row.key.objects[i]:row.key.asset;
        fwprintf(log_file,L" key%u=%llx:%llx:%llx:%u:%u:%u key%u_reserved=%u",i,object.address,object.object_class,object.outer,
            object.index,object.name_index,object.name_number,i,object.reserved);}
    for(unsigned i=0;i<4;++i)fwprintf(log_file,L" slot%u=%llu",i,row.key.slots[i]);
    fwprintf(log_file,L" right_grip=%.17g,%.17g,%.17g,%.17g,%.17g,%.17g solved_pose=%.17g,%.17g,%.17g,%.17g,%.17g,%.17g",
        row.right_grip.position.x,row.right_grip.position.y,row.right_grip.position.z,
        row.right_grip.rotation.pitch,row.right_grip.rotation.yaw,row.right_grip.rotation.roll,
        row.solved_pose.position.x,row.solved_pose.position.y,row.solved_pose.position.z,
        row.solved_pose.rotation.pitch,row.solved_pose.rotation.yaw,row.solved_pose.rotation.roll);
    freehands::print_matrix(L"bind_matrix",row.matrix);
    fwprintf(log_file,L" bind_matrix_hex=");auto const* raw=reinterpret_cast<unsigned char const*>(row.matrix.data());
    for(unsigned i=0;i<16*sizeof(float);++i)fwprintf(log_file,L"%02x",unsigned(raw[i]));
    value(L"initial_read_after_bind",row.initial_read_after_bind);value(L"quantized_wanted",row.quantized_wanted);
    fwprintf(log_file,L"\n");fflush(log_file);
}
void log(const wchar_t* phase){
    if(!recorded||!log_file)return;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    const bool hold_written_candidate=!component_hold::state.closed&&!wcscmp(phase,L"written");
    const auto hold_written_epoch=trial.epoch,hold_written_lease=hand_lease;
    const auto hold_written_sequence=plan_sequence,hold_written_revision=plan_revision;
    const auto hold_written_key=lease.key();
    if(hold_written_candidate)component_hold::state.written_ok=false;
#endif
    State readback{};const bool known=read(lease.key(),readback);
    std::array<float,16> matrix{};int tick{};
    const bool world_known=world(lease.key(),matrix,tick);
    std::array<unsigned,3> raw{};const bool raw_known=known&&absolute_storage(lease.key(),raw);
    AssetInput asset_observation{};const bool asset_known=known&&world_known&&read_asset_input(lease.key(),asset_observation);
    fwprintf(log_file,L"hand_weapon_component_sample schema=2 phase=%ls hand_lease=%u plan_revision=%llu "
        L"sequence=%llu action_epoch=%llu equipment_phase=%u readback_known=%u world_known=%u mesh_tick=%d "
        L"native_started_qpc=%llu xr_pid=%llu xr_start=%llu stream_epoch=%llu reference_epoch=%llu "
        L"reference_generation=%llu left_tracking_epoch=%llu right_tracking_epoch=%llu "
        L"source=%u profile_generation=%llu stereo_ns=%lld input_ns=%lld dirty_mask=%u owned_mask=%u revoked_mask=%u",
        phase,hand_lease,plan_revision,plan_sequence,action_epoch,trial.xr_equipment_phase,unsigned(known),unsigned(world_known),tick,
        certificate.native_started_qpc,certificate.xr_publisher_pid,certificate.xr_publisher_start,
        certificate.stream_epoch,certificate.reference_epoch,certificate.reference_generation,
        certificate.tracking_epoch[0],certificate.tracking_epoch[1],certificate.source,
        certificate.profile_generation,certificate.stereo_received_ns,certificate.input_received_ns,
        lease.dirty_mask(),lease.owned_mask(),lease.revoked_mask());
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    fwprintf(log_file,L" component_interval=%llu",component_continuous::state.interval_id);
#endif
    for(unsigned i=0;i<9;++i){const auto& object=i<8?lease.key().objects[i]:lease.key().asset;
        fwprintf(log_file,L" key%u=%llx:%llx:%llx:%u:%u:%u",i,object.address,object.object_class,object.outer,
            object.index,object.name_index,object.name_number);}
    for(unsigned i=0;i<4;++i)fwprintf(log_file,L" slot%u=%llu",i,lease.key().slots[i]);
    value(L"baseline",lease.original_value());value(L"target",target);value(L"readback",readback.value);
    // Exact storage evidence includes unrelated packed bits; ownership and
    // equality cover only the three logical flags, never those unrelated bits.
    fwprintf(log_file,L" absolute_storage_known=%u baseline_absolute_storage=",unsigned(raw_known));
    for(auto bits:baseline_absolute_storage)fwprintf(log_file,L"%08x",bits);
    fwprintf(log_file,L" target_absolute_storage=");for(auto bits:target_absolute_storage)fwprintf(log_file,L"%08x",bits);
    fwprintf(log_file,L" readback_absolute_storage=");for(auto bits:raw)fwprintf(log_file,L"%08x",bits);
    freehands::print_matrix(L"gun",matrix);geometry_log(asset_observation,asset_known);fwprintf(log_file,L"\n");fflush(log_file);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    // Only this completed written sample may support arm. A nested action or
    // pre_restore log can never replace it or revive the closed writer.
    if(hold_written_candidate){
        auto& observation=hold_detail::evidence.written;observation={};
        observation.attempted=true;observation.raw=readback;observation.asset=asset_observation;
        observation.matrix=matrix;observation.mesh_tick=tick;observation.packed=raw;
        observation.read_known=known;observation.world_known=world_known;observation.packed_known=raw_known;
        observation.asset_known=asset_known;
        observation.valid=known&&world_known&&raw_known&&asset_known&&target_geometry.known&&
            readback.known&&readback.key==hold_written_key&&
            gears_weapon::mesh_component_exact(readback.value,lease.owned_value())&&
            same_asset_identity_schema(target_geometry.input,asset_observation)&&
            target_geometry.input.origin==asset_observation.origin&&target_geometry.input.rotation==asset_observation.rotation;
        component_hold::state.written_ok=observation.valid&&!component_hold::state.closed&&
            trial.epoch==hold_written_epoch&&hand_lease==hold_written_lease&&plan_sequence==hold_written_sequence&&
            plan_revision==hold_written_revision&&lease.key()==hold_written_key&&lease.has_snapshot()&&
            lease.dirty()&&!lease.faulted()&&!lease.revoked_mask()&&!trial.ended&&
            !stop_requested.load(std::memory_order_acquire)&&!shutdown_requested.load(std::memory_order_acquire);
        component_hold::state.written_epoch=hold_written_epoch;component_hold::state.written_lease=hold_written_lease;
        component_hold::state.written_sequence=hold_written_sequence;component_hold::state.written_revision=hold_written_revision;
    }
#endif
}
} // namespace component

bool component_dirty(){return component::lease.dirty();}
bool component_faulted(){return component::lease.faulted();}
bool component_owned(bool forward){
    component::State state{};
    const bool faulted=component_faulted();
    component_names::Scope names_scope(forward&&!faulted);
    if(faulted||!component::lease.has_snapshot()||
       !component_tail_cost::test(component_tail_cost::Stage::owned_read,[&]{return component::read(component::lease.key(),state,forward&&!faulted);})||
       !gears_weapon::mesh_component_exact(state.value,component::lease.owned_value())){
        component::lease.relinquish();return false;
    }
    // A geometry-only rejection must leave exact raw ownership available to
    // the existing drain. Default/rollback checks remain the original path.
    return !forward||(component::target_geometry.known&&
        component::asset_input_matches(component::lease.key(),component::target_geometry.input));
}
bool component_related(uptr object){
    if(component::key_known){
        // SetWeaponFromSlot is owned by InvManager, not necessarily Pawn/PC.
        // Every frozen participant is a boundary even while stow is clean.
        for(auto const& key:component::fixed_key.objects)if(object==key.address)return true;
        return false;
    }
    // Before the first mount, the authenticated hand anchor supplies the two
    // additional participants absent from the ordinary hand slot watch list.
    if(!trial.anchored)return false;
    uptr inv{},mesh{};
    return (vrequip::stow::pointer(trial.anchor.pawn,L"InvManager",inv)&&object==inv)||
        (vrequip::stow::pointer(trial.anchor.weapon,L"Mesh",mesh,true)&&object==mesh);
}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
void component_reset_binding(unsigned mask){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(mask&2){
        auto& lifecycle=component_continuous::state;
        if(lifecycle.interval==component_continuous::Interval::owned&&!lifecycle.closed)
            component_continuous::suspend_interval(L"persistent_right_binding_reset",false);
        else if(lifecycle.interval==component_continuous::Interval::waiting&&component::mount.bound()){
            lifecycle.pause=component_continuous::Pause::boundary;lifecycle.resume_stable=false;trial.stability.reset();
        }
    }
#else
    if((mask&2)&&component_continuous::state.begun&&!component_continuous::state.closed){
        component_continuous::close(L"continuous_right_binding_reset",true);
        end(7,L"continuous_right_binding_reset");
    }
#endif
    if(mask&2)component::mount.reset();
}
#else
void component_reset_binding(unsigned mask){if(mask&2)component::mount.reset();}
#endif
void component_relinquish(){
    component::lease.relinquish();component::mount.reset();count_add(component::relinquished);
}
void component_pre_restore(){if(component_dirty()){
    component_names::Scope names_scope(true);component::log(L"pre_restore");
}}
void component_log_written(){if(component_dirty())component::log(L"written");}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
void component_cancel_forward(){component::lease.cancel_forward();}
bool component_has_interval(){return component::lease.has_snapshot();}
bool component_forward_open(){return !component_continuous::state.closed&&!component_continuous::state.terminal;}
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
bool component_pause_owner_healthy(){
    return component::lease.has_snapshot()&&!component::lease.forward_stopped()&&!component::lease.faulted()&&
        !component::lease.relinquished()&&!component::lease.revoked_mask()&&component::mount.bound();
}
#endif
bool component_hand_only_ready(){
    auto const& lifecycle=component_continuous::state;
    const bool waiting=lifecycle.interval==component_continuous::Interval::waiting&&!lifecycle.closed;
    const bool suspended=lifecycle.interval==component_continuous::Interval::suspended&&lifecycle.closed&&
        lifecycle.drain_attempted&&lifecycle.last_drain_result;
    return (waiting||suspended)&&!lifecycle.terminal&&!lifecycle.failure_known&&!lifecycle.cleanup_failed&&
        !lifecycle.deferred&&!lifecycle.finishing&&!lifecycle.action_depth&&!trial.releasing&&!dual_draining&&
        !component_dirty()&&!component_faulted()&&!trial.lease.faulted()&&!hand_writes_relinquished&&
        !component::lease.relinquished()&&!component::lease.revoked_mask()&&!trial.ended&&
        !stop_requested.load(std::memory_order_acquire)&&!shutdown_requested.load(std::memory_order_acquire);
}
bool component_resume_interval(){
    auto& lifecycle=component_continuous::state;
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
       lifecycle.interval!=component_continuous::Interval::suspended||!lifecycle.closed||
       lifecycle.terminal||trial.ended||lifecycle.failure_known||lifecycle.cleanup_failed||
       lifecycle.operation_depth||lifecycle.action_depth||lifecycle.finishing||dual_draining||trial.releasing||
       !lifecycle.drain_attempted||!lifecycle.last_drain_result||lifecycle.hand_only_journal||journals_dirty()||
       trial.lease.faulted()||component_faulted()||hand_writes_relinquished||
       component::lease.relinquished()||component::lease.revoked_mask()||
       stop_requested.load(std::memory_order_acquire)||shutdown_requested.load(std::memory_order_acquire))return false;
    // Only the closed, healthy journal is reset. Session/failure/diagnostic
    // state and the fixed-instance identity survive; no old Plan is reused.
    if(!component::lease.reset())return false;
    if(lifecycle.pause!=component_continuous::Pause::age){
        component::mount.reset();component::target_geometry={};
    }
    component::recorded=false;
    lifecycle.closed=false;lifecycle.deferred=false;lifecycle.drain_attempted=false;lifecycle.last_drain_result=false;
    lifecycle.reason={};lifecycle.interval=component_continuous::Interval::waiting;
    return true;
}
#endif
#endif
bool component_restore(){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component::lease.cancel_forward();
    if(!component_dirty())return !component_faulted();
#else
    if(!component_dirty())return true;
#endif
    component_trace::Scope trace_scope;component_trace::gate(component_trace::Stage::restore_enter,true);
    component::Backend backend{component::lease.key(),nullptr,true};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    const bool clean=component::lease.yield(backend);
#else
    const bool clean=component::lease.restore(backend);
#endif
    if(clean)count_add(component::restored);
    component::recorded=false;publish_journals();
    return component_trace::gate(component_trace::Stage::restore_result,clean&&!component_faulted(),unsigned(clean));
}
bool component_reset(){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    if(component_continuous::state.operation_depth||component_continuous::state.action_depth||
       component_continuous::state.finishing||dual_draining||trial.releasing)return false;
#endif
    if(!component::lease.reset())return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::state={};
#endif
    component_trace::reset();component_cost::reset();component_tail_cost::reset();component::reset_first_bind();
    component::target_geometry={};
    component::mount.reset();component::fixed_key={};component::key_known=component::recorded=false;
    component::terminal_dumped=component::terminal_clean_dumped=false;
    component::acquired=component::restored=component::setter_calls=component::relinquished=0;
    component::hand_lease=0;component::self={};return true;
}
void component_dump_end(){
    if(!trial.ended||!log_file)return;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    if(component_continuous::state.operation_depth||component_continuous::state.action_depth||
       component_continuous::state.finishing||trial.releasing||dual_draining||!component_continuous::state.finalized)return;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    component_hold::dump();
#endif
    const bool first_bind_clean=!journals_dirty();
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    reach_diagnostic::dump(first_bind_clean);
#endif
    component_cost::dump(first_bind_clean);
    component_tail_cost::dump(!journals_dirty());
    component::dump_first_bind(first_bind_clean);
    component_trace::DumpAtExit trace_dump;
    const bool clean=!component_dirty();
    if(component::terminal_dumped&&(!clean||component::terminal_clean_dumped))return;
    component::terminal_dumped=true;if(clean)component::terminal_clean_dumped=true;
    fwprintf(log_file,L"hand_weapon_component_end schema=1 acquired=%llu restored=%llu setter_calls=%llu "
        L"relinquished=%llu dirty=%u faulted=%u dirty_mask=%u owned_mask=%u revoked_mask=%u "
        L"hand_dirty=%u safe_to_unload=%u phase1_only=1 right_hand_only=1 ownership_proved=0\n",
        component::acquired,component::restored,component::setter_calls,component::relinquished,
        unsigned(component_dirty()),unsigned(component_faulted()),component::lease.dirty_mask(),
        component::lease.owned_mask(),component::lease.revoked_mask(),unsigned(trial.lease.dirty()),
        unsigned(!journals_dirty()));fflush(log_file);
}
bool component_apply(gears_hand_stream::Plan const& plan,gears_weapon::HandWorldFrame const& frame,
                     Snapshot const& hand,unsigned mask,unsigned lease_number){
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component_continuous::Operation operation;
    if(!component_forward_open()||trial.ended||trial.lease.dirty()||trial.lease.faulted())return false;
    if(trial.xr_equipment_phase!=1||!(mask&2))return !component_continuous::state.begun&&!component_dirty();
#else
    if(trial.xr_equipment_phase!=1||!(mask&2))return !component_dirty();
#endif
    component_cost::begin(plan.ticket.sequence,plan.revision,lease_number);
    component_cost::Span initial_cost(component_cost::Kind::initial,0,!component::lease.faulted());
    component_trace::begin();component_trace::Scope trace_scope;
    component_trace::gate(component_trace::Stage::acquire_enter,true);
    component::Key key{};
    component::Value wanted{};
    component::TargetGeometry geometry{};
    {
        component_names::Scope names_scope(!component::lease.faulted());
        component_cost::Owner cost_owner(0,!component::lease.faulted());
        if(!component_trace::gate(component_trace::Stage::capture,component::capture(hand,key))||
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
            !component_trace::gate(component_trace::Stage::component_clean,
                !component::lease.has_snapshot()||(!component::lease.forward_stopped()&&!component::lease.revoked_mask()))||
#else
           !component_trace::gate(component_trace::Stage::component_clean,!component_dirty())||
#endif
           !component_trace::gate(component_trace::Stage::component_not_faulted,!component_faulted()))return false;
        if(component::key_known&&!component_trace::gate(component_trace::Stage::key_match,key==component::fixed_key))return false;
        component::fixed_key=key;component::key_known=true;
        std::array<float,16> matrix{};int tick{};
        if(!component_trace::gate(component_trace::Stage::world,component::world(key,matrix,tick)))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
        if(component_continuous::state.interval==component_continuous::Interval::waiting&&
           component_continuous::state.pause==component_continuous::Pause::age&&component::mount.bound()){
            if(trial.epoch!=component::action_epoch||(plan.bind_mask&2)||(plan.reset_binding_mask&2)||
               !component::target_geometry.known||!component::asset_input_matches(key,component::target_geometry.input))return false;
        }
#endif
        bool record_first_bind{};
        if(!component::mount.bound()){
            record_first_bind=!component::first_bind.attempted;
            if(record_first_bind){
                auto& evidence=component::first_bind;evidence.attempted=true;
                evidence.matrix=matrix;evidence.key=key;evidence.right_grip=frame.right;
                evidence.pawn_mesh_tick=tick;evidence.hand_mesh_tick=hand.mesh_tick;evidence.hand_mesh_tick_known=hand.mesh_tick_known;
            }
            const bool bound=component::mount.bind(frame.right,matrix,false);
            if(record_first_bind){component::first_bind.bind_return_known=true;component::first_bind.bind_result=bound;}
            if(!component_trace::gate(component_trace::Stage::mount_bind,bound))return false;
        }
        gears_vr::Viewpoint pose{};
        const bool solved=component::mount.solve(frame.right,pose);
        if(record_first_bind){
            component::first_bind.solve_return_known=true;component::first_bind.solve_result=solved;component::first_bind.solved_pose=pose;
        }
        if(!component_trace::gate(component_trace::Stage::mount_solve,solved))return false;
        component::State before{};
        const bool initial_read=component::read(key,before,true);
        if(record_first_bind){
            component::first_bind.initial_read_return_known=true;component::first_bind.initial_read_result=initial_read;
            component::first_bind.initial_read_state_known=before.known;component::first_bind.initial_read_after_bind=before.value;
        }
        if(!component_trace::gate(component_trace::Stage::initial_read,initial_read))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        if(component::lease.has_snapshot()&&(!(key==component::lease.key())||!before.known||
           !gears_weapon::mesh_component_exact(before.value,component::lease.owned_value())))return false;
#endif
        if(!component::read_asset_input(key,geometry.input)||!component::zero_asset_origin(geometry.input))return false;
        std::array<std::int32_t,3> rot_origin{};std::memcpy(rot_origin.data(),geometry.input.rotation.data(),12);
        geometry.model=pose;
        if(!gears_weapon::model_to_component_rotation(pose.rotation,rot_origin,geometry.control))return false;
        geometry.known=true;
        wanted=before.value;wanted.absolute[0]=wanted.absolute[1]=true;
        std::array<uptr,3> flag_addresses{};std::array<unsigned,3> flag_masks{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        std::array<unsigned,3> update_storage{};
        if(!component_trace::gate(component_trace::Stage::absolute_storage,
            component::absolute_storage(key,update_storage,&flag_addresses,&flag_masks,true)))return false;
        if(!component::lease.has_snapshot())component::baseline_absolute_storage=update_storage;
        component::target_absolute_storage=update_storage;
#else
        if(!component_trace::gate(component_trace::Stage::absolute_storage,
            component::absolute_storage(key,component::baseline_absolute_storage,&flag_addresses,&flag_masks,true)))return false;
        component::target_absolute_storage=component::baseline_absolute_storage;
#endif
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(flag_addresses[i]==flag_addresses[j]){
            auto& bits=component::target_absolute_storage[i];
            bits=wanted.absolute[j]?bits|flag_masks[j]:bits&~flag_masks[j];
        }
        const playground::Vector position{float(pose.position.x),float(pose.position.y),float(pose.position.z)};
        const playground::Rotation rotation{gears_vr::to_rotator_units(geometry.control.pitch),
            gears_vr::to_rotator_units(geometry.control.yaw),gears_vr::to_rotator_units(geometry.control.roll)};
        std::memcpy(wanted.translation.data(),&position,12);std::memcpy(wanted.rotation.data(),&rotation,12);
        if(record_first_bind){component::first_bind.quantized_wanted=wanted;component::first_bind.quantized_wanted_known=true;}
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    const component::UpdateFrame update_frame{plan,key,geometry.input,mask,trial.epoch,trial.restored,
        trial.setter_calls,trial.target_writes,leases.load(std::memory_order_acquire)};
    component::Forward forward{update_frame.plan,update_frame.mask,{},update_frame.key,update_frame.asset};
    component::Backend backend{update_frame.key,&forward};
#else
    component::Forward forward{plan,mask,{},key,geometry.input};component::Backend backend{key,&forward};
#endif
    component::target_geometry=geometry;
    component::target=wanted;component::hand_lease=lease_number;component::plan_sequence=plan.ticket.sequence;
    component::plan_revision=plan.revision;component::action_epoch=plan.context.action_epoch;
    component::certificate=plan.ticket.certificate;
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    component::recorded=trial.record_lease;
#else
    component::recorded=true;
#endif
    initial_cost.finish(true);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    component::ContinuousAuthorize authorize{forward,update_frame};backend.continuous_authorize=&authorize;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    const bool updating=component::lease.has_snapshot();
    auto& lifecycle=component_continuous::state;
    if(!updating){
        if(lifecycle.interval!=component_continuous::Interval::waiting||!lifecycle.resume_stable||
           lifecycle.interval_id==std::numeric_limits<std::uint64_t>::max())return false;
        ++lifecycle.interval_id;lifecycle.interval=component_continuous::Interval::owned;lifecycle.pause=component_continuous::Pause::none;
    }else if(lifecycle.interval!=component_continuous::Interval::owned)return false;
    lifecycle.begun=true;
#else
    component_continuous::state.begun=true;
    const bool updating=component::lease.has_snapshot();
#endif
    if(updating)count_add(component_continuous::state.update_attempts);else count_add(component_continuous::state.acquire_attempts);
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    component_trace::AttemptScope attempt(plan.ticket.sequence,plan.revision,lease_number,lifecycle.interval_id,updating);
#endif
    const bool ok=updating?component::lease.update(key,wanted,update_frame,authorize,backend):
        component::lease.acquire(key,wanted,update_frame,authorize,backend);
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    attempt.finish(ok);
#endif
    if(ok)count_add(component_continuous::state.commits);
#else
    const bool ok=component::lease.acquire(key,wanted,backend);
#endif
    component_trace::gate(component_trace::Stage::acquire_result,ok);
    if(ok)count_add(component::acquired);
    return ok;
}

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
// Shared exact hand readback; component ownership is checked by the caller.
bool component_hand_values_match(Snapshot const& before,Snapshot const& written,unsigned mask,
                                unsigned setters_before,unsigned writes_before,std::array<policy::Target,4> const& targets){
    const unsigned channels=((mask&1)?12u:0u)|((mask&2)?3u:0u);
    unsigned count{};for(unsigned i=0;i<4;++i)if(channels&(1u<<i))++count;
    const auto last=std::numeric_limits<unsigned>::max();
    if(setters_before>last-count||writes_before>last-count||trial.setter_calls!=setters_before+count||
       trial.target_writes!=writes_before+count)return false;
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
    if(!tail_values_match(before,written,mask))return false;
#endif
    for(unsigned i=0;i<8;++i){
        auto const& value=written.controls[i];auto const& old=before.controls[i];
        unsigned selected=4;for(unsigned channel=0;channel<4;++channel)
            if(indices[channel]==i&&(channels&(1u<<channel)))selected=channel;
        if(selected==4){
            if(!gears_hand_ik::exact(value.strength,old.strength)||!gears_hand_ik::exact(value.effector,old.effector)||
               std::memcmp(&value.rotation,&old.rotation,sizeof(value.rotation)))return false;
        }else{
            if(!gears_hand_ik::exact(value.strength,gears_hand_ik::Triple{{1.f,1.f,0.f}}))return false;
            policy::Target target{};
            if(selected&1){
                std::memcpy(target.data(),&value.rotation,12);
                if(!gears_hand_ik::exact(value.effector,old.effector))return false;
            }else{
                std::memcpy(target.data(),value.effector.data(),12);
                if(std::memcmp(&value.rotation,&old.rotation,sizeof(value.rotation)))return false;
            }
            if(target!=targets[selected])return false;
        }
    }
    return component::hand_direct_matches(written);
}
bool component_after_hand_only(Snapshot const& before,Snapshot const& written,unsigned epoch,unsigned mask,unsigned number,
                               unsigned setters_before,unsigned writes_before,std::array<policy::Target,4> const& targets){
    if(!component_hand_only_ready()||!component_continuous::state.hand_only_journal||
       trial.epoch!=epoch||trial.tick_epoch!=epoch||!mask||(mask&~3u)||trial.applied_mask!=mask||leases.load()!=number||
       !trial.lease.dirty()||trial.lease.faulted()||hand_writes_relinquished||
       stop_requested.load(std::memory_order_acquire)||shutdown_requested.load(std::memory_order_acquire)||
       trial.targets!=targets||!same_layout(before,written)||!xr_identity(written))return false;
    return component_hand_values_match(before,written,mask,setters_before,writes_before,targets)&&
        component_hand_only_ready()&&trial.epoch==epoch&&trial.tick_epoch==epoch;
}
#endif
bool component_after_hand(Snapshot const& before,Snapshot const& written,unsigned mask,unsigned number,
                          unsigned setters_before,unsigned writes_before,std::array<policy::Target,4> const& targets){
    if(!component_forward_open()||trial.ended||trial.epoch!=component::action_epoch||trial.tick_epoch!=trial.epoch||
       trial.xr_equipment_phase!=1||!mask||!(mask&2)||trial.applied_mask!=mask||leases.load()!=number||
       !trial.lease.dirty()||trial.lease.faulted()||hand_writes_relinquished||
       stop_requested.load(std::memory_order_acquire)||shutdown_requested.load(std::memory_order_acquire)||
       trial.targets!=targets||!same_layout(before,written)||!xr_identity(written))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    if(!component_hand_values_match(before,written,mask,setters_before,writes_before,targets))return false;
#else
    const unsigned channels=((mask&1)?12u:0u)|((mask&2)?3u:0u);
    unsigned count{};for(unsigned i=0;i<4;++i)if(channels&(1u<<i))++count;
    const auto last=std::numeric_limits<unsigned>::max();
    if(setters_before>last-count||writes_before>last-count||trial.setter_calls!=setters_before+count||
       trial.target_writes!=writes_before+count)return false;
    for(unsigned i=0;i<8;++i){
        auto const& value=written.controls[i];auto const& old=before.controls[i];
        unsigned selected=4;for(unsigned channel=0;channel<4;++channel)
            if(indices[channel]==i&&(channels&(1u<<channel)))selected=channel;
        if(selected==4){
            if(!gears_hand_ik::exact(value.strength,old.strength)||!gears_hand_ik::exact(value.effector,old.effector)||
               std::memcmp(&value.rotation,&old.rotation,sizeof(value.rotation)))return false;
        }else{
            if(!gears_hand_ik::exact(value.strength,gears_hand_ik::Triple{{1.f,1.f,0.f}}))return false;
            policy::Target target{};
            if(selected&1){
                std::memcpy(target.data(),&value.rotation,12);
                if(!gears_hand_ik::exact(value.effector,old.effector))return false;
            }else{
                std::memcpy(target.data(),value.effector.data(),12);
                if(std::memcmp(&value.rotation,&old.rotation,sizeof(value.rotation)))return false;
            }
            if(target!=targets[selected])return false;
        }
    }
#endif
    component::State live{};
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
    return component::read_direct(component::lease.key(),live)&&live.known&&
#else
    return component::hand_direct_matches(written)&&component::read_direct(component::lease.key(),live)&&live.known&&
#endif
        live.key==component::lease.key()&&!component::lease.forward_stopped()&&!component_faulted()&&
        !component::lease.revoked_mask()&&gears_weapon::mesh_component_exact(live.value,component::lease.owned_value())&&
        component::asset_input_matches(component::lease.key(),component::target_geometry.input)&&
        component_forward_open()&&!trial.ended;
}
#endif

bool bridge_before_allowed(uptr object,uptr function){
    // Foreign threads cannot read the private GT state or borrow its token.
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    if(component::consume_self(object,function))return true;
    // Only a registered, successfully named, known non-boundary call bypasses
    // related-object lookup. Unknown calls keep the original fallback below.
    wchar_t passive_name[96]{};
    if(registered(function)&&name(function,passive_name,96)&&!component_action_name(passive_name))return true;
    before_action(object,function,true);
    // Only related mutating action requests are blocked, not passive getters.
    // before_action synchronously drains/revokes its Plan for all watched names.
    if(!related(object)||!registered(function))return true;
    wchar_t label[96]{};if(!name(function,label,96))return false;
    return !component_action_name(label)||equipment_ready();
}

#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
// HOLD_IMPLEMENTATION_BEGIN
namespace component_hold {
// No new ticket is consumed. This single blocked observation invalidates the
// acquire Plan; the retained certificate/sequence are historical evidence only.
void set_reason(const wchar_t* reason){
    if(!reason)reason=L"cancelled";
    state.reason={};std::size_t i{};
    for(;i+1<state.reason.size()&&reason[i];++i)state.reason[i]=reason[i];
    state.reason_truncated=reason[i]!=0;
}
void cancel(const wchar_t* reason){
    if(!state.closed)return;
    if(state.armed||state.arming){
        state.armed=state.arming=false;++state.generation;
        set_reason(reason);
    }
    state.dispatch=nullptr;
}
void before_end(const wchar_t* reason){
    if(!state.closed)return;
    cancel(reason);
    if(!dual_draining&&!state.ending&&journals_dirty()){
        state.ending=true;drain_journals(L"hold_terminal_drain");state.ending=false;
    }
}
void finish(const wchar_t* reason,bool complete=false){
    if(trial.ended){before_end(reason);return;}
    state.complete=complete;cancel(reason);
    // end's prehook drains first, including when restore recursively ends.
    end(complete?5u:7u,reason);
}
bool live(std::uint64_t generation){
    const auto now=GetTickCount64();
    return state.closed&&state.armed&&!state.arming&&state.generation==generation&&
        !trial.ended&&active_visible.load(std::memory_order_acquire)&&
        !stop_requested.load(std::memory_order_acquire)&&!shutdown_requested.load(std::memory_order_acquire)&&
        now>=state.began&&now<=state.deadline&&trial.epoch==state.epoch&&
        trial.xr_equipment_phase==1&&component::lease.has_snapshot()&&component::lease.dirty()&&
        !component::lease.faulted()&&!component::lease.revoked_mask()&&
        component::lease.key()==component::hold_detail::evidence.key&&
        gears_weapon::mesh_component_exact(component::lease.original_value(),component::hold_detail::evidence.original)&&
        gears_weapon::mesh_component_exact(component::lease.owned_value(),component::hold_detail::evidence.owned)&&
        component::setter_calls==state.component_setters;
}
bool same_asset(component::AssetInput const& value){
    const auto& frozen=component::hold_detail::evidence.geometry.input;
    return component::same_asset_identity_schema(frozen,value)&&
        frozen.origin==value.origin&&frozen.rotation==value.rotation;
}
bool copy_hand(Snapshot const& value,component::hold_detail::HandWritten& out){
    // Fixed evidence storage, never a copied vector/heap owner. Overflow is
    // explicitly unknown and cannot arm; no partial/truncated passing record.
    if(value.identity.size()>out.identity.size())return false;
    for(unsigned i=0;i<8;++i)if(value.controls[i].fixed.size()>out.fixed[i].size())return false;
    out.pc=value.pc;out.pawn=value.pawn;out.mesh=value.mesh;out.tree=value.tree;out.weapon=value.weapon;
    out.identity_bytes=static_cast<unsigned>(value.identity.size());
    std::copy(value.identity.begin(),value.identity.end(),out.identity.begin());
    for(unsigned i=0;i<8;++i){
        const auto& control=value.controls[i];out.controls[i]=control.object;
        out.strength[i]=control.strength;out.effector[i]=control.effector;
        static_assert(sizeof(control.rotation)==12);std::memcpy(out.rotation[i].data(),&control.rotation,12);
        out.fixed_bytes[i]=static_cast<unsigned>(control.fixed.size());
        std::copy(control.fixed.begin(),control.fixed.end(),out.fixed[i].begin());
    }
    out.mesh_world=value.mesh_world;out.left=value.left;out.right=value.right;
    out.ms=value.ms;out.world_time=value.world_time;out.last_render_time=value.last_render_time;out.mesh_tick=value.mesh_tick;
    out.control_tick_tag=value.control_tick_tag;out.mesh_tick_known=value.mesh_tick_known;
    out.control_tag_known=value.control_tag_known;out.action_idle=value.action_idle;out.known=true;return true;
}
void observe(component::hold_detail::Observation& row,std::uint64_t generation){
    row.attempted=true;row.begin_ms=GetTickCount64();
    LARGE_INTEGER qpc{};if(QueryPerformanceCounter(&qpc)&&qpc.QuadPart>0)row.qpc_begin=std::uint64_t(qpc.QuadPart);
    const auto key=component::hold_detail::evidence.key;
    // Full original read (fast=false): normally eight slot VM getters. world
    // adds four more. These are engine boundaries inside the observer itself.
    row.read_known=component::read(key,row.raw,false);
    if(live(generation)&&row.read_known){
        row.world_known=component::world(key,row.matrix,row.mesh_tick);
        if(live(generation)&&row.world_known){
            row.packed_known=component::absolute_storage(key,row.packed);
            row.asset_known=component::read_asset_input(key,row.asset);
        }
    }
    row.end_ms=GetTickCount64();qpc={};
    if(QueryPerformanceCounter(&qpc)&&qpc.QuadPart>0)row.qpc_end=std::uint64_t(qpc.QuadPart);
    row.thread=GetCurrentThreadId();row.epoch=trial.epoch;row.hand_dirty=trial.lease.dirty();
    row.component_fault=component::lease.faulted();row.pending=component::lease.dirty_mask();
    row.owned=component::lease.owned_mask();row.revoked=component::lease.revoked_mask();
    row.hand_setters=trial.setter_calls;row.hand_writes=trial.target_writes;row.component_setters=component::setter_calls;
    row.stop=stop_requested.load(std::memory_order_acquire);row.shutdown=shutdown_requested.load(std::memory_order_acquire);
    row.ended=trial.ended;
    row.exact=row.read_known&&row.raw.known&&row.raw.key==key&&
        gears_weapon::mesh_component_exact(row.raw.value,component::hold_detail::evidence.owned);
    row.asset_match=row.asset_known&&same_asset(row.asset);
    row.comparison_known=row.read_known&&row.raw.known&&row.asset_known&&row.asset.known;
    // Recheck after every possible recursive engine callback, never a cached
    // admission from the beginning of the observer. This grants no write.
    row.valid=live(generation)&&!row.hand_dirty&&row.exact&&row.world_known&&row.packed_known&&row.asset_match;
}
void arm(gears_hand_stream::Plan const& plan,unsigned mask,Snapshot const& written){
    if(state.closed||trial.xr_equipment_phase!=1||!(mask&2)||component::acquired!=1)return;
    // Fence before any prospective additional engine boundary. This version
    // reuses the actual written log copies and performs no arm getter.
    state.closed=true;state.arming=true;set_reason(L"arming");++state.generation;
    auto& evidence=component::hold_detail::evidence;
    const auto epoch=trial.epoch;
    gears_hand_stream::Context blocked{};blocked.action_epoch=epoch;blocked.observed_ms=GetTickCount64();
    blocked.idle_ready=false;blocked.clean=false;
    observe_xr_policy(blocked,nullptr,blocked.observed_ms);
    if(!state.written_ok||state.written_epoch!=epoch||state.written_lease!=component::hand_lease||
       state.written_sequence!=plan.ticket.sequence||state.written_revision!=plan.revision||
       !current||!current->outer()||written.pc!=current->object||
       !component::lease.has_snapshot()||!component::lease.dirty()||component::lease.faulted()||
       component::lease.revoked_mask()||!trial.lease.dirty()||trial.lease.faulted()||
       trial.applied_mask!=mask||trial.epoch!=plan.context.action_epoch||
       !active_visible.load(std::memory_order_acquire)||trial.ended||
       stop_requested.load(std::memory_order_acquire)||shutdown_requested.load(std::memory_order_acquire)||
       blocked.observed_ms<trial.began||blocked.observed_ms>trial.deadline){
        finish(L"hold_arm_written_or_identity_unknown");return;
    }
    evidence.key=component::lease.key();evidence.original=component::lease.original_value();
    evidence.owned=component::lease.owned_value();evidence.geometry=component::target_geometry;
    evidence.certificate=plan.ticket.certificate;
    if(!copy_hand(written,evidence.hand_written)){finish(L"hold_hand_written_capacity_unknown");return;}
    evidence.sequence=plan.ticket.sequence;evidence.revision=plan.revision;
    evidence.lease=component::hand_lease;evidence.mask=mask;evidence.epoch=epoch;
    evidence.dirty=component::lease.dirty_mask();evidence.owned_mask=component::lease.owned_mask();
    evidence.revoked=component::lease.revoked_mask();
    evidence.baseline_packed=component::baseline_absolute_storage;evidence.target_packed=component::target_absolute_storage;
    if(!evidence.written.valid||!evidence.geometry.known||
       !gears_weapon::mesh_component_exact(evidence.written.raw.value,evidence.owned)||
       !(evidence.written.raw.key==evidence.key)||!same_asset(evidence.written.asset)){
        finish(L"hold_arm_incomplete_written");return;
    }
    state.pc=current->object;state.function=current->function;state.epoch=epoch;state.mask=mask;
    state.deadline=trial.deadline;state.began=trial.began;
    state.hand_setters=trial.setter_calls;state.hand_writes=trial.target_writes;
    state.component_setters=component::setter_calls;
    state.arming=false;state.armed=true;set_reason(L"armed");
    safe_to_unload.store(false,std::memory_order_release);
}
bool pre_tick(){
    if(!state.closed)return false;
    if(!state.armed||!live(state.generation)){finish(L"hold_pre_cancel_or_watchdog");return true;}
    const auto generation=state.generation;
    const bool hand_before=trial.lease.dirty();
    if(!state.hand_restore_observed)state.hand_before=hand_before;
    bool restored=true;
    if(hand_before){
        component_names::Scope names_scope(false);
        component_names::Scope hand_names_scope(true);
        restored=restore(L"hold_pre_tick_hand_restore");
    }
    if(!state.hand_restore_observed){state.hand_after=trial.lease.dirty();state.hand_restore_observed=true;}
    publish_journals();
    if(!restored||trial.lease.dirty()||!live(generation)){finish(L"hold_hand_restore_or_callback_cancel");return true;}
    trial.tick_epoch=trial.epoch;return true;
}
bool post_tick(){
    if(!state.closed)return false;
    if(!state.armed||!live(state.generation))finish(L"hold_post_cancel_or_watchdog");
    return true; // Closed/aborted also never reaches reader, policy or acquire.
}
void before_original(Token& token,bool qualified){
    if(!state.armed||!qualified||!token.outer())return;
    if(current!=&token||token.object!=state.pc||token.function!=state.function||
       state.dispatch||state.completed>=target_ticks||!live(state.generation)){
        finish(L"hold_original_pre_identity_or_reentry");return;
    }
    auto& pair=component::hold_detail::evidence.pairs[state.completed];
    pair.ordinal=state.completed+1;pair.serial=++state.dispatch_serial;pair.generation=state.generation;
    pair.token=reinterpret_cast<uptr>(&token);pair.object=token.object;pair.function=token.function;
    state.dispatch=&token;pair.entered=true;
    observe(pair.before,pair.generation);
    if(!pair.before.valid||state.dispatch!=&token||!live(pair.generation))
        finish(L"hold_original_pre_unknown_or_changed");
}
void after_original(Token& token,bool qualified){
    if(!qualified)return;
    if(state.completed<target_ticks){
        auto& returned_pair=component::hold_detail::evidence.pairs[state.completed];
        if(returned_pair.entered&&returned_pair.token==reinterpret_cast<uptr>(&token))returned_pair.returned=true;
    }
    if(state.dispatch!=&token)return;
    auto& pair=component::hold_detail::evidence.pairs[state.completed];
    pair.returned=true;
    observe(pair.after,pair.generation);
    if(!pair.after.valid||state.dispatch!=&token||!live(pair.generation)){
        finish(L"hold_original_post_unknown_or_changed");return;
    }
    pair.completed=true;state.dispatch=nullptr;++state.completed;
    if(state.completed==target_ticks)finish(L"hold_two_original_dispatches_complete",true);
}
void token_exit(Token* token){
    if(state.dispatch==token)finish(L"hold_original_post_missing");
}
void print_key(component::Key const& key,bool known){
    for(unsigned i=0;i<9;++i){
        fwprintf(log_file,L" key%u=",i);
        if(known)component::asset_key_log(i<8?key.objects[i]:key.asset);else fwprintf(log_file,L"unknown");
    }
    for(unsigned i=0;i<4;++i){
        fwprintf(log_file,L" slot%u=",i);
        if(known)fwprintf(log_file,L"%llu",key.slots[i]);else fwprintf(log_file,L"unknown");
    }
}
void print_observation(const wchar_t* phase,unsigned ordinal,component::hold_detail::Observation const& row){
    fwprintf(log_file,L"hand_weapon_component_hold_observation schema=1 phase=%ls ordinal=%u attempted=%u "
        L"read_known=%u raw_state_known=%u world_known=%u asset_known=%u packed_known=%u comparison_known=%u valid=%u "
        L"metadata_known=%u ms_labels_known=%u qpc_begin_known=%u qpc_end_known=%u "
        L"thread=%u epoch=%u begin_ms=%llu end_ms=%llu qpc_begin=%llu qpc_end=%llu mesh_tick=%d "
        L"hand_dirty=%u component_fault=%u pending=%u owned=%u revoked=%u hand_setters=%u hand_target_writes=%u "
        L"component_setters=%llu stop=%u shutdown=%u ended=%u non_atomic=1 observer_slot_getters_normally=12",
        phase,ordinal,unsigned(row.attempted),unsigned(row.read_known),unsigned(row.raw.known),unsigned(row.world_known),
        unsigned(row.asset_known),unsigned(row.packed_known),unsigned(row.comparison_known),unsigned(row.valid),
        unsigned(row.thread!=0),unsigned(row.thread!=0),unsigned(row.qpc_begin!=0),unsigned(row.qpc_end!=0),
        unsigned(row.thread),row.epoch,row.begin_ms,row.end_ms,row.qpc_begin,row.qpc_end,row.mesh_tick,
        unsigned(row.hand_dirty),unsigned(row.component_fault),row.pending,row.owned,row.revoked,row.hand_setters,row.hand_writes,
        row.component_setters,unsigned(row.stop),unsigned(row.shutdown),unsigned(row.ended));
    if(row.comparison_known)fwprintf(log_file,L" exact=%u asset_match=%u",unsigned(row.exact),unsigned(row.asset_match));
    else fwprintf(log_file,L" exact=unknown asset_match=unknown");
    const bool raw_known=row.read_known&&row.raw.known;
    if(raw_known)component::value(L"raw",row.raw.value);
    else fwprintf(log_file,L" raw_absolute=unknown raw_translation=unknown raw_rotation=unknown");
    print_key(row.raw.key,raw_known);
    fwprintf(log_file,L" world_matrix=");
    if(row.world_known){for(unsigned i=0;i<16;++i)fwprintf(log_file,L"%ls%.9g",i?L",":L"",double(row.matrix[i]));}
    else fwprintf(log_file,L"unknown");
    fwprintf(log_file,L" packed_absolute=");
    if(row.packed_known)fwprintf(log_file,L"%08x:%08x:%08x",row.packed[0],row.packed[1],row.packed[2]);
    else fwprintf(log_file,L"unknown");
    component::asset_input_log(L"observed",row.asset,row.asset_known&&row.asset.known);
    fwprintf(log_file,L"\n");
}
void dump(){
    if(!state.closed||state.dumped||!trial.ended||current||dual_draining||state.observer_busy||
       state.ending||journals_dirty()||!log_file)return;
    state.dumped=true;
    const auto& evidence=component::hold_detail::evidence;
    fwprintf(log_file,L"hand_weapon_component_hold schema=1 selected_ticks=2 completed_ticks=%u two_pairs_complete=%u "
        L"closed=1 armed=%u generation=%llu epoch=%u mask=%u deadline=%llu began=%llu "
        L"hand_dirty_before=%u hand_dirty_after=%u drain_called=%u first_hand_result=%u first_component_result=%u "
        L"last_hand_result=%u last_component_result=%u terminal_status=%u terminal_epoch=%u "
        L"final_read_attempted=%u final_read_ok=%u final_original_exact=%u stop=%u shutdown=%u reason=%ls reason_truncated=%u "
        L"observer_getters_are_engine_boundaries=1 non_atomic=1 engine_reconsumption_proved=0\n",
        state.completed,unsigned(state.complete),unsigned(state.armed),state.generation,state.epoch,state.mask,
        state.deadline,state.began,unsigned(state.hand_before),unsigned(state.hand_after),unsigned(state.drain_called),
        unsigned(state.hand_result),unsigned(state.component_result),unsigned(state.last_hand_result),unsigned(state.last_component_result),
        status.load(std::memory_order_acquire),trial.epoch,unsigned(evidence.final_read_attempted),
        unsigned(evidence.final_read_ok),unsigned(evidence.final_read_ok&&evidence.final_read.known&&
            evidence.final_read.key==evidence.key&&gears_weapon::mesh_component_exact(evidence.final_read.value,evidence.original)),
        unsigned(stop_requested.load()),unsigned(shutdown_requested.load()),state.reason.data(),unsigned(state.reason_truncated));
    const bool arm_known=state.pc!=0;
    fwprintf(log_file,L"hand_weapon_component_hold_arm schema=1 lease=%u sequence=%llu revision=%llu epoch=%u mask=%u "
        L"arm_known=%u "
        L"hand_setter_baseline=%u hand_target_write_baseline=%u component_setter_baseline=%llu "
        L"dirty_mask=%u owned_mask=%u revoked_mask=%u geometry_known=%u hand_written_known=%u "
        L"baseline_packed=%08x:%08x:%08x target_packed=%08x:%08x:%08x",
        evidence.lease,evidence.sequence,evidence.revision,evidence.epoch,evidence.mask,unsigned(arm_known),state.hand_setters,state.hand_writes,
        state.component_setters,evidence.dirty,evidence.owned_mask,evidence.revoked,unsigned(evidence.geometry.known),
        unsigned(evidence.hand_written.known),evidence.baseline_packed[0],evidence.baseline_packed[1],evidence.baseline_packed[2],
        evidence.target_packed[0],evidence.target_packed[1],evidence.target_packed[2]);
    if(arm_known){component::value(L"original",evidence.original);component::value(L"owned",evidence.owned);}
    else fwprintf(log_file,L" original_absolute=unknown original_translation=unknown original_rotation=unknown "
        L"owned_absolute=unknown owned_translation=unknown owned_rotation=unknown");
    print_key(evidence.key,arm_known);
    const auto& geometry=evidence.geometry;
    if(geometry.known)fwprintf(log_file,L" model_position=%.17g,%.17g,%.17g model_rotation_radians=%.17g,%.17g,%.17g "
        L"control_rotation_radians=%.17g,%.17g,%.17g",geometry.model.position.x,geometry.model.position.y,geometry.model.position.z,
        geometry.model.rotation.pitch,geometry.model.rotation.yaw,geometry.model.rotation.roll,
        geometry.control.pitch,geometry.control.yaw,geometry.control.roll);
    else fwprintf(log_file,L" model_position=unknown model_rotation_radians=unknown control_rotation_radians=unknown");
    component::asset_input_log(L"frozen",geometry.input,geometry.known&&geometry.input.known);fwprintf(log_file,L"\n");
    print_observation(L"written",0,evidence.written);
    // Raw POD copies include all keys, schema, original/owned bytes, D/C and
    // historical certificate. Availability fields, size and encoding are explicit.
    const auto hex=[](const wchar_t* label,const void* value,std::size_t bytes){
        fwprintf(log_file,L"hand_weapon_component_hold_payload schema=1 label=%ls bytes=%zu encoding=native_pod_le hex=",label,bytes);
        const auto* raw=static_cast<unsigned char const*>(value);
        for(std::size_t i=0;i<bytes;++i)fwprintf(log_file,L"%02x",unsigned(raw[i]));
        fwprintf(log_file,L"\n");
    };
    hex(L"arm_and_pairs",&evidence,sizeof(evidence));
    for(unsigned i=0;i<target_ticks;++i){
        const auto& pair=evidence.pairs[i];
        fwprintf(log_file,L"hand_weapon_component_hold_pair schema=1 ordinal=%u serial=%llu generation=%llu "
            L"token=%p object=%p function=%p entered=%u returned=%u complete=%u pre_attempted=%u pre_valid=%u "
            L"post_attempted=%u post_valid=%u pre_begin_ms=%llu pre_end_ms=%llu post_begin_ms=%llu post_end_ms=%llu "
            L"pre_mesh_tick=%d post_mesh_tick=%d pre_qpc_begin=%llu pre_qpc_end=%llu post_qpc_begin=%llu post_qpc_end=%llu\n",
            i+1,pair.serial,pair.generation,reinterpret_cast<void*>(pair.token),reinterpret_cast<void*>(pair.object),
            reinterpret_cast<void*>(pair.function),unsigned(pair.entered),unsigned(pair.returned),unsigned(pair.completed),
            unsigned(pair.before.attempted),unsigned(pair.before.valid),unsigned(pair.after.attempted),unsigned(pair.after.valid),
            pair.before.begin_ms,pair.before.end_ms,pair.after.begin_ms,pair.after.end_ms,pair.before.mesh_tick,pair.after.mesh_tick,
            pair.before.qpc_begin,pair.before.qpc_end,pair.after.qpc_begin,pair.after.qpc_end);
        print_observation(L"pre",i+1,pair.before);print_observation(L"post",i+1,pair.after);
    }
    fflush(log_file);
}
void reset(){state={};component::hold_detail::evidence={};}
} // namespace component_hold
// HOLD_IMPLEMENTATION_END
#endif
