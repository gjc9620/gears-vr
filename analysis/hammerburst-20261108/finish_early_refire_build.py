from pathlib import Path
import json,hashlib,difflib
from prepare_early_released_refire import HELPER
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'history-origin-r1';P=A/'early-released-refire-r1'
assert not (P/'prepared.json').exists();check=json.loads((P/'early-refire-test-result.json').read_text());assert check['passed']
assert HELPER in (P/'source/tools/playground/weapon_hammerburst_burst_event.inl').read_text()
dll=P/'build/Release/GearsPlayground6DofUnified3.dll';log=(P/'build.log').read_bytes();text=log.decode('utf-16' if log[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace');assert dll.exists() and 'error C' not in text
parent=json.loads((BASE/'prepared.json').read_text());configure=[s.replace(str(BASE),str(P)) for s in parent['configure_command']];build=[s.replace(str(BASE),str(P)) for s in parent['build_command']]
proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Existing pre-original held-muzzle stage services/delivers only captured HB refire with prior real-UP lifetime, no owned input/first/cancel/new event. Original service/delivery/claim,source100ms/muzzle20ms/event250ms/counter6 unchanged; no new input or timer.',actual_adapter_checks=427,added_cases=24)
(P/'prepared.json').write_text(json.dumps(proof,indent=2))
rels=['tools/playground/'+n for n in ('weapon_hammerburst_burst_event.inl','weapon_hammerburst_burst_runtime.h','weapon_lancer_r2_native.inl')]
patch=''.join(''.join(difflib.unified_diff((BASE/'source'/rel).read_text().splitlines(True),(P/'source'/rel).read_text().splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)) for rel in rels)
(R/'docs/hammerburst-early-released-refire.patch').write_text(patch)
p=A/'full-requirements-audit-20261110.json';v=json.loads(p.read_text());assert len(v['requirements'])==13;v['current_candidate']='Private early-released-refire-r1 compiled '+proof['dll_sha256'][:8]+',427 actual adapter fixture checks/24 new cases pass; earlier existing original refire delivery after certifiedUP only. Original transaction/service/delivery/six/250ms/100ms/20ms unchanged. Not yet game tested; all13 remain.';v['requirements_complete']=False;p.write_text(json.dumps(v,indent=2))
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 已认证UP续发提前到原pre-original枪口阶段\n\nPrivate PG'+proof['dll_sha256'][:8]+' early-released-refire-r1完整编译；只对已捕获refire、已有真实UP lifetime、input不持有、非first/cancel且原有限事务count1..5，现有pre-original fresh muzzle准备后调用原service/deliver。原事件/capture/claim/六发/100ms/20ms/250ms、身份切枪tracking与post-Input新按下/首发逻辑保持。24新增case/92check+原335，实际adapter fixture共427通过：早180ms与晚310ms同一原事件对照、22有效/否定变体、重复pre/post不多打。engine/certificate/source为有界fake，不能作game证明。初次fullbuild缺前置接口声明、fixture混合auto和无reset成员导致编译失败，补声明、拆变量并用空事务赋值修正，未放宽生产判定；最终build/check0。Nativeb9ab7a82/XR不变，未运行新候选，13项保留。\n')
print(proof['dll_sha256'])
