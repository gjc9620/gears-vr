#include "ui_observer.h"
#include "ui_runtime_binding.h"
#include <map>
#include <vector>
#include <cassert>
#include <cstdio>

struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    unsigned reads{};
    void block(std::uintptr_t address,std::size_t size){blocks[address].resize(size);}
    template<class T> void put(std::uintptr_t address,T const& value){
        for(auto& item:blocks)if(address>=item.first&&address-item.first<=item.second.size()&&sizeof(value)<=item.second.size()-(address-item.first)){
            std::memcpy(item.second.data()+address-item.first,&value,sizeof(value));return;}
        assert(false);
    }
    bool read(void* out,const void* source,std::size_t size){++reads;auto address=reinterpret_cast<std::uintptr_t>(source);
        for(auto const& item:blocks)if(address>=item.first&&address-item.first<=item.second.size()&&size<=item.second.size()-(address-item.first)){
            std::memcpy(out,item.second.data()+address-item.first,size);return true;}return false;
    }
};
struct Fixture {
    gears_ui::Config c;Memory m;std::uintptr_t frame=0x900000,canvas=0x910000,weapon=0x920000;
    unsigned class_id{},function_class{},hud_class{},inventory_class{},weapon_class{};
    Fixture(){
        c.base=0x10000000;c.objects_header=0x1000;c.names_header=0x1100;
        c.object_internal_index=8;c.object_name=16;c.object_class=32;c.object_outer=40;c.field_super=0x80;
        c.name_entry_index=0;c.name_entry_text=16;c.property_offset_internal=0xb8;
        m.block(c.objects_header,16);m.block(c.names_header,16);m.block(0x2000,1001*8);m.block(0x5000,1001*8);
        m.put(c.objects_header,gears_ui::Registry{0x2000,1001,1001});m.put(c.names_header,gears_ui::Registry{0x5000,1001,1001});
        class_id=add(L"Class",0);c.identities[class_id].cls=c.identities[class_id].address;rewrite(class_id);
        function_class=add(L"Function",class_id);hud_class=add(L"WarHUD",class_id);
        inventory_class=add(L"WarInventoryManager",class_id);weapon_class=add(L"WarWeapon",class_id);
        c.role_canvas_class=add(L"Canvas",class_id);
        c.role_controller=add(L"Controller",class_id);c.role_camera=add(L"Camera",class_id);c.role_pawn=add(L"Pawn",class_id);
        c.role_hud=add(L"HUD",hud_class);c.role_inventory=add(L"Inventory",inventory_class);
        unsigned offset=0x150;
#define UI_TEST_FIELD(name) c.name=offset;offset+=8;c.name##_property=add(L#name,class_id);m.put(c.identities[c.name##_property].address+c.property_offset_internal,c.name);
        GEARS_UI_FIELDS(UI_TEST_FIELD)
#undef UI_TEST_FIELD
        link(c.role_controller,c.controller_pawn,c.role_pawn);link(c.role_controller,c.controller_hud,c.role_hud);
        link(c.role_pawn,c.pawn_inventory,c.role_inventory);link(c.role_controller,c.player_camera,c.role_camera);
        link(c.role_camera,c.camera_owner,c.role_controller);link(c.role_hud,c.hud_owner,c.role_controller);link(c.role_pawn,c.pawn_controller,c.role_controller);
        for(unsigned i=0;i<4;++i){c.bindings[i].rva=gears_ui::binding_rvas[i];c.bindings[i].bytes=gears_ui::binding_bytes[i];
            m.block(c.base+c.bindings[i].rva,16);m.put(c.base+c.bindings[i].rva,c.bindings[i].bytes);
            if(i){auto id=add(i==1?L"DrawTile":i==2?L"DrawMaterialTile":L"DrawText",function_class,c.identities[c.role_canvas_class].address);
                c.bindings[i].function=id;m.put(c.identities[id].address+0x114,c.base+c.bindings[i].rva);}
        }
        c.scope_count=2;
        for(unsigned i=0;i<2;++i){auto owner=i?weapon_class:hud_class;auto function=add(i?L"DrawAdditionalCrosshair":L"DrawWeaponCrosshair",function_class,c.identities[owner].address);
            auto script=std::uintptr_t(0xa00000+i*0x100);m.block(script,32);c.scopes[i]={1,owner,function,script,32};
            m.put(c.identities[function].address+0x8c,gears_ui::Registry{script,32,32});m.put(c.identities[function].address+0x114,c.base+gears_ui::binding_rvas[0]);}
        m.block(frame,64);set_frame(0,c.identities[c.role_hud].address);
        dynamic(canvas,900,c.role_canvas_class);dynamic(weapon,901,weapon_class);m.put(weapon+c.weapon_instigator,c.identities[c.role_pawn].address);
        float clip=1920,cur=100;std::int32_t size=1920;
        m.put(canvas+c.canvas_clip_x,clip);m.put(canvas+c.canvas_clip_y,clip);
        m.put(canvas+c.canvas_cur_x,cur);m.put(canvas+c.canvas_cur_y,cur);m.put(canvas+c.canvas_org_x,cur);m.put(canvas+c.canvas_org_y,cur);
        m.put(canvas+c.canvas_size_x,size);m.put(canvas+c.canvas_size_y,size);
    }
    unsigned add(const wchar_t* name,unsigned cls,std::uintptr_t outer=0){
        unsigned id=c.identity_count++;auto& value=c.identities[id];value.address=0x100000+id*0x1000;value.index=id+1;value.name_index=id+1;
        value.cls=id?c.identities[cls].address:0;value.outer=outer;std::wcsncpy(value.name.data(),name,value.name.size()-1);
        m.block(value.address,1024);m.block(0x500000+id*0x200,256);rewrite(id);return id;
    }
    void rewrite(unsigned id){auto& value=c.identities[id];auto entry=std::uintptr_t(0x500000+id*0x200);
        m.put(value.address+c.object_internal_index,value.index);m.put(value.address+c.object_name,value.name_index);
        m.put(value.address+c.object_name+4,value.number);m.put(value.address+c.object_class,value.cls);m.put(value.address+c.object_outer,value.outer);
        m.put(0x2000+value.index*8,value.address);m.put(0x5000+value.name_index*8,entry);m.put(entry,value.name_index);m.put(entry+c.name_entry_text,value.name);
    }
    void link(unsigned role,unsigned offset,unsigned other){m.put(c.identities[role].address+offset,c.identities[other].address);}
    void dynamic(std::uintptr_t address,unsigned index,unsigned cls){m.block(address,1024);m.put(address+c.object_internal_index,index);
        m.put(address+c.object_name,c.identities[cls].name_index);m.put(address+c.object_class,c.identities[cls].address);m.put(0x2000+index*8,address);}
    void set_frame(unsigned scope,std::uintptr_t object){m.put(frame+0x18,c.identities[c.scopes[scope].function].address);m.put(frame+0x20,object);m.put(frame+0x28,c.scopes[scope].script);}
};
void test_canvas_ancestry(){
    Fixture f;auto read=[&](void* out,const void* address,std::size_t size){return f.m.read(out,address,size);};
    auto& root_function=f.c.identities[f.c.scopes[0].function];
    root_function.name={};std::wcscpy(root_function.name.data(),L"PostRender");
    f.rewrite(f.c.scopes[0].function);f.c.scopes[0].kind=3;
    gears_ui::ScopeContext root{};
    assert(gears_ui::match_scope(f.c,read,f.c.identities[f.c.role_hud].address,f.frame,root));
    auto set=[&](std::uintptr_t address,std::uintptr_t node,std::uintptr_t object,std::uintptr_t code,std::uintptr_t previous){
        f.m.block(address,64);f.m.put(address+0x18,node);f.m.put(address+0x20,object);
        f.m.put(address+0x28,code);f.m.put(address+0x38,previous);
    };
    const std::uintptr_t child=0x930000,helper=0x940000;
    auto reset=[&]{
        f.set_frame(0,root.object);f.m.put(f.frame+0x38,std::uintptr_t(0));
        set(child,f.c.identities[f.c.scopes[1].function].address,f.weapon,f.c.scopes[1].script+4,f.frame);
        set(helper,0xde00,f.canvas,0xdead,child);
    };
    reset();
    gears_ui::ScopeContext out{};std::uintptr_t matched{};unsigned depth=99;
    // Direct current caller is the root; no artificial child invocation.
    assert(gears_ui::classify_canvas_scope(f.c,read,root,f.frame,f.frame,out,matched));
    assert(out.kind==3&&out.function==root.function&&matched==f.frame);
    // Helper -> weapon scope -> HUD root: Frame.Object is the weapon, not Canvas.
    assert(gears_ui::classify_canvas_scope(f.c,read,root,f.frame,helper,out,matched,&depth));
    assert(out.scope_index==1&&out.object==f.weapon&&out.kind==1&&matched==child&&depth==1);
    assert(gears_ui::classify_canvas_scope(f.c,read,root,f.frame,child,out,matched)&&matched==child);
    f.m.put(helper+0x38,f.frame);
    assert(gears_ui::classify_canvas_scope(f.c,read,root,f.frame,helper,out,matched)&&matched==f.frame);
    // The root instruction pointer legitimately advances after VM entry.
    f.m.put(f.frame+0x28,f.c.scopes[0].script+8);
    assert(gears_ui::classify_canvas_scope(f.c,read,root,f.frame,helper,out,matched)&&out.code==f.c.scopes[0].script+8);
    auto reject=[&](gears_ui::ScopeContext context,std::uintptr_t start){
        out={99,98,97,96,95};matched=94;depth=93;
        assert(!gears_ui::classify_canvas_scope(f.c,read,context,f.frame,start,out,matched,&depth));
        assert(out.kind==99&&out.scope_index==98&&out.function==97&&out.object==96&&out.code==95&&matched==94&&depth==93);
    };
    // A recognized but invalid child blocks parent attribution.
    reset();f.m.put(child+0x20,f.canvas);reject(root,helper);
    reset();f.m.put(child+0x28,f.c.scopes[1].script+32);reject(root,helper);
    reset();auto child_function=f.c.identities[f.c.scopes[1].function].address;
    f.m.put(child_function+f.c.object_name,unsigned(999));reject(root,helper);f.rewrite(f.c.scopes[1].function);
    reset();f.m.put(child_function+0x8c,gears_ui::Registry{f.c.scopes[1].script+8,32,32});reject(root,helper);
    f.m.put(child_function+0x8c,gears_ui::Registry{f.c.scopes[1].script,32,32});
    reset();f.m.put(child+0x38,helper);reject(root,helper); // cycle
    reset();f.m.put(helper+0x38,std::uintptr_t(0));reject(root,helper); // missing root
    reset();f.m.put(helper+0x38,std::uintptr_t(0xdead00));reject(root,helper); // read failure
    reset();reject(root,helper+1); // unaligned frame
    reset();f.m.put(f.frame+0x20,f.weapon);reject(root,helper); // mutated root
    reset();f.m.put(f.frame+0x18,std::uintptr_t(0xde00));reject(root,helper);
    reset();f.m.put(f.frame+0x28,f.c.scopes[0].script+32);reject(root,helper);
    reset();auto forged=root;forged.object=f.weapon;reject(forged,helper);
    forged=root;forged.code=f.c.scopes[0].script+32;reject(forged,helper);
    // Fixed work bound, including the root, even when all helpers are unknown.
    reset();for(unsigned i=0;i<gears_ui::canvas_frame_limit;++i){auto a=std::uintptr_t(0x950000+i*0x100);
        set(a,0xde00,f.canvas,0xdead,i+1==gears_ui::canvas_frame_limit?f.frame:a+0x100);}
    reject(root,0x950000); // twelve helpers plus root exceed the limit
    f.m.reads=0;
    assert(gears_ui::classify_canvas_scope(f.c,read,root,f.frame,0x950100,out,matched)&&matched==f.frame);
    assert(f.m.reads<200); // Unknown frames add only two fixed-size reads each.
    out={99,98,97,96,95};matched=94;
    assert(!gears_ui::classify_canvas_scope(f.c,[](void*,const void*,std::size_t){return false;},root,f.frame,f.frame,out,matched));
    assert(out.kind==99&&out.function==97&&matched==94);
}
static void test_runtime_binding(){
    Fixture f;
    const auto pawn_class=f.add(L"WarPawn",f.class_id),camera_class=f.add(L"WarPlayerCamera",f.class_id),
        local_class=f.add(L"WarLocalPlayer",f.class_id),level_class=f.add(L"Level",f.class_id);
    for(auto cls:{pawn_class,camera_class,local_class,level_class}){f.c.identities[cls].check_super=1;f.rewrite(cls);}
    const auto level=f.add(L"PersistentLevel",level_class),local=f.add(L"LocalPlayer",local_class);
    f.c.identities[f.c.role_pawn].cls=f.c.identities[pawn_class].address;
    f.c.identities[f.c.role_pawn].outer=f.c.identities[level].address;f.rewrite(f.c.role_pawn);
    f.c.identities[f.c.role_camera].cls=f.c.identities[camera_class].address;f.rewrite(f.c.role_camera);
    f.c.identities[f.c.role_hud].outer=f.c.identities[f.c.role_controller].address;f.rewrite(f.c.role_hud);
    f.c.identities[f.c.role_inventory].outer=f.c.identities[f.c.role_pawn].address;f.rewrite(f.c.role_inventory);
    gears_anchor::Config a{};a.objects_header=f.c.objects_header;a.names_header=f.c.names_header;
    a.identity_count=f.c.identity_count;for(unsigned i=0;i<a.identity_count;++i)a.identities[i]=f.c.identities[i];
    a.object_internal_index=f.c.object_internal_index;a.object_name=f.c.object_name;a.object_class=f.c.object_class;a.object_outer=f.c.object_outer;
    a.field_super=f.c.field_super;a.name_entry_index=f.c.name_entry_index;a.name_entry_text=f.c.name_entry_text;
    a.role_controller=f.c.role_controller;a.role_pawn=f.c.role_pawn;a.role_camera=f.c.role_camera;a.role_local=local;a.role_level=level;
    a.controller_pawn=f.c.controller_pawn;a.pawn_controller=f.c.pawn_controller;a.player_camera=f.c.player_camera;a.pc_owner=f.c.camera_owner;
    a.player=0x2e0;a.camera_view_target=0x2e8;a.camera_timestamp=0x2f0;a.pawn_location=0x300;a.eye_height=0x310;a.base_eye_height=0x314;
    f.link(f.c.role_controller,a.player,local);f.link(f.c.role_camera,a.camera_view_target,f.c.role_pawn);
    f.m.put(f.c.identities[f.c.role_camera].address+a.camera_timestamp,1.f);
    const auto startup=f.c;auto read=[&](void* out,const void* address,std::size_t size){return f.m.read(out,address,size);};
    gears_ui_binding::Binding before{},after{};assert(gears_ui_binding::acquire(startup,a,read,before));
    gears_ui_binding::Receipts receipts;receipts.begin(7,before);
    assert(receipts.check(7,before)==gears_ui_binding::ReceiptStatus::pending);
    receipts.finish(7,true);assert(receipts.check(7,before)==gears_ui_binding::ReceiptStatus::current);
    const auto pawn=f.add(L"RespawnedPawn",pawn_class,f.c.identities[level].address);
    const auto inventory=f.add(L"RespawnedInventory",f.inventory_class,f.c.identities[pawn].address);
    const auto hud=f.add(L"RespawnedHUD",f.hud_class,f.c.identities[f.c.role_controller].address);
    f.link(f.c.role_controller,f.c.controller_pawn,pawn);f.link(pawn,f.c.pawn_controller,f.c.role_controller);
    f.link(pawn,f.c.pawn_inventory,inventory);f.link(f.c.role_controller,f.c.controller_hud,hud);
    f.link(hud,f.c.hud_owner,f.c.role_controller);f.link(f.c.role_camera,a.camera_view_target,pawn);
    assert(!gears_ui::quick_environment(startup,gears_ui::Reader<decltype(read)>{read}));
    assert(gears_ui_binding::acquire(startup,a,read,after)&&after.pawn.address==f.c.identities[pawn].address);
    assert(!gears_ui_binding::same(before,after));assert(receipts.check(7,after)==gears_ui_binding::ReceiptStatus::changed);
    const auto candidate=gears_ui_binding::config(startup,after);gears_ui::ScopeContext scope{};
    f.set_frame(0,after.hud.address);assert(gears_ui::match_scope(candidate,read,after.hud.address,f.frame,scope));
    f.set_frame(1,f.weapon);assert(!gears_ui::match_scope(candidate,read,f.weapon,f.frame,scope));
    f.m.put(f.weapon+f.c.weapon_instigator,after.pawn.address);assert(gears_ui::match_scope(candidate,read,f.weapon,f.frame,scope));
    assert(startup.identities[startup.role_pawn].address==before.pawn.address); // Immutable certificate.
    receipts.begin(8,after);receipts.finish(8,false);assert(receipts.check(8,after)==gears_ui_binding::ReceiptStatus::changed);
    receipts.begin(519,after);receipts.finish(7,true);assert(receipts.check(7,before)==gears_ui_binding::ReceiptStatus::retired);
    assert(receipts.check(519,after)==gears_ui_binding::ReceiptStatus::pending);
    receipts.retire_all();receipts.finish(519,true);
    assert(receipts.check(519,after)==gears_ui_binding::ReceiptStatus::retired);
    receipts.begin(520,after);receipts.finish(520,true);
    assert(receipts.check(520,after)==gears_ui_binding::ReceiptStatus::current);

    f.m.put(after.hud.address+f.c.hud_owner,std::uintptr_t(0xdead));assert(!gears_ui_binding::acquire(startup,a,read,before));
    f.link(hud,f.c.hud_owner,f.c.role_controller);
    f.m.put(after.inventory.address+f.c.object_class,f.c.identities[f.weapon_class].address);assert(!gears_ui_binding::acquire(startup,a,read,before));
    f.rewrite(inventory);f.m.put(f.c.identities[f.c.role_camera].address+a.camera_view_target,std::uintptr_t(0xdead));
    assert(!gears_ui_binding::acquire(startup,a,read,before));
    f.link(f.c.role_camera,a.camera_view_target,pawn);
    bool switched=false;auto changing=[&](void* out,const void* address,std::size_t size){
        const bool ok=f.m.read(out,address,size);
        if(!switched&&reinterpret_cast<std::uintptr_t>(address)==after.hud.address&&size==96){
            switched=true;f.m.put(f.c.identities[f.c.role_controller].address+f.c.controller_pawn,std::uintptr_t(0xdead));}
        return ok;
    };
    auto unchanged=after;
    assert(!gears_ui_binding::acquire(startup,a,changing,unchanged)&&switched&&gears_ui_binding::same(unchanged,after));
    f.link(f.c.role_controller,f.c.controller_pawn,pawn);
    f.m.put(after.inventory.address+f.c.object_outer,std::uintptr_t(0xdead));
    assert(!gears_ui_binding::acquire(startup,a,read,before));f.rewrite(inventory);
    auto mismatched=a;++mismatched.controller_pawn;assert(!gears_ui_binding::acquire(startup,mismatched,read,before));
    assert(gears_ui_binding::acquire(startup,a,read,before)&&gears_ui_binding::same(before,after));
}
int main(){
    test_runtime_binding();
    test_canvas_ancestry();
    Fixture f;auto read=[&](void* out,const void* address,std::size_t size){return f.m.read(out,address,size);};
    assert(gears_ui::validate(f.c,read));gears_ui::ScopeContext scope{};
    auto padded=f.c.identities[f.c.role_hud].name;padded[4]=L'X';padded[5]=L'Y';
    f.m.put(0x500000+f.c.role_hud*0x200+f.c.name_entry_text,padded);
    assert(gears_ui::validate(f.c,read)); // Bytes following the NUL are unrelated allocation padding.
    assert(gears_ui::match_scope(f.c,read,f.c.identities[f.c.role_hud].address,f.frame,scope));
    gears_ui::CanvasSample sample{};
    // Native wrapper frame belongs to the caller, not the Canvas native function.
    f.m.put(f.frame+0x18,std::uintptr_t(0xdead));f.m.put(f.frame+0x20,std::uintptr_t(0xbeef));f.m.put(f.frame+0x28,std::uintptr_t(0xcafe));
    assert(gears_ui::sample_canvas(f.c,read,scope,f.canvas,f.frame,sample)&&sample.frame_node==0xdead&&sample.frame_code==0xcafe);
    assert(!gears_ui::match_scope(f.c,read,scope.object,f.frame,scope)&&scope.function==sample.scope.function);
    f.set_frame(0,scope.object);f.m.put(f.frame+0x28,f.c.scopes[0].script+32);
    assert(!gears_ui::match_scope(f.c,read,scope.object,f.frame,scope));
    f.set_frame(1,f.weapon);gears_ui::ScopeContext nested{};assert(gears_ui::match_scope(f.c,read,f.weapon,f.frame,nested));
    auto subclass=f.add(L"Weap_AssaultRifle",f.class_id);f.c.supers[0]={subclass,f.c.identities[f.weapon_class].address};f.c.super_count=1;
    f.m.put(f.c.identities[subclass].address+f.c.field_super,f.c.identities[f.weapon_class].address);
    f.m.put(f.weapon+f.c.object_class,f.c.identities[subclass].address);
    assert(gears_ui::match_scope(f.c,read,f.weapon,f.frame,nested));
    f.m.put(f.c.identities[subclass].address+f.c.field_super,std::uintptr_t(0xdead));
    assert(!gears_ui::sample_canvas(f.c,read,nested,f.canvas,f.frame,sample));
    f.m.put(f.c.identities[subclass].address+f.c.field_super,f.c.identities[f.weapon_class].address);
    struct Restore {gears_ui::ScopeContext& tls;gears_ui::ScopeContext saved;~Restore(){tls=saved;}};
    auto parent=scope;try{Restore restore{scope,scope};scope=nested;throw 1;}catch(int){}
    assert(scope.function==parent.function&&scope.object==parent.object);
    f.m.put(f.weapon+f.c.weapon_instigator,std::uintptr_t(0xdead));
    assert(!gears_ui::sample_canvas(f.c,read,nested,f.canvas,f.frame,sample));
    f.set_frame(0,parent.object);f.m.put(f.canvas+f.c.canvas_cur_x,(std::numeric_limits<float>::quiet_NaN)());
    assert(!gears_ui::sample_canvas(f.c,read,parent,f.canvas,f.frame,sample));
    f.m.put(f.canvas+f.c.canvas_cur_x,100.f);assert(!gears_ui::sample_canvas(f.c,read,parent,0xdead,f.frame,sample));
    auto forged=parent;forged.code=f.c.scopes[0].script+32;assert(!gears_ui::sample_canvas(f.c,read,forged,f.canvas,f.frame,sample));
    f.m.put(f.canvas+f.c.canvas_size_x,std::int32_t(0));assert(!gears_ui::sample_canvas(f.c,read,parent,f.canvas,f.frame,sample));
    f.m.put(f.canvas+f.c.canvas_size_x,std::int32_t(1920));f.m.put(parent.object+f.c.hud_canvas,std::uintptr_t(0xdead));
    assert(!gears_ui::sample_canvas(f.c,read,parent,f.canvas,f.frame,sample));f.m.put(parent.object+f.c.hud_canvas,std::uintptr_t(0));
    auto hud=f.c.identities[f.c.role_hud].address;f.m.put(hud+f.c.object_name,unsigned(999));
    assert(!gears_ui::validate(f.c,read)&&!gears_ui::sample_canvas(f.c,read,parent,f.canvas,f.frame,sample));
    f.rewrite(f.c.role_hud);f.m.put(f.c.identities[f.c.role_controller].address+f.c.controller_hud,std::uintptr_t(0xdead));
    assert(!gears_ui::sample_canvas(f.c,read,parent,f.canvas,f.frame,sample));
    f.link(f.c.role_controller,f.c.controller_hud,f.c.role_hud);
    auto invalid=f.c;invalid.scope_count=9;assert(!gears_ui::validate(invalid,read));
    invalid=f.c;invalid.scopes[0].script=(std::numeric_limits<std::uintptr_t>::max)()-4;invalid.scopes[0].size=32;
    assert(!gears_ui::validate(invalid,read));
    invalid=f.c;invalid.canvas_cur_x++;assert(!gears_ui::validate(invalid,read));
    f.m.put(f.c.base+gears_ui::binding_rvas[0],std::array<unsigned char,16>{});
    assert(!gears_ui::validate(f.c,read)&&gears_ui::validate(f.c,read,false));
    assert(gears_ui::match_scope(f.c,read,parent.object,f.frame,scope));
    assert(gears_ui::sample_canvas(f.c,read,scope,f.canvas,f.frame,sample));
    assert(!gears_ui::sample_canvas(f.c,[](void*,const void*,std::size_t){return false;},scope,f.canvas,f.frame,sample));
    printf("UI observer: authenticated Canvas ancestry, registry/owner/script bounds, finite scalars and nested restore passed\n");
}
