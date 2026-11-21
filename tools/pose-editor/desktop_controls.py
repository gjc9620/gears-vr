"""Human-operated controls for this candidate's isolated desktop simulator only."""
from __future__ import annotations

import json
import math
import os
from pathlib import Path
import time
import tkinter as tk
from tkinter import ttk


def hand_defaults(side):
    return dict(hand=side, posX=.2 if side else -.2, posY=-.3, posZ=-.4,
                yaw=0., pitch=-.3, roll=0., worldSpace=0, trigger=0., squeeze=0.,
                buttonA=0, buttonB=0, menuButton=0, stickClick=0, stickX=0., stickY=0.)


class Commands:
    """One pending command at a time; never overwrite an unread button edge."""
    def __init__(self, directory):
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=True)
        self.hands = [hand_defaults(0), hand_defaults(1)]
        self.queue = []
        self.inflight = None
        self.head = dict(x=0., y=1.7, z=0., yaw=0., pitch=0., roll=0.)

    def enqueue(self, side, callback=None):
        payload = dict(self.hands[side])
        if not all(math.isfinite(float(v)) for v in payload.values()):
            raise ValueError("Controller values must be finite")
        # Slider motion can coalesce. A button edge is always a separate item.
        if callback is None and self.queue and self.queue[-1][1] is None and self.queue[-1][0]['hand'] == side:
            self.queue[-1] = (payload, None)
        else:
            self.queue.append((payload, callback))

    def tick(self):
        target = self.directory/'controller_pose_command.json'
        if self.inflight and not target.exists():
            _, callback = self.inflight
            self.inflight = None
            if callback:
                callback()
        if self.inflight or target.exists() or not self.queue:
            return
        payload, callback = self.queue[0]
        raw = json.dumps(payload, separators=(',', ':'), allow_nan=False).encode()
        if len(raw) >= 511:
            raise ValueError("Controller command exceeds simulator reader bound")
        temporary = target.with_suffix('.tmp')
        temporary.write_bytes(raw)
        os.replace(temporary, target)
        self.queue.pop(0)
        self.inflight = payload, callback

    def heartbeat(self):
        temporary = self.directory/'desktop-heartbeat.tmp'
        temporary.write_text('active', 'ascii')
        os.replace(temporary, self.directory/'desktop-heartbeat')

    def release(self):
        self.queue.clear()
        for side in range(2):
            for name in ('trigger', 'squeeze', 'buttonA', 'buttonB', 'menuButton', 'stickClick', 'stickX', 'stickY'):
                self.hands[side][name] = 0
            self.enqueue(side)


