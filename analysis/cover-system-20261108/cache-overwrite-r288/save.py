from pathlib import Path
import hashlib,json,subprocess,datetime
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=r).strip()
receipt=json.loads((o/'native-build-v3-result.json').read_text());assert receipt['dll_sha256']==sha(o/'native-build-v3/GearsCapture.dll')
for rel,digest in receipt['project_dependencies'].items():assert sha(o/'native-source-v3'/rel)==digest
test=json.loads((o/'cached-source-v4-result.json').read_text());assert test['test_exit']==0
for rel,digest in test['source_sha256'].items():
 assert sha(Path(test['host_source'])/rel)==digest,rel
 if (o/'native-source-v3'/rel).exists():assert sha(o/'native-source-v3'/rel)==digest,('test/native dependency differs',rel)
upstream=Path('J:/gearsVr/analysis/ida/camera-110878-20261014/section_1000.bin.i64');assert sha(upstream)=='a87a4802eadec9403a4f3fcbfde330d2f11a087754b25c5ab4cb5d31f2c9eab0'
proof=json.loads((o/'cached-copy-execution-proof.json').read_text());proof.update(authoritative_thunk_bytes='ff25b34bda00',authoritative_iat_rva='0x131ca18',ida_operand_text='0x131ca08 (not used; conflicts with raw displacement and actual runtime slot)',disk_export_name_address_matches_recorded=True,recorded_export_bytes_differ_from_disk=True,r283_full_crt_body_not_captured=True,current_native_v3_checks_full_body_and_tail_before_after=True,first_native_v2='compiled address/name binding only; not game-run or selected after stronger full-code guard added',historical_failures=['decoder build missing PendingBranch namespace shim; fixed local decoder only','first source test iota integer narrowing under WX; typed char seed fixed'],upstream_ida_unchanged=True)
(o/'cached-copy-execution-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text());a=r/'docs/cover-system-development-20261108.md';b=Path(g['development_copy']['path']);assert a.read_bytes()==b.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r288 原始缓存整数组拷贝的认证路径接入：源码/离线/编译通过，因果待游戏验证

上轮为实际证据进展，本轮未重复相同游戏。用既有离线IDA CLI分析私有DB副本，原始f7940/facb0代码分别2569/4055bytes与已保存原始.text逐字节相符、SHA与原证书一致；f5b78及缓存分支汇编/伪码保留。上游DB hash a87a4802…c9eab0未变，不动原游戏、Quest或owner会话。证实两个原实现有epoch相同且cache.count==out.count时的整数组复制：canonical Local capacity==count115、out header不同于node+0x80时，正常返回前原函数调用copy thunk，随后恢复count115。现有非cached decoder/blend见证明确排除这条路径，但这还不是r287因果证明；镜像/其他forward仍可能需接入。

新增cached_pose_overwrite_source和私有live witness，复用既有F7940/FACB0 hook和原子overwrite Registry BlendTicket，不添hook/骨骼写入者或直接清债。仅当前注册Mesh、canonical Local/Desired全115、cache115且不别名、Local capacity115免重分配、node/mesh epoch一致、当前Node identity/code/world通过时观察。原函数正常返回后独立重读Source/Cache/Local/Desired、全部cache字节稳定且Local等于cache；相等只是附加检查，原始代码/实际调用才是写入证据。Registry仍要求后续epoch严格大于安装epoch、真实完成、原consumer已结束、全mask及原有身份/退休检查。

copy thunk原始6bytes ff25b34bda00权威解析IAT RVA131ca18，实际r283既有生产档案也记录该slot。IDA operand字符串显示131ca08存在冲突，不能据此接入；按raw bytes和实际slot处理。r283档案回读显示System32 VCRUNTIME140.dll memcpy export RVA1f020，内存入口E91BDBFFFF跳至1cb40，与当前磁盘默认dispatch不同，不能仅凭模块名/地址放行。私有CRT离线分析确认该实际实现memcpy_avx_ermsb_Intel，完整1041bytes SHA470810…369d1；固定115*32=3680字节、Source/Dest不重叠，排除CopyDown、>=8192 rep-movs和>=0x180000 NT路径，正常YMM尾表29400的9个目标都在同一已核对函数内。Native-v3额外在调用前后核对当前System32模块/GetProcAddress、实际dispatch5bytes、完整实现1041bytes和尾表36bytes。r283未保存完整CRT函数内存，不能倒推当时已通过本新门；由下一真实运行执行当前字节核对。v2只校验export绑定的编译稿未运行/未选用。

112项纯数据/代码连续性检查0失败，覆盖wrong epochs/arrays/alias/Desired、copy损坏/中途变化、CRT实现/尾表/dispatch改动；这不是game或写入权限证明。首版iota narrowing/WX及局部档案decoder namespace编译失败均保留后修正。最终Native-v3在r284-v2冻结生产树仅改4个文件，434项目依赖全部冻结内，flags不变、PHYSX OFF；DLL71789359a915ec5030fc516313c74cee3a0344c39a128eaadf17d1de4c087573编译链接通过。加入cover forward镜像/FACB0分类计数，仅区分下一真实失败路径。尚无本新版本运行/Grip/头显验收，不称挂起已修复。
'''
raw=a.read_bytes()+text.replace('\n','\r\n').encode();a.write_bytes(raw);b.write_bytes(raw);g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=['tools/native-capture/'+n for n in ('fresh_pose_producer_live.inl','cached_pose_overwrite_source.h','cached_pose_overwrite_live.inl','cached_pose_memcpy_certificate.h','cached_pose_overwrite_source_test.cpp')]+[str(a.relative_to(r)),str(gp.relative_to(r))]+[str(p.relative_to(r)) for p in o.iterdir() if p.is_file() and p.suffix in ('.py','.cpp','.json','.log','.txt')]
subprocess.run(['git','add','-f','--',*files],cwd=r,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE);subprocess.run(['git','diff','--cached','--check'],cwd=r,check=True)
v=subprocess.run(['git','commit','-m','Attest original cached-array copies and exact CRT implementation before pose retirement'],cwd=r,check=True,stdout=subprocess.PIPE);print(v.stdout.decode().splitlines()[0])
