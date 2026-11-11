"""Make old synthetic actor fixtures model the authenticated embedded Actor."""
from pathlib import Path
import re
R=Path(__file__).resolve().parents[3];D=R/'tools/native-capture'
p=D/'cover_physx_query_test.cpp';s=p.read_text()
anchor='    bool read(void* out,void const* address,std::size_t n){'
assert s.count(anchor)==1
s=s.replace(anchor,'    void actor_owner(std::uintptr_t at,std::uintptr_t image){put<std::uintptr_t>(at+0x28,image+0x2324f8);put<std::uintptr_t>(at+0x30,at);}\n'+anchor)
p.write_text(s)
names=('query','asset_actors','collider','components','actor_attachments','shape_owner')
for name in names:
    p=D/('cover_physx_'+name+'_test.cpp');s=p.read_text()
    s,count=re.subn(r'(\b(?:m|memory)\.put<std::uintptr_t>\(([^,]+),base\+0x2330a8\);)',
        lambda m:m[1]+m[1].split('.')[0]+'.actor_owner('+m[2]+',base);',s)
    assert count,name
    p.write_text(s)
replacements={
 'asset_actors':[("extra_actor+0x40,0)","extra_actor+0x40,1)"),("extra_internal+0x10,scope.actor)","extra_actor+0x30,scope.actor)")],
 'collider':[("0x160040,0)","0x160040,1)"),("0x1c0010,0x1d0000)","0x160030,0x1d0000)"),("0x1e0010,0x160000)","0x1d0030,0x160000)")],
 'components':[("0x210010,0x160000)","0x1f0030,0x160000)")],
 'component_assets':[("0x210010,0x160000)","0x1f0030,0x160000)")],
 'actor_attachments':[("0x690010,0x160000)","0x680030,0x160000)")],
 'shape_owner':[(">(internal_actor_at+0x10,other_actor_at)",">(0x160030,other_actor_at)"),("(a)==internal_actor_at+0x10)","(a)==0x160030)")]
}
for name,pairs in replacements.items():
    p=D/('cover_physx_'+name+'_test.cpp');s=p.read_text()
    for old,new in pairs:assert s.count(old)==1,(name,old);s=s.replace(old,new)
    p.write_text(s)
print('Updated explicit owner setup and corresponding ownership-corruption stimuli; no checks removed.')
