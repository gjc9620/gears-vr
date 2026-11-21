# Private controller reader's Stereo open failure diagnosis

HI right hand restored exit 66 It's already in the original.`StereoBridge::open`Not yet.400- It's a little bit of a sauna.1/age matches the cycle. Old stderr only remembers stage=stereo, and cannot distinguish between opening mutex, mapping, mapping view, zero waiting or initial packet check failed, nor can it be found that the file is out of date.

Candidatures from Actual`0c707034…fe179e` Reader's freeze 9 Part-source and original MSVC flags, only private`stereo_bridge.h`and`reader.cpp`I'm sorry.7 All source/test, exe and old reply are retained; shared headers, Native, XR, Enderer, regular starter. All sources, works and test derivatives are in signored `analysis/stereo-reader-open-diagnostic-20261027/`, not in the release package.

Select the exe:`validation-1793056273353595900/reader.exe`，SHA 256 `01d8b9db36bfa57b580dc98d47e0969eec1b31be32d44e8b03759fcfc11ffc83`Seal`candidate-manifest.json` SHA `0dac77b18fde597cadac1660f246be2757c89093683e61b1ab30a0e2951c649b`,Practical build/validation`result.json` SHA `b66b388aa1e9234e957d2d733d3de405af30d10c01ad6d6f242bb3f8769ea0fb`2 files plus`delta.patch` SHA `098147ac281eac3b975355d7d0b010c0955b2964eb1ad644c45445def069e846`. Manifest contains original 9 Sourcehash, accurate flags, compiler actual 7 Private include and counterpart identity.

Old Stderr`reader_open stage=stereo code=66...`holding;only adding one line to failure`reader_stereo_open_v1 key=value...`. stdout, exit 64／65／66／2／0 And the original 400 ms Match/Dual Sourceage Code maintained, diagnosis cannot be retried or changed. The original two Open sequences are kept and mutex failed to continue using the original amapping; access rights, MapView length, zero waiting, copying and packet checking order are maintained. Diagnosis only consumes immediate results as originally called and copied packaget without adding object opening, sample reading, QPC query, waiting or permission.

| Fields | Interpretation and validity |
| --- | --- |
| reason | The first actual rejection position of the original open/first reading path; not final cause proof |
| Called/ok/win by API 32_valid／win 32 | Only after failure to call, follow-up Win 32 Read LastError before calling; not using olderrr as valid when successfully called |
| wait_called／wait_result | The blogger says that the government is not a party to the law.0; WAILD only has effective Win 32 Error |
| Releasing Fields | Original release call result; failure of ordinary copy release did not prevent acceptance and this candidate has not added the door |
| Packet_copid and packet fields | Only from original copied values; default zero is not valid packet if not copied, and copying does not represent subsequent verification approval |
| qpc field/observed_frequency | The results of the original packet verifications were available from QueryPerformanceFrequency, without adding additional queries |

WAIT_ABANDONED is still released and rejected on its original path. Failed record kept after the previous close/cleanup; follow-up normal`read`The first open diagnosis is not covered.4 No change, the new members fall only into this private exe category.

Directed validation using original mechanical test view and candidate 19 Win.32 Subsistence scene contrast, compare return, call order, name/authority/magnification size/0 Wait and cover each package that is rejected, cleanup over LastError, successful API old error, ReleaseMutex failed to accept the diagnosis, and normal read does not overwrite the diagnosis. Test view only removes the duplicate SteeoFrame include from its original head, then changes the original file by macrorename category. All packet tests its static bytes without Game, XR or target process RAM reading.

Use the current Python process ' s own real PID/create time to execute the original exe and candidate each 3 Small CLI check: Parameters 64, illegal parameters 64 Short of your own Stereo object 66;exiting in line with emptystdout, and candidate for the correct recording of the error on OpenMutex/OpenFileMapping 2. This check does not create a Bridger, does not replay controller commands or verify historical HI failure. No unmodified package of controller observer returns. One round of independent Astra read-only incremental reviews is P-free 1／P 2, still not the operational cause of conclusion.

Next step is to pool this reader, and Body's capture rejection and temporary saving.`5fdc2f`Sample candidate is verified in a serial with the original template. This round delivers only diagnostic products, without new Game/SDK sessions, freeness conclusions, play-through or default integration; do not reset right hand commands that have closed an unknown execution status on an HI session.

