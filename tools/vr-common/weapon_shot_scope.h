#pragma once
#include "weapon_support_grip.h"
#include "../native-capture/ui_call_boundary.h"
#include <limits>

// Synchronous fire-invocation lifetime, independent of diagnostic row budgets,
// R2 edges, getter counts and the Native left-hand arbiter. No engine calls.
namespace gears_weapon_shot {
using gears_weapon_hand::ObjectIdentity;
using gears_weapon_hand::GripSnapshot;
enum class Role:std::uint32_t {opaque,war_instant,engine_instant,war_projectile,engine_projectile,shotgun_custom,shotgun_pellets};
struct Binding {
    // Function, declaring class, package, Core.Function, Core.Class.
    std::array<ObjectIdentity,5> identities{};
    std::uint64_t script{},script_hash{},exec{};
    std::uint32_t bytes{},capacity{},flags{};
    std::uint16_t native_index{},reserved{};
    Role role{};
    bool valid() const noexcept {
        if(role==Role::opaque||unsigned(role)>unsigned(Role::shotgun_pellets)||reserved||native_index||
           !script||!script_hash||!exec||!bytes||bytes>capacity||capacity>131072||script>UINT64_MAX-bytes)return false;
        for(auto const& id:identities)if(!gears_weapon_hand::identity_valid(id))return false;
        return identities[0].outer==identities[1].address&&identities[1].outer==identities[2].address&&
            !identities[2].outer&&identities[0].klass==identities[3].address&&identities[1].klass==identities[4].address;
    }
};
inline bool same(Binding const& a,Binding const& b) noexcept {
    for(std::size_t i=0;i<a.identities.size();++i)if(!gears_weapon_hand::same(a.identities[i],b.identities[i]))return false;
    return a.script==b.script&&a.script_hash==b.script_hash&&a.exec==b.exec&&a.bytes==b.bytes&&a.capacity==b.capacity&&
        a.flags==b.flags&&a.native_index==b.native_index&&a.reserved==b.reserved&&a.role==b.role;
}
struct Shot {
    std::uint64_t id{};ObjectIdentity weapon{};GripSnapshot grip{};Role root{};
};
struct Layer {Shot shot{};Binding binding{};std::uint64_t caller{};bool masked{true};};
struct Token {
    std::uint64_t call{},previous_call{};std::uint32_t previous_depth{},entered{};
};
struct Packet {
    std::uint64_t call{},shot{};ObjectIdentity weapon{};GripSnapshot grip{};Role root{};
    ObjectIdentity mode_function{};std::uint64_t script_hash{};Role mode{};
};
inline bool same_grip(GripSnapshot const& a,GripSnapshot const& b) noexcept {
    using gears_weapon_hand::same;
    return a.grip==b.grip&&same(a.key,b.key)&&same(a.world,b.world)&&same(a.controller,b.controller)&&
        same(a.pawn,b.pawn)&&same(a.mesh,b.mesh)&&same(a.weapon,b.weapon)&&
        a.input_session==b.input_session&&a.input_sequence==b.input_sequence&&a.space_epoch==b.space_epoch&&
        a.profile_generation==b.profile_generation&&a.input_epoch==b.input_epoch&&a.reference_generation==b.reference_generation&&
        a.mount_revision==b.mount_revision&&a.press_token==b.press_token&&same(a.left_hand,b.left_hand)&&
        a.committed_ns==b.committed_ns&&a.stereo_received_ns==b.stereo_received_ns&&a.input_received_ns==b.input_received_ns;
}
static_assert(std::is_trivially_copyable_v<Token>&&std::is_standard_layout_v<Token>);
static_assert(std::is_trivially_copyable_v<Packet>);
inline bool script_pc(Binding const& binding,std::uintptr_t pc) noexcept {
    return binding.valid()&&pc>=binding.script&&pc-binding.script<binding.bytes;
}
class Stack {
    static constexpr unsigned limit=12;
    std::array<Layer,limit> layers_{};
    std::uint64_t next_call_{1},next_shot_{1},call_{};
    std::uint32_t depth_{};bool poisoned_{};
    Token push() noexcept {
        if(poisoned_||next_call_==(std::numeric_limits<std::uint64_t>::max)()||depth_==UINT32_MAX){
            poisoned_=true;return {};
        }
        Token token{next_call_++,call_,depth_,1};call_=token.call;++depth_;
        if(depth_<=limit)layers_[depth_-1]={};
        return token;
    }
    Layer const* active() const noexcept {
        return !poisoned_&&depth_&&depth_<=limit&&!layers_[depth_-1].masked?&layers_[depth_-1]:nullptr;
    }
public:
    unsigned depth() const noexcept {return depth_;}
    bool poisoned() const noexcept {return poisoned_;}
    Token mask() noexcept {return push();}
    template<class Read>bool aliases(Binding const& binding,ObjectIdentity const& weapon,
                                    std::uintptr_t caller,Read read) const noexcept {
        auto const* parent=active();if(!parent||!binding.valid()||!gears_weapon_hand::same(parent->shot.weapon,weapon))return false;
        const bool pair=(parent->binding.role==Role::war_instant&&binding.role==Role::engine_instant)||
            (parent->binding.role==Role::shotgun_custom&&binding.role==Role::shotgun_pellets);
        if(!pair)return false;
        gears_ui_call_boundary::Frame frame{};
        return caller&&gears_ui_call_boundary::frame(read,caller,frame)&&frame.node==parent->binding.identities[0].address&&
            frame.object==weapon.address&&frame.previous==parent->caller&&script_pc(parent->binding,frame.code);
    }
    template<class Read>Token enter(Binding const& binding,ObjectIdentity const& weapon,
                                   std::uintptr_t caller,GripSnapshot const& frozen,std::int64_t now_ns,Read read) noexcept {
        const auto* previous=active();const bool alias=aliases(binding,weapon,caller,read);
        Shot inherited{};if(alias)inherited=previous->shot;
        const auto token=push();if(!token.entered||depth_>limit)return token;
        if(!binding.valid()||!gears_weapon_hand::identity_valid(weapon)||!caller||
           (binding.role==Role::shotgun_pellets&&!alias))return token;
        auto& layer=layers_[depth_-1];layer.binding=binding;layer.caller=caller;
        if(alias)layer.shot=inherited;
        else{
            if(next_shot_==(std::numeric_limits<std::uint64_t>::max)()){poisoned_=true;return token;}
            layer.shot={next_shot_++,weapon,{},binding.role};
            if(frozen.current(frozen.key,weapon,now_ns)&&gears_weapon_hand::identity_valid(frozen.world)&&
               gears_weapon_hand::identity_valid(frozen.controller)&&gears_weapon_hand::identity_valid(frozen.pawn)&&
               gears_weapon_hand::identity_valid(frozen.mesh))layer.shot.grip=frozen;
        }
        layer.masked=false;return token;
    }
    void leave(Token const& token) noexcept {
        if(!token.entered)return;
        if(!depth_||call_!=token.call||depth_!=std::uint64_t(token.previous_depth)+1){poisoned_=true;return;}
        if(depth_<=limit)layers_[depth_-1]={};
        depth_=token.previous_depth;call_=token.previous_call;
    }
    // The incoming caller must reach the actual currently executing mode's
    // body before reaching its entry caller. RDX itself is never its new body.
    template<class Read>bool packet(ObjectIdentity const& weapon,std::uintptr_t caller,Read read,Packet& out) const noexcept {
        out={};auto const* layer=active();
        if(!layer||!gears_weapon_hand::same(layer->shot.weapon,weapon)||
           (layer->shot.root!=Role::war_instant&&layer->shot.root!=Role::engine_instant&&layer->shot.root!=Role::shotgun_custom))return false;
        std::array<std::uintptr_t,gears_ui_call_boundary::frame_limit> seen{};
        for(unsigned depth=0;depth<seen.size();++depth){
            if(!caller||caller==layer->caller)return false;
            for(unsigned i=0;i<depth;++i)if(seen[i]==caller)return false;
            seen[depth]=caller;gears_ui_call_boundary::Frame frame{};
            if(!gears_ui_call_boundary::frame(read,caller,frame))return false;
            if(frame.node==layer->binding.identities[0].address&&frame.object==weapon.address&&
               frame.previous==layer->caller&&script_pc(layer->binding,frame.code)){
                out={call_,layer->shot.id,weapon,layer->shot.grip,layer->shot.root,
                    layer->binding.identities[0],layer->binding.script_hash,layer->binding.role};return true;
            }
            caller=frame.previous;
        }
        return false;
    }
    bool current(Packet const& packet) const noexcept {
        auto const* layer=active();
        return layer&&packet.call==call_&&packet.shot==layer->shot.id&&
            gears_weapon_hand::same(packet.weapon,layer->shot.weapon)&&packet.root==layer->shot.root&&
            same_grip(packet.grip,layer->shot.grip)&&gears_weapon_hand::same(packet.mode_function,layer->binding.identities[0])&&
            packet.script_hash==layer->binding.script_hash&&packet.mode==layer->binding.role;
    }
    bool binding(Binding& out,ObjectIdentity& weapon) const noexcept {
        auto const* layer=active();if(!layer)return false;
        out=layer->binding;weapon=layer->shot.weapon;return true;
    }
    // Only after enclosing PG hook admission is stopped and callbacks drained.
    // Counters deliberately persist; diagnostics never reset shot identity.
    bool drained() const noexcept {return !depth_;}
};
} // namespace gears_weapon_shot
