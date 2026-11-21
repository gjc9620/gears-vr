#define GEARS_TROIKA_AXES_FIXTURE_ONLY 1
#include "weapon_troika_control_axes_test.cpp"
namespace {
namespace troika_mounted_parts {
#include "weapon_troika_control_binding.inl"
}
namespace binding=troika_mounted_parts::control_binding;
struct BindingMemory:AxisMemory {
    struct Row {uptr head{};meta::RawName bone{};int draw{};unsigned pad{};};
    static_assert(sizeof(Row)==24);
    uptr anim_node{},anim_tree{},tree{},list_type{},array_type{},list_field{},index_field{},list_inner{},index_inner{};
    uptr bone_field{},head_field{},draw_field{},next_field{},animations{},dummy{};
    uptr skeletal_class{},asset{},asset_property{},parent_anim{},refs{},refs_inner{};
    std::array<Row,3> rows{};std::array<unsigned char,13> indices{};
    std::array<std::array<unsigned char,68>,13> references{};
    BindingMemory(){
        anim_node=cls(L"AnimNode",engine,object_class);anim_tree=cls(L"AnimTree",engine,anim_node);
        tree=node(L"OriginalTree",anim_tree,mesh);
        animations=property(mesh_class,L"Animations",object_type,0x130,8);put(animations,0xd0,anim_node);put(mesh,0x130,tree);
        array_type=cls(L"ArrayProperty",core);list_type=node(L"SkelControlListHead",cls(L"ScriptStruct",core),anim_tree);put(list_type,0x88,24u);
        bone_field=property(list_type,L"BoneName",name_class,8,8); // Deliberately not a hard-coded BoneName-first layout.
        head_field=property(list_type,L"ControlHead",object_type,0,8);put(head_field,0xd0,control_base);
        draw_field=property(list_type,L"DrawY",cls(L"IntProperty",core),16,4);
        list_field=property(anim_tree,L"SkelControlLists",array_type,0x100,16);
        list_inner=property(list_field,L"SkelControlLists",struct_class,0,24);put(list_field,0xd0,list_inner);put(list_inner,0xd0,list_type);
        index_field=property(mesh_class,L"SkelControlIndex",array_type,0x140,16);
        index_inner=property(index_field,L"SkelControlIndex",byte_class,0,1);put(index_field,0xd0,index_inner);
        skeletal_class=cls(L"SkeletalMesh",engine);const auto asset_package=node(L"Geist_TroikaCabal",package_type);
        asset=node(L"TroikaTurret",skeletal_class,asset_package);
        asset_property=property(mesh_class,L"SkeletalMesh",object_type,0x150,8);put(asset_property,0xd0,skeletal_class);put(mesh,0x150,asset);
        parent_anim=property(mesh_class,L"ParentAnimComponent",component_type,0x158,8);put(parent_anim,0xd0,mesh_class);put(mesh,0x158,uptr(0));
        refs=property(skeletal_class,L"RefSkeleton",array_type,0x100,16);
        refs_inner=property(refs,L"RefSkeleton",cls(L"IntProperty",core),0,4);put(refs,0xd0,refs_inner);
        const wchar_t* bone_names[]={L"b_troika_root",L"b_troika_gun_latitude",L"b_troika_gun_longitude",L"b_troika_spinner_L",
            L"b_troika_spinner_R",L"b_troika_port_R",L"b_troika_port_L",L"b_troika_muzzle_R",L"b_troika_muzzle_L",
            L"b_troika_handleroot",L"b_troika_handle_L",L"b_troika_handle_R",L"b_troika_playerref"};
        constexpr int parents[]={0,0,1,2,2,2,2,2,2,2,9,9,1};
        for(unsigned i=0;i<13;++i){const auto name=text(bone_names[i]);
            std::memcpy(references[i].data(),&name,8);const std::array<float,4> rotation={0,0,0,1};
            std::memcpy(references[i].data()+12,rotation.data(),16);std::memcpy(references[i].data()+56,&parents[i],4);}
        put(asset,0x100,meta::Table{reinterpret_cast<uptr>(references.data()),13,13});
        next_field=property(control_base,L"NextControl",object_type,0x180,8);put(next_field,0xd0,control_base);
        dummy=node(L"UnrelatedControl",control_base,mesh);put(dummy,0x100,mesh);put(dummy,0x180,uptr(0));
        put(latitude,0x180,uptr(0));put(longitude,0x180,uptr(0));
        rows[0]={dummy,text(L"b_troika_root"),10,0};
        rows[1]={longitude,text(L"b_troika_gun_longitude"),11,0};
        rows[2]={latitude,text(L"b_troika_gun_latitude"),12,0};
        indices.fill(255);indices[1]=2;indices[2]=1; // Original lists need not be in bone order.
        put(tree,0x100,meta::Table{reinterpret_cast<uptr>(rows.data()),3,3});
        put(mesh,0x140,meta::Table{reinterpret_cast<uptr>(indices.data()),13,13});tables();
    }
    bool configured(axes::Snapshot& config){return sample(config);}
    bool bound(axes::Snapshot const& config,binding::Snapshot& out,std::function<bool()> current=[](){return true;}){
        return binding::read(identity(turret),identity(mesh),config,current,out);
    }
};
}
#if !defined(GEARS_TROIKA_BINDING_REUSE_FIXTURE)
int main(){
    binding::Snapshot sentinel{};std::memset(&sentinel,0x59,sizeof sentinel);
    {
        auto m=std::make_unique<BindingMemory>();axes::Snapshot config{};CHECK(m->configured(config));
        auto out=sentinel;CHECK(m->bound(config,out));CHECK(out.tree.address==m->tree&&out.node_count==3&&out.stride==24);
        CHECK(out.list_index[0]==2&&out.list_index[1]==1&&out.bone_index[0]==1&&out.bone_index[1]==2);
        CHECK(out.index_values[1]==2&&out.index_values[2]==1);
        auto const first=out;CHECK(m->bound(config,out));CHECK(!std::memcmp(&out,&first,sizeof out));
    }
    {
        auto m=std::make_unique<BindingMemory>();axes::Snapshot config{};CHECK(m->configured(config));
        m->rows[0].head=0;m->rows[2].head=m->dummy;m->put(m->dummy,0x180,m->latitude);
        auto out=sentinel;CHECK(m->bound(config,out));CHECK(out.node_count==3&&out.list_index[0]==2); // A cached control may be inside a chain.
    }
    {
        auto m=std::make_unique<BindingMemory>();axes::Snapshot config{};CHECK(m->configured(config));
        m->rows[1].bone=m->text(L"b_troika_gun_latitude");m->rows[2].bone=m->text(L"b_troika_gun_longitude");
        m->indices[1]=1;m->indices[2]=2;auto out=sentinel;
        CHECK(m->bound(config,out)); // Actual list/index agreement wins over the control's name.
        CHECK(out.bone_index[0]==2&&out.bone_index[1]==1&&out.list_index[0]==2&&out.list_index[1]==1);
    }
    for(unsigned fault=0;fault<43;++fault){
        auto m=std::make_unique<BindingMemory>();axes::Snapshot config{};CHECK(m->configured(config));auto out=sentinel;
        if(fault==0)m->put(m->mesh,0x130,uptr(0));
        if(fault==1)m->put(m->mesh,0x130,m->latitude);
        if(fault==2)m->put(m->animations,0xd0,m->control_base);
        if(fault==3)m->put(m->tree,0x100,meta::Table{0,3,3});
        if(fault==4)m->put(m->tree,0x100,meta::Table{reinterpret_cast<uptr>(m->rows.data()),3,33});
        if(fault==5)m->put(m->list_inner,0xd0,m->vector_type);
        if(fault==6)m->put(m->list_inner,0x40,m->anim_tree);
        if(fault==7)m->put(m->list_inner,0x74,16u);
        if(fault==8)m->put(m->bone_field,0x94,0u);
        if(fault==9)m->put(m->head_field,0xd0,m->control_class);
        if(fault==10)m->put(m->draw_field,0x68,m->bone_field);
        if(fault==11)m->indices[1]=1;
        if(fault==12)m->indices[4]=2; // A second bone cannot drive latitude through the same list.
        if(fault==13)m->indices[4]=3;
        if(fault==14)m->put(m->mesh,0x140,meta::Table{reinterpret_cast<uptr>(m->indices.data()),12,13});
        if(fault==15)m->put(m->index_inner,0x50,m->object_type);
        if(fault==16)m->put(m->index_inner,0x74,4u);
        if(fault==17)m->rows[2].head=m->longitude;
        if(fault==18)m->rows[2].bone=m->text(L"b_troika_root");
        if(fault==19)m->rows[2].bone=m->text(L"b_troika_gun_longitude");
        if(fault==20)m->put(m->latitude,0x180,m->latitude);
        if(fault==21)m->put(m->latitude,0x180,m->dummy); // Cross-list sharing is ambiguous even without a cycle.
        if(fault==22)m->put(m->dummy,0x100,m->weapon);
        if(fault==23)m->put(m->next_field,0xd0,m->control_class);
        if(fault==24)m->put(m->turret,0x130,m->longitude);
        if(fault==25)m->rows[2].head=0;
        if(fault==26)m->rows[2].bone.number=1;
        if(fault==27)m->put(m->latitude,8,999u);
        if(fault==28)fail_read=reinterpret_cast<uptr>(&m->rows[2].head);
        if(fault==29)m->put(m->index_field,0xd0,m->list_inner);
        if(fault==30)m->put(m->list_type,0x40,m->control_class);
        if(fault==31)m->put(m->head_field,0x50,m->component_type);
        if(fault==32)m->put(m->asset,0x48,m->text(L"OtherAsset"));
        if(fault==33)m->put(m->mesh,0x158,m->mesh);
        if(fault==34)m->put(m->asset,0x100,meta::Table{reinterpret_cast<uptr>(m->references.data()),12,13});
        if(fault==35)m->put(m->refs_inner,0x50,m->byte_class);
        if(fault==36)m->put(m->refs_inner,0x74,68u); // Script dummy width is not native stride.
        if(fault==37){const int parent=0;std::memcpy(m->references[2].data()+56,&parent,4);}
        if(fault==38){const int parent=1;std::memcpy(m->references[9].data()+56,&parent,4);}
        if(fault==39){const auto wrong=m->text(L"WrongBone");std::memcpy(m->references[1].data(),&wrong,8);}
        if(fault==40){const float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(m->references[1].data()+28,&nan,4);}
        if(fault==41){const float nonunit=2;std::memcpy(m->references[1].data()+24,&nonunit,4);}
        if(fault==42)fail_read=reinterpret_cast<uptr>(m->references[12].data()+67);
        const auto accepted=m->bound(config,out);if(accepted)std::printf("unexpected accepted fault=%u\n",fault);
        CHECK(!accepted);CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    for(unsigned mode=0;mode<8;++mode){
        auto m=std::make_unique<BindingMemory>();axes::Snapshot config{};CHECK(m->configured(config));auto out=sentinel;unsigned calls{};
        CHECK(!m->bound(config,out,[&](){++calls;if(calls==2){
            if(mode==0)m->indices[1]=1;
            if(mode==1)m->rows[2].head=0;
            if(mode==2)m->rows[2].draw++;
            if(mode==3)m->put(m->latitude,0x180,m->dummy);
            if(mode==4)m->put(m->mesh,0x130,m->latitude);
            if(mode==5)m->put(m->turret,0x130,m->longitude);
            if(mode==6)m->put(m->head_field,0x94,8u);
            if(mode==7)return false;
        }return true;}));CHECK(calls==2);CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    {
        auto m=std::make_unique<BindingMemory>();axes::Snapshot config{};CHECK(m->configured(config));auto out=sentinel;
        mutate_read=reinterpret_cast<uptr>(&m->rows[2].head);mutation=[&](){m->rows[2].head=0;};
        CHECK(!m->bound(config,out));CHECK(!std::memcmp(&out,&sentinel,sizeof out));
        CHECK(!m->bound(config,out,[](){return false;}));CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    std::printf("Troika original active control-to-bone binding: %u checks passed; actual Reader on owned synthetic metadata/tree/index arrays, no engine/aim/fire proof.\n",checks);
}
#endif
