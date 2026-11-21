"""Pin the exact portable Mod kernels used by this independent workbench build."""
from pathlib import Path
import hashlib
import json
import re

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent/'vr-common'
ROOTS = ('pose_math.h', 'controller_input_frame.h', 'body_slots.h',
         'weapon_lancer_front_handle.h', 'weapon_lancer_chainsaw.h',
         'weapon_lancer_insertion.h', 'weapon_lancer_insertion_assist.h',
         'weapon_lancer_reload_policy.h', 'weapon_reload_model.h',
         'weapon_two_hand_hold.h', 'weapon_lancer_sight_zero.h',
         'hand_authored_mount_profile.h', 'weapon_lancer_wrist_mount.h',
         'weapon_lancer_held_contact.h', 'weapon_lancer_owner_grabposes.h',
         'weapon_lancer_main_hand.h', 'weapon_lancer_natural_hand.h',
         'weapon_lancer_part_hand.h', 'weapon_lancer_magazine_pose.h')


def freeze():
    pending = list(ROOTS)
    result = {}
    while pending:
        name = pending.pop()
        if name in result:
            continue
        path = (SOURCE/name).resolve()
        if not path.is_relative_to(SOURCE.resolve()):
            raise ValueError('Kernel include left the shared source tree')
        raw = path.read_bytes()
        result[name] = raw
        for included in re.findall(r'^\s*#\s*include\s*"([^"]+)"', raw.decode('utf-8-sig'), re.M):
            relative = (Path(name).parent/included).as_posix()
            pending.append(relative)
    destination = HERE/'game_rules'
    destination.mkdir(exist_ok=True)
    for name, raw in result.items():
        target = destination/name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    report = dict(kind='portable_mod_kernel_snapshot', roots=list(ROOTS),
                  files={name:hashlib.sha256(raw).hexdigest() for name, raw in sorted(result.items())},
                  scope='Shared geometry/state machines. No original game engine, recoil, audio or animation execution.')
    (HERE/'assets/game-rules.json').write_text(json.dumps(report, indent=2), 'utf-8')
    print(json.dumps(dict(files=len(result), bytes=sum(map(len, result.values())))))


if __name__ == '__main__':
    freeze()
