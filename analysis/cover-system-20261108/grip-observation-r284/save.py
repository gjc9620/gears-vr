from pathlib import Path
import hashlib,json,subprocess,datetime
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=r).strip()
receipt=json.loads((o/'native-build-v2-result.json').read_text());assert receipt['dll_sha256']==sha(o/'native-build-v2/GearsCapture.dll')
assert sha(r/'tools/native-capture/cover_input_live.inl')==sha(o/'native-source-v2/tools/native-capture/cover_input_live.inl')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text());a=r/'docs/cover-system-development-20261108.md';b=Path(g['development_copy']['path']);assert a.read_bytes()==b.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r284 左Grip实际检查准备：生产交互状态可观察，映射纯数学验证通过

仅在既有Input summary输出interaction phase/edge/request/owns/virtual offset/certified，供下一实际Grip检查分清获取/拖动/松手与C0视角；未改交互逻辑或第二仲裁器。r284 Native-v2在r282冻结树只改Input一文件，431依赖与flags/PHYSX OFF不变，DLL c4055b2136e42319e9ed9a1d28ae93155bcc542edaa0c9b6c02eff7f3a036e4d编译通过。

新有界operator沿已有Source1 hand_move/gesture/pad路径，先用释放状态三轴5cm探针和实际hand_world求raw→world映射，检查单位/正交/奇异/回到原位及当前Stereo序列/新增body消费/view；不假定reference朝向零或旧preview轴。纯数学16项正/负例通过，无game/input调用。随后仅当前CT2左右原生L2点：接触/新Grip获取/12cm拖动/释放保持/再抓新request/反向回零/释放，HMD固定，禁碰撞查询，期望phase与owns和virtual/FullGoal共同通过；仍须真实运行及完整图像验证。operator源已解析，尚无Grip运行结果，不将此准备算交互完成。GrabPose/完整手臂拟合及其他需求保持未完成。
'''
raw=a.read_bytes()+text.replace('\n','\r\n').encode();a.write_bytes(raw);b.write_bytes(raw);g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=['tools/native-capture/cover_input_live.inl',str(a.relative_to(r)),str(gp.relative_to(r))]+[str(p.relative_to(r)) for p in o.iterdir() if p.is_file() and p.suffix in ('.py','.json','.log')]
subprocess.run(['git','add','-f','--',*files],cwd=r,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE);subprocess.run(['git','diff','--cached','--check'],cwd=r,check=True)
v=subprocess.run(['git','commit','-m','Expose cover Grip state and prepare measured L2 interaction sequence'],cwd=r,check=True,stdout=subprocess.PIPE);print(v.stdout.decode().splitlines()[0])
