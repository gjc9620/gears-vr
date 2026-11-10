"""Interpret cause bits only from a matched actual prepare rejection."""
from pathlib import Path
import importlib.util,json,sys
H=Path(__file__).resolve().parent
def decode(text,compose,thread):
    v=[int(x) for x in text.split(',')]
    if len(v)!=7 or any(x<0 for x in v):raise ValueError('Malformed source cause witness')
    observed,entry,actual_compose,actual_thread,stage,verdict,causes=v
    if not observed:
        if any(v):raise ValueError('Unobserved source cause has payload')
        return dict(observed=False,body_source_age=None,equipment_age=None)
    if (observed!=1 or not entry or actual_compose!=compose or actual_thread!=thread or
            not compose or not thread or stage!=0 or verdict!=3 or causes not in (1,2,3)):
        raise ValueError('Source cause key/stage/verdict mismatch')
    return dict(observed=True,entry=entry,compose=compose,thread=thread,stage=stage,verdict=verdict,causes=causes,
        body_source_age=bool(causes&1),equipment_age=bool(causes&2))
def inspect(report):
    native=json.loads((H/'native-build-result.json').read_text())['dll_sha256']
    spec=importlib.util.spec_from_file_location('cause_guard',H.parent/'guard-witness-r137/inspect_guard_probe.py')
    base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
    result=base.inspect(report,native);raw=result['raw']
    cause=decode(raw['source_age_cause'],int(raw['compose']),int(raw['thread']))
    if cause['observed']:
        owner=result['owner_guard']
        if owner['name']!='context' or owner.get('context_verdict')!='source_age_expired' or owner.get('context_detail')!=6:
            raise ValueError('Cause is not paired with the actual context detail6 refusal')
    result['source_age_cause']=cause
    return result
def selftest():
    count=0
    assert decode('0,0,0,0,0,0,0',42,7)['body_source_age'] is None;count+=1
    row=[1,4096,42,7,0,3,1]
    for causes in (1,2,3):
        good=row.copy();good[-1]=causes;parsed=decode(','.join(map(str,good)),42,7)
        assert parsed['body_source_age']==bool(causes&1) and parsed['equipment_age']==bool(causes&2);count+=1
    bad_rows=[]
    for at,value in ((0,0),(0,2),(1,0),(2,43),(3,8),(4,1),(5,2),(6,0),(6,4)):
        bad=row.copy();bad[at]=value;bad_rows.append(bad)
    bad_rows.extend((row[:-1],row+[0]))
    for bad in bad_rows:
        try:decode(','.join(map(str,bad)),42,7);raise AssertionError('Unmatched cause accepted')
        except ValueError:pass
        count+=1
    print(json.dumps(dict(cases=count,failures=0,scope='Cause interpretation only; no runtime')))
if __name__=='__main__':
    if sys.argv[1:]==['--selftest']:selftest()
    else:print(json.dumps(inspect(json.loads(Path(sys.argv[1]).read_text(encoding='utf-8-sig'))),indent=2))
