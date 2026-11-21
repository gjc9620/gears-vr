#include "query_partial_fields.h"
#include <cwchar>
#include <cwctype>
#include <vector>
#include <cstdio>
#include <cstdlib>
using Address=unsigned;
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d\n",__LINE__);std::abort();}}while(false)
struct Node {Address parent{},child{},next{};wchar_t const* label=L"";bool readable=true;};
struct Reader {
    std::vector<Node> nodes{1};unsigned visits{};
    Address add(Node n){nodes.push_back(n);return unsigned(nodes.size()-1);}
    Address super(Address a){return nodes.at(a).parent;}
    Address children(Address a){return nodes.at(a).child;}
    Address next(Address a){return nodes.at(a).next;}
    bool name(Address a,wchar_t* text,std::size_t n){++visits;auto const& v=nodes.at(a);if(!v.readable||!n)return false;const auto length=(std::min)(std::wcslen(v.label),n-1);std::wmemcpy(text,v.label,length);text[length]=0;return true;}
    bool equal(wchar_t const* a,wchar_t const* b){while(*a&&*b){if(std::towlower(*a++)!=std::towlower(*b++))return false;}return *a==*b;}
};
Address scalar(Address cls,wchar_t const* label,Reader& r){
    for(unsigned depth=0;cls&&depth<40;++depth,cls=r.super(cls)){
        auto f=r.children(cls);for(unsigned n=0;f&&n<4096;++n,f=r.next(f)){
            wchar_t text[128]{};if(r.name(f,text,128)&&r.equal(text,label))return f;
        }
    }return 0;
}
auto compare(Address cls,Reader& r){
    constexpr wchar_t const* labels[]={L"Mesh",L"WeaponMesh"};
    r.visits=0;const auto batched=query_partial_fields::select(cls,labels,r);const auto visits=r.visits;
    r.visits=0;std::array<Address,2> old{scalar(cls,labels[0],r),scalar(cls,labels[1],r)};
    CHECK(batched==old);CHECK(visits<=r.visits);return batched;
}
int main(){
    Reader r;auto base=r.add({}),derived=r.add({base});
    auto bm=r.add({0,0,0,L"Mesh"}),wm=r.add({0,0,0,L"WeaponMesh"});r.nodes[bm].next=wm;r.nodes[base].child=bm;
    CHECK(compare(derived,r)[0]==bm); // Inherited links.
    auto shadow=r.add({0,0,0,L"mEsH"});r.nodes[derived].child=shadow;
    CHECK(compare(derived,r)[0]==shadow); // First derived match, case insensitive.
    auto duplicate=r.add({0,0,0,L"Mesh"});r.nodes[shadow].next=duplicate;
    CHECK(compare(derived,r)[0]==shadow); // First same-class match.
    r.nodes[shadow].next=0;
    r.nodes[base].child=wm;CHECK(compare(derived,r)[1]==wm);
    r.nodes[base].child=0;CHECK(compare(derived,r)[0]==shadow&&compare(derived,r)[1]==0);
    r.nodes[shadow].readable=false;CHECK(compare(derived,r)[0]==0); // Unreadable name skipped.
    CHECK(compare(0,r)[0]==0);
    Reader wide;auto c=wide.add({});Address first{},last{};
    for(unsigned i=0;i<4098;++i){auto f=wide.add({0,0,0,i==4095?L"Mesh":i==4096?L"WeaponMesh":L"Other"});if(last)wide.nodes[last].next=f;else first=f;last=f;}
    wide.nodes[c].child=first;auto bounded=compare(c,wide);CHECK(bounded[0]!=0&&bounded[1]==0);
    Reader deep;Address parent{};for(unsigned i=0;i<41;++i)parent=deep.add({parent});
    auto hidden=deep.add({0,0,0,L"Mesh"});deep.nodes[1].child=hidden;CHECK(compare(parent,deep)[0]==0);
    // Both selected near the tail: one traversal rather than two.
    wide.nodes[first+2000].label=L"Mesh";wide.nodes[first+3000].label=L"WeaponMesh";
    constexpr wchar_t const* labels[]={L"Mesh",L"WeaponMesh"};wide.visits=0;
    auto selected=query_partial_fields::select(c,labels,wide);const auto batch_visits=wide.visits;
    wide.visits=0;CHECK(selected[0]==scalar(c,L"Mesh",wide));CHECK(selected[1]==scalar(c,L"WeaponMesh",wide));
    CHECK(batch_visits==3001&&wide.visits==5002);
    std::printf("QueryPartialFieldsTest: %u checks; equivalent scalar selection, missing/shadow/base/unreadable/40x4096 bounds; 5002 -> 3001 name visits; no FPS claim\n",checks);
}
