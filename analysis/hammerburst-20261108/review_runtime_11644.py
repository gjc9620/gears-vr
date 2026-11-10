"""Prepare bounded contact sheets of the existing, failed 11644 recording.

No runtime, input, deletion, or acceptance inference. Preserve capture errors.
"""
from pathlib import Path
import hashlib
import json
from PIL import Image, ImageDraw

AREA = Path(__file__).resolve().parent
ROOT = AREA.parents[1]
SOURCE = ROOT / 'analysis/instances/hb-cloak-1110-221519/actions/menu-firstperson-11644-221626'
OUTPUT = AREA / 'runtime-11644-media-review'


def main():
    recording = SOURCE / 'recording.json'
    data = json.loads(recording.read_text(encoding='utf-8'))
    frames = data['frames']
    assert len(frames) == 306
    OUTPUT.mkdir(exist_ok=True)
    selected = sorted(set(round(i * (len(frames)-1) / 47) for i in range(48)))
    pages = []
    for page in range(3):
        sheet = Image.new('RGB', (1440, 1040), '#202020')
        draw = ImageDraw.Draw(sheet)
        entries = []
        for cell, index in enumerate(selected[page*16:(page+1)*16]):
            frame = frames[index]
            path = SOURCE / frame['file']
            with Image.open(path) as original:
                original.load()
                size = original.size
                thumb = original.convert('RGB')
                thumb.thumbnail((352, 226))
            x, y = (cell % 4)*360, (cell//4)*260
            sheet.paste(thumb, (x, y+25))
            draw.text((x+4,y+5), f"{index:05d}  {frame['time']:.3f}s", fill='white')
            entries.append(dict(index=index, file=frame['file'], time=frame['time'],
                                original_size=size, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
        target = OUTPUT / f'overview-{page+1}.jpg'
        sheet.save(target, quality=91)
        pages.append(dict(path=str(target), entries=entries))
    proof = dict(game=11644, source=str(SOURCE), frames=len(frames),
                 recording_sha256=hashlib.sha256(recording.read_bytes()).hexdigest(),
                 capture_errors=data['errors'], start_time=frames[0]['time'], end_time=frames[-1]['time'],
                 sample_count=len(selected), overview_pages=pages,
                 overview_reviewed=False, full_fit_accepted=False, physical_headset_accepted=False,
                 limitation='Partial recording: preserve capture failure; contact sheets are samples, not every frame or all-angle proof.')
    (OUTPUT/'prepared.json').write_text(json.dumps(proof,indent=2),encoding='utf-8')
    print(json.dumps({key:proof[key] for key in ('frames','sample_count','start_time','end_time','capture_errors')}))


if __name__ == '__main__':
    main()
