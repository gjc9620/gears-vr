# Candidate startup armspan calibration

This path is limited to a source-worktree 6DoF candidate. The frozen 0.1.0
manifest, installed launcher EXE, native/XR artifacts, and portable-package
allowlist are unchanged. Enable both boolean capabilities
`controller_input_observer` and `armspan_startup_calibration` in a separately
validated candidate manifest, together with `startup_menu` and the existing
crosshair/UI/anchor requirements. Do not turn these on for an XR build without
the existing `--controller-input` publisher.

The launcher UI shows bilingual instructions and progress. The CLI uses the
same gate; `start --remeasure-armspan` requests a replacement measurement.
Missing or invalid records require measurement even without that flag. A valid
record is reused without opening a calibration XR child.

The first-run sequence is:

1. Reserve the owned startup-menu game through the existing launcher path.
2. Start one owned `OpenXRPoseProbe.exe` with its existing continuous-session,
   owner identity, startup-menu window, and controller-input options. Native
   gameplay activation has not occurred.
3. Copy Stereo-v4 and GVC3 mappings under their existing mutexes through a
   read-only view. Validate the exact hand/stereo ticket, ring retention,
   process creation times, publisher identity, QPC frequency, runtime source0,
   focus, tracking, geometry, and maximum 100 ms age. Mismatched protocol or
   process identity aborts; temporary tracking loss resets the hold.
4. Require both squeezes at least 0.8, arms on their proper sides in a
   heading-relative T-pose, a plausible 0.9–2.4 m span, and a steady pose within
   8 cm. The hold needs two uninterrupted seconds and at least 60 unique
   samples. Repeated polling cannot pad the count. Session, reference-space,
   input-profile, and input-epoch changes start a new hold.
5. Immediately write schema1 to
   `%LOCALAPPDATA%/GearsVR/weapon-calibration.json` using a flushed temporary file
   in the same directory and `os.replace`. Existing bytes are preserved until
   replacement succeeds. A changed record is refused rather than overwritten.
6. Close the consumer's own mapping/mutex handles, stop and wait for its own XR
   child, and require orderly exit before handing off. Timeout, cancellation,
   early exit, identity change, or cleanup failure prevents Native activation.
   Failure before a successful measurement and atomic save preserves the prior
   record. Once saved, the new valid record remains even if later XR cleanup
   fails; Native still stays inactive. No rollback is attempted, because it
   could overwrite another writer's intervening update.
7. Load the saved record into the normal runner before its Native command is
   built. The activation callback requires the same valid value, closing the
   record-disappearance/replacement gap during handoff.

The profile has exactly `schema`, `armspan_metres`, and
`hands_below_head_metres`. CLI and UI reject duplicate keys, invalid UTF-8,
boolean/string numbers, nonfinite values, unsupported schema, extra keys, and
out-of-range measurements. The record uses UTF-8 without BOM, matching the
frozen Native consumer. Autonomous behind-window tests cannot create a
personal measurement; they may reuse an existing valid profile.

Validation performed without a game or IDA:

```powershell
py -3 -m unittest discover -s tools/vr-launcher -p 'test_armspan*.py' -v
py -3 -m unittest discover -s tools/vr-launcher -p test_launcher.py -v
py -3 -m unittest discover -s tools/native-capture -p test_weapon_calibration.py -v
powershell -NoProfile -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1 -OutputDirectory analysis/vr-launcher-armspan -NoInstall
```

`armspan_wire_layout_test.cpp` is a standalone x64 `/W4 /WX` compile against the
existing C++ publisher headers; no CMake/default build change is needed. The
Python mapping tests use temporary uniquely named Windows objects and verify
that the last consumer close removes them. Startup lifecycle tests use fake
children, explicitly check wait/cleanup order and immediate persistence, and
never spawn OpenXR or the game.

Dedicated headset instructions, a headset progress display, slot try-grab
confirmation, real-controller runtime acceptance, and packaging are pending.
The saved span is a controller-to-controller T-pose measurement; this does not
establish character IK correctness or hand-animation ownership.

