#pragma once
#include <windows.h>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include "../vr-common/weapon_hand_client.h"
#include "../vr-common/weapon_equipment_model.h"
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
#include "../vr-common/weapon_call_boundary.h"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
#include "../vr-common/weapon_action_gate.h"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
#include "../vr-common/weapon_boundary_admission.h"
#endif
#ifdef NDEBUG
#error WeaponHandClientOfflineTest requires active assertions
#endif
namespace {
using uptr=std::uintptr_t;
uptr base{},fail_read{},mutate_read{};HMODULE module{};
std::atomic<DWORD> game_thread{};std::atomic<bool> enabled{true};
std::function<void()> mutation;
bool safe_copy(void* out,uptr from,std::size_t bytes) noexcept {
    if(!from||from==fail_read)return false;
    __try{std::memcpy(out,reinterpret_cast<void const*>(from),bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool copy(void* out,uptr from,std::size_t bytes){
    const auto result=safe_copy(out,from,bytes);
    if(result&&mutate_read==from){mutate_read=0;auto callback=std::move(mutation);callback();}
    return result;
}
namespace playground {uptr arena_pawn{};}
namespace vrequip {gears_weapon::WeaponEquipmentModel model;}
#include "weapon_grip_damage_metadata.inl"
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
namespace weaponcall {
gears_weapon_hand::Reply attach(void*) noexcept;
bool detached() noexcept;
bool local_quiescent() noexcept;
}
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
namespace weaponaction {
gears_weapon_hand::Reply attach(void*) noexcept;
bool detached() noexcept;
bool local_quiescent() noexcept;
}
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_BOUNDARY_TRIAL)
namespace weaponboundary {
gears_weapon_hand::Reply attach(void*) noexcept;
void seal() noexcept;
bool detached() noexcept;
bool local_quiescent() noexcept;
}
#endif
#include "weapon_hand_client.inl"
namespace meta=gripdamage_metadata;
namespace abi=gears_weapon_hand;
// Owned local memory only. These are artificial reflected objects; actual
// production copy/Reader/adapter/latched equipment model operate on them.
struct alignas(16) Block {std::array<unsigned char,512> bytes{};};
struct Memory {
    void* reservation{};
    std::array<Block,512> nodes{},entries{};
    std::array<uptr,4096> objects{},names{};
    std::array<std::wstring,512> labels{};
    unsigned node_count{},name_count{};
    uptr core{},warfare{},engine{},class_type{},package_type{},object_type{},component_type{},bool_type{};
    uptr actor_class{},pawn_class{},pc_class{},weapon_class{},world_class{},mesh_class{},concrete{};
    uptr world{},pc{},pawn{},mesh{},weapon{},switch_property{},mesh_property{};
    gears_weapon::EquipmentSnapshot equipment{};
    abi::Frame frame{};
    template<class T>static void put(uptr address,std::size_t offset,T const& value){
        std::memcpy(reinterpret_cast<void*>(address+offset),&value,sizeof(value));
    }
    template<class T>static T get(uptr address,std::size_t offset){
        T result{};std::memcpy(&result,reinterpret_cast<void const*>(address+offset),sizeof(result));return result;
    }
    meta::RawName text(std::wstring const& label){
        for(unsigned i=0;i<name_count;++i)if(labels[i]==label)return {1000+i,0};
        assert(name_count<entries.size()&&label.size()<200);const auto slot=name_count++;
        labels[slot]=label;const auto index=1000+slot;
        const auto address=reinterpret_cast<uptr>(entries[slot].bytes.data());
        put(address,0,index);std::memcpy(reinterpret_cast<void*>(address+20),label.c_str(),(label.size()+1)*sizeof(wchar_t));
        names[index]=address;return {index,0};
    }
    uptr node(std::wstring const& label,uptr type=0,uptr outer=0){
        assert(node_count<nodes.size());const auto slot=node_count++;
        const auto address=reinterpret_cast<uptr>(nodes[slot].bytes.data());const auto index=1001+slot;
        put(address,8,index);put(address,0x40,outer);put(address,0x48,text(label));
        put(address,0x50,type?type:(class_type?class_type:address));put(address,0x88,512u);objects[index]=address;return address;
    }
    uptr cls(wchar_t const* label,uptr package,uptr parent=0){auto p=node(label,class_type,package);put(p,0x60,parent);return p;}
    uptr property(uptr owner,wchar_t const* label,uptr type,unsigned offset,unsigned width,unsigned mask=0){
        const auto p=node(label,type,owner);put(p,0x70,1u);put(p,0x74,width);put(p,0x94,offset);
        if(mask)put(p,0xd0,mask);
        auto previous=get<uptr>(owner,0x80);
        if(!previous)put(owner,0x80,p);
        else {while(get<uptr>(previous,0x68))previous=get<uptr>(previous,0x68);put(previous,0x68,p);}
        return p;
    }
    abi::ObjectIdentity identity(uptr p){auto name=get<meta::RawName>(p,0x48);
        return {p,get<uptr>(p,0x40),get<uptr>(p,0x50),get<unsigned>(p,8),name.index,name.number,1};}
    void tables(){
        put(base,0x1c24730,meta::Table{reinterpret_cast<uptr>(objects.data()),int(1001+node_count),int(objects.size())});
        put(base,0x1c34800,meta::Table{reinterpret_cast<uptr>(names.data()),int(1000+name_count),int(names.size())});
    }
    explicit Memory(bool grenade=false){
        reservation=VirtualAlloc(nullptr,0x1c40000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(reservation);
        base=reinterpret_cast<uptr>(reservation);game_thread.store(GetCurrentThreadId());enabled.store(true);
        core=node(L"Core");class_type=node(L"Class",0,core);put(class_type,0x50,class_type);
        package_type=node(L"Package",class_type,core);put(core,0x50,package_type);
        warfare=node(L"WarfareGame",package_type);engine=node(L"Engine",package_type);
        object_type=node(L"ObjectProperty",class_type,core);bool_type=node(L"BoolProperty",class_type,core);
        component_type=node(L"ComponentProperty",class_type,core);
        actor_class=cls(L"Actor",engine);world_class=cls(L"WorldInfo",engine,actor_class);
        pc_class=cls(L"WarPC",warfare,actor_class);pawn_class=cls(L"WarPawn",warfare,actor_class);
        mesh_class=cls(L"SkeletalMeshComponent",engine);weapon_class=cls(L"WarWeapon",warfare,actor_class);
        concrete=grenade?cls(L"Weap_FragGrenade",warfare,cls(L"Weap_Grenade",warfare,weapon_class)):
            cls(L"Weap_AssaultRifle",warfare,weapon_class);
        property(actor_class,L"WorldInfo",object_type,0x100,8);
        property(actor_class,L"bDeleteMe",bool_type,0x108,4,4);
        property(pc_class,L"Pawn",object_type,0x110,8);
        property(pawn_class,L"Controller",object_type,0x110,8);mesh_property=property(pawn_class,L"Mesh",component_type,0x118,8);
        property(pawn_class,L"Weapon",object_type,0x120,8);
        switch_property=property(pawn_class,L"bSwitchingWeapons",bool_type,0x128,4,1);
        property(pawn_class,L"bNoWeaponFiring",bool_type,0x128,4,2);
        property(weapon_class,L"Instigator",object_type,0x110,8);
        world=node(L"LiveWorld",world_class,engine);pc=node(L"LivePC",pc_class,world);
        pawn=node(L"LivePawn",pawn_class,world);mesh=node(L"LiveMesh",mesh_class,pawn);
        weapon=node(L"LiveWeapon",concrete,pawn);
        put(pc,0x100,world);put(pc,0x110,pawn);put(pawn,0x110,pc);put(pawn,0x118,mesh);put(pawn,0x120,weapon);
        put(weapon,0x110,pawn);tables();
        frame.world=identity(world);frame.controller=identity(pc);frame.pawn=identity(pawn);frame.mesh=identity(mesh);frame.weapon=identity(weapon);
        gears_weapon::EquipmentSnapshot game{};game.world=world;game.pawn=pawn;game.current_weapon=weapon;game.can_switch=true;
        const auto slot=grenade?1u:2u;game.weapon_in_slot[slot]=weapon;game.ammo_known[slot]=game.ammo_available[slot]=true;
        game.throwable_count_known=true;game.throwable_count=5;
        vrequip::model.reset();vrequip::model.observe(game);
        const auto decision=vrequip::model.tap(grenade?gears_weapon::BodySlot::grenade:gears_weapon::BodySlot::weapon_2,game);
        assert(decision.event==gears_weapon::EquipmentEvent::request_unholster);
        equipment=game;weaponhand::equipment_hold_committed(pc,game);
        assert(weaponhand::held_binding.known);
    }
    ~Memory(){mutation={};mutate_read=fail_read=0;weaponhand::held_binding={};vrequip::model.reset();base=0;VirtualFree(reservation,0,MEM_RELEASE);}
    bool read(abi::GripGameState& out){return weaponhand::observe(nullptr,frame,weaponhand::clock_ns(nullptr),out);}
};
}
