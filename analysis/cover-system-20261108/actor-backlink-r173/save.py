"""Checkpoint the authenticated ownership repair before a new game run."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['owner_links_game_repaired']
p=H.parent/'isolated-r105/prepare.py';s=p.read_text(encoding='utf-8-sig').rstrip()+'\n'
compile(s,str(p),'exec');assert "parser.add_argument('--native-r173'" in s;p.write_text(s,encoding='utf-8')
findings=dict(original_binary_sha256=v['binary_sha256'],runtime_source='owner-link-detail-r171/runtime-r172/owner-links.json',
    observed='The asset reader read Body+0x10=28 and rejected before original dispatch.',
    static_observation='NpActor constructor constructs embedded Actor at +0x28 and writes public owner at +0x30. Embedded Actor table is 2324f8; +0x40 is optional dynamic Body.',
    evidence_ids=[v['original_ctor_evidence'],v['embedded_actor_ctor_evidence'],'ev_8859060b68c549911d8541dabbd42a2f7c8139577c940524e2211fb878d7114c'],
    inference='Old reverse check interprets a distinct object field as public owner; authenticate the actual embedded-owner relation instead.',
    unknown=['Exact semantic name of the r172 value28','Actual game result of corrected ownership chain','Complete shape/world/body sweep coverage','Final unclipped cover view/headset experience'],
    provider='REA/Ghidra12.1.4; IDA unavailable because this REA registration had no IDA configuration; no configuration changed',
    execution='Original DLL read as data only; analysis session exported and closed; no game started',
    evidence_bundle=v['evidence_bundle'],evidence_bundle_sha256=v['evidence_bundle_sha256'])
(H/'findings.json').write_text(json.dumps(findings,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r173' not in old
entry='''
## r173：原二进制确认NpActor内嵌拥有者，五个读取器关联修复已编译（2026-11-11）

本轮progress。从r172实际asset Body+0x10=28失败继续，REA打开精确PhysXCore SHA22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48。IDA provider未配置，显式改用Ghidra12.1.4，未改安装/注册。原NpcActor构造1328b0的汇编明确LEA [RSI+28]调用1204c0，并MOV [RSI+30],RSI；后者写入Actor vtable2324f8并将其+18（即NpcActor+40）初始化为空。NpActor后续debug分支分别显示该指针为空的Static和非空的Dynamic。因此旧body/internal+10==public Actor不是已认证的拥有者链。NpcActor+40动态对象与嵌入Actor是两类对象，不能混用；原28的具体语义仍未知，不称flags字段。完整32份REA证据导出owner-link-detail-r171/actor-layout-rea-evidence.json后正常关闭；findings链接原汇编Evidence IDs和局限，无新游戏/原DLL执行。

新增共用纯读取cover_physx_actor_backlink.h：认证NpcActor+28的嵌入Actor表2324f8，读取NpcActor+30并要求它等于当前public Actor。asset/collider/components与actor_shapes/shape_owner五个实际读取器都改为该真实链；保持原primary表、Scene、当前Body指针、各形状/public/internal/compound成员关系、同源guard、两次读取和关闭比较。Body非空仍必须格式有效，合法静态Body空值独立处理；self匹配允许精确全链一致的静态null Body，不凭null或Actor身份单独放行。所有来源、世界覆盖、排除与移动许可仍各守原规则，不增加writer、调用、输入或grant，不续旧时间/放宽年龄。新增读取有成本，不称零成本。

旧/新12项生产reader同输入对照：正确embedded owner但Body+10=28，旧拒绝新接受；embedded owner错误但伪造旧Body+10==Actor，旧接受新拒绝；静态Body为空与错误嵌入类型也分别对照。既有夹具补实际+28/+30字段；原reverse破坏和race刺激移至真实拥有者字段，非法指针刺激由合法null改为1，未删除原检查。asset92/collider94/components110/optional asset169/attachments150/shape owner519/actor shapes77、路径391/OFF51、观察89、完整owned sweep146通过，合计1900；另有旧基线12与解释器23。静态全链匹配有正例，静态shape与非空collider Body冲突仍拒绝。最初夹具脚本一次substring歧义中止后精确修正；checks-v2/v3路径66项失败源于旧host缺嵌入Actor字段，补齐基础和cylinder夹具后checks-v4原391通过，原失败保留。不是游戏实跑结论。

Native f585196291ca8481ef9ffd8db17032a60e2de6c2eff1b58a847cb8f51e8426cd完整编译链接0，exact r171冻结树仅八文件产品delta，413项目依赖、外部worktree0、GEARS flags保持，其他冻结源逐项相同。原始时间/新鲜度/唯一writer和所有不相关源不变；prepare新增--native-r173认证精确delta。source/delta、检查、前后反例、原始失败、来源manifest、依赖、build及核对在actor-backlink-r173。

下一步Git保存后以新实例短查：原版碰撞是否真正开始/返回、真实owner链是否通过，再处理第一个实际剩余失败；不重跑未变年龄候选，不跳过异场景对象或放宽关联。当前仅源码/离线/编译，未准备/启动该候选；实际owner_links修复、完整CV01–24/AC01–22、严重自身模型遮挡和头显仍未验收，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],checks=1900,build_only=True,game_started=False)))
