"""Lancer authoring panel. Standard-library only; never writes game memory."""
from __future__ import annotations

import argparse
import copy
import json
import math
import os
from pathlib import Path
import struct
import tempfile
import time

SIGNATURE = "3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193"
SPACE = "main_grip_forward_right_up_metres"
FINGERS = ("食指", "中指", "无名指", "小指", "拇指")
PARTS = ("main_grip", "support", "magazine", "charging", "starter")
PART_NAMES = ("主握把 · 右手", "护木 · 左手", "弹匣 · 左手", "拉栓 E · 左手", "电锯握柄 C · 左手")


def baseline(part="main_grip"):
    return {"schema": 2, "weapon": "Lancer", "side": "right" if part=="main_grip" else "left", "part": part,
            "geometry": SIGNATURE, "space": SPACE, "rotation_order": "Rz_Ry_Rx_degrees",
            "palm_metres": [0., 0., 0.], "palm_degrees": [0., 0., 0.],
            "finger_degrees": [[0., 0., 0.] for _ in range(15)]}


def quaternion(angles):
    x, y, z = [math.radians(v) / 2 for v in angles]
    sx, sy, sz = math.sin(x), math.sin(y), math.sin(z)
    cx, cy, cz = math.cos(x), math.cos(y), math.cos(z)
    return (sx*cy*cz-cx*sy*sz, cx*sy*cz+sx*cy*sz,
            cx*cy*sz-sx*sy*cz, cx*cy*cz+sx*sy*sz)


def validate(profile):
    if not isinstance(profile, dict):
        raise ValueError("姿势文件必须是一个对象")
    if profile.get("part") not in PARTS:
        raise ValueError("请选择受支持的 Lancer 部件")
    base = baseline(profile["part"])
    if set(profile) != set(base):
        raise ValueError("姿势字段缺失或包含未知字段")
    for key in ("schema", "weapon", "side", "part", "geometry", "space", "rotation_order"):
        if profile.get(key) != base[key]:
            raise ValueError(f"不支持的 {key}；需要原骨架 Lancer 对应部件")

    def vector(v):
        if not isinstance(v, list) or len(v) != 3:
            raise ValueError("每个位置／角度必须有三个数值")
        if any(type(x) not in (int, float) or not math.isfinite(x) for x in v):
            raise ValueError("数值必须有限，不能包含 NaN 或无穷大")
    vector(profile["palm_metres"])
    if math.hypot(*profile["palm_metres"]) > .05:
        raise ValueError("掌心位移总长度不能超过 50 mm")
    vector(profile["palm_degrees"])
    if abs(quaternion(profile["palm_degrees"])[3]) < math.cos(math.radians(45)/2):
        raise ValueError("掌心合成旋转不能超过 45°")
    joints = profile["finger_degrees"]
    if not isinstance(joints, list) or len(joints) != 15:
        raise ValueError("必须包含十五个手指关节")
    for joint in joints:
        vector(joint)
        if abs(quaternion(joint)[3]) < math.cos(math.radians(90)/2):
            raise ValueError("单关节合成修正不能超过 90°")
    return profile


def checksum(data):
    h = 14695981039346656037
    for b in data:
        h = ((h ^ b) * 1099511628211) & ((1 << 64)-1)
    return h


def encode(profile, revision, enabled, volumes=True, poses=True):
    validate(profile)
    if type(revision) is not int or not 0 < revision < 1 << 64:
        raise ValueError("版本号无效")
    values = list(profile["palm_metres"]) + list(quaternion(profile["palm_degrees"]))
    for joint in profile["finger_degrees"]:
        values.extend(quaternion(joint))
    flags = PARTS.index(profile["part"]) | (256 if volumes else 0) | (512 if poses else 0)
    payload = b"GVRPOSE2" + struct.pack("<QII32s67d", revision, int(enabled), flags,
                                       bytes.fromhex(SIGNATURE), *values)
    assert len(payload) == 592
    return payload + struct.pack("<Q", checksum(payload))


