#pragma once
#include "graphics_lock.h"

// Independent route admission; no hooks, native writer or thread discovery.
namespace gears_graphics_lock::routes {
inline constexpr std::uintptr_t menu_return_rva=0x28047a,direct_commit_return_rva=0x2ad008;
inline constexpr std::array<std::uintptr_t,2> collector_return_rvas{{0x1eedbf,0x1f9a9b}};
struct Call {std::uint32_t thread{};std::uintptr_t return_rva{};};
struct QueuedProof {bool verified{};std::uint32_t thread{};std::uintptr_t return_rva{};};
struct Context {gears_graphics_lock::Context policy{};std::array<QueuedProof,8> queued{};};
enum class Role {menu,collector,direct_commit,queued_commit};
enum class Status {disabled,admitted,other_mode,unknown_caller,thread_missing,no_collector_scope,
                   scope_thread_mismatch,scope_poisoned,queued_unverified,policy_fault};
struct Decision {
    Status status{Status::disabled};Role role{};Result result{};Call call{};std::uintptr_t settings{};
    bool protected_now()const{return status==Status::admitted&&result.protected_now();}
};
inline Decision reject(Role role,Call call,Status status,Reason reason) {
    return {status,role,{reason},call,0};
}
inline gears_graphics_lock::Context settings_context(Context const& context,Call call) {
    auto copy=context.policy;
    // Entry/callsite identity admits this boundary. The launcher's thread or
    // old single owner/consumer fields are not an engine thread authority.
    copy.owner_thread=call.thread;copy.owner_thread_verified=call.thread!=0;return copy;
}
template<class Reader> Decision menu_before(Context const& context,std::uintptr_t settings,
    std::int32_t mode,Call call,Reader& read) {
    if(!context.policy.enabled)return {};
    if(mode!=dlss_menu_mode)return reject(Role::menu,call,Status::other_mode,Reason::other_mode);
    if(call.return_rva!=menu_return_rva)return reject(Role::menu,call,Status::unknown_caller,Reason::identity_unverified);
    if(!call.thread)return reject(Role::menu,call,Status::thread_missing,Reason::thread_unverified);
    auto copy=settings_context(context,call);auto result=menu(copy,settings,mode,call.thread,read);
    return {result.protected_now()?Status::admitted:Status::policy_fault,Role::menu,result,call,settings};
}
template<class Reader,class CompareExchange> Decision collector_before(Context const& context,
    std::uintptr_t settings,Call call,Reader& read,CompareExchange& cas) {
    if(!context.policy.enabled)return {};
    bool known=false;for(auto rva:collector_return_rvas)if(call.return_rva==rva)known=true;
    if(!known)return reject(Role::collector,call,Status::unknown_caller,Reason::identity_unverified);
    if(!call.thread)return reject(Role::collector,call,Status::thread_missing,Reason::thread_unverified);
    auto copy=settings_context(context,call);
    auto result=gears_graphics_lock::collector_before(copy,settings,call.thread,read,cas);
    return {result.protected_now()?Status::admitted:Status::policy_fault,Role::collector,result,call,settings};
}

// At most eight nested collector frames; no heap allocation. Rejected nested
// collectors mask outer scopes too, so a failed child cannot borrow admission.
class CollectorScope {
    struct Frame {std::uint64_t token{};std::uintptr_t settings{};std::uint32_t thread{};bool live{},admitted{};};
    struct Stack {std::array<Frame,8> frames{};std::size_t depth{};std::uint64_t next{};bool poisoned{},exhausted{};};
    static Stack& current(){static thread_local Stack stack;return stack;}
    Stack* owner_{};std::size_t slot_{};std::uint64_t token_{};
public:
    explicit CollectorScope(Decision const& decision) {
        auto& stack=current();owner_=&stack;slot_=stack.depth++;
        if(stack.next==std::numeric_limits<std::uint64_t>::max()) {stack.exhausted=stack.poisoned=true;return;}
        token_=++stack.next;
        if(slot_>=stack.frames.size()){stack.poisoned=true;return;}
        stack.frames[slot_]={token_,decision.settings,decision.call.thread,true,
            decision.role==Role::collector&&decision.protected_now()};
    }
    CollectorScope(CollectorScope const&)=delete;CollectorScope& operator=(CollectorScope const&)=delete;
    ~CollectorScope() {
        // A hook-local scope must be destroyed on its constructing OS thread.
        // Foreign threads cannot inherit the TLS stack or authorize direct work.
        if(owner_!=&current())return;
        auto& stack=*owner_;
        if(stack.depth!=slot_+1)stack.poisoned=true;
        if(slot_<stack.frames.size()) {
            auto& frame=stack.frames[slot_];
            if(frame.token!=token_){stack.poisoned=true;return;}
            frame.live=false;
        }else if(stack.depth==slot_+1)--stack.depth;
        while(stack.depth&&stack.depth<=stack.frames.size()&&!stack.frames[stack.depth-1].live)--stack.depth;
        if(!stack.depth&&!stack.exhausted)stack.poisoned=false;
    }
    static Status admit_direct(std::uintptr_t settings,Call call) {
        auto const& stack=current();
        if(stack.poisoned||stack.exhausted||stack.depth>stack.frames.size())return Status::scope_poisoned;
        if(!stack.depth)return Status::no_collector_scope;
        auto const& frame=stack.frames[stack.depth-1];
        if(!frame.live||!frame.admitted||frame.settings!=settings)return Status::no_collector_scope;
        if(frame.thread!=call.thread)return Status::scope_thread_mismatch;
        return Status::admitted;
    }
};

template<class Reader,class CompareExchange> Decision commit_before(Context const& context,
    std::uintptr_t command,Call call,Reader& read,CompareExchange& cas) {
    if(!context.policy.enabled)return {};
    const auto role=call.return_rva==direct_commit_return_rva?Role::direct_commit:Role::queued_commit;
    if(!call.thread)return reject(role,call,Status::thread_missing,Reason::thread_unverified);
    if(role==Role::direct_commit) {
        const auto status=CollectorScope::admit_direct(context.policy.admission.singleton,call);
        if(status!=Status::admitted)return reject(role,call,status,Reason::thread_unverified);
    }else {
        bool known=false;
        for(auto const& proof:context.queued)if(proof.verified&&proof.thread&&proof.return_rva&&
            proof.return_rva!=direct_commit_return_rva&&proof.thread==call.thread&&proof.return_rva==call.return_rva)known=true;
        if(!known)return reject(role,call,Status::queued_unverified,Reason::thread_unverified);
    }
    auto copy=context.policy;copy.consumer_thread=call.thread;copy.consumer_thread_verified=true;
    auto result=gears_graphics_lock::commit_before(copy,command,call.thread,read,cas);
    return {result.protected_now()?Status::admitted:Status::policy_fault,role,result,call,0};
}

template<class Reader,class Original> auto invoke_menu(Context const& context,std::uintptr_t settings,
    std::int32_t mode,Call call,Reader& read,Original&& original,Decision& decision)
    -> decltype(original(settings,mode)) {
    using Return=decltype(original(settings,mode));static_assert(std::is_integral<Return>::value,"Preserve menu success EAX ABI");
    decision=menu_before(context,settings,mode,call,read);
    if(decision.result.bypass_original)return static_cast<Return>(menu_success_eax);
    return std::forward<Original>(original)(settings,mode);
}
template<class Reader,class CompareExchange,class Original> decltype(auto) invoke_collector(Context const& context,
    std::uintptr_t settings,Call call,Reader& read,CompareExchange& cas,Original&& original,Decision& decision) {
    decision=collector_before(context,settings,call,read,cas);CollectorScope scope(decision);
    return std::forward<Original>(original)(settings);
}
template<class Reader,class CompareExchange,class Original> decltype(auto) invoke_commit(Context const& context,
    std::uintptr_t command,Call call,Reader& read,CompareExchange& cas,Original&& original,Decision& decision) {
    decision=commit_before(context,command,call,read,cas);
    return std::forward<Original>(original)(command);
}
} // namespace gears_graphics_lock::routes
