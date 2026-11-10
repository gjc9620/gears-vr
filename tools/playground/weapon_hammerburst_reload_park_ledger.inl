// Included inside hammerburst_runtime. Private ledger preparation only:
// contains no hand lease, input sample, native call or animation permission.
// The live adapter must restore/stow its actual display and original owner
// before publishing this plan, then authenticate a new held source to rebind.
namespace park_ledger {
struct Plan {
    inv::Transaction transaction;
    Stage resume_stage{Stage::idle};
    bool original_at_feed{},valid{};
};
bool prepare(Stage phase,inv::Transaction const& source,std::uint64_t original_item,
    std::uint64_t replacement_item,bool extraction_complete,Plan& output) noexcept {
    if(source.pending()||!source.inventory().valid()||!original_item)return false;
    Plan candidate{};candidate.transaction=source;
    auto& ledger=candidate.transaction;
    const auto before=source.inventory();
    if(phase==Stage::acquiring||phase==Stage::opening||phase==Stage::opened||
       (phase==Stage::old_magazine&&!extraction_complete)){
        // The untouched/partly pulled original must be restored at the feed
        // by the live adapter. Keep its stable item ID and original opening
        // intent; no new grab or physical extraction is inferred here.
        if(!ledger.rebind_retained_original(before.identity,before,original_item))return false;
        candidate.original_at_feed=true;
        candidate.resume_stage=phase==Stage::acquiring?Stage::opening:
            phase==Stage::old_magazine?Stage::opened:phase;
    }else{
        if(phase==Stage::old_magazine){
            if(!extraction_complete||!ledger.stow_original(original_item))return false;
            candidate.resume_stage=Stage::waiting_ammo;
        }else if(phase==Stage::new_magazine){
            if(!replacement_item||!ledger.release_item(replacement_item))return false;
            candidate.resume_stage=Stage::waiting_ammo;
        }else if(phase==Stage::waiting_ammo)candidate.resume_stage=phase;
        else if(phase==Stage::closing){
            if(!ledger.feed_present())return false;
            candidate.resume_stage=phase;
        }else return false;
        if(!ledger.interrupt()||!ledger.rebind_suspended(before.identity,before))return false;
    }
    const auto after=ledger.inventory();
    if(!inv::counts_equal(before,after)||before.revision!=after.revision||ledger.pending()||ledger.reserved())return false;
    candidate.valid=true;output=candidate;return true;
}
bool rebind(Plan const& saved,inv::Inventory const& observed,std::uint64_t original_item,Plan& output) noexcept {
    if(!saved.valid||!observed.valid()||
       !inv::same_instance_across_hold(saved.transaction.inventory().identity,observed.identity))return false;
    Plan candidate=saved;
    const auto previous=candidate.transaction.inventory().identity;
    const bool accepted=candidate.original_at_feed?
        candidate.transaction.rebind_retained_original(previous,observed,original_item):
        candidate.transaction.rebind_suspended(previous,observed);
    if(!accepted)return false;
    output=candidate;return true;
}
}
