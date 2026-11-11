#ifndef GEARS_COVER_STRUCT_FIELD_LIVE_INCLUDED
#define GEARS_COVER_STRUCT_FIELD_LIVE_INCLUDED
// Include after weapon_hand_scene_binding. Caller owns current metadata/GT
// bracket and binding mutex. Extends cover readers to ScriptStruct fields;
// does not change the shared Class-only property selector or grant authority.
namespace cover_struct_field_live {
namespace b=weapon_hand_scene_binding;
inline bool select(b::Proof& proof,gears_anchor::Identity const& structure,unsigned expected_extent,
                   wchar_t const* name,wchar_t const* kind,unsigned width,unsigned offset,
                   wchar_t const* reference,b::Property& output){
    if(!name||!kind||!width||!expected_extent||expected_extent>0x10000||width>expected_extent||offset>expected_extent-width||!b::metadata_current())return false;
    gears_anchor::Identity owner{};unsigned extent{};std::uintptr_t child{};
    if(!b::certified(proof,structure.address,owner)||!b::same(owner,structure)||owner.number||
       !b::kind(proof,owner,L"ScriptStruct")||!b::source_field(proof,extent,owner.address,0x88)||extent!=expected_extent||
       !b::source_field(proof,child,owner.address,0x80))return false;
    std::array<std::uintptr_t,256> visited{};unsigned count{},matches{};b::Property selected{};
    while(child&&count<visited.size()){
      for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
      gears_anchor::Identity id{};if(!b::certified(proof,child,id)||id.outer!=owner.address)return false;
      if(!std::wcscmp(id.name.data(),name)){
        if(++matches!=1||id.number||!b::kind(proof,id,kind))return false;
        selected.id=id;selected.owner=owner;
        if(!b::source_field(proof,selected.dimension,child,0x70)||selected.dimension!=1||
           !b::source_field(proof,selected.width,child,0x74)||selected.width!=width||
           !b::source_field(proof,selected.offset,child,0x94)||selected.offset!=offset)return false;
        if(reference){std::uintptr_t target{};unsigned target_extent{};
          if(!b::source_field(proof,target,child,0xd0)||!b::certified(proof,target,selected.reference)||
             selected.reference.number||!b::named(selected.reference,reference)||!b::kind(proof,selected.reference,L"ScriptStruct")||
             !b::source_field(proof,target_extent,target,0x88)||target_extent!=width)return false;}
      }
      // Complete membership, including siblings AFTER the selected property.
      if(!b::source_field(proof,child,child,0x68))return false;
    }
    if(child||matches!=1||!b::current(proof)||!b::metadata_current())return false;
    output=selected;return true;
}
}
#endif
