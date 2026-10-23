#include "weapon_component_continuous_lease.h"
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

unsigned checks{},cases{};
void check(bool value,char const* text){++checks;if(!value)throw std::runtime_error(text);}
using Value=gears_weapon::WeaponMeshComponentValue;
using Bytes=gears_weapon::MeshComponentBytes12;
using Absolute=gears_weapon::MeshComponentAbsolute;
struct Key {
    std::uint64_t address{0x1234},generation{9};
    bool operator==(Key const& b)const noexcept{return address==b.address&&generation==b.generation;}
};
struct Frame {std::uint64_t sequence{1};unsigned epoch{3};};
using Lease=gears_weapon::WeaponMeshComponentContinuousLease<Key>;
using State=gears_weapon::WeaponMeshComponentState<Key>;
static_assert(!std::is_copy_constructible<Lease>::value&&!std::is_move_constructible<Lease>::value,"journal cannot duplicate ownership by copy/move");
bool equal(Value const& a,Value const& b){return gears_weapon::mesh_component_exact(a,b);}
Value value(unsigned seed){
    Value out{};out.absolute={bool(seed&1),bool(seed&2),bool(seed&4)};
    for(unsigned i=0;i<12;++i){out.translation[i]=static_cast<unsigned char>(seed+i);out.rotation[i]=static_cast<unsigned char>(seed+30+i);}
    return out;
}
// These are only synthetic byte writes for the real Backend interface. The
// tests do not reimplement any protocol state transition or ownership model.
void put(Value& out,Value const& in,unsigned group){
    if(group==1)out.absolute=in.absolute;
    else if(group==2)out.translation=in.translation;
    else out.rotation=in.rotation;
}
enum class Mode {normal,no_write_false,complete_false,mixed_false};
struct SyntheticBackend {
    Lease* owner{};Key live_key{};Value live=value(2),first_original=value(2);
    unsigned reads{},forward_vm{},restore_vm{},behavior_group{};
    bool fail_read_once{},unknown_once{},fail_read_after_vm{},behavior_restore{};
    bool skip_permit{},write_without_permit{},reuse_permit{};
    Mode behavior=Mode::normal;
    std::function<void(unsigned)> before_permit,after_forward_vm;
    std::vector<unsigned> forward_groups,restore_groups;
    bool read(State& out){
        ++reads;out={};if(fail_read_once){fail_read_once=false;return false;}
        out.key=live_key;out.value=live;out.known=!unknown_once;unknown_once=false;return true;
    }
    bool authenticate(Key const& key,Value const& expected)const{return key==live_key&&equal(live,expected);}
    bool perform(unsigned group,Value const& proposed,bool restoring){
        check((owner->dirty_mask()&group)!=0,"pending already set before possible VM write");
        check(equal(owner->original_value(),first_original),"first original immutable at every setter");
        if(restoring){++restore_vm;restore_groups.push_back(group);}else{++forward_vm;forward_groups.push_back(group);}
        Mode selected=Mode::normal;
        if(group==behavior_group&&restoring==behavior_restore){selected=behavior;behavior=Mode::normal;}
        if(selected==Mode::mixed_false){
            if(group==1)live.absolute[0]=proposed.absolute[0];
            else if(group==2)for(unsigned i=0;i<6;++i)live.translation[i]=proposed.translation[i];
            else for(unsigned i=0;i<6;++i)live.rotation[i]=proposed.rotation[i];
        }else if(selected!=Mode::no_write_false)put(live,proposed,group);
        if(!restoring&&after_forward_vm)after_forward_vm(group);
        if(fail_read_after_vm){fail_read_after_vm=false;fail_read_once=true;}
        return selected==Mode::normal;
    }
    template<class Permit>bool forward(Key const& key,Value const& expected,Value const& proposed,unsigned group,Permit& permit){
        if(!authenticate(key,expected))return false;
        check((owner->dirty_mask()&group)!=0,"pending precedes authorization callback");
        if(before_permit)before_permit(group); // Synthetic earlier engine/preparation work.
        if(skip_permit){
            // Deliberate invalid-Backend negative fixture, never a production fallback.
            if(write_without_permit)perform(group,proposed,false);
            return true;
        }
        const bool allowed=permit();
        if(reuse_permit){check(!permit(),"permit second call rejects reuse");return false;}
        if(!allowed)return false;
        // Pure expected-byte check after callback; no engine boundary between
        // successful final permit and this one synthetic VM mutation.
        if(!authenticate(key,expected))return false;
        return perform(group,proposed,false);
    }
    bool restore(Key const& key,Value const& expected,Value const& proposed,unsigned group){
        if(!authenticate(key,expected))return false;
        return perform(group,proposed,true);
    }
    template<class Permit>bool set_absolute(Key const& key,Value const& expected,Absolute const& wanted,Permit& permit){auto v=expected;v.absolute=wanted;return forward(key,expected,v,1,permit);}
    template<class Permit>bool set_translation(Key const& key,Value const& expected,Bytes const& wanted,Permit& permit){auto v=expected;v.translation=wanted;return forward(key,expected,v,2,permit);}
    template<class Permit>bool set_rotation(Key const& key,Value const& expected,Bytes const& wanted,Permit& permit){auto v=expected;v.rotation=wanted;return forward(key,expected,v,4,permit);}
    bool set_absolute(Key const& key,Value const& expected,Absolute const& wanted){auto v=expected;v.absolute=wanted;return restore(key,expected,v,1);}
    bool set_translation(Key const& key,Value const& expected,Bytes const& wanted){auto v=expected;v.translation=wanted;return restore(key,expected,v,2);}
    bool set_rotation(Key const& key,Value const& expected,Bytes const& wanted){auto v=expected;v.rotation=wanted;return restore(key,expected,v,4);}
};
struct Authorize {
    Lease* owner{};SyntheticBackend* backend{};Frame expected{};
    bool live_fresh{true};unsigned live_epoch{3},denied_group{99};
    std::function<void(unsigned)> callback;
    std::vector<unsigned> groups;
    bool operator()(Frame const& frame,Key const& key,Value const& previous,Value const& candidate,unsigned group){
        groups.push_back(group);
        check(frame.sequence==expected.sequence&&frame.epoch==expected.epoch,"entry-frozen independent frame value");
        check(key==backend->live_key,"caller authorizer checks current independent identity");
        if(group){
            check((owner->dirty_mask()&group)!=0,"authorization sees journal before setter");
            for(unsigned other:{1u,2u,4u})if(other!=group){auto a=previous;put(a,candidate,other);check(equal(a,previous),"candidate setter changes only its named group");}
        }
        if(callback)callback(group);
        return live_fresh&&frame.epoch==live_epoch&&group!=denied_group;
    }
};
struct Fixture {
    Lease lease;SyntheticBackend backend;Frame frame;Authorize authorize;
    Fixture(){backend.owner=&lease;authorize.owner=&lease;authorize.backend=&backend;authorize.expected=frame;}
    bool acquire(Value const& wanted){return lease.acquire(backend.live_key,wanted,frame,authorize,backend);}
    void next(){++frame.sequence;authorize.expected=frame;authorize.groups.clear();}
    bool update(Value const& wanted){return lease.update(backend.live_key,wanted,frame,authorize,backend);}
    void started(){check(acquire(value(7)),"initial actual acquire");check(lease.dirty_mask()==7,"three groups initially owned");next();}
};
void blocked( Fixture& f){
    unsigned const calls=f.backend.forward_vm,auths=static_cast<unsigned>(f.authorize.groups.size());
    check(!f.update(value(17)),"stopped/faulted session rejects further update");
    check(f.backend.forward_vm==calls&&f.authorize.groups.size()==auths,"no cached permission revives update");
}
void original_restored(Fixture& f){
    auto const auths=f.authorize.groups.size();check(f.lease.yield(f.backend),"journal yield complete");
    check(!f.lease.dirty()&&equal(f.backend.live,f.backend.first_original),"exact first original all groups");
    check(f.authorize.groups.size()==auths,"yield never needs a fresh pose authorization");
}
void run(char const* name,std::function<void()> test){test();++cases;std::printf("case %u %s passed\n",cases,name);}
int main(){
    try {
        run("multiple owned-to-owned updates retain first original",[]{
            Fixture f;f.started();check(f.update(value(12)),"first actual update");f.next();check(f.update(value(17)),"second actual update");
            check(equal(f.lease.original_value(),value(2))&&equal(f.lease.owned_value(),value(17)),"original versus latest owned distinct");
            check(f.authorize.groups==std::vector<unsigned>({0,1,2,4}),"new frame independently authorizes each final setter");
            original_restored(f);blocked(f);check(f.lease.reset(),"explicit drained reset");
            f.backend.live=f.backend.first_original=value(23);f.next();check(f.acquire(value(27)),"new explicit interval takes new original");
            check(equal(f.lease.original_value(),value(23)),"new original only after drained reset");original_restored(f);
        });
        run("no-change frame still has independent admission",[]{
            Fixture f;f.started();unsigned const writes=f.backend.forward_vm;check(f.update(value(7)),"no-change update");
            check(f.authorize.groups==std::vector<unsigned>({0})&&f.backend.forward_vm==writes,"no-change authorization not cached or skipped");
            f.next();f.authorize.live_fresh=false;check(!f.update(value(7)),"stale no-change admission rejected");blocked(f);original_restored(f);
        });
        run("fresh final callback after prior preparation boundary",[]{
            Fixture f;f.started();auto const writes=f.backend.forward_vm;
            f.backend.before_permit=[&](unsigned group){if(group==2)f.authorize.live_fresh=false;};
            check(!f.update(value(12)),"later final permission independently rejects");
            check(f.backend.forward_vm==writes+1&&f.authorize.groups==std::vector<unsigned>({0,1,2}),"admission/group1 true not reused for group2");
            f.backend.before_permit={};blocked(f);original_restored(f);
        });
        run("caller frame and wanted mutation cannot alter frozen transaction",[]{
            Fixture f;Value desired=value(7);Value const frozen=desired;
            f.backend.after_forward_vm=[&](unsigned group){if(group==1){f.frame.sequence=900;f.frame.epoch=99;desired=value(51);}};
            check(f.acquire(desired),"acquire uses immutable entry snapshots");check(equal(f.backend.live,frozen),"remaining groups use original desired snapshot");
            check(f.authorize.groups==std::vector<unsigned>({0,1,2,4}),"all callbacks see original frame, each called fresh");f.backend.after_forward_vm={};original_restored(f);
        });
        run("current epoch change invalidates the next final permit",[]{
            Fixture f;f.started();auto const writes=f.backend.forward_vm;
            f.backend.before_permit=[&](unsigned group){if(group==2)++f.authorize.live_epoch;};
            check(!f.update(value(12))&&f.backend.forward_vm==writes+1,"old frame not allowed after independent live epoch changes");
            f.backend.before_permit={};blocked(f);original_restored(f);
        });
        for(auto mode:{Mode::no_write_false,Mode::complete_false})run(mode==Mode::no_write_false?"false setter with no write retains old owned":"false setter with complete write owns exact new",[mode]{
            Fixture f;f.started();f.backend.behavior_group=2;f.backend.behavior=mode;
            check(!f.update(value(12)),"false setter fails forward update");
            check(f.lease.dirty_mask()==7&&f.lease.owned_mask()==7&&f.lease.faulted(),"full journal remains owned after exact old/new readback");
            check(f.lease.owned_value().translation==(mode==Mode::no_write_false?value(7).translation:value(12).translation),"readback selects actually observed old/new bytes");
            blocked(f);original_restored(f);
        });
        run("mixed update revokes group and ABA cannot restore permission",[]{
            Fixture f;f.started();f.backend.behavior_group=2;f.backend.behavior=Mode::mixed_false;
            check(!f.update(value(12)),"mixed update rejected");check(f.lease.dirty_mask()==7&&(f.lease.revoked_mask()&2),"mixed pending retained and revoked");
            check(!f.lease.yield(f.backend)&&f.lease.dirty_mask()==2,"other two groups clean, mixed group retained");auto const writes=f.backend.restore_vm;
            f.backend.live.translation=value(7).translation;check(!f.lease.yield(f.backend),"old-owned ABA no permission");
            f.backend.live.translation=value(12).translation;check(!f.lease.yield(f.backend),"new-owned ABA no permission");
            check(f.backend.restore_vm==writes&&!f.lease.reset(),"neither ABA nor reset clears unresolved journal");
            f.lease.relinquish();check(f.lease.relinquished()&&f.lease.dirty()&&!f.lease.reset(),"explicit external relinquish recorded, not fake clean");
            f.backend.live.translation=value(2).translation;check(f.lease.yield(f.backend)&&!f.lease.dirty(),"same-Key exact original resolves read-only");
            check(f.backend.restore_vm==writes&&f.lease.forward_stopped()&&f.lease.faulted(),"read-only resolution does not resume forward");
        });
        run("initial false no-write can clear only verified original",[]{
            Fixture f;f.backend.behavior_group=1;f.backend.behavior=Mode::no_write_false;
            check(!f.acquire(value(7)),"initial no-write fails acquisition");
            check(!f.lease.dirty()&&f.lease.has_snapshot()&&f.lease.faulted()&&f.lease.forward_stopped(),"verified original is drained but not an acquired clean permission");
            check(equal(f.lease.original_value(),value(2))&&equal(f.backend.live,value(2)),"first original still retained");blocked(f);
        });
        run("initial mixed Absolute write stays pending",[]{
            Fixture f;f.backend.behavior_group=1;f.backend.behavior=Mode::mixed_false;
            check(!f.acquire(value(7))&&f.lease.dirty_mask()==1&&f.lease.revoked_mask()==1,"initial mixed flags revoked");
            check(!f.lease.yield(f.backend)&&f.backend.restore_vm==0&&!f.lease.reset(),"mixed flags not overwritten or reset");
        });
        run("mixed Rotation write remains independently revoked",[]{
            Fixture f;f.started();f.backend.behavior_group=4;f.backend.behavior=Mode::mixed_false;
            check(!f.update(value(12))&&(f.lease.revoked_mask()&4),"mixed rotation rejected");
            check(!f.lease.yield(f.backend)&&f.lease.dirty_mask()==4,"other two groups restore without overwriting mixed rotation");
        });
        run("unknown post-write read revokes all pending",[]{
            Fixture f;f.started();f.backend.fail_read_after_vm=true;
            check(!f.update(value(12))&&f.lease.dirty_mask()==7&&f.lease.owned_mask()==0,"unknown post-read retains revoked journals");
            check(!f.lease.yield(f.backend)&&f.backend.restore_vm==0,"later exact targets not treated as authority");
            f.backend.live=f.backend.first_original;check(f.lease.yield(f.backend)&&!f.lease.dirty(),"original-only read resolution");
        });
        run("unknown known-bit read retains pending",[]{
            Fixture f;f.started();f.backend.unknown_once=true;
            check(!f.update(value(12))&&f.lease.owned_mask()==0&&f.lease.dirty_mask()==7,"unknown State rejected");blocked(f);
        });
        run("Key loss and old-generation ABA cannot revive",[]{
            Fixture f;f.started();auto const original_key=f.backend.live_key;++f.backend.live_key.generation;
            check(!f.update(value(12))&&f.lease.owned_mask()==0,"supplied different generation revokes");
            f.backend.live_key=original_key;check(!f.lease.yield(f.backend)&&f.backend.restore_vm==0,"matching old Key alone is not permission");
            f.backend.live=f.backend.first_original;check(f.lease.yield(f.backend),"same Key exact original read-only clear");
        });
        run("foreign owned mutation yields only unaffected groups",[]{
            Fixture f;f.started();f.backend.live.rotation.fill(231);
            check(!f.update(value(12))&&(f.lease.revoked_mask()&4),"foreign rotation revoked");
            check(!f.lease.yield(f.backend)&&f.backend.restore_groups==std::vector<unsigned>({1,2}),"foreign group untouched, other groups restored");
            auto const writes=f.backend.restore_vm;f.backend.live.rotation=value(7).rotation;
            check(!f.lease.yield(f.backend)&&f.backend.restore_vm==writes,"foreign write-return ABA stays revoked");
            f.backend.live.rotation=value(2).rotation;check(f.lease.yield(f.backend),"original can clear foreign journal read-only");
        });
        run("unexpected original return cannot silently reopen forward",[]{
            Fixture f;f.started();f.backend.live.absolute=value(2).absolute;
            check(!f.update(value(12))&&f.lease.dirty_mask()==6,"original observed read-only but forward interval stopped");blocked(f);original_restored(f);
        });
        run("Stop cancellation only allows yield",[]{
            Fixture f;f.started();auto const reads=f.backend.reads;f.lease.cancel_forward();blocked(f);
            check(f.backend.reads==reads&&f.lease.owned_mask()==7,"cancel does not fabricate new reads or revoke needed cleanup");original_restored(f);
        });
        run("callback cancellation rejects returned true",[]{
            Fixture f;f.started();auto const writes=f.backend.forward_vm;
            f.authorize.callback=[&](unsigned group){if(group==2)f.lease.cancel_forward();};
            check(!f.update(value(12))&&f.backend.forward_vm==writes+1,"cancel in final callback stops group2 setter");
            f.authorize.callback={};blocked(f);original_restored(f);
        });
        run("callback revocation rejects returned true",[]{
            Fixture f;f.started();auto const writes=f.backend.forward_vm;
            f.authorize.callback=[&](unsigned group){if(group==1)f.lease.revoke();};
            check(!f.update(value(12))&&f.backend.forward_vm==writes&&f.lease.owned_mask()==0,"revocation inside final callback invalidates its true result");
            f.authorize.callback={};check(!f.lease.yield(f.backend)&&f.backend.restore_vm==0,"revoked targets do not restore on matching readback");
        });
        run("cancel during VM records written bytes then stops later groups",[]{
            Fixture f;f.started();auto const writes=f.backend.forward_vm;
            f.backend.after_forward_vm=[&](unsigned group){if(group==1)f.lease.cancel_forward();};
            check(!f.update(value(12))&&f.backend.forward_vm==writes+1,"no later setter after Stop callback");
            check(f.lease.owned_value().absolute==value(12).absolute,"readback preserves actual completed first setter");
            f.backend.after_forward_vm={};original_restored(f);
        });
        run("revoke during VM never regrants cleanup writes",[]{
            Fixture f;f.started();f.backend.after_forward_vm=[&](unsigned){f.lease.revoke();};
            check(!f.update(value(12))&&f.lease.owned_mask()==0&&f.lease.dirty_mask()==7,"revoked mid-set pending retained");
            f.backend.after_forward_vm={};check(!f.lease.yield(f.backend)&&f.backend.restore_vm==0,"no automatic recovery permission from readback");
        });
        run("reentrant update invalidates final permit",[]{
            Fixture f;f.started();auto const writes=f.backend.forward_vm;
            f.authorize.callback=[&](unsigned group){if(group==1)check(!f.update(value(17)),"nested update refused while busy");};
            check(!f.update(value(12))&&f.backend.forward_vm==writes&&f.lease.owned_mask()==0,"reentrant authorization returned true is still rejected");
            f.authorize.callback={};check(!f.lease.yield(f.backend)&&f.backend.restore_vm==0,"reentry revocation survives later reads");
        });
        run("missing final permit is not accepted",[]{
            Fixture f;f.started();f.backend.skip_permit=true;auto const writes=f.backend.forward_vm;
            check(!f.update(value(12))&&f.backend.forward_vm==writes,"old three-argument/pre-admission permission is no fallback");
            f.backend.skip_permit=false;original_restored(f);
        });
        run("invalid Backend write without permit is not claimed owned",[]{
            Fixture f;f.started();f.backend.skip_permit=f.backend.write_without_permit=true;
            check(!f.update(value(12))&&(f.lease.revoked_mask()&1)&&f.lease.dirty_mask()==7,"unauthorized changed group revoked on readback");
            f.backend.skip_permit=false;check(!f.lease.yield(f.backend)&&f.lease.dirty_mask()==1,"invalid Backend cannot create rollback permission");
        });
        run("permit is single-use",[]{
            Fixture f;f.started();f.backend.reuse_permit=true;auto const writes=f.backend.forward_vm;
            check(!f.update(value(12))&&f.backend.forward_vm==writes&&f.lease.owned_mask()==0,"second permit refuses cached true reuse");
        });
        for(auto mode:{Mode::complete_false,Mode::mixed_false})run(mode==Mode::complete_false?"yield false-after-complete exact original drains only journal":"mixed yield remains pending and revoked",[mode]{
            Fixture f;f.started();f.lease.cancel_forward();f.backend.behavior_group=2;f.backend.behavior=mode;f.backend.behavior_restore=true;
            bool const drained=f.lease.yield(f.backend);check(drained==(mode==Mode::complete_false),"yield result follows exact original readback");
            check(f.lease.faulted()&&f.lease.forward_stopped(),"yield does not clear sticky failure or resume");
            if(mode==Mode::mixed_false)check(f.lease.dirty_mask()==2&&!f.lease.reset(),"mixed handback cannot manufacture clean");
        });
        run("callback-side mutation cannot reuse pre-callback read set",[]{
            Fixture f;f.started();f.authorize.callback=[&](unsigned group){if(group==2)f.backend.live.rotation.fill(202);};
            auto const writes=f.backend.forward_vm;check(!f.update(value(12))&&f.backend.forward_vm==writes+1,"Backend final expected comparison rejects changed context");
            check((f.lease.revoked_mask()&4)!=0,"foreign side change remains revoked");
        });
        run("never-owned foreign bytes are not overwritten or called globally clean",[]{
            Fixture f;Value target=value(2);target.rotation=value(7).rotation;
            check(f.acquire(target)&&f.lease.dirty_mask()==4,"only rotation owned");f.next();f.backend.live.absolute=value(7).absolute;
            check(!f.update(value(12)),"foreign never-owned Absolute stops forward");
            check(f.lease.yield(f.backend)&&f.lease.faulted()&&!equal(f.backend.live,f.backend.first_original),"drained self journal is not a global clean Context");
        });
        run("reacquire cannot replace original of an existing interval",[]{
            Fixture f;f.started();check(!f.acquire(value(17))&&f.lease.forward_stopped(),"acquire on owned interval rejects and stops");
            check(equal(f.lease.original_value(),value(2))&&!f.lease.reset(),"original cannot be replaced while dirty");original_restored(f);
        });
        std::printf("{\"passed\":true,\"cases\":%u,\"checks\":%u,\"actual_class\":\"WeaponMeshComponentContinuousLease\",\"Backend_VM\":\"synthetic\",\"runtime_or_cost_proved\":false}\n",cases,checks);return 0;
    }catch(std::exception const& e){std::fprintf(stderr,"FAIL after %u cases/%u checks: %s\n",cases,checks,e.what());return 1;}
}
