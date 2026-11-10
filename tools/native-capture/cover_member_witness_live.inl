#ifndef GEARS_COVER_MEMBER_WITNESS_LIVE_INCLUDED
#define GEARS_COVER_MEMBER_WITNESS_LIVE_INCLUDED
// Include after weapon_hand_scene_binding. Only typed Cover readers own this
// storage; shared Proof capacity/layout and all writers remain unchanged.
namespace cover_member_witness_live {
namespace binding=weapon_hand_scene_binding;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
template<unsigned Capacity> struct Storage {
    static_assert(Capacity>0&&Capacity<=4096);
    struct Member {gears_anchor::Identity identity{};std::uintptr_t next{};};
    struct Chain {std::uintptr_t owner{},head{};unsigned first{},count{};};
    std::array<Member,Capacity> members{};
    std::array<unsigned,Capacity*2> index{};
    std::array<Chain,Capacity> chains{};
    unsigned count{},chain_count{};bool exhausted{};
    void clear()noexcept{count=chain_count=0;exhausted=false;index.fill(0);}
    // Reuse only a fully read chain in this sample. The caller still captures
    // the current head in its Proof; current() re-reads EVERY member at closing.
    bool match_chain(std::uintptr_t owner,std::uintptr_t head,gears_anchor::Identity const& selected,bool& present)const{
        present=false;
        for(unsigned c=0;c<chain_count;++c){auto const& chain=chains[c];if(chain.owner!=owner)continue;
            present=true;if(chain.head!=head||chain.first>count||chain.count>count-chain.first)return false;
            unsigned matches{};
            for(unsigned n=0;n<chain.count;++n){auto const& id=members[chain.first+n].identity;
                if(!std::wcscmp(id.name.data(),selected.name.data())){
                    if(++matches!=1||!binding::same(id,selected))return false;
                }
            }
            return matches==1;
        }
        return false;
    }
    bool finish_chain(std::uintptr_t owner,std::uintptr_t head,unsigned first,unsigned walked)noexcept{
        // Each first complete declaring-owner chain contributes unique rows.
        // A partial chain or shared member cannot become a cache certificate.
        if(!walked||first>count||count-first!=walked)return false;
        if(chain_count==chains.size()){exhausted=true;return false;}
        chains[chain_count++]={owner,head,first,walked};return true;
    }
    bool remember(gears_anchor::Identity const& identity,std::uintptr_t next)noexcept{
        auto slot=std::size_t((identity.address>>4)^(identity.address>>33))%index.size();
        for(unsigned probe=0;probe<index.size();++probe){
            const auto found=index[slot];
            if(found){auto const& member=members[found-1];
                if(member.identity.address==identity.address)return binding::same(member.identity,identity)&&member.next==next;
            }else{
                if(count==members.size()){exhausted=true;return false;}
                members[count]={identity,next};index[slot]=++count;return true;
            }
            slot=(slot+1)%index.size();
        }
        exhausted=true;return false;
    }
    bool link(gears_anchor::Identity const& identity,std::uintptr_t& next){
        std::uintptr_t again{};
        return binding::field(next,identity.address,0x68)&&
            binding::field(again,identity.address,0x68)&&next==again&&remember(identity,next);
    }
    bool current()const{
        if(exhausted)return false;
        for(unsigned n=0;n<count;++n){auto const& member=members[n];gears_anchor::Identity identity{};std::uintptr_t next{};
            if(!binding::identity(member.identity.address,identity)||!binding::same(member.identity,identity)||
               !binding::field(next,member.identity.address,0x68)||next!=member.next)return false;
        }
        return true;
    }
};
using Store=Storage<4096>;
#endif
}
#endif
