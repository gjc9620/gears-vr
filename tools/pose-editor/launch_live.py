"""Owner-started launcher plus editor; no attach to an existing game."""
from pathlib import Path
import argparse
import os
import subprocess
import time
import tkinter as tk
from tkinter import messagebox
from pose_editor import Editor


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--desktop', action='store_true', help='Use isolated mouse-controlled simulator')
    args = parser.parse_args()
    package = Path(__file__).resolve().parent.parent
    launcher = package / "GearsVR Launcher.exe"
    if not launcher.is_file():
        raise FileNotFoundError("Run this entry from the packaged live editor candidate")
    channel = Path("J:/gearsVr/analysis/pose-editor-live-20261111") / ("manual-"+str(time.time_ns()))
    channel.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env["GEARSVR_POSE_EDITOR_CHANNEL"] = str(channel)
    env["GEARSVR_MANIFEST"] = str(package/"tools/vr-launcher/manifest-6dof-unified.json")
    env["GEARSVR_PYTHON"] = str(package/"runtime/python/python.exe")
    if args.desktop:
        simulator_data = channel/'simulator'
        simulator_data.mkdir()
        env['GEARSVR_DESKTOP_EDITOR'] = '1'
        env['GEARSVR_DESKTOP_SIMULATOR_DATA'] = str(simulator_data)
        env['XR_SIMULATOR_DESKTOP_PANEL'] = '1'
        env['XR_SIMULATOR_FORCE_FOCUS'] = '1'
    # The normal launcher owns startup, physical tracking, audio, saves and
    # shutdown. Closing the editor disables preview without stopping the game.
    subprocess.Popen([str(launcher)], cwd=package, env=env)
    root = tk.Tk()
    editor = Editor(root, channel)
    panel = None
    if args.desktop:
        from desktop_controls import DesktopControls
        panel = DesktopControls(root, simulator_data)
        from tkinter import ttk
        ttk.Button(root, text='显示桌面双手控制', command=panel.window.deiconify).pack(side='bottom')
    try:
        root.mainloop()
    finally:
        if panel:
            panel.close()


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        messagebox.showerror("GearsVR 实时编辑器", str(error))