class DesktopControls:
    def __init__(self, root, directory):
        self.root = root
        self.window = tk.Toplevel(root)
        self.window.title('GearsVR · 桌面双手控制（模拟器）')
        self.window.geometry('700x690')
        self.window.minsize(620, 600)
        self.window.protocol('WM_DELETE_WINDOW', self.hide)
        self.commands = Commands(directory)
        self.closed = False
        self.variables = [{}, {}]
        self.last_heartbeat = 0.
        self.status = tk.StringVar(value='等待本次桌面模拟器启动')
        ttk.Label(self.window, text='不用 VR：先移动手，再抓握，最后在 Editor 调手型',
                  font=('Microsoft YaHei UI', 12, 'bold')).pack(anchor='w', padx=14, pady=(14, 6))
        ttk.Label(self.window, text='位置相对视点：X 向右，Y 向上，Z 向后。位置单位 cm，角度单位 °。\n'
                  'Grip／扳机是保持开关；亮起后可切到 Editor，按“全部松开”结束抓握。',
                  wraplength=660).pack(anchor='w', padx=14)
        notebook = ttk.Notebook(self.window)
        notebook.pack(fill='both', expand=True, padx=14, pady=8)
        for side, title in ((1, '右手 · 主握把'), (0, '左手 · 护木／弹匣／E／C')):
            frame = ttk.Frame(notebook, padding=12)
            notebook.add(frame, text=title)
            frame.columnconfigure(1, weight=1)
            for row, (key, label, low, high, scale) in enumerate((
                    ('posX', '左右 X', -110, 110, 100), ('posY', '上下 Y', -130, 50, 100),
                    ('posZ', '前后 Z', -130, 60, 100), ('yaw', '左右转 Yaw', -180, 180, 180/math.pi),
                    ('pitch', '抬低 Pitch', -180, 180, 180/math.pi), ('roll', '翻腕 Roll', -180, 180, 180/math.pi))):
                value = tk.DoubleVar(value=self.commands.hands[side][key]*scale)
                self.variables[side][key] = value, scale
                ttk.Label(frame, text=label, width=13).grid(row=row, column=0, sticky='w')
                ttk.Scale(frame, from_=low, to=high, variable=value,
                          command=lambda _, s=side: self.move(s)).grid(row=row, column=1, sticky='ew', padx=6, pady=6)
                spin = ttk.Spinbox(frame, from_=low, to=high, increment=1., width=8, textvariable=value,
                                   command=lambda s=side: self.move(s))
                spin.grid(row=row, column=2)
                spin.bind('<Return>', lambda _, s=side: self.move(s))
                spin.bind('<FocusOut>', lambda _, s=side: self.move(s))
            buttons = ttk.Frame(frame)
            buttons.grid(row=6, column=0, columnspan=3, sticky='w', pady=10)
            for label, key in (('Grip 抓握保持', 'squeeze'), ('扳机／捏取保持', 'trigger')):
                variable = tk.BooleanVar()
                ttk.Checkbutton(buttons, text=label, variable=variable,
                                command=lambda s=side, k=key, v=variable: self.toggle(s, k, v.get())).pack(side='left', padx=5)
                self.variables[side][key] = variable, 1
            actions = ttk.Frame(frame)
            actions.grid(row=7, column=0, columnspan=3, sticky='w', pady=5)
            for label, key in ((('A' if side else 'X')+' 按一下', 'buttonA'),
                               (('B' if side else 'Y')+' 按一下', 'buttonB'), ('菜单 按一下', 'menuButton')):
                ttk.Button(actions, text=label, command=lambda s=side, k=key: self.pulse(s, k)).pack(side='left', padx=4)
            ttk.Button(frame, text='这只手回到初始位置（同时松开）',
                       command=lambda s=side: self.reset(s)).grid(row=8, column=0, columnspan=3, sticky='w', pady=10)
        view = ttk.LabelFrame(self.window, text='视点（只移动模拟头部，不移动游戏角色）', padding=8)
        view.pack(fill='x', padx=14)
        for key, label in (('yaw', '左右看'), ('pitch', '上下看')):
            var = tk.DoubleVar(value=0.)
            ttk.Label(view, text=label).pack(side='left')
            ttk.Scale(view, from_=-120 if key=='yaw' else -70, to=120 if key=='yaw' else 70,
                      variable=var, command=lambda _, k=key, v=var: self.look(k, v.get())).pack(side='left', fill='x', expand=True, padx=6)
        ttk.Button(self.window, text='全部松开（Grip／扳机／按钮）', command=self.release).pack(pady=10)
        ttk.Label(self.window, textvariable=self.status, wraplength=660).pack(anchor='w', padx=14, pady=(0, 10))
        root.after(25, self.tick)

    def move(self, side):
        try:
            for key in ('posX', 'posY', 'posZ', 'yaw', 'pitch', 'roll'):
                value, scale = self.variables[side][key]
                self.commands.hands[side][key] = round(value.get()/scale, 6)
            self.commands.enqueue(side)
        except (ValueError, tk.TclError):
            self.status.set('请输入有效数值后按回车')

    def toggle(self, side, key, value):
        self.commands.hands[side][key] = int(value)
        self.commands.enqueue(side, lambda: None)  # Preserve this exact edge.

    def pulse(self, side, key):
        if self.commands.hands[side][key]:
            return
        self.commands.hands[side][key] = 1
        self.commands.enqueue(side, lambda: self.root.after(180, lambda: self.toggle(side, key, False)))

    def reset(self, side):
        self.commands.hands[side] = hand_defaults(side)
        for key, (variable, scale) in self.variables[side].items():
            variable.set(self.commands.hands[side][key]*scale)
        self.commands.enqueue(side, lambda: None)

    def look(self, key, degrees):
        self.commands.head[key] = degrees  # Head protocol uses degrees.
        target = self.commands.directory/'head_pose_command.json'
        temporary = target.with_suffix('.tmp')
        temporary.write_text(json.dumps(self.commands.head, allow_nan=False), 'utf-8')
        os.replace(temporary, target)

    def release(self):
        self.commands.release()
        for side in range(2):
            for key in ('squeeze', 'trigger'):
                self.variables[side][key][0].set(False)

    def hide(self):
        self.release()
        self.window.withdraw()

    def tick(self):
        if self.closed:
            return
        try:
            now = time.monotonic()
            if now-self.last_heartbeat > .25:
                self.commands.heartbeat()
                self.last_heartbeat = now
            self.commands.tick()
            status = self.commands.directory/'runtime_status.json'
            if status.exists() and time.time()-status.stat().st_mtime < 2.5:
                self.status.set('模拟器正在运行 · 控制命令按顺序发送。实际抓握是否成立请看游戏中的手模。')
            else:
                self.status.set('等待本次桌面模拟器 · 请在同时打开的启动器中选择本地模拟器并启动 VR')
        except (OSError, ValueError) as error:
            self.status.set('控制命令暂未送达：'+str(error))
        self.root.after(25, self.tick)

    def close(self):
        self.closed = True
        # The simulator releases all buttons after 1 second without heartbeat,
        # including abnormal Python termination. Never touch another session.
        (self.commands.directory/'desktop-heartbeat').unlink(missing_ok=True)

