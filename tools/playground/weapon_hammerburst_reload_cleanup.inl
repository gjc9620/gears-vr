// Included inside hammerburst_runtime. Resolve existing dispatched accounting
// only; cleanup must never initiate a new perfect-reward call after stop/switch.
namespace cleanup_completion {
bool settle(inv::Transaction& owned_transaction,inv::ObservedInventory& owned_observer,
    inv::Inventory const& opening_inventory,lancerreload_state::RewardReceipt& reward_receipt) noexcept {
    inv::Request request{};
    if(!owned_transaction.dispatched_request(request))return true;
    inv::Inventory counts{};
    if(!owned_observer.sample_owned(request.identity,counts))return false;
    bool applied=false;
    auto result=gears_weapon::ReloadTxnResult::none;
    if(request.call==gears_weapon::ReloadTxnCall::insert){
        if(inv::counts_equal(counts,request.expected))applied=true;
        else if(!inv::counts_equal(counts,request.before))return false;
    }else if(request.result==gears_weapon::ReloadTxnResult::perfect){
        if(!reward_receipt.dispatched&&!reward_receipt.applied){
            // Insertion is already committed in `before`. Reject only the
            // uncalled reward/finish receipt, never subtract those rounds.
            if(!inv::counts_equal(counts,request.before))return false;
        }else{
            // A call already dispatched must drain its existing hand-back,
            // exact bonus correction and observations, without redispatch.
            if(!lancerreload_state::perfect_reward(request,opening_inventory,owned_observer,reward_receipt)||
               !owned_observer.sample_owned(request.identity,counts)||!inv::counts_equal(counts,request.expected))return false;
            applied=true;result=gears_weapon::ReloadTxnResult::perfect;
        }
    }else{
        if(!inv::counts_equal(counts,request.expected))return false;
        applied=true;
        if(request.result!=gears_weapon::ReloadTxnResult::inspection)result=request.result;
    }
    return owned_transaction.acknowledge({request.token,applied,counts,result});
}
}
