from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
REL='tools/native-capture/full_body_c0_native_live.inl'
def apply(text):
 current=(ROOT/REL).read_text(encoding='utf-8')
 start='std::array<PendingFrame,32> pending_frames{};unsigned pending_frame_count{};'
 end='// This private callback is installed by Native, never supplied by PG.'
 assert text.count(start)==current.count(start)==1 and text.count(end)==current.count(end)==1
 text=text[:text.index(start)]+current[current.index(start):current.index(end)]+text[text.index(end):]
 edits=[('if(state.kernel.pending()){note_pending();','if(state.kernel.pending()){note_pending(e);'),
 ('if(state.kernel.failed())note_pending();','if(state.kernel.failed())note_pending(e);'),
 ('    cover_consumption_observation::summary(f);','    cover_consumption_observation::summary(f);\n    cover_pending_observation::export_record();cover_pending_observation::summary(f);')]
 for a,b in edits:
  assert text.count(a)==1,a;text=text.replace(a,b)
 return text
