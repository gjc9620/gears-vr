"""Decode a same-measurement first witness, against the exact artifact only."""
from pathlib import Path
import importlib.util,json,sys
H=Path(__file__).resolve().parent
FIELDS=('mode','valid','non_age_valid','expired','controllers_match','verdict','hard_detail',
    'now','stereo_received','controller_received','controller_sync','metadata_begin','metadata_end','tick_begin','tick_end',
    'limit','published','ticket','reference_revision','size','abi','flags','status')
def decode(text):
    values=[int(x) for x in text.split(',')]
    if len(values)!=len(FIELDS) or any(v<0 for i,v in enumerate(values) if i not in range(7,15)):
        raise ValueError('Malformed core witness')
    w=dict(zip(FIELDS,values))
    if w['mode'] not in (1,2) or w['valid']>1 or w['non_age_valid']>1 or w['expired']>3 or w['controllers_match']>2:
        raise ValueError('Unknown core witness values')
    if w['limit']!=50000000:raise ValueError('Different measurement policy')
    if w['mode']==1 and w['verdict']!=4294967295:raise ValueError('Wrong Core source mode')
    if w['mode']==2 and w['verdict']>2:raise ValueError('Wrong Tick source verdict')
    w['legacy_age_only_proven']=bool(w['mode']==1 and not w['valid'] and w['non_age_valid'] and
        (w['expired'] or w['controllers_match']==0))
    return w
def inspect(report,expected_native=None):
    native=expected_native if expected_native is not None else json.loads((H/'native-build-result.json').read_text())['dll_sha256']
    spec=importlib.util.spec_from_file_location('core_pose_base',H.parent/'pose-probe-r126/inspect_pose_probe.py')
    base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
    result=base.inspect(report,native);result['core_witness']=None
    row=next((r for r in result['rows'] if r['name']=='measurement'),None)
    if row and row['detail']==2:
        result['core_witness']=decode(report['native_status']['cover_pose_probe_core_8'])
    return result
def selftest():
    row=[1,0,1,2,1,4294967295,0,2000000000,1949999999,1997000000,1996000000,0,0,0,0,
        50000000,1999000000,291,302,1832,1,7,1]
    assert decode(','.join(map(str,row)))['legacy_age_only_proven']
    for at,value in ((2,0),(0,2)):
        changed=row.copy();changed[at]=value
        if at==0:changed[5]=2
        assert not decode(','.join(map(str,changed)))['legacy_age_only_proven']
    changed=row.copy();changed[8]=-1;assert decode(','.join(map(str,changed)))['stereo_received']==-1
    for changed in (row[:-1],row+[0],row[:15]+[100000000]+row[16:], [3]+row[1:]):
        try:decode(','.join(map(str,changed)));raise AssertionError('Invalid witness accepted')
        except ValueError:pass
    print(json.dumps(dict(cases=8,failures=0,scope='CSV interpretation only')))
if __name__=='__main__':
    if sys.argv[1:]==['--selftest']:selftest()
    else:print(json.dumps(inspect(json.loads(Path(sys.argv[1]).read_text(encoding='utf-8-sig'))),indent=2))
