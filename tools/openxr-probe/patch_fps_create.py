from pathlib import Path
p = Path(r"J:\codexWorktree\gearsVr\tools\openxr-probe\pose_probe.cpp")
lines = p.read_text(encoding="utf-8").splitlines()
old = '        if(stereo_game_pid&&!armspan_ui_mode&&max_composition_layers>=2)fps_panel.reset(new TransitionCard(device.Get(),queue.Get(),headset_fps_pixels::rgba("--")));'
new = '        if(stereo_game_pid&&!armspan_ui_mode&&max_composition_layers>=2){try{fps_panel.reset(new TransitionCard(device.Get(),queue.Get(),headset_fps_pixels::rgba("--")));}catch(const std::exception& error){fprintf(stderr,"FPS panel optional creation failure: %s\n",error.what());fps_panel.reset();++fps_disabled;}}'
index = lines.index(old)
lines[index] = new
p.write_text("\n".join(lines) + "\n", encoding="utf-8")
print("patched")
