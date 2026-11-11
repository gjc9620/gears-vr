"""Decode the two first-refusal witnesses against this frozen Native only."""
from pathlib import Path
import importlib.util,json,sys
H=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('cover_path_inspect',H.parent/'path-probe-r134/inspect_path_probe.py')
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
OWNER=('none','session','captured','prepared','pending','entry','context_source','input_source','context','cover_read','cover_match','metadata_read','metadata_match','backend_metadata_match')
SESSION=('none','stopped','physics_read','physics_match','collider_read','collider_match','component_read','component_match','query_source_match','asset_match','collider_source_match','component_source_match','owner_begin','owner_end')
def witness(value,names):
    row=[int(x) for x in value.split(',')]
    if len(row)!=5 or any(x<0 for x in row):raise ValueError('Malformed guard witness')
    calls,failed,stage,detail,extra=row
    if failed>calls or stage>=len(names) or bool(failed)!=bool(stage):raise ValueError('Inconsistent guard witness')
    return dict(calls=calls,failed_call=failed,stage=stage,name=names[stage],detail=detail,extra=extra)
def inspect(report,expected_native=None):
    native=expected_native or json.loads((H/'native-build-result.json').read_text())['dll_sha256']
    result=base.inspect(report,native)
    for key,names in (('owner_guard',OWNER),('session_guard',SESSION)):
        if key not in result['raw']:raise ValueError('Missing exact guard witness')
        result[key]=witness(result['raw'][key],names)
    owner=result['owner_guard']
    if owner['name']=='context':
        values=('verified','denied','fault','source_age_expired')
        owner['context_verdict']=values[owner['detail']] if owner['detail']<len(values) else 'unknown_'+str(owner['detail'])
        owner['context_detail']=owner['extra']
    return result
def selftest():
    assert witness('4,3,8,3,6',OWNER)['name']=='context'
    assert witness('2,1,2,3,0',SESSION)['name']=='physics_read'
    assert not witness('8,0,0,0,0',OWNER)['failed_call']
    assert witness('4,3,12,3,0',SESSION)['name']=='owner_begin'
    assert witness('4,3,13,3,0',SESSION)['name']=='owner_end'
    for value in ('0,1,2,0,0','2,1,99,0,0','2,0,3,0,0','2,1,0,0,0','-1,0,0,0,0','2,1,8,0'):
        try:witness(value,OWNER);raise AssertionError('Invalid witness accepted')
        except ValueError:pass
    print(json.dumps(dict(cases=11,failures=0,scope='witness interpretation only')))
if __name__=='__main__':
    if sys.argv[1:]==['--selftest']:selftest()
    else:print(json.dumps(inspect(json.loads(Path(sys.argv[1]).read_text(encoding='utf-8-sig'))),indent=2))
