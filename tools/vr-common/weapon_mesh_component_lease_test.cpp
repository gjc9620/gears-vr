#include "weapon_mesh_component_lease.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>
#include <vector>

namespace {
unsigned checks{};
void check(bool value,int line){
    ++checks;if(!value){std::fprintf(stderr,"FAIL line %d\n",line);std::exit(1);}
}
#define CHECK(value) check(bool(value),__LINE__)
struct Key {
    unsigned long long object=10,generation=20,controller=30,action_epoch=40;
    bool operator==(Key const& b) const noexcept {
        return object==b.object&&generation==b.generation&&controller==b.controller&&action_epoch==b.action_epoch;
    }
};
using Lease=gears_weapon::WeaponMeshComponentLease<Key>;
using Value=gears_weapon::WeaponMeshComponentValue;
using State=gears_weapon::WeaponMeshComponentState<Key>;
using Bytes=gears_weapon::MeshComponentBytes12;
using Absolute=gears_weapon::MeshComponentAbsolute;
Bytes bytes(unsigned seed){
    Bytes out{};for(unsigned i=0;i<out.size();++i)out[i]=static_cast<unsigned char>(seed+17*i);return out;
}
Value original(){return {{{false,true,false}},bytes(1),bytes(2)};}
Value target(){return {{{true,false,true}},bytes(101),bytes(102)};}
Value foreign(){return {{{true,true,true}},bytes(201),bytes(202)};}
bool exact(Value const& a,Value const& b){return gears_weapon::mesh_component_exact(a,b);}
bool field(Value const& a,Value const& b,unsigned group){
    if(group==1)return a.absolute==b.absolute;
    if(group==2)return a.translation==b.translation;
    return a.rotation==b.rotation;
}
void assign(Value& to,Value const& from,unsigned group){
    if(group==1)to.absolute=from.absolute;
    else if(group==2)to.translation=from.translation;
    else to.rotation=from.rotation;
}
struct Backend {
    enum class Mode {normal,fail_before,fail_after,no_change,partial};
    struct Call {unsigned group;Value expected,wanted;bool guard_passed;};
    State current{Key{},original(),true};
    int reads{};
    std::map<int,Mode> modes;
    std::set<int> failed_reads;
    std::vector<Call> calls;
    std::function<void(Backend&,int)> before_read,before_write,after_write;
    unsigned rejected_expected{};
    bool read(State& out){
        const auto ordinal=reads++;
        if(before_read)before_read(*this,ordinal);
        if(failed_reads.count(ordinal))return false;
        out=current;return true;
    }
    bool write(Key const& key,Value const& expected,Value const& wanted,unsigned group){
        const auto ordinal=static_cast<int>(calls.size());
        if(before_write)before_write(*this,ordinal);
        const bool valid=current.known&&current.key==key&&exact(current.value,expected);
        calls.push_back({group,expected,wanted,valid});
        if(!valid){++rejected_expected;return false;}
        const auto found=modes.find(ordinal);const auto mode=found==modes.end()?Mode::normal:found->second;
        if(mode==Mode::fail_before)return false;
        if(mode==Mode::partial){
            if(group==1)current.value.absolute[0]=wanted.absolute[0];
            else if(group==2)current.value.translation[0]=wanted.translation[0];
            else current.value.rotation[0]=wanted.rotation[0];
        }else if(mode!=Mode::no_change)assign(current.value,wanted,group);
        if(after_write)after_write(*this,ordinal);
        return mode!=Mode::fail_after&&mode!=Mode::partial;
    }
    bool set_absolute(Key const& key,Value const& expected,Absolute const& wanted){
        auto next=expected;next.absolute=wanted;return write(key,expected,next,1);
    }
    bool set_translation(Key const& key,Value const& expected,Bytes const& wanted){
        auto next=expected;next.translation=wanted;return write(key,expected,next,2);
    }
    bool set_rotation(Key const& key,Value const& expected,Bytes const& wanted){
        auto next=expected;next.rotation=wanted;return write(key,expected,next,4);
    }
};
void happy_path_and_exact_bytes(){
    Backend b;Lease lease;const auto key=b.current.key;const auto wanted=target();
    CHECK(!lease.dirty()&&!lease.faulted()&&!lease.has_snapshot());
    CHECK(lease.acquire(key,wanted,b));
    CHECK(lease.has_snapshot()&&lease.key()==key&&exact(lease.original_value(),original()));
    CHECK(lease.dirty_mask()==7&&lease.owned_mask()==7&&lease.revoked_mask()==0);
    CHECK(exact(b.current.value,wanted)&&exact(lease.owned_value(),wanted)&&!lease.reset());
    CHECK(lease.restore(b)&&exact(b.current.value,original())&&!lease.dirty()&&!lease.faulted());
    const auto call_count=b.calls.size();const auto read_count=b.reads;
    CHECK(lease.restore(b)&&b.calls.size()==call_count&&b.reads==read_count);
    // A new explicit clean acquisition snapshots a later generation/transform.
    ++b.current.key.generation;b.current.value.translation=bytes(52);const auto later=b.current;
    CHECK(lease.acquire(later.key,wanted,b)&&lease.restore(b)&&exact(b.current.value,later.value));
    CHECK(lease.reset()&&!lease.has_snapshot()&&!lease.faulted());
    // Translation is opaque binary32 storage, including signed-zero bits.
    b.current.value=original();b.current.value.translation={};b.current.value.translation[3]=0x80;
    const auto signed_zero=b.current.value;auto positive_zero=signed_zero;positive_zero.translation[3]=0;
    CHECK(lease.acquire(b.current.key,positive_zero,b)&&lease.dirty_mask()==2);
    CHECK(lease.restore(b)&&exact(b.current.value,signed_zero));
}
void no_op_and_unowned_foreign_fields(){
    Backend b;Lease noop;
    CHECK(noop.acquire(b.current.key,b.current.value,b)&&!noop.dirty()&&b.calls.empty());
    CHECK(noop.restore(b)&&b.calls.empty());
    Lease lease;auto wanted=original();wanted.translation=target().translation;
    CHECK(lease.acquire(b.current.key,wanted,b)&&lease.owned_mask()==2);
    b.current.value.absolute=foreign().absolute;
    CHECK(lease.restore(b)&&!lease.dirty()&&lease.faulted());
    CHECK(b.current.value.absolute==foreign().absolute&&b.current.value.translation==original().translation);
    CHECK(b.calls.size()==2&&b.calls[0].group==2&&b.calls[1].group==2);
    CHECK(lease.reset()&&!lease.faulted()); // Only an explicit drained lifecycle reset.
}
void unknown_preflight_never_writes(){
    for(unsigned kind=0;kind<4;++kind){
        Backend b;Lease lease;const auto expected=b.current.key;
        if(kind==0)b.failed_reads.insert(0);
        if(kind==1)b.current.known=false;
        if(kind==2)++b.current.key.generation; // Same address, different instance.
        if(kind==3)++b.current.key.action_epoch;
        CHECK(!lease.acquire(expected,target(),b)&&lease.faulted()&&!lease.dirty());
        CHECK(!lease.has_snapshot()&&b.calls.empty());
        CHECK(!lease.acquire(expected,target(),b)&&b.calls.empty());
        CHECK(lease.reset());
    }
}
void failed_apply_can_still_have_written(){
    for(int group=0;group<3;++group){
        for(const auto mode:{Backend::Mode::fail_before,Backend::Mode::fail_after,Backend::Mode::no_change}){
            Backend b;Lease lease;b.modes[group]=mode;
            CHECK(!lease.acquire(b.current.key,target(),b));
            CHECK(lease.faulted()&&!lease.dirty()&&exact(b.current.value,original()));
            const auto calls=b.calls.size();CHECK(!lease.acquire(b.current.key,target(),b)&&b.calls.size()==calls);
            CHECK(lease.restore(b)&&b.calls.size()==calls&&lease.faulted());
        }
    }
}
void torn_group_is_not_guessed_or_reacquired(){
    for(unsigned group=1,index=0;group<=4;group<<=1,++index){
        Backend b;Lease lease;b.modes[static_cast<int>(index)]=Backend::Mode::partial;
        CHECK(!lease.acquire(b.current.key,target(),b));
        CHECK(lease.dirty_mask()==group&&lease.revoked_mask()==group&&lease.owned_mask()==0&&lease.faulted());
        const auto partial=b.current.value;CHECK(!field(partial,original(),group)&&!field(partial,target(),group));
        for(unsigned other=1;other<=4;other<<=1)if(other!=group)CHECK(field(partial,original(),other));
        const auto calls=b.calls.size();CHECK(!lease.reset()&&!lease.restore(b)&&b.calls.size()==calls);
        assign(b.current.value,target(),group);
        CHECK(!lease.restore(b)&&b.calls.size()==calls&&lease.dirty());
        assign(b.current.value,original(),group); // External exact restoration, read only.
        CHECK(lease.restore(b)&&!lease.dirty()&&lease.faulted()&&b.calls.size()==calls);
        CHECK(lease.revoked_mask()==group&&lease.reset());
    }
}
void post_call_read_failure_revokes_remaining_writes(){
    for(int failed_group=0;failed_group<3;++failed_group){
        Backend b;Lease lease;b.after_write=[failed_group](Backend& live,int call){
            if(call==failed_group)live.failed_reads.insert(live.reads);
        };
        CHECK(!lease.acquire(b.current.key,target(),b));
        const auto pending=(1u<<static_cast<unsigned>(failed_group+1))-1u;
        CHECK(lease.dirty_mask()==pending&&lease.revoked_mask()==pending&&lease.owned_mask()==0);
        CHECK(lease.faulted()&&b.calls.size()==static_cast<unsigned>(failed_group+1));
        const auto calls=b.calls.size();CHECK(!lease.restore(b)&&b.calls.size()==calls);
        b.current.value=original();CHECK(lease.restore(b)&&!lease.dirty()&&lease.faulted()&&b.calls.size()==calls);
    }
}
void foreign_owned_group_does_not_block_other_cleanup(){
    for(unsigned group=1;group<=4;group<<=1){
        Backend b;Lease lease;CHECK(lease.acquire(b.current.key,target(),b));
        assign(b.current.value,foreign(),group);const auto changed=b.current.value;
        CHECK(!lease.restore(b)&&lease.dirty_mask()==group&&lease.revoked_mask()==group&&lease.owned_mask()==0);
        CHECK(field(b.current.value,changed,group));
        for(unsigned other=1;other<=4;other<<=1)if(other!=group)CHECK(field(b.current.value,original(),other));
        const auto calls=b.calls.size();assign(b.current.value,target(),group);
        CHECK(!lease.restore(b)&&b.calls.size()==calls); // Owned-looking ABA cannot reopen this group.
        assign(b.current.value,original(),group);CHECK(lease.restore(b)&&lease.faulted()&&b.calls.size()==calls);
    }
}
void identity_unknown_and_reused_address_never_mean_clean(){
    for(unsigned kind=0;kind<3;++kind){
        Backend b;Lease lease;const auto key=b.current.key;CHECK(lease.acquire(key,target(),b));
        if(kind==0)b.failed_reads.insert(b.reads);
        if(kind==1)b.current.known=false;
        if(kind==2)++b.current.key.generation;
        const auto calls=b.calls.size();CHECK(!lease.restore(b));
        CHECK(lease.dirty_mask()==7&&lease.revoked_mask()==7&&lease.owned_mask()==0&&lease.faulted());
        b.current.known=true;b.current.value=original();++b.current.key.generation;
        CHECK(!lease.restore(b)&&lease.dirty()&&b.calls.size()==calls);
        b.current.key=key;b.current.value=target();CHECK(!lease.restore(b)&&b.calls.size()==calls);
        b.current.value=original();CHECK(lease.restore(b)&&!lease.dirty()&&lease.faulted()&&b.calls.size()==calls);
    }
}
void failed_restore_preserves_ownership_or_revokes_a_torn_group(){
    for(unsigned group=1,index=0;group<=4;group<<=1,++index){
        for(const auto mode:{Backend::Mode::fail_before,Backend::Mode::fail_after,Backend::Mode::no_change,Backend::Mode::partial}){
            Backend b;Lease lease;CHECK(lease.acquire(b.current.key,target(),b));
            b.modes[3+static_cast<int>(index)]=mode;const bool clean=lease.restore(b);
            CHECK(lease.faulted());
            if(mode==Backend::Mode::fail_after){CHECK(clean&&!lease.dirty()&&exact(b.current.value,original()));continue;}
            CHECK(!clean&&lease.dirty_mask()==group);
            for(unsigned other=1;other<=4;other<<=1)if(other!=group)CHECK(field(b.current.value,original(),other));
            if(mode==Backend::Mode::partial){
                CHECK(lease.revoked_mask()==group&&lease.owned_mask()==0);
                const auto calls=b.calls.size();CHECK(!lease.restore(b)&&b.calls.size()==calls);
                assign(b.current.value,original(),group);CHECK(lease.restore(b)&&b.calls.size()==calls);
            }else{
                CHECK(lease.revoked_mask()==0&&lease.owned_mask()==group);
                CHECK(lease.restore(b)&&exact(b.current.value,original())); // Still authenticated, still owned.
            }
            CHECK(!lease.dirty()&&lease.faulted());
        }
    }
}
void backend_rechecks_expected_value_and_key_before_writing(){
    Backend b;Lease lease;
    b.before_write=[](Backend& live,int call){if(call==1)live.current.value.rotation=foreign().rotation;};
    CHECK(!lease.acquire(b.current.key,target(),b)&&b.rejected_expected==1);
    CHECK(!lease.dirty()&&lease.faulted()&&b.current.value.rotation==foreign().rotation);
    CHECK(b.current.value.absolute==original().absolute&&b.current.value.translation==original().translation);
    Backend reused;Lease other;const auto key=reused.current.key;
    reused.before_write=[](Backend& live,int call){if(call==2)++live.current.key.generation;};
    CHECK(!other.acquire(key,target(),reused)&&reused.rejected_expected==1);
    CHECK(other.dirty_mask()==7&&other.owned_mask()==0&&other.revoked_mask()==7);
    const auto calls=reused.calls.size();reused.current.value=original();CHECK(!other.restore(reused)&&reused.calls.size()==calls);
    reused.current.key=key;CHECK(other.restore(reused)&&other.faulted()&&reused.calls.size()==calls);
}
void earlier_group_returning_to_original_aborts_acquisition(){
    Backend b;Lease lease;
    b.after_write=[](Backend& live,int call){
        if(call==1)live.current.value.absolute=original().absolute;
    };
    CHECK(!lease.acquire(b.current.key,target(),b));
    CHECK(lease.faulted()&&!lease.dirty()&&exact(b.current.value,original()));
    CHECK(b.calls.size()==3&&b.calls[0].group==1&&b.calls[1].group==2&&b.calls[2].group==2);
}
void busy_and_reentrant_requests_cannot_replace_the_journal(){
    Backend b;Lease lease;const auto key=b.current.key;CHECK(lease.acquire(key,target(),b));
    auto other=key;++other.generation;const auto calls=b.calls.size();
    CHECK(!lease.acquire(other,foreign(),b)&&lease.key()==key&&b.calls.size()==calls&&lease.dirty_mask()==7);
    CHECK(lease.restore(b)&&lease.faulted()&&lease.reset());
    Backend nested;Lease recursive;bool attempted=false;
    nested.after_write=[&](Backend& live,int call){if(call==0){attempted=true;CHECK(!recursive.restore(live));}};
    CHECK(!recursive.acquire(nested.current.key,target(),nested)&&attempted);
    CHECK(recursive.dirty_mask()==1&&recursive.revoked_mask()==1&&recursive.owned_mask()==0);
    CHECK(nested.calls.size()==1&&!recursive.reset());
    nested.current.value=original();CHECK(recursive.restore(nested)&&recursive.faulted()&&nested.calls.size()==1);
}
void caller_aliases_are_snapshotted(){
    Backend b;Lease lease;auto wanted=target();const auto copy=wanted;
    b.before_read=[&](Backend&,int read){if(read==0)wanted=foreign();};
    CHECK(lease.acquire(b.current.key,wanted,b)&&exact(b.current.value,copy));
    CHECK(lease.restore(b));
    Backend changed;Lease other;
    changed.before_read=[](Backend& live,int read){if(read==0)++live.current.key.generation;};
    CHECK(!other.acquire(changed.current.key,target(),changed)&&changed.calls.empty()&&other.faulted());
}
void owned_value_getter_alias_survives_clean_reacquisition(){
    Backend b;Lease lease;
    CHECK(lease.acquire(b.current.key,target(),b)&&lease.restore(b));
    const auto desired=lease.owned_value();const auto next_original=foreign();
    CHECK(exact(desired,original())&&!exact(desired,next_original));
    b.current.value=next_original;
    CHECK(lease.acquire(b.current.key,lease.owned_value(),b));
    CHECK(exact(b.current.value,desired)&&exact(lease.owned_value(),desired));
    CHECK(exact(lease.original_value(),next_original)&&lease.dirty_mask()==7&&lease.owned_mask()==7&&!lease.faulted());
    CHECK(lease.restore(b)&&exact(b.current.value,next_original)&&!lease.dirty()&&!lease.faulted());
}
void original_value_getter_alias_survives_clean_reacquisition(){
    Backend b;Lease lease;
    CHECK(lease.acquire(b.current.key,target(),b)&&lease.restore(b));
    const auto desired=lease.original_value();const auto next_original=foreign();
    CHECK(exact(desired,original())&&!exact(desired,next_original));
    b.current.value=next_original;
    CHECK(lease.acquire(b.current.key,lease.original_value(),b));
    CHECK(exact(b.current.value,desired)&&exact(lease.owned_value(),desired));
    CHECK(exact(lease.original_value(),next_original)&&lease.dirty_mask()==7&&lease.owned_mask()==7&&!lease.faulted());
    CHECK(lease.restore(b)&&exact(b.current.value,next_original)&&!lease.dirty()&&!lease.faulted());
}
}
int main(){
    happy_path_and_exact_bytes();no_op_and_unowned_foreign_fields();unknown_preflight_never_writes();
    failed_apply_can_still_have_written();torn_group_is_not_guessed_or_reacquired();
    post_call_read_failure_revokes_remaining_writes();foreign_owned_group_does_not_block_other_cleanup();
    identity_unknown_and_reused_address_never_mean_clean();failed_restore_preserves_ownership_or_revokes_a_torn_group();
    backend_rechecks_expected_value_and_key_before_writing();busy_and_reentrant_requests_cannot_replace_the_journal();
    earlier_group_returning_to_original_aborts_acquisition();caller_aliases_are_snapshotted();
    owned_value_getter_alias_survives_clean_reacquisition();original_value_getter_alias_survives_clean_reacquisition();
    std::printf("weapon mesh component lease: %u CPU checks passed (partial calls, exact bytes, foreign fields, identity and cleanup)\n",checks);
}
