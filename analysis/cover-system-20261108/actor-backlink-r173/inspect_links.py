"""Interpret copied owner-link values without granting geometry or movement."""
from pathlib import Path
import importlib.util,json
H=Path(__file__).resolve().parent
FIELDS=('observed','reader','phase','branch','thread','actor','table','expected_scene','observed_scene','internal','reverse','reverse_read')
def pointer(value,bytes):return value>0 and not value&7 and value+bytes-1<=2**64-1
def decode(status,key,code,started):
    observed_key=[int(x) for x in status['cover_path_probe_link_key'].split(',')]
    raw=[int(x) for x in status['cover_path_probe_link_failure'].split(',')]
    if observed_key!=key or len(key)!=5 or any(x<=0 for x in key):raise ValueError('Wrong query key')
    if len(raw)!=len(FIELDS) or any(x<0 or x>2**64-1 for x in raw):raise ValueError('Malformed link witness')
    if not raw[0]:
        if any(raw):raise ValueError('Unobserved row has payload')
        return dict(observed=False,exact_reader_unknown=True,missing_is_unknown=True)
    r=dict(zip(FIELDS,raw))
    if r['observed']!=1 or r['reader'] not in (1,2,3) or r['phase'] not in (1,2) or r['branch'] not in (1,3):raise ValueError('Invalid witness enum')
    if code!=12 or r['thread']!=key[-1] or (r['phase']==2 and not started):raise ValueError('Witness/query disagreement')
    if not pointer(r['actor'],0x48) or not pointer(r['table'],8) or not pointer(r['expected_scene'],0x278):raise ValueError('Invalid qualified address')
    scene_bad=r['expected_scene']!=r['observed_scene'];internal_bad=bool(r['internal']) and not pointer(r['internal'],0x18)
    if r['branch']==1:
        if r['reverse_read'] or r['reverse'] or not (scene_bad or internal_bad):raise ValueError('Invalid early relation failure')
    elif r['reverse_read']!=1 or scene_bad or internal_bad or r['reverse']==r['actor']:raise ValueError('Invalid reverse relation failure')
    return dict(observed=True,exact_reader_unknown=False,reader=('none','asset','collider','components')[r['reader']],
        phase=('none','before_dispatch','after_dispatch')[r['phase']],row=r,
        scene_mismatch=scene_bad,internal_invalid=internal_bad,
        reverse_mismatch=(r['reverse']!=r['actor']) if r['reverse_read'] else None,
        backlink_source='NpActor+0x30 embedded Actor owner' if r['branch']==3 else None,
        missing_is_unknown=True,scope='Already-read failure values only; not an ownership or movement grant')
def inspect(report,expected_native):
    spec=importlib.util.spec_from_file_location('r171_age',H.parent/'source-age-detail-r169/inspect_age.py')
    previous=importlib.util.module_from_spec(spec);spec.loader.exec_module(previous)
    result=previous.inspect(report,expected_native);q=result['timing']['query'];raw=q['raw']
    key=[int(raw[k]) for k in ('world','pawn','scene','compose','thread')]
    result['owner_links']=decode(report['native_status'],key,int(raw['code']),q['original_started'])
    return result
def selftest():
    key=[11,22,33,44,55];checks=0
    def packet(row):return {'cover_path_probe_link_key':','.join(map(str,key)),
        'cover_path_probe_link_failure':','.join(map(str,row))}
    blank=[0]*12;assert not decode(packet(blank),key,12,0)['observed'];checks+=1
    example=[1,1,1,1,55,0x160000,0x1802330a8,0x170000,0x180000,0x1a0000,0,0]
    for reader in (1,2,3):
        for branch in (1,3):
            row=example.copy();row[1]=reader;row[3]=branch
            if branch==3:row[8]=row[7];row[10]=0x190000;row[11]=1
            assert decode(packet(row),key,12,0)['reader']==('none','asset','collider','components')[reader];checks+=1
    internal=example.copy();internal[8]=internal[7];internal[9]=1
    assert decode(packet(internal),key,12,0)['internal_invalid'];checks+=1
    static=example.copy();static[3]=3;static[8]=static[7];static[9]=0;static[10]=0x190000;static[11]=1
    assert not decode(packet(static),key,12,0)['internal_invalid'];checks+=1
    bad=[]
    for at,value in ((0,2),(1,4),(2,0),(2,2),(3,0),(4,56),(5,0),(7,0),(8,0x170000),(10,1),(11,1)):
        row=example.copy();row[at]=value;bad.append(packet(row))
    bad.append(packet([0]+example[1:]));bad.append(dict(packet(example),cover_path_probe_link_key='11,22,33,45,55'))
    for value in bad:
        try:decode(value,key,12,0);raise AssertionError('Bad witness accepted')
        except ValueError:pass
        checks+=1
    try:decode(packet(example),key,11,0);raise AssertionError('Wrong return code accepted')
    except ValueError:pass
    checks+=1
    print(json.dumps(dict(cases=checks,failures=0,scope='Copied owner-link interpretation; no game')))
if __name__=='__main__':selftest()

