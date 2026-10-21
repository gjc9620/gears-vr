// Included after UiMaterialContext. Independent of every owned target/ledger.
#include "game_ui_material.h"
namespace game_material {
using Upload=uintptr_t(__fastcall*)(void*,unsigned,unsigned,void*,unsigned);
Upload original_upload{};void* upload_target{};
gears_game_material::Completion completion;
std::atomic<uint64_t> consumers{},completed{},uploads{},draws{},ordinary_draws{},hud_draws{},rejected{},failed{},pending_presents{};
struct DrawFailure {unsigned reason{},kind{},width{},height{},pair_rejection{},plane_rejection{};uint64_t ticket{};uintptr_t target{};};
std::array<std::atomic<uint64_t>,9> reject_reasons{};
std::array<DrawFailure,9> first_rejects{}; // Draw writes and print reads under frame_hook_mutex.
struct ConsumerScope;
thread_local ConsumerScope* current{};
thread_local gears_game_material::UploadCertificate* uploading{};
struct ConsumerScope {
    UiMaterialContext* context{};ConsumerScope* previous{};bool entered{},drew{},bad{},ended{};
    explicit ConsumerScope(UiMaterialContext* c):context(c),previous(current){
        if(c&&recording.load()&&frame_link_diagnostics&&stereo_gpu_publish){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
            entered=completion.begin();if(entered){current=this;++consumers;}}
    }
    void end(bool complete){if(!entered||ended)return;
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(completion.end(drew,complete&&!bad))frame_link.invalidate();
        if(complete&&!bad)++completed;else ++failed;ended=true;
    }
    ~ConsumerScope(){end(false);current=previous;}
};
bool block_present(){if(completion.blocks_present()){++pending_presents;return true;}return false;}
uintptr_t __fastcall hook_upload(void* native_context,unsigned primitive,unsigned primitives,void* vertices,unsigned stride){
    if(stereo_publish_bypass)return original_upload(native_context,primitive,primitives,vertices,stride);
    Scope scope;if(!scope.world())return original_upload(native_context,primitive,primitives,vertices,stride);
    auto* previous=uploading;uploading=nullptr;gears_game_material::UploadCertificate certificate{};
    if(current&&current->entered&&!current->bad&&current->context==ui_material_context&&recording.load()){
        auto& c=*ui_material_context;
        if(gears_game_material::plan_kind(c.consumer.receipt.kind)!=gears_game_material::Plan::reject&&
           primitive==1&&primitives==2&&stride==32&&
           gears_ui_material::capture_quad({base},safe_copy,c.consumer,certificate.quad)&&
           !certificate.quad.index_data&&certificate.quad.source==reinterpret_cast<uintptr_t>(vertices)){
            certificate.context=reinterpret_cast<uintptr_t>(&c);certificate.native_context=reinterpret_cast<uintptr_t>(native_context);
            uploading=&certificate;++uploads;
        }else {current->bad=true;++rejected;++reject_reasons[0];}
    }
    struct Restore {gears_game_material::UploadCertificate* previous;~Restore(){uploading=previous;}} restore{previous};
    return original_upload(native_context,primitive,primitives,vertices,stride); // Exactly once.
}
bool upload_hash_matches(){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,0x2fb> bytes{};
    unsigned char digest[32]{};char hex[65]{};
    bool ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+0x1af054),bytes.size())&&
        BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,bytes.data(),ULONG(bytes.size()),0)>=0&&BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    return ok&&!strcmp(hex,"4cf2c5b249b2abb2decb11cae8adf0eacc3a08833345691e63fcbaf7eae7c41a");
}
bool reserve(){
    if(!upload_hash_matches())return false;
    upload_target=reinterpret_cast<void*>(base+0x1af054);
    return MH_CreateHook(upload_target,reinterpret_cast<void*>(hook_upload),reinterpret_cast<void**>(&original_upload))==MH_OK&&upload_hash_matches();
}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"game_ui_material enabled=1 consumers=%llu completed=%llu uploads=%llu draws=%llu ordinary_draws=%llu hud_draws=%llu rejected=%llu failed=%llu pending_presents=%llu pending=%u kind0_semantic_hud_proven=0 original_consumer_once=1\n",
    consumers.load(),completed.load(),uploads.load(),draws.load(),ordinary_draws.load(),hud_draws.load(),rejected.load(),failed.load(),pending_presents.load(),completion.pending());
    const char* names[]={"upload","upload_kind","quad_list","om_target","pair","view_plane","lookup","plan","registration"};
    for(unsigned i=0;i<reject_reasons.size();++i)if(reject_reasons[i].load()){
        const auto& d=first_rejects[i];
        fprintf(file,"game_ui_material_reject reason=%s count=%llu first_recorded=%u kind=%u target=%llx size=%u,%u ticket=%llu pair_reject=%u plane_reject=%u\n",
            names[i],reject_reasons[i].load(),d.reason?1:0,d.kind,static_cast<unsigned long long>(d.target),d.width,d.height,d.ticket,d.pair_rejection,d.plane_rejection);
    }
}
}