---

## 中文

# 候选人启动武器校准

此路径仅限于源头工作树 6 联邦情报局候选人 冻结了 0.1.0
清单、安装的发射装置EXE、原生/XR文物和便携式包装
允许列表不变。启用两个布尔能力
`controller_input_observer` and `armspan_startup_calibration` 单独使用
合格候选人名单及 `startup_menu` 和现有的
横线/ UI/ 锁定要求。 XR 建构时不要打开这些要求, 不需要
现有 `--controller-input` publisher.

发射装置UI显示双语指示和进展。
同一大门; `start --remeasure-armspan` 要求替换计量。
缺少或无效的记录即使没有这个标记也需要测量。
记录在不打开校准XR子的情况下被重新使用。

第一运行顺序是:

1. 通过现有的启动器路径保留自有的启动菜单游戏 。
2. 开始一个自有 `OpenXRPoseProbe.exe` 及其现有的连续会议,
   拥有者身份、启动菜单窗口和控制器输入选项。 原生
   游戏游戏激活尚未实现 。
3. 复制立体声五4 和 全球价值链3 其现有变种下通过一个
   验证准确的手/立体票、戒指保留、
   进程创建时间、发布者身份、QPC频率、运行时间源0,
   焦点、跟踪、几何和最大值 100 ms 年龄。 协议不合时宜或
   进程身份中止; 临时跟踪丢失重置控件。
4. 需要至少两个挤压 0.8 手臂放在右侧
   标题相对的 T- pose, 一个可信的 0.9–2.4 平面,平面在内部
   8 cm. 搁置需要两秒钟,至少 60 unique
   。重复的投票无法支持计数。
   输入配置,输入时段更改开始新的抓取。
5. 立即写出计划1 to
   `%LOCALAPPDATA%/GearsVR/weapon-calibration.json` 使用冲洗的临时文件
   在同一目录中, `os.replace`中。将保留到
   替换成功。拒绝更改记录,而不是覆盖。
6. 关闭消费者自己的绘图/mutex手柄,停止并等待自己的XR
   暂停 取消 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停 暂停
   提前退出,身份改变,或清理失败,阻止了原生激活.
   成功测量之前的失败和原子保存保留了先前的
   记录。保存后,即使稍后XR清理,新有效记录也仍然有效
   失败; 本地仍然不活动。 没有尝试回滚, 因为
   可能覆盖另一个作家的干预更新。
7. 在保存的记录上载到正常的运行器中, 然后再使用本地命令 。
   。激活回调需要相同的有效值,关闭
   移交期间记录消失/更换缺口。

档案里有 `schema`, `armspan_metres`,以及
`hands_below_head_metres`。CLI和UI拒绝重复密钥,无效的UTF -8,
布尔/字符串数字、非无限值、不支持的计数器、额外密钥,以及
记录使用UTF-8 匹配到
本地消费者。自动窗口后测试无法创建
个人测量;它们可能重新使用现有有效剖面。

未经游戏或IDA而执行的验证 :

```powershell
py -3 -m unittest discover -s tools/vr-launcher -p 'test_armspan*.py' -v
py -3 -m unittest discover -s tools/vr-launcher -p test_launcher.py -v
py -3 -m unittest discover -s tools/native-capture -p test_weapon_calibration.py -v
powershell -NoProfile -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1 -OutputDirectory analysis/vr-launcher-armspan -NoInstall
```

`armspan_wire_layout_test.cpp` 是一个独立的 x64 `/W4 /WX` 对照
现有的 C++ 发布器头; 不需要 CMake/ 默认构建更改。
Python 映射测试使用临时的、唯一命名的 Windows 对象并验证
最后一个消费者关闭它们。
儿童,明确检查等候/清理命令和立即坚持不懈;
永远不要产下OpenXR或游戏.

专用耳机指令, 耳机进度显示, 插槽尝试
正在等待确认、实时控制器运行时间接收和包装。
保存的跨度是一个控制器到控制器 T-pose 的测量; 这不是
确定字符IK正确性或手动所有权。