---

## 中文

# 私有控制器reader的Stereo打开失败诊断

HI右手恢复的退出66已经限定在原`StereoBridge::open`，尚未进入400ms的Source1／age匹配循环。旧stderr只记stage=stereo，不能区分打开mutex、mapping、映射视图、零等待或首份packet检查失败，也不能据错误文案认定freshness过期。

本候选从实际`0c707034…fe179e` reader的冻结9份源和原MSVC flags派生，只改变私有`stereo_bridge.h`与`reader.cpp`。原7份源／测试、原exe和旧回执均保留；共享headers、Native、XR、renderer、正常启动器不改。所有源、工件和测试派生都位于ignored `analysis/stereo-reader-open-diagnostic-20261027/`，不属于发布包。

选定exe：`validation-1793056273353595900/reader.exe`，SHA256 `01d8b9db36bfa57b580dc98d47e0969eec1b31be32d44e8b03759fcfc11ffc83`。封存`candidate-manifest.json` SHA `0dac77b18fde597cadac1660f246be2757c89093683e61b1ab30a0e2951c649b`，实际构建／验证`result.json` SHA `b66b388aa1e9234e957d2d733d3de405af30d10c01ad6d6f242bb3f8769ea0fb`，两文件增量`delta.patch` SHA `098147ac281eac3b975355d7d0b010c0955b2964eb1ad644c45445def069e846`。manifest包含原9源hash、准确flags、编译器实际7个私有include和对应源身份。

原stderr行`reader_open stage=stereo code=66...`保持；失败时只追加一行`reader_stereo_open_v1 key=value...`。stdout、退出64／65／66／2／0与原400ms匹配／双来源age代码保持，诊断不能重试或改变接受条件。原两次Open顺序保持，mutex失败也继续执行原mapping打开；访问权限、MapView长度、零等待、复制与packet校验次序保持。诊断只消费原调用的即时结果和已经复制的packet，不增加对象打开、样本读取、QPC查询、等待或权限。

| 字段 | 解释与有效性 |
| --- | --- |
| reason | 原open/初读路径实际首个拒绝位置；不是最终根因证明 |
| 各API的called／ok／win32_valid／win32 | 只在失败调用之后、后续Win32调用之前读LastError；成功调用不把旧error当有效值 |
| wait_called／wait_result | 原WaitForSingleObject(mutex,0)的结果；仅WAIT_FAILED有有效Win32错误 |
| release字段 | 原释放调用结果；普通copy后的释放失败原来未阻止接受，本候选也不增加此门 |
| packet_copied及packet字段 | 只来自原已复制值；没有复制时默认零不是有效packet，已复制也不代表后继校验通过 |
| qpc字段／observed_frequency | 原packet校验中既有QueryPerformanceFrequency的结果，没有增加查询 |

WAIT_ABANDONED仍按原路径释放并拒绝。失败记录在原close/cleanup之后保留；后续普通`read`不覆盖初次open诊断。packet ABI与原版本4不变，新增成员只属于此私有exe中的接收类。

定向验证使用原头的机械测试视图与候选做19个Win32替身场景对照，比较返回、调用顺序、名称／权限／映射大小／0等待，并覆盖每个packet首拒、cleanup覆盖LastError、成功API陈旧错误、ReleaseMutex失败仍接受、普通read不覆盖诊断。测试视图只从原头删除重复的StereoFrame include，再通过宏重命名类，不修改原文件。所有packet均为测试自身静态字节，没有Game、XR或目标进程RAM读取。

另用当前Python进程自身的真实PID／创建时间执行原exe和候选各3个小CLI检查：参数64、非法参数64、缺自身Stereo对象66；退出与空stdout一致，候选如实记录原OpenMutex/OpenFileMapping的错误2。此检查不创建bridge producer，不重放控制器命令，也不验证历史HI失败原因。没有重跑未改的整套controller observer回归。一轮独立Astra只读增量审阅无P1／P2，仍不是运行时根因结论。

下一步由统筹将此reader、Body的capture首拒候选和已暂存`5fdc2f`采样候选按原模板一次串行验证。此轮只交付诊断产物，没有新Game／SDK会话、freshness结论、玩法通过或默认整合；不重放已关闭HI会话中未知执行状态的右手命令。
