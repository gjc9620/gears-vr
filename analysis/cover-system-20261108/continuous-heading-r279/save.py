from pathlib import Path
import hashlib,json,subprocess,datetime
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def git(*args,input=None):return subprocess.run(['git',*args],cwd=root,input=input,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True).stdout
assert not git('diff','--cached','--name-only').strip()
receipt=json.loads((out/'native-build-v4-result.json').read_text());assert receipt['dll_sha256']==sha(out/'native-build-v4/GearsCapture.dll')
for rel,digest in receipt['project_dependencies'].items():assert sha(out/'native-source-v4'/rel)==digest
for name in ['cover_view_heading','cover_l2_grab_live','cover_input_live','cover_shared_input_live','cover_full_goal_live']:
 r=json.loads((out/(name+'-result.json')).read_text());assert r['build_exit']==0 and r['test_exit']==0
 for rel,digest in r['source_sha256'].items():assert sha(root/rel)==digest,rel
fail={'phase':'r279 initial snapshot preparation','result':'unique anchor assertion rejected before compiler/game','reason':'EyeSample substring also appears in two unrelated struct declarations','resolution':'use indented local declaration anchor; incomplete native-source-v2 preserved; final v4 build passed','first_heading_test':'wrong test component index scaled zero instead of right vector; corrected test; 19 checks pass'}
(out/'preparation-history.json').write_text(json.dumps(fail,indent=2)+'\n')
guard=root/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(guard.read_text())
a=root/'docs/cover-system-development-20261108.md';b=Path(g['development_copy']['path'])
assert a.read_bytes()==b.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r279 连续观察朝向与原生L2手坐标接入（编译/离线通过，尚未游戏验证）

按owner范围继续：遮挡与碰撞后置，左右参考原生L2 action3/4点，向上仅CT2矮掩体参考popup点，不增加表面/碰撞门。L2点与位移范围保持wall轴，当前左手世界位置改经独立view_anchor换算；single/shared Input诊断同步。90度独立视角左右获取、缺失视角不伪造抓点、CT1不上探均离线通过。

BodyGoal只保存最近已认证standing BASE观察水平轴的几何校准；进入cover重新使用当前neutral eye，不保存或刷新旧眼位/追踪/骨架/写入许可。每次provider独立验证Scene/当前avatar/world/mesh/asset/reference几何、Native进程/会话、XR session/space/reference及当前Stereo新鲜性；失效时撤回Cover视角，shared武器仲裁继续。已在cover启动或参考重绑无有效standing校准时，仅用当前独立认证的pre-preview Scene BASE提取水平轴，拒绝非刚性/反射/垂直/非有限矩阵，绝不退回Pawn+pi。当前HMD仍随后仅应用一次。drain清校准；普通compose视角失效不会破坏进入前几何朝向。

离线五项：heading19、L2抓点22、Input88、shared25、FullGoal52全部0失败。最终r279 Native-v4编译链接通过，430份项目依赖全部位于冻结source-v4，PHYSX_QUERIES仍OFF，其余flags不变；DLL 97335d629a043e0e68a70e25186757b45539dab992aa710d3e5cea620bc1cd4b。v3在最后provider返回值防护补充前已编译通过，仅作为历史，不交付。初次快照准备的非唯一anchor拒绝和初版测试错误均保留，不算运行成功。本节只有源码/离线/编译证据，进掩体连续画面、真实Grip链和实体头显仍待验证。Quest/公版/固定手动包未变。
'''
data=a.read_bytes()+text.replace('\n','\r\n').encode('utf-8');a.write_bytes(data);b.write_bytes(data)
g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();guard.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def index_replace(path,edits):
 data=git('show','HEAD:'+path).decode('utf-8')
 for old,new in edits:
  assert data.count(old)==1,(path,old);data=data.replace(old,new)
 blob=git('hash-object','-w','--stdin',input=data.encode()).decode().strip()
 git('update-index','--cacheinfo','100644,'+blob+','+path)
index_replace('tools/native-capture/capture.cpp',[('#include "../vr-common/cover_view.h"','#include "../vr-common/cover_view.h"\n#include "../vr-common/cover_view_heading.h"')])
index_replace('tools/native-capture/cover_shared_input_live.inl',[
 ('        neutral::EyeSample eye{};','        neutral::EyeSample eye{};const auto incoming_view=view;'),
 ('frame.view_anchor=frame.anchor; // View remains independent of the C0 wall/body basis.','frame.view_anchor=input::view_anchor(scene,eye,stereo,incoming_view);'),
 ('cover_tracking_point(frame.anchor,result.view,stereo.reference,hands.hands[0].grip.position','cover_tracking_point(frame.view_anchor,result.view,stereo.reference,hands.hands[0].grip.position')])
files=['tools/native-capture/'+n for n in ['cover_input_live.inl','cover_l2_grab_live.inl','body_c0_goal_policy_live.inl','cover_view_heading_test.cpp','cover_l2_grab_live_test.cpp','cover_input_live_test.cpp']]+['tools/vr-common/cover_view_heading.h','docs/cover-system-development-20261108.md',str(guard.relative_to(root))]
files += [str(p.relative_to(root)) for p in out.glob('*.py')]+[str(p.relative_to(root)) for p in out.glob('*.json')]+[str(p.relative_to(root)) for p in out.glob('*-build.log')]+[str(p.relative_to(root)) for p in out.glob('*-test.log')]
git('add','-f','--',*files);git('diff','--cached','--check')
print(git('diff','--cached','--stat').decode())
print(git('commit','-m','Preserve authenticated observation heading and map L2 grabs through view basis').decode())
