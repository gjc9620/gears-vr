#pragma once
// Owner-thread diagnostic only. No read, clock, lock or authorization is added.
namespace gears_cover_query_guard {
struct Witness {
    unsigned calls{},failed_call{},stage{},detail{},extra{};
    void enter()noexcept{++calls;}
    bool reject(unsigned where,unsigned why=0,unsigned context=0)noexcept{
        if(!failed_call){failed_call=calls;stage=where;detail=why;extra=context;}
        return false;
    }
};
enum class SessionStage:unsigned {none,stopped,physics_read,physics_match,collider_read,collider_match,
    component_read,component_match,query_source_match,asset_match,collider_source_match,component_source_match};
enum class OwnerStage:unsigned {none,session,captured,prepared,pending,entry,context_source,input_source,
    context,cover_read,cover_match,metadata_read,metadata_match,backend_metadata_match};
}
