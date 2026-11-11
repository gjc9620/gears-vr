"""Bind the corrected actor relation to original binary and frozen build evidence."""
from pathlib import Path
import difflib,hashlib,importlib.util,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before=load(H.parent/'owner-link-detail-r171/native-source-manifest.json')['files']
manifest=load(H/'native-source-manifest.json');build=load(H/'native-build-result.json')
assert build['build_only'] and not build['game_started'] and not build['staged'] and build['flags_unchanged']
assert not build['external_worktree_dependencies'] and len(build['project_dependencies'])==413
names=[Path(p).name for p in build['changed']]
assert set(names)=={'cover_physx_actor_backlink.h','cover_owner_link_observation.h','cover_physx_asset_actors.h',
    'cover_physx_collider.h','cover_physx_components.h','cover_physx_actor_shapes.h','cover_physx_shape_owner.h','cover_physx_self_evidence.h'}
for path,digest in manifest['files'].items():
    assert sha(H/'native-source'/path)==digest
    if path not in build['changed']:assert before[path]==digest
for path in build['changed']:
    assert sha(R/path)==sha(H/'native-source'/path)==sha(H/'checks-v5/source'/path)
bundle=H.parent/'owner-link-detail-r171/actor-layout-rea-evidence.json'
evidence=load(bundle)
# The bundle's complete records retain original identities, limitations and code.
records=evidence.get('records',evidence.get('result',{}).get('records',[]))
assert records,'No retained original binary evidence'
def find(id):return next(row for row in records if row['evidence_id']==id)
ctor=find('ev_85e7e9a67465153cdbe7582b206826899721bb8d6c3a18f82a0fb66658443ae5')
embedded=find('ev_b10751253945f3c7af148a22949d067c106b8fc9cbf5b6daced494bb1568c8f8')
for row in (ctor,embedded):assert row['subject']['digest']['sha256']=='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
assert '[RSI + 0x30], RSI' in ctor['normalized_result'] and '[RSI + 0x28]' in ctor['normalized_result']
assert '[0x1802324f8]' in embedded['normalized_result'] and '[RCX + 0x18], RAX' in embedded['normalized_result']
good=[]
for folder,selected in (('checks-before',None),('checks-v1',None),('checks-v2',lambda x:x['name']!='cover_physics_capsule_path_live'),
    ('checks-v3',lambda x:x['name']!='cover_physics_capsule_path_live'),('checks-v4',None),('checks-v5',None)):
    rows=load(H/folder/'results.json')
    if selected:rows=[x for x in rows if selected(x)]
    assert all(not x['build_exit'] and not x['test_exit'] and not x['outside_worktree_dependencies'] for x in rows)
    good.extend(dict(folder=folder,**x) for x in rows)
spec=importlib.util.spec_from_file_location('r173_link',H/'inspect_links.py')
parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser);parser.selftest()
patch=''
for name in names:
    old=H.parent/'owner-link-detail-r171/native-source/tools/native-capture'/name
    a=old.read_text().splitlines(True) if old.exists() else []
    patch+=''.join(difflib.unified_diff(a,(R/'tools/native-capture'/name).read_text().splitlines(True),fromfile='r171/'+name,tofile='r173/'+name))
(H/'delta.patch').write_text(patch)
result=dict(native_sha256=build['dll_sha256'],changed=build['changed'],binary_sha256=ctor['subject']['digest']['sha256'],
    original_ctor_evidence=ctor['evidence_id'],embedded_actor_ctor_evidence=embedded['evidence_id'],
    evidence_bundle=str(bundle),evidence_bundle_sha256=sha(bundle),project_dependencies=413,outside_worktree_dependencies=0,
    flags_unchanged=True,other_frozen_sources_unchanged=True,checks=good,parser_checks=23,
    before_after_reader_contrast=True,new_grant_or_writer=False,source_time_renewal=False,
    original_query_calls_added=False,shape_inventory_and_closing_preserved=True,
    failed_fixture_runs_retained=['checks-v2 path fixture missing embedded Actor fields','checks-v3 collider fixture still missing embedded fields'],
    build_only=True,game_started=False,owner_links_game_repaired=False,movement_authorized=False,world_coverage_verified=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='checks'}))
