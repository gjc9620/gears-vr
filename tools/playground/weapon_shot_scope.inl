// Actual CallFunction participant. Called only inside handanimcall::hook's
// existing Active lease. No additional hook, engine call, input or damage write.
namespace weaponshot {
namespace shot=gears_weapon_shot;
namespace meta=gripdamage_metadata;
namespace abi=gears_weapon_hand;
thread_local shot::Stack stack;
bool read(void* out,void const* address,std::size_t bytes) noexcept {
    return copy(out,reinterpret_cast<uptr>(address),bytes);
}
enum class Probe {helper,candidate,unknown};
Probe probe(uptr callee) noexcept {
    meta::Table objects{},names{};meta::RawName name{},again{};uptr entry{},last{};unsigned entry_index{};
    if(!meta::tables(base,objects,names)||!meta::read_at(callee,0x48,name)||name.index>=unsigned(names.count)||
       !meta::read_at(names.data,uptr(name.index)*8,entry)||!entry||!meta::read_at(entry,0,entry_index)||entry_index!=name.index)
        return Probe::unknown;
    constexpr wchar_t const* labels[]={L"InstantFire",L"ProjectileFire",L"CustomFire",L"FireAllPellets"};
    unsigned possible=15;bool candidate{};
    for(unsigned i=0;i<16&&possible;++i){
        wchar_t value{};if(!meta::read_at(entry,20+uptr(i)*2,value))return Probe::unknown;
        for(unsigned j=0;j<4;++j)if(possible&(1u<<j)){
            if(labels[j][i]!=value)possible&=~(1u<<j);
            else if(!value){candidate=true;possible=0;break;}
        }
    }
    if(!meta::read_at(callee,0x48,again)||again.index!=name.index||again.number!=name.number||
       !meta::read_at(names.data,uptr(name.index)*8,last)||last!=entry)return Probe::unknown;
    return candidate?Probe::candidate:Probe::helper;
}
abi::ObjectIdentity identity(meta::Identity const& id) noexcept {
    return {id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};
}
bool hash_script(uptr address,unsigned bytes,std::uint64_t& hash) noexcept {
    if(!address||!bytes||bytes>131072||address>UINTPTR_MAX-bytes)return false;
    std::array<unsigned char,256> buffer{};std::uint64_t value=14695981039346656037ull;
    for(unsigned offset=0;offset<bytes;){
        const auto length=(std::min)(unsigned(buffer.size()),bytes-offset);
        if(!copy(buffer.data(),address+offset,length))return false;
        for(unsigned i=0;i<length;++i){value^=buffer[i];value*=1099511628211ull;}
        offset+=length;
    }
    hash=value;return hash!=0;
}
bool bind(uptr callee,shot::Binding& out) noexcept {
    out={};meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_path};
    unsigned function{},type{},owner_type{};
    if(!meta::tables(base,schema.objects,schema.names)||!r.path(callee,function)||
       !r.path(schema.identities[function].klass,type)||!weaponhand::core_type(r,type,L"Function"))return false;
    const auto owner=r.locate(schema.identities[function].outer);
    if(owner==meta::invalid_slot||!r.path(schema.identities[owner].klass,owner_type)||
       !weaponhand::core_type(r,owner_type,L"Class"))return false;
    const auto package=r.locate(schema.identities[owner].outer);
    if(package==meta::invalid_slot||schema.identities[package].outer)return false;
    shot::Binding binding{};
    const bool war=r.named(owner,L"WarWeapon")&&r.named(package,L"WarfareGame");
    const bool engine=r.named(owner,L"Weapon")&&r.named(package,L"Engine");
    const bool shotgun=r.named(owner,L"Weap_Shotgun")&&r.named(package,L"WarfareGame");
    if((war||engine)&&r.named(function,L"InstantFire"))binding.role=war?shot::Role::war_instant:shot::Role::engine_instant;
    else if((war||engine)&&r.named(function,L"ProjectileFire"))binding.role=war?shot::Role::war_projectile:shot::Role::engine_projectile;
    else if(shotgun&&r.named(function,L"CustomFire"))binding.role=shot::Role::shotgun_custom;
    else if(shotgun&&r.named(function,L"FireAllPellets"))binding.role=shot::Role::shotgun_pellets;
    else return false;
    const std::array<unsigned,5> slots{function,owner,package,type,owner_type};
    for(std::size_t i=0;i<slots.size();++i)binding.identities[i]=identity(schema.identities[slots[i]]);
    meta::Table script{};
    if(!r.at(callee,0x8c,script)||script.count<=0||script.capacity<script.count||script.capacity>131072||
       !r.at(callee,0xf4,binding.flags)||!r.at(callee,0xf8,binding.native_index)||!r.at(callee,0x114,binding.exec)||
       base>UINTPTR_MAX-0x1d6ca0||binding.exec!=base+0x1d6ca0)return false;
    binding.script=script.data;binding.bytes=unsigned(script.count);binding.capacity=unsigned(script.capacity);
    if(!hash_script(binding.script,binding.bytes,binding.script_hash)||!binding.valid())return false;
    const auto count=schema.identity_count;
    for(unsigned i=0;i<count;++i){unsigned slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    meta::Table current{},objects{},names{};unsigned flags{};std::uint16_t native{};uptr exec{};
    if(!r.at(callee,0x8c,current)||!meta::same_table(current,script)||!r.at(callee,0xf4,flags)||flags!=binding.flags||
       !r.at(callee,0xf8,native)||native!=binding.native_index||!r.at(callee,0x114,exec)||exec!=binding.exec||
       !meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names))return false;
    out=binding;return true;
}
bool receiver(uptr self,abi::ObjectIdentity& out) noexcept {
    out={};meta::Table objects{},names{};meta::Identity id{};meta::NameText text{};
    if(!meta::tables(base,objects,names)||!meta::raw_identity(objects,names,self,id,text))return false;
    out=identity(id);return abi::identity_valid(out);
}
bool caller_known(uptr address) noexcept {
    gears_ui_call_boundary::Frame frame{},after{};meta::Snapshot schema{};
    meta::Reader r{schema,meta::Stage::filter};unsigned node{},type{},object{};meta::Table script{};
    if(!gears_ui_call_boundary::frame(read,address,frame)||!meta::tables(base,schema.objects,schema.names)||
       !r.path(frame.node,node)||!r.path(schema.identities[node].klass,type)||!weaponhand::core_type(r,type,L"Function")||
       !r.path(frame.object,object)||!r.at(frame.node,0x8c,script)||!script.data||script.count<=0||
       script.count>script.capacity||script.capacity>131072||script.data>UINTPTR_MAX-unsigned(script.count)||
       frame.code<script.data||frame.code-script.data>=unsigned(script.count))return false;
    return gears_ui_call_boundary::frame(read,address,after)&&frame.node==after.node&&frame.object==after.object&&
        frame.code==after.code&&frame.previous==after.previous;
}
bool active_binding_current() noexcept {
    shot::Binding current{},fresh{};abi::ObjectIdentity weapon{},actual{};
    return stack.binding(current,weapon)&&bind(current.identities[0].address,fresh)&&shot::same(current,fresh)&&
        receiver(weapon.address,actual)&&abi::same(actual,weapon);
}
shot::Token enter(uptr self,uptr caller,uptr callee) noexcept {
    if(!weaponhand::current_gt(nullptr)||!enabled.load(std::memory_order_acquire))return {};
    const auto candidate=probe(callee);
    if(candidate==Probe::helper)return {};
    shot::Binding binding{};abi::ObjectIdentity weapon{};
    if(candidate==Probe::unknown||!bind(callee,binding)||!receiver(self,weapon)||!caller_known(caller))return stack.mask();
    shot::Binding parent{};abi::ObjectIdentity parent_weapon{};
    if(stack.binding(parent,parent_weapon)){
        if(!active_binding_current())return stack.mask();
    }
    abi::GripSnapshot frozen{};
    if(!stack.aliases(binding,weapon,caller,read)&&binding.role!=shot::Role::shotgun_pellets)
        weaponhand::client.snapshot_on_gt(frozen);
    return stack.enter(binding,weapon,caller,frozen,weaponhand::clock_ns(nullptr),read);
}
void leave(shot::Token const& token) noexcept {stack.leave(token);}
bool packet(uptr self,uptr caller,shot::Packet& out) noexcept {
    out={};if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||
       weaponhand::client.phase()!=abi::ClientPhase::active||!active_binding_current())return false;
    abi::ObjectIdentity weapon{};
    if(!receiver(self,weapon)||!stack.packet(weapon,caller,read,out))return false;
    // The state is frozen at mode entry. Tracking loss later in the SAME shot
    // does not reread grip; a stopped/replaced Native registration cannot write.
    if(out.grip.grip==abi::Grip::unknown||!weaponhand::client.registration_alive(
       out.grip.key.registration_cookie,out.grip.key.native_started_qpc)){out={};return false;}
    abi::Frame frame{};frame.world=out.grip.world;frame.controller=out.grip.controller;
    frame.pawn=out.grip.pawn;frame.mesh=out.grip.mesh;frame.weapon=out.grip.weapon;
    abi::GripGameState current{};
    if(!weaponhand::observe(nullptr,frame,weaponhand::clock_ns(nullptr),current)||!weaponhand::client.registration_alive(
       out.grip.key.registration_cookie,out.grip.key.native_started_qpc)){out={};return false;}
    return true;
}
bool current(shot::Packet const& before,uptr self,uptr caller) noexcept {
    shot::Packet after{};
    return stack.current(before)&&packet(self,caller,after)&&before.call==after.call&&before.shot==after.shot;
}
bool write_admitted(shot::Packet const& packet) noexcept {
    return enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)&&stack.current(packet)&&
        weaponhand::client.registration_alive(packet.grip.key.registration_cookie,packet.grip.key.native_started_qpc);
}
} // namespace weaponshot
