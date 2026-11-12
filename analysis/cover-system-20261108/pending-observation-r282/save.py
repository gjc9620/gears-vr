from pathlib import Path
import hashlib,json,subprocess,datetime
from overlay import ROOT as root,REL,apply
out=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def git(*args,input=None):return subprocess.run(['git',*args],cwd=root,input=input,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True).stdout
assert not git('diff','--cached','--name-only').strip()
for version in ('host-source-v2','native-source-v2'):
 manifest=json.loads((out/(version+'-manifest.json')).read_text())['files']
 for rel,digest in manifest.items():assert sha(out/version/rel)==digest,rel
assert json.loads((out/'host-result-v2.json').read_text())['test_exit']==0
receipt=json.loads((out/'native-build-v2-result.json').read_text());assert receipt['dll_sha256']==sha(out/'native-build-v2/GearsCapture.dll')
data=apply(git('show','HEAD:'+REL).decode());assert data==(out/'native-source-v2'/REL).read_text(encoding='utf-8')
blob=git('hash-object','-w','--stdin',input=data.encode()).decode().strip();git('update-index','--cacheinfo','100644,'+blob+','+REL)
(out/'validation-review.json').write_text(json.dumps(dict(host_checks=187,host_failures=0,first_host_test='3 later view-age failures because observer test advanced the synthetic fixture clock; retained failure; restore fixture clock before independent view tests',runtime_proven=False,observes_only=True,does_not_retire_or_restore=True,game_thread_file_io=False,first_published_snapshot_immutable=True,source_baseline='r279-v4 production; Host BodyGoal uses r277 for observer-only scope',native_sha256=receipt['dll_sha256']),indent=2)+'\n')
gp=root/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text());a=root/'docs/cover-system-development-20261108.md';b=Path(g['development_copy']['path'])
assert a.read_bytes()==b.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r282 挂起覆写的不可变观察记录：离线/编译通过

针对r280/r281记录缺口，新增cover_pending_observation，仅既有diagnostic_first_failure开启、当前GT、持有cover source且确有未完成字段时采样。同一Kernel serial每20ms最多一份、32份后release发布，换serial在未发布期间重开；原始PendingObservation/真实branch cache/实际尝试Compose编号一并保存。发布后不再写，外部collector acquire后导出cover-pending-observation.json，完全不依赖stopping/callbacks/obligations归零；GT不能导出。缺少真实branch记录时保留空，不伪造分支。既有first_pending硬失败保留语义仍在；未改身份/时效/原子写入/退休/恢复规则或单写入者。

真实Host Native/Backend/Kernel/Compose消费后测试187检查0失败，覆盖关闭/异线程/非cover排除、采样间隔、32项边界、发布不变、GT拒绝IO、外部在债务仍在且未stopping时导出/回读，以及Kernel债务/mask不变。Host集成使用冻结r277 BodyGoal作为观察器测试边界，不冒称覆盖r279新朝向。初版测试推进假时钟640ms后导致后续独立view-age断言失败3项，已保持该失败并在观察器测试结束恢复fixture时钟；这不是修改产品时效。r282-v2完整Native在r279-v4冻结生产树上只叠加2文件，431份项目依赖冻结内，全部flags不变/PHYSX_QUERIES=OFF；DLL c964ba78e59d7950e1a2e3b069ecb4c047b8399aa8333bcd2cf1b7f033159906。尚未在游戏取得新记录/修复挂起，不作可玩验收。
'''
raw=a.read_bytes()+text.replace('\n','\r\n').encode();a.write_bytes(raw);b.write_bytes(raw);g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=['tools/native-capture/'+x for x in ('cover_pending_observation.inl','cover_pending_observation_test.inl','cover_c0_policy_live_test.cpp')]+[str(a.relative_to(root)),str(gp.relative_to(root))]
files += [str(p.relative_to(root)) for p in out.iterdir() if p.is_file() and p.suffix in ('.py','.json','.log')]
git('add','-f','--',*files);git('diff','--cached','--check');result=git('commit','-m','Publish immutable bounded cover overwrite observations while debt remains');print(result.decode().splitlines()[0])
