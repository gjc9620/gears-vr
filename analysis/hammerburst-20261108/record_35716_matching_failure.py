from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;R=A.parents[1];N=A/'native-render-age-r1/source/tools'
raw=(A/'runtime-35716-text/capture-status.txt').read_bytes();text=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
def vector(key):return [int(n) for n in re.search(r'\b'+key+r'=([^\s]+)',text)[1].split(',')]
core=vector('shared_grip_handoff_2');source=vector('shared_grip_handoff_source_2');state=vector('shared_grip_handoff_state_2')
assert core[0]==583 and source==[3,3,4834,0,0,17745700,0]
bridge=N/'vr-common/controller_input_bridge.h';s=bridge.read_text();assert 'header_invalid,publisher_changed,not_published_yet,evicted,frame_mismatch' in s
assert 'if(!lock_now(mutex_,contended))return finish(Code::mutex_unavailable);' in s
v=dict(runtime=35716,frame=583,handoff=core,state=state,source=source,
 decoded=dict(matching_code=3,matching_name='mutex_unavailable',controller_status=3,requested_sequence=4834,latest_sequence_unknown=0,selected_sequence_unknown=0,stereo_age_at_matching_ns=17745700,selected_age_unknown_ns=0),
 source_files={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (bridge,N/'native-capture/cover_shared_input_live.inl',N/'native-capture/cover_input_live.inl')},
 meaning='Original read_matching rejected lock_now before copying its packet. No packet sequence/timestamps were read; zero fields mean unknown, not eviction or stale packet. Existing source identity check preceded mutex attempt. Controller absence then reached broker prepare stage1, lifecycle4->5 followed; exact input ancestry is not safe to assume.',
 limits='lock_now maps WAIT_TIMEOUT, WAIT_ABANDONED (after existing ReleaseMutex), WAIT_FAILED and null mutex to false; Code3 alone does not prove ordinary contention. No evidence to change freshness, retry acquisition or preserve an input grant across this failure.',
 next='Preserve exact result of the original one WaitForSingleObject(mutex,0) in receiver-local diagnostic; retain existing contended boolean. No second lock attempt, wait duration change, source query, packet layout or writer change. Export bounded actual failure on existing GT summary. Test timeout versus abandoned/failed separately before considering any transport-gap policy.')
(A/'runtime-35716-matching-failure.json').write_text(json.dumps(v,indent=2),encoding='utf-8')
p=A/'runtime-35716-first-failure-analysis.json';a=json.loads(p.read_text());a['matching_failure']=v;a['next_bounded_source_question']=v['next'];p.write_text(json.dumps(a,indent=2),encoding='utf-8')
p=A/'active-runtime-wait.json';a=json.loads(p.read_text(encoding='utf-8-sig'));a['next']=v['next'];p.write_text(json.dumps(a,indent=2),encoding='utf-8')
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n35716现存handoff-source精确解码：583→requested4834，read_status3=ControllerMatchingRead::mutex_unavailable，controller_status3，Stereo17.7457ms；packet复制之前lock_now即失败，latest/selected0是未知而非序号0/驱逐。lock_now把WAIT_TIMEOUT、ABANDONED（原逻辑释放后拒绝）、FAILED等映成false，不能仅凭code3宣称普通竞争。下一步保留原一次WaitForSingleObject(0)结果及现有contended标志，区分普通占用与异常，保持一次读取/原时限/数据布局/生产者与输入保护。\n')
print(json.dumps(v['decoded']))
