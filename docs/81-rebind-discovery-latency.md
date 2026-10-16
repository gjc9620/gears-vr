# 世界重绑定的重复解析开销

2026-10-17。为拆分检查点按键到Prepare前的等待，使用ignored诊断wrapper计时现有函数；每次调用原函数一次，不改变重试、身份认证、输入或生命周期。最多保存1024条阶段事件，记录实际源码哈希。未新增游戏Hook、未打开IDA，也未扩大逆向资料库。

## 修改前的实际证据

会话continuous-stereo-24912-20261017-060404-b9bd5c4b连续两轮检查点恢复，generation1→3。prepare总耗时3347.32／3306.62ms，第一次resolve_camera_once分别913.69／962.99ms；之后acquire_observer_preparation又执行完整fresh Resolver.resolve，分别854.84／747.80ms。后续Anchor、UI、Crosshair和发布都成功。它证明有重复解析，不证明整个等待都来自这一项。

## 修复与保持的校验

提交5e86b5c仅在已有HookEntryAttestation的build_candidate路径省略第一次resolve_camera_once。完成准确PID、创建时间、owner、DLL路径绑定及reverify之后，以保留的完整本地玩家身份中的地址和已认证模块基址作为比较输入。acquire仍在每次尝试创建新Resolver并完整resolve，校验本地玩家地址／模块基址，再构建全部证书；返回的camera和证书全部来自本次fresh解析。后续完整本地玩家身份、脚本、属性、链接、源一致性和最终attestation复核均保留。没有跨次缓存、复用旧证书或改变原生格式。

没有attestation的路径继续第一次完整解析。78项相关测试通过，覆盖重试重新建Resolver、迟到／旧完整身份、错误模块、错误绑定／generation、新鲜返回值和原路径。

## 修改后实际回归

会话continuous-stereo-8328-20261017-060659-813abb4a，默认Caption38／Borderless1原生文件未变。两轮检查点恢复均成功，generation1→3；prepare分别2152.52／2295.12ms，阶段记录内不再有prepare.resolve_camera_once，fresh acquire与各证书校验仍在。两轮分别取得新4K双眼GPU收据，连同初始共3图；原生清理和正常游戏关闭通过。

修改前／后两轮prepare均值约3326.97／2223.82ms，减少约1103.15ms（33.16%）。样本仅各两轮、不同进程和时间，不能当稳定因果收益；唯一直接确认的结构变化是少了一次完整解析，原先这项约0.91–0.96秒。实际按键到脚本观察恢复，修改前8865.29／5990.17ms，修改后5839.39／6112.28ms；后者第二轮反而略长，因此不能把prepare减少推广为所有加载／端到端恢复同比提速。

前述恢复观察使用准确keydown send_ns，且等待至少10个epoch内提交，晚于首次新代projection。观察和prepare的总差包含游戏加载、轮询、世界观察和调度，目前没有完整拆开。正常观察本身约0.9–1.3秒且周期运行，可能仍有优化空间；未证明它造成头显卡顿，不能直接降低观察或放松认证。

原始证据：analysis/runtime/checkpoint45／46-worker-timing.jsonl、两轮audit和normal-close.json，汇总rebind-discovery46-comparison.json。测试没有证明机枪、所有CG、长时间游玩或真头显连续性。第一版仍未完成。

## 独立候选

analysis/release-staging/caption38-rebind5-20261017含5e86b5c，700文件、43,420,731字节，清单SHA256 b5aa01defa729b0c6482425dd5d0e5e2a8b9103a2cbeec050bdc88ea6d8c006b，隔离预检通过。上述实际运行使用仓库默认候选，未从此新快照执行；它不是新包干净机器／头显验收，旧快照保留。
