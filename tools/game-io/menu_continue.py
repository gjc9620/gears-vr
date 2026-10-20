"""Continue campaign using locally reviewed menu templates, never blind Enter.

Templates and profile are local analysis artifacts. Stops on an unexpected
window, resolution, order, ambiguous image or F8. No global keyboard input.
"""
import argparse
import datetime
import hashlib
import json
import pathlib
import time
from PIL import Image
from game_io import game_window, snapshot, interrupted, GAME, user
from confirm_window import main as confirm

ROOT=pathlib.Path(__file__).resolve().parents[2]
ORDER=('start','online_error','main_campaign','campaign_single','single_continue')
ROI=(90,125,465,270)


def pixels(path):
    with Image.open(path) as image:
        if image.size!=(640,360):raise RuntimeError('Template/image requires 640 x 360')
        return list(image.convert('L').crop(ROI).tobytes())


def score(current,reference):
    active=[abs(a-b) for a,b in zip(current,reference) if max(a,b)>60]
    return sum(active)/len(active) if len(active)>=100 else 255


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile',type=pathlib.Path,default=ROOT/'analysis/game-io/menu-templates.json')
    args=parser.parse_args()
    profile=json.loads(args.profile.read_text(encoding='utf-8'))
    version=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text(encoding='utf-8'))
    if hashlib.sha256(GAME.read_bytes()).hexdigest()!=version['sha256']:raise RuntimeError('Game version differs from reviewed build')
    references={}
    for name in ORDER:
        item=profile['templates'][name];path=pathlib.Path(item['path'])
        if hashlib.sha256(path.read_bytes()).hexdigest()!=item['sha256']:raise RuntimeError('Reviewed template changed')
        references[name]=pixels(path)
    target=game_window();deadline=time.monotonic()+90;step=0;stable=0
    log=[];stem=ROOT/'analysis/game-io'/('menu-run-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    try:
        while step<len(ORDER):
            if time.monotonic()>deadline or interrupted():raise RuntimeError('Menu deadline/stop reached')
            if game_window()!=target:raise RuntimeError('Game process/window changed')
            if user.IsIconic(target[1]):
                # Restore only the already verified game HWND. Capture cannot
                # acquire a minimized window; foreground activation is optional.
                user.ShowWindow(target[1],9)
                stable=0
                log.append({'expected':ORDER[step],'restored_minimized_window':True})
                time.sleep(.4)
                continue
            shot=snapshot('menu-'+str(time.time_ns()),640)
            if shot['pid']!=target[0]:raise RuntimeError('Game target changed')
            if shot['source_width']!=1920 or shot['source_height']!=1080:
                # Startup may briefly use a different client size. Never send
                # input until the reviewed resolution is restored; deadline
                # still bounds incompatible permanent settings.
                stable=0
                log.append({'expected':ORDER[step],'image':shot['image'],'waiting_for_resolution':[shot['source_width'],shot['source_height']]})
                time.sleep(.4)
                continue
            current=pixels(shot['image']);ranked=sorted((score(current,ref),name) for name,ref in references.items())
            best,name=ranked[0];margin=ranked[1][0]-best
            accepted=name==ORDER[step] and best<12 and margin>12 and not shot.get('possibly_blank')
            log.append({'expected':ORDER[step],'image':shot['image'],'ranked':ranked,'accepted':accepted})
            stable=stable+1 if accepted else 0
            if stable>=2:
                if game_window()!=target or interrupted():raise RuntimeError('Game changed before confirmation')
                confirm();step+=1;stable=0
                print(json.dumps({'confirmed':name,'distance':round(best,2),'margin':round(margin,2)}),flush=True)
                time.sleep(1)
            else:time.sleep(.4)
        print(json.dumps({'menu_confirmations':step,'loading_started':True,'level_verified':False}))
    finally:
        stem.with_suffix('.json').write_text(json.dumps({'target':target,'confirmed_steps':step,'samples':log},indent=2),encoding='utf-8')


if __name__=='__main__':main()
