from pathlib import Path
import json,hashlib,datetime
h=Path(__file__).resolve().parent;r=h.parents[2];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();v=json.loads((h/'test-result.json').read_text())
assert v['passed'] and v['checks']==69 and v['instrument_sha256']==sha(h/'instrument.py') and v['actual_source_sha256']==sha(r/'analysis/solo-m1-20261028/cover_boundary_chain.py')
verification=dict(checks=69,passed=True,scope='One fresh pre-input candidate observation only after an exact GObjects +1 strict-close refusal. Shared reader barriers, original A, inputs and Native queries are unchanged. Old sample stays rejected; one extra read globally, no retries after actions or unknown query results.',sources={str(p.relative_to(r)):sha(p) for p in [h/'instrument.py',r/'analysis/cover-system-20261108/isolated-r105/prepare_operator.py',r/'analysis/cover-system-20261108/isolated-r105/run.py']},game_started=False,input_published=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25)
(h/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r255' not in old
add='''
## r255：候选阶段一次完整新读，仅针对已拒绝的GObjects同址/同容量新增1项（2026-11-12）

针对r247/r254两次已保存前置拒绝，新增私有operator源码instrument，作用点仅原A/ordinary approach/Native path开始之前的CoverLink候选读取。旧q.finish仍无参数、保留World/identity/metadata/registry/所有值双关闭；没有启用allow_registry_growth，也不修改共享cover_state/world_state或原cover_boundary_chain源文件。若严格关闭因RuntimeError原确切原因拒绝，且difference为query.r.objects_header本身、16原字节解析后同非零对齐data/同capacity、count恰+1且合法，旧样本照常作废并保存原拒绝；先持久写全选择过程一次retry-used，再完整observer.verify，创建全新CoverQuery/身份/字段/Slots/world计算，并重新运行全部原finish。只有新q关闭成功才进入candidate列表。

最多整个选择过程一次额外新读，第二次拒绝或另一个link再发生此问题即终止；不把追加事件当作旧样本通过的理由。identity/字段/name registry、重分配/容量变化、count+2/缩小、缺失差异、预算错误、未知原因或取消均不重读。observer身份复核失败时也不创建第二q。此处没有任何pad/tap/原A/Native query调用；原动作及未知/已尝试query绝不重派。

69检查通过：实际源码经过既有r149/r180后仍能唯一定位/AST解析；合成新旧q serial验证旧结果不泄露，先保存拒绝和verify再新读、同link再拒绝、跨link全局上限、身份失效、错误类型/差异/登记表/指针/容量/次数、取消、双instrument拒绝；strict finish不传放宽参数。原始header字节为分类依据，decoded convenience字段不能覆盖原文。均为离线synthetic reader/AST，未启动游戏/输入。prepare_operator纳入此变换并保存独立source SHA，runner在存在该receipt时校验同一源码SHA和既有完整冻结operator SHA；旧准备若未带字段仍由其原operator SHA保护。Native631c2cf8…1501不变，未重编译。

下一步新实例重新完整准备/import/14身份检查后定向验证实际候选与r253去重路径；不等价于已有实际retry或path完成。工程25%、CV24/AC22与所有body/world/view/headset缺口保持，publicv71/固定入口/Quest保持。证据candidate-read-retry-r255。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(verification))
