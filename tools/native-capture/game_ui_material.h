#pragma once
#include "ui_material_batch.h"
#include <limits>
namespace gears_game_material {
enum class Plan {reject,ordinary,hud};
inline Plan plan_kind(unsigned kind,bool hud_plane_enabled=true){
    return kind==0?Plan::ordinary:(kind==2||kind==3?(hud_plane_enabled?Plan::hud:Plan::ordinary):Plan::reject);
}
struct UploadCertificate {std::uintptr_t context{},native_context{};gears_ui_material::Quad quad{};};
inline bool upload_matches(UploadCertificate const& u,std::uintptr_t context,std::uintptr_t list,std::uintptr_t native_list,
                           unsigned stride,unsigned topology,gears_ui_material::Quad const& quad){
    return context&&u.context==context&&u.native_context&&list&&list==native_list&&stride==32&&topology==5&&
        !quad.index_data&&!u.quad.index_data&&quad.source&&quad.source==u.quad.source&&quad.vertices==u.quad.vertices;
}
// Caller holds the existing frame lock. A pending consumer blocks publication,
// without waiting for that consumer or using an unconfirmed draw as evidence.
class Completion {
    unsigned pending_{};bool broken_{};
public:
    bool begin(){if(broken_||pending_==std::numeric_limits<unsigned>::max()){broken_=true;return false;}++pending_;return true;}
    bool end(bool drew,bool complete){if(!pending_){broken_=true;return true;}--pending_;return drew&&!complete;}
    bool blocks_present()const{return broken_||pending_!=0;}
    unsigned pending()const{return pending_;}
};
}
