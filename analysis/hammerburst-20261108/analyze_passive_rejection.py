from pathlib import Path
import json,re,sys,collections
A=Path(__file__).resolve().parent
def main():
    source=Path(sys.argv[1]);raw=source.read_bytes()
    text=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig')
    names=('target_gate','request_frame','request_sequence','current_sequence','observed_ns','request_sync_ns','socket_gate','socket_source_frame','socket_frozen_frame','socket_source_sequence','socket_frozen_sequence','socket_source_cookie','socket_frozen_cookie','socket_source_lifecycle','socket_frozen_lifecycle')
    rows=[]
    for match in re.finditer(r'\bbody_passive_rejection_(\d+)=([^\s]+)',text):
        values=[int(x) for x in match[2].split(',')];assert len(values)==len(names)
        row=dict(zip(names,values));row.update(index=int(match[1]),original_request_age_ms=(row['observed_ns']-row['request_sync_ns'])/1e6)
        rows.append(row)
    proof=json.loads((A/'native-passive-rejection-r1/prepared.json').read_text())
    summary=dict(records=len(rows),bounded_capacity=64,target_gates=dict(collections.Counter(r['target_gate'] for r in rows)),socket_gates=dict(collections.Counter(r['socket_gate'] for r in rows if r['target_gate']==8)),socket_gate_definitions=proof['socket_gates'],not_a_repair=True)
    result=dict(source=str(source),summary=summary,rows=rows,limits='Original passive target and original socket-admission first-failure returns only. No new reads/clocks, no weakened source or action permission. Whole C0 non-admission or absent fresh PG publication can be separate failures.')
    Path(sys.argv[2]).write_text(json.dumps(result,indent=2));print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