def atomic_write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=path.name+".", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def save_profile(path, profile):
    validate(profile)
    atomic_write(path, (json.dumps(profile, ensure_ascii=False, indent=2)+"\n").encode("utf-8"))


class Editor:
    def __init__(self, root, channel):
        import tkinter as tk
        from tkinter import ttk
        self.tk, self.ttk, self.root = tk, ttk, root
        self.channel = Path(channel).absolute()
        self.profile = baseline()
        self.profiles = {part: baseline(part) for part in PARTS}
        self.part_name = tk.StringVar(value=PART_NAMES[0])
        self.show_volumes = tk.BooleanVar(value=True)
        self.show_poses = tk.BooleanVar(value=True)
        self.undo_stack, self.redo_stack = [], []
        self.revision = 0
        self.pending = None
        self.updating = False
        self.live = tk.BooleanVar(value=True)
        self.selected = tk.StringVar(value="食指 · 根节")
        self.message = tk.StringVar(value="离线编辑：尚未发送预览")
        self.connection = tk.StringVar(value="未连接运行时")
        self.receipts = tk.StringVar(value="配置接收 —    手指事务 —    枪体写入 —")
        self.variables = {}
        self.session = None
        self.last_counter = None
        self.last_progress = 0.
        root.title("GearsVR · Lancer 实时 Grab 编辑器")
        root.geometry("1030x830")
        root.minsize(950, 760)
        style = ttk.Style(root)
        style.theme_use("clam")
        style.configure("TLabel", font=("Microsoft YaHei UI", 10))
        style.configure("Title.TLabel", font=("Microsoft YaHei UI", 19, "bold"))
        style.configure("TButton", padding=7)
        canvas = tk.Canvas(root, highlightthickness=0)
        scroll = ttk.Scrollbar(root, orient="vertical", command=canvas.yview)
        scroll.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)
        canvas.configure(yscrollcommand=scroll.set)
        main = ttk.Frame(canvas, padding=22)
        item = canvas.create_window(0, 0, window=main, anchor="nw")
        main.bind("<Configure>", lambda _: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind("<Configure>", lambda event: canvas.itemconfigure(item, width=event.width))
        root.bind("<MouseWheel>", lambda event: canvas.yview_scroll(-int(event.delta/120), "units"))
        ttk.Label(main, text="LANCER / GRAB VOLUME + GRAB POSE", style="Title.TLabel").pack(anchor="w")
        ttk.Label(main, text="调整原有姿势的偏移 · 固定骨长 · 原执行路径", foreground="#526477").pack(anchor="w", pady=(4, 13))
        bar = ttk.Frame(main)
        bar.pack(fill="x")
        ttk.Checkbutton(bar, text="实时预览", variable=self.live, command=self.toggle_live).pack(side="left")
        for label, command in (("发送当前", self.publish), ("恢复原版基线", self.restore),
                               ("撤销", self.undo), ("重做", self.redo),
                               ("载入姿势…", self.load), ("保存姿势…", self.save)):
            ttk.Button(bar, text=label, command=command).pack(side="left", padx=3)
        options = ttk.Frame(main)
        options.pack(fill="x", pady=(10, 0))
        choose_part = ttk.Combobox(options, textvariable=self.part_name, values=PART_NAMES, state="readonly", width=24)
        choose_part.pack(side="left", padx=(0, 12))
        choose_part.bind("<<ComboboxSelected>>", lambda _: self.change_part())
        ttk.Checkbutton(options, text="显示 Grab Volume", variable=self.show_volumes, command=self.schedule).pack(side="left")
        ttk.Checkbutton(options, text="显示所选 Grab Pose 骨架", variable=self.show_poses, command=self.schedule).pack(side="left", padx=12)
        channel_row = ttk.Frame(main)
        channel_row.pack(fill="x", pady=(12, 4))
        self.channel_label = ttk.Label(channel_row, text=str(self.channel), foreground="#526477")
        self.channel_label.pack(side="left", fill="x", expand=True)
        ttk.Button(channel_row, text="选择会话目录…", command=self.choose_channel).pack(side="right")
        ttk.Label(main, textvariable=self.connection).pack(anchor="w")
        ttk.Label(main, textvariable=self.receipts, foreground="#345f83").pack(anchor="w", pady=(3, 10))
        palm = ttk.LabelFrame(main, text="所选部件的吸附后姿势偏移", padding=12)
        palm.pack(fill="x")
        ttk.Label(palm, text="主握把使用 Grip 局部轴，会影响枪体与枪口；左手部件使用腕局部轴，调整现有合法抓握的皮肤。",
                  foreground="#79542c").grid(row=0, column=0, columnspan=4, sticky="w", pady=(0, 7))
        for i, axis in enumerate("XYZ"):
            self.control(palm, i+1, "p"+axis, axis+" 位移 (mm)", -50, 50)
            self.control(palm, i+4, "r"+axis, axis+" 旋转 (°)", -45, 45)
        hand = ttk.LabelFrame(main, text="手指姿势：相对当前合法握持姿势的局部旋转", padding=12)
        hand.pack(fill="x", pady=(13, 0))
        names = [f"{finger} · {joint}" for finger in FINGERS for joint in ("根节", "中节", "末节")]
        choose = ttk.Combobox(hand, textvariable=self.selected, values=names, state="readonly", width=25)
        choose.grid(row=0, column=0, sticky="w", pady=(0, 8))
        choose.bind("<<ComboboxSelected>>", lambda _: self.refresh())
        self.names = names
        ttk.Label(hand, text="每根手指 3 节；仅修改旋转，保留原平移和缩放。", foreground="#526477").grid(row=0, column=1, columnspan=3, sticky="w")
        for i, axis in enumerate("XYZ"):
            self.control(hand, i+1, "f"+axis, axis+" 修正 (°)", -90, 90)
        tools = ttk.Frame(hand)
        tools.grid(row=4, column=0, columnspan=4, sticky="w", pady=(8, 0))
        ttk.Button(tools, text="清除此关节", command=self.clear_joint).pack(side="left")
        ttk.Button(tools, text="当前手指三节 +5° Z", command=lambda: self.bend_finger(5)).pack(side="left", padx=6)
        ttk.Button(tools, text="当前手指三节 −5° Z", command=lambda: self.bend_finger(-5)).pack(side="left")
        ttk.Label(main, textvariable=self.message, wraplength=950, foreground="#234d70").pack(anchor="w", pady=(14, 4))
        ttk.Label(main, text="红色：实际吸附区域；青／紫：插入与保持范围；黄色：所选 Grab Pose 骨架。\n"
                  "关闭窗口会发送恢复指令；保留调校请保存姿势。预览骨架不产生抓取权限。",
                  foreground="#526477").pack(anchor="w", pady=(5, 0))
        self.refresh()
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(250, self.poll)
        self.schedule()

    def change_part(self):
        self.profiles[self.profile["part"]] = copy.deepcopy(self.profile)
        part = PARTS[PART_NAMES.index(self.part_name.get())]
        self.profile = copy.deepcopy(self.profiles[part])
        self.undo_stack.clear()
        self.redo_stack.clear()
        self.refresh()
        self.schedule()

    def control(self, parent, row, key, label, low, high):
        tk, ttk = self.tk, self.ttk
        variable = tk.DoubleVar(value=0.)
        self.variables[key] = variable
        ttk.Label(parent, text=label, width=17).grid(row=row, column=0, sticky="w")
        slider = ttk.Scale(parent, variable=variable, from_=low, to=high,
                           command=lambda _: self.changed())
        slider.grid(row=row, column=1, sticky="ew", padx=12, pady=4)
        spin = ttk.Spinbox(parent, textvariable=variable, from_=low, to=high, increment=.5, width=9,
                          command=self.changed)
        spin.grid(row=row, column=2)
        spin.bind("<Return>", lambda _: self.changed())
        spin.bind("<FocusOut>", lambda _: self.changed())
        ttk.Button(parent, text="归零", command=lambda: self.zero(key)).grid(row=row, column=3, padx=(8, 0))
        parent.columnconfigure(1, weight=1)

    def refresh(self):
        self.updating = True
        joint = self.names.index(self.selected.get())
        for i, axis in enumerate("XYZ"):
            self.variables["p"+axis].set(round(self.profile["palm_metres"][i]*1000, 3))
            self.variables["r"+axis].set(round(self.profile["palm_degrees"][i], 3))
            self.variables["f"+axis].set(round(self.profile["finger_degrees"][joint][i], 3))
        self.updating = False

    def remember(self):
        self.undo_stack.append(copy.deepcopy(self.profile))
        self.undo_stack = self.undo_stack[-150:]
        self.redo_stack.clear()

    def changed(self):
        if self.updating:
            return
        candidate = copy.deepcopy(self.profile)
        joint = self.names.index(self.selected.get())
        try:
            for i, axis in enumerate("XYZ"):
                candidate["palm_metres"][i] = self.variables["p"+axis].get()/1000
                candidate["palm_degrees"][i] = self.variables["r"+axis].get()
                candidate["finger_degrees"][joint][i] = self.variables["f"+axis].get()
            validate(candidate)
        except (ValueError, self.tk.TclError) as exc:
            if self.pending:
                self.root.after_cancel(self.pending)
                self.pending = None
            self.message.set(f"未发送：{exc}")
            return
        if candidate != self.profile:
            self.remember()
            self.profile = candidate
            self.schedule()

    def zero(self, key):
        self.variables[key].set(0.)
        self.changed()

    def schedule(self):
        if self.pending:
            self.root.after_cancel(self.pending)
        self.pending = None
        if self.live.get():
            self.pending = self.root.after(140, self.publish)
        else:
            self.message.set("草稿已修改；勾选实时预览或点“发送当前”查看效果")

    def publish(self, enabled=True):
        if self.pending:
            self.root.after_cancel(self.pending)
        self.pending = None
        try:
            self.revision = max(time.time_ns(), self.revision+1)
            atomic_write(self.channel/"preview.bin", encode(self.profile, self.revision, enabled, self.show_volumes.get(), self.show_poses.get()))
            self.message.set(f"已发送版本 {self.revision}；等待实际运行时回执" if enabled else "已发送恢复基线；等待运行时确认")
        except (OSError, ValueError) as exc:
            self.message.set(f"发送失败：{exc}")
            return False
        return True

    def toggle_live(self):
        self.publish(self.live.get())

    def restore(self):
        self.remember()
        self.profiles = {part: baseline(part) for part in PARTS}
        self.profile = baseline(PARTS[PART_NAMES.index(self.part_name.get())])
        self.refresh()
        self.live.set(False)
        self.publish(False)

    def undo(self):
        if self.undo_stack:
            self.redo_stack.append(copy.deepcopy(self.profile))
            self.profile = self.undo_stack.pop()
            self.refresh()
            self.schedule()

    def redo(self):
        if self.redo_stack:
            self.undo_stack.append(copy.deepcopy(self.profile))
            self.profile = self.redo_stack.pop()
            self.refresh()
            self.schedule()

    def clear_joint(self):
        self.remember()
        self.profile["finger_degrees"][self.names.index(self.selected.get())] = [0., 0., 0.]
        self.refresh()
        self.schedule()

    def bend_finger(self, amount):
        candidate = copy.deepcopy(self.profile)
        start = self.names.index(self.selected.get())//3*3
        for joint in candidate["finger_degrees"][start:start+3]:
            joint[2] += amount
        try:
            validate(candidate)
        except ValueError as exc:
            self.message.set(str(exc))
            return
        self.remember()
        self.profile = candidate
        self.refresh()
        self.schedule()

    def save(self):
        from tkinter import filedialog
        path = filedialog.asksaveasfilename(defaultextension=".json", initialfile="lancer-right-main.json",
                                          filetypes=[("Pose JSON", "*.json")])
        if path:
            try:
                save_profile(path, self.profile)
                self.message.set(f"已保存姿势：{path}")
            except (OSError, ValueError) as exc:
                self.message.set(str(exc))

    def load(self):
        from tkinter import filedialog
        path = filedialog.askopenfilename(filetypes=[("Pose JSON", "*.json")])
        if path:
            try:
                candidate = validate(json.loads(Path(path).read_text(encoding="utf-8-sig")))
                self.remember()
                self.profile = candidate
                self.part_name.set(PART_NAMES[PARTS.index(candidate["part"])])
                self.profiles[candidate["part"]] = copy.deepcopy(candidate)
                self.refresh()
                self.schedule()
            except (OSError, ValueError) as exc:
                self.message.set(f"载入失败：{exc}")

    def choose_channel(self):
        from tkinter import filedialog
        path = filedialog.askdirectory(title="选择此私有运行会话的 PoseEditor channel 目录")
        if path and Path(path).absolute() != self.channel:
            if self.revision and not self.publish(False):
                return
            self.live.set(False)
            self.channel = Path(path).absolute()
            self.channel_label.configure(text=str(self.channel))
            self.session = None
            self.revision = 0
            self.connection.set("目录已切换；等待此会话运行时")

    def poll(self):
        try:
            path = self.channel/"status.json"
            status = json.loads(path.read_text(encoding="utf-8"))
            recent = time.time()-path.stat().st_mtime < 2.5 and status.get("running") is True
            session = (status["pid"], status["creation"])
            if self.session and self.session != session:
                self.live.set(False)
                self.message.set("运行时身份改变：实时预览已关闭；请确认新会话后重新发送")
                self.publish(False)
            self.session = session
            counter = (status.get("finger_compose"), status.get("mount_frame"), status.get("preview_frame"))
            if counter != self.last_counter:
                self.last_progress = time.monotonic()
                self.last_counter = counter
            moving = time.monotonic()-self.last_progress < 2.5
            self.connection.set(f"{'已连接' if recent else '运行时已停止／回执过期'} · PID {session[0]} · "
                                f"{'预览启用' if status.get('enabled') else '基线模式'} · {status.get('input_status')}")
            def result(field):
                if not recent:
                    return "回执过期"
                return "本版本 ✓" if self.revision and status.get(field) == self.revision else "等待本版本"
            self.receipts.set(f"配置接收：{result('received_revision')}    手指事务：{result('finger_revision')}    "
                              f"Grab Pose 绘制：{result('preview_revision')}    {'有新事务' if moving and recent else '暂无新事务'}")
        except (OSError, ValueError, KeyError):
            self.connection.set("未连接运行时 · 可以离线编辑和保存姿势")
            self.receipts.set("配置接收 —    手指事务 —    枪体写入 —")
        self.root.after(250, self.poll)

    def close(self):
        if self.pending:
            self.root.after_cancel(self.pending)
        if self.revision and not self.publish(False):
            from tkinter import messagebox
            messagebox.showerror("恢复基线未送达", "无法写入恢复指令。请恢复目录访问后再次关闭窗口。")
            return
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--channel", type=Path, default=Path(__file__).parent/"session")
    parser.add_argument("--emit-baseline", type=Path)
    args = parser.parse_args()
    if args.emit_baseline:
        save_profile(args.emit_baseline, baseline())
        return
    import tkinter as tk
    root = tk.Tk()
    Editor(root, args.channel)
    root.mainloop()


if __name__ == "__main__":
    main()
