// Read the current original inventory on the owning game thread. A snapshot
// is not permission to start/finish reload; the physical transaction owns that.
namespace lancerreload_inventory {
namespace hand=gears_weapon_hand;
struct Identity {
    hand::ObjectIdentity world{},pawn{},weapon{};
    std::uint64_t game_start{},native_start{},registration{},lifecycle{},world_generation{},
        hold_generation{},session{},space{},input{};
    bool valid() const noexcept {
        return hand::identity_valid(world)&&hand::identity_valid(pawn)&&hand::identity_valid(weapon)&&
            game_start&&native_start&&registration&&lifecycle&&world_generation&&hold_generation&&session&&space&&input;
    }
    bool operator==(Identity const& b) const noexcept {
        return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&
            game_start==b.game_start&&native_start==b.native_start&&registration==b.registration&&
            lifecycle==b.lifecycle&&world_generation==b.world_generation&&hold_generation==b.hold_generation&&
            session==b.session&&space==b.space&&input==b.input;
    }
};
using Inventory=gears_weapon::ReloadTxnInventory<Identity>;
using Transaction=gears_weapon::ReloadTransaction<Identity>;
Identity identity(hand::Frame const& f) noexcept {
    return {f.world,f.pawn,f.weapon,f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,
        f.key.lifecycle,f.key.world_generation,vrequip::model.hold_generation(),
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch};
}
Identity identity(gears_committed_interaction::Frame const& f) noexcept {
    const auto& r=f.right;
    return {r.world,r.pawn,r.weapon,r.parent.game_start,r.parent.native_started_qpc,r.parent.registration_cookie,
        r.parent.lifecycle,r.parent.world_generation,r.hold_generation,
        r.stereo.session,r.stereo.space_epoch,r.controllers.input_epoch};
}
bool same_instance_across_hold(Identity const& before,Identity const& after) noexcept {
    if(!before.valid()||!after.valid()||after.hold_generation<before.hold_generation||
       after.lifecycle<before.lifecycle)return false;
    // Old hand grants remain invalid. A fresh grab may rebind private saved
    // mechanics after the native hand lifecycle advances during a switch.
    auto normalized=after;normalized.hold_generation=before.hold_generation;
    normalized.lifecycle=before.lifecycle;
    return normalized==before; // World/session/input/native registration remain exact.
}
struct Counts {
    int capacity{},used{},reserve{};
    bool infinite{};
    bool operator==(Counts const& b) const noexcept {
        return capacity==b.capacity&&used==b.used&&reserve==b.reserve&&infinite==b.infinite;
    }
};
bool integer(uptr object,const wchar_t* name,int& value) noexcept {
    if(!registered(object))return false;
    const auto klass=get<uptr>(object+0x50),p=playground::property(object,name);
    if(!klass||!p||get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=4||
       !playground::named(get<uptr>(p+0x50),L"IntProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),extent=get<unsigned>(klass+0x88);
    if(offset>extent||4>extent-offset||object>UINTPTR_MAX-offset-4)return false;
    int before{},after{};
    if(!copy(&before,object+offset,4)||playground::property(object,name)!=p||
       get<unsigned>(p+0x94)!=offset||!copy(&after,object+offset,4)||before!=after)return false;
    value=before;return true;
}
bool query(uptr weapon,Counts& out) noexcept {
    Counts c{};
    playground::Args size(weapon,L"GetMagazineSize");
    if(!vrequip::result_property(size,L"FloatProperty",4)||!size.run())return false;
    const float capacity=size.output<float>();
    if(!std::isfinite(capacity)||capacity<1||capacity>10000||std::floor(capacity)!=capacity)return false;
    c.capacity=int(capacity);
    if(!integer(weapon,L"AmmoUsedCount",c.used)||c.used<0||c.used>c.capacity||
       !integer(weapon,L"SpareAmmoCount",c.reserve)||c.reserve<0||c.reserve>1000000)return false;
    playground::Args infinite(weapon,L"HasInfiniteSpareAmmo");
    if(!vrequip::result_property(infinite,L"BoolProperty",4)||!infinite.run())return false;
    const auto mask=get<unsigned>(infinite.prop(L"ReturnValue")+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    c.infinite=(infinite.output<unsigned>()&mask)!=0;
    out=c;return true;
}
bool read(hand::Frame const& frame,Counts& out) noexcept {
    out={};
    if(!nativeequipment::current(nullptr,frame)||
       (!playground::instance(frame.weapon.address,L"Weap_AssaultRifle")&&
        !playground::instance(frame.weapon.address,L"Weap_LocustAssaultRifle")&&
        !playground::instance(frame.weapon.address,L"Weap_COGPistol"))||
       !vrequip::model.held_for(frame.world.address,frame.pawn.address,frame.weapon.address))return false;
    Counts a{},b{};
    if(!query(frame.weapon.address,a)||!nativeequipment::current(nullptr,frame)||
       !query(frame.weapon.address,b)||!(a==b)||!nativeequipment::current(nullptr,frame))return false;
    out=a;return true;
}
// Revision is this owner's observation revision, NOT a native inventory field.
// Changes to counts/policy advance it. Same-pointer recycled actors or a new
// hold/session cannot be mistaken for a continuation of the prior transaction.
class ObservedInventory {
    Identity bound_{};
    Counts last_{};
    std::uint64_t revision_{};
public:
    bool rebind_hold(hand::Frame const& frame,Identity const& expected,Inventory& out) noexcept {
        out={};const auto next=identity(frame);
        if(!(bound_==expected)||(!(next==expected)&&!same_instance_across_hold(expected,next))||
           !nativeequipment::current(nullptr,frame))return false;
        Inventory prior{};Counts current_counts{};
        if(!sample_owned(expected,prior)||!read(frame,current_counts)||
           !(current_counts==last_)||!(next==identity(frame))||
           !nativeequipment::current(nullptr,frame))return false;
        // Use a copied observer while preparing a coordinated ledger rebind;
        // caller publishes it only after the ledger accepts the same counts.
        bound_=next;out=prior;out.identity=next;return out.valid();
    }
    // Read-only receipt for an already dispatched transaction. Tracking may
    // end during original state exit; it must not be required to acknowledge
    // that exit. This cannot authorize a new inventory write or hand action.
    bool sample_owned(Identity const& expected,Inventory& out) noexcept {
        out={};
        if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire)||
           !revision_||!(expected==bound_))return false;
        hand::ObjectIdentity actual{};
        for(const auto& object:{expected.world,expected.pawn,expected.weapon}){
            if(!nativeequipment::identify(nullptr,object.address,actual)||!hand::same(actual,object))return false;
        }
        if(playground::pointer(expected.weapon.address,L"Instigator")!=expected.pawn.address||
           playground::pointer(expected.pawn.address,L"WorldInfo")!=expected.world.address)return false;
        Counts counts{};
        if(!query(expected.weapon.address,counts))return false;
        if(!(counts==last_)){
            if(revision_==UINT64_MAX)return false;
            ++revision_;last_=counts;
        }
        out={bound_,revision_,unsigned(counts.capacity),unsigned(counts.capacity-counts.used),
            unsigned(counts.reserve),counts.infinite};
        return out.valid();
    }
    bool sample(hand::Frame const& frame,Inventory& out) noexcept {
        out={};const auto before=identity(frame);
        if(!before.valid()||(revision_&&!(before==bound_)))return false;
        Counts counts{};
        if(!read(frame,counts)||!(before==identity(frame)))return false;
        if(!revision_){bound_=before;last_=counts;revision_=1;}
        else if(!(counts==last_)){
            if(revision_==UINT64_MAX)return false;
            ++revision_;last_=counts;
        }
        out={bound_,revision_,unsigned(counts.capacity),unsigned(counts.capacity-counts.used),
            unsigned(counts.reserve),counts.infinite};
        return out.valid();
    }
};
// Validate the setter before a transaction can acquire inventory ownership.
// Exported UnrealScript mislabels its parameter as local; runtime metadata is
// authoritative. No invocation, guessed native address or raw inventory write.
bool setter(uptr weapon,const wchar_t* name,playground::Args& call) noexcept {
    if(call.r.object!=weapon||!call.valid||call.r.size!=4||
       !playground::named(call.r.function,name))return false;
    const auto p=call.prop(L"NewValue");
    return p&&get<unsigned>(p+0x70)==1&&get<unsigned>(p+0x74)==4&&
        get<unsigned>(p+0x94)==0&&
        playground::named(get<uptr>(p+0x50),L"IntProperty")&&
        (get<unsigned long long>(p+0x78)&0x580)==0x80;
}
bool setters_available(hand::Frame const& frame) noexcept {
    if(!nativeequipment::current(nullptr,frame))return false;
    playground::Args used(frame.weapon.address,L"SetAmmoUsedCount");
    playground::Args spare(frame.weapon.address,L"SetSpareAmmoCount");
    return setter(frame.weapon.address,L"SetAmmoUsedCount",used)&&
        setter(frame.weapon.address,L"SetSpareAmmoCount",spare)&&
        nativeequipment::current(nullptr,frame);
}
enum class CommitResult { rejected,committed,reconciliation_required };
using Request=gears_weapon::ReloadTxnRequest<Identity>;
using Receipt=gears_weapon::ReloadTxnReceipt<Identity>;
using OwnsReload=bool(*)(hand::Frame const&,std::uint64_t);
bool counts_equal(Inventory const& a,Inventory const& b) noexcept {
    return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&
        a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;
}
bool set_integer(hand::Frame const& frame,const wchar_t* function,int value,
                 OwnsReload owns,std::uint64_t serial) noexcept {
    if(!owns||!owns(frame,serial)||!nativeequipment::current(nullptr,frame))return false;
    playground::Args call(frame.weapon.address,function);
    return setter(frame.weapon.address,function,call)&&call.set(L"NewValue",value).run();
}
// One writer per physical reload owner. The original-state owner supplies the
// lease check; merely being in Reloading is insufficient. A dispatched token
// cannot be replayed even when its outcome requires later reconciliation.
class InsertionWriter {
    Identity bound_{};
    std::uint64_t last_token_{};
public:
    unsigned phase{};
    CommitResult commit(hand::Frame const& frame,Request const& request,
        ObservedInventory& observer,OwnsReload owns,Receipt& receipt) noexcept {
        receipt={};phase=1;
        if(!request.token||request.token<=last_token_||!request.reload_serial||!request.item_serial||
           request.call!=gears_weapon::ReloadTxnCall::insert||!request.before.valid()||
           !(request.identity==request.before.identity)||!(request.identity==identity(frame))||
           (last_token_&&!(bound_==request.identity))||!owns||!owns(frame,request.reload_serial))
            return CommitResult::rejected;
        phase=2;const auto& before=request.before;const auto& expected=request.expected;
        if(!(expected.identity==before.identity)||expected.capacity!=before.capacity||
           expected.infinite_reserve!=before.infinite_reserve||expected.loaded<=before.loaded||
           expected.loaded>before.capacity||
           (before.infinite_reserve?expected.reserve!=before.reserve:
              (expected.reserve>before.reserve||before.reserve-expected.reserve!=expected.loaded-before.loaded)))
            return CommitResult::rejected;
        phase=3;Inventory current_counts{};
        if(!observer.sample(frame,current_counts))return CommitResult::rejected;
        phase=4;
        if(!counts_equal(current_counts,before)||current_counts.revision!=before.revision)return CommitResult::rejected;
        phase=5;if(!setters_available(frame))return CommitResult::rejected;
        phase=6;if(!owns(frame,request.reload_serial))return CommitResult::rejected;
        bound_=request.identity;last_token_=request.token;
        receipt.token=request.token;
        // Reserve first: a partial call cannot create extra fireable rounds.
        // Infinite spare uses the original policy and never rewrites reserve.
        if(!before.infinite_reserve){
            phase=7;
            const bool invoked=set_integer(frame,L"SetSpareAmmoCount",int(expected.reserve),owns,request.reload_serial);
            // This is a receipt for the setter already dispatched, not a new
            // hand action. Observe the owned objects even if that call used
            // the remainder of the tracked frame's freshness budget. The
            // next setter still independently requires fresh reload ownership.
            if(!observer.sample_owned(request.identity,current_counts))return CommitResult::reconciliation_required;
            if(counts_equal(current_counts,before)){
                receipt.observed=current_counts;return CommitResult::rejected;
            }
            auto intermediate=before;intermediate.reserve=expected.reserve;
            if(!invoked||!counts_equal(current_counts,intermediate))return CommitResult::reconciliation_required;
        }
        phase=8;
        const bool invoked=set_integer(frame,L"SetAmmoUsedCount",int(expected.capacity-expected.loaded),owns,request.reload_serial);
        if(!observer.sample_owned(request.identity,current_counts))return CommitResult::reconciliation_required;
        if(invoked&&counts_equal(current_counts,expected)){
            receipt.accepted=true;receipt.observed=current_counts;return CommitResult::committed;
        }
        if(counts_equal(current_counts,before)){
            receipt.observed=current_counts;return CommitResult::rejected;
        }
        phase=9;auto intermediate=before;intermediate.reserve=expected.reserve;
        if(!before.infinite_reserve&&counts_equal(current_counts,intermediate)&&owns(frame,request.reload_serial)){
            // Compensate ONLY the exact partial state produced by this token.
            // Unexpected concurrent counts or lost ownership must not be overwritten.
            const bool restored=set_integer(frame,L"SetSpareAmmoCount",int(before.reserve),owns,request.reload_serial);
            if(restored&&observer.sample(frame,current_counts)&&counts_equal(current_counts,before)){
                receipt.observed=current_counts;return CommitResult::rejected;
            }
        }
        return CommitResult::reconciliation_required;
    }
};
}
