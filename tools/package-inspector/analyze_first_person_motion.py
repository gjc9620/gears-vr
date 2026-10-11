"""Read-only motion/recoil audit from local original/mod package inventories.

Hashes cover serialized exports, not runtime behavior. Native animation tails
are compared without assuming a decoded track format.
"""
import argparse
import hashlib
import json
import pathlib
import re
from compare_default_properties import table, locate_defaults


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=pathlib.Path, default=pathlib.Path('analysis/first-person'))
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    rows = {side: table(args.root / (side + '-WarfareGame.tsv')) for side in ('original', 'mod')}
    by_path = {side: {row['Path']: row for row in values} for side, values in rows.items()}
    names = {side: {int(row['Index']): row['Name'] for row in table(args.root / (side + '-WarfareGame.tsv.names.tsv'))}
             for side in rows}
    exports = {side: {int(row['Index']): row['Path'] for row in values} for side, values in rows.items()}
    paths = {'original': args.root.parent / 'packages/decompressed/WarfareGame.u',
             'mod': args.root / 'decompressed/WarfareGame.u'}
    streams = {side: path.open('rb') for side, path in paths.items()}
    def properties(side, row):
        stream = streams[side]
        stream.seek(int(row['Offset']))
        raw = stream.read(int(row['Size']))
        if len(raw) != int(row['Size']):
            raise ValueError('Export truncated')
        props, end, tail, start = locate_defaults(raw, names[side], exports[side])
        return {key: value['value'] for key, value in props.items()}, raw[end:]
    report = dict(groups={}, camera_animations=[], selected_defaults=[], errors=[])
    patterns = {
        'recoil_controllers': lambda r: r['Class'] in ('WarSkelCtrl_Recoil', 'GameSkelCtrl_Recoil'),
        'weapon_class_defaults': lambda r: 'Default__' in r['Path'] and (r['Class'].startswith('Weap_') or r['Class'] in ('WarWeapon', 'WarVehicleWeapon')),
        'shooting_functions': lambda r: r['Class'] == 'Function' and re.search(r'Recoil|FireViewShake|AimError|Spread|ScreenShake', r['Path'], re.I),
        'camera_modifier_defaults': lambda r: 'Default__WarCamMod_' in r['Path'] and r['Class'] not in ('Class', 'Function'),
    }
    try:
        for label, predicate in patterns.items():
            group = []
            for path, old in by_path['original'].items():
                if not predicate(old):
                    continue
                new = by_path['mod'].get(path)
                group.append(dict(path=path, cls=old['Class'], same_bytes=bool(new and old['SHA256'] == new['SHA256'])))
            report['groups'][label] = group
        for path, old in by_path['original'].items():
            new = by_path['mod'].get(path)
            if not new:
                continue
            camera_anim = old['Class'] == 'AnimSequence' and 'AnimSet_CameraMovement' in path
            selected = path in ("WarWeapon'WarfareGame.Default__WarWeapon'",
                "SkeletalMeshComponent'WarfareGame.Default__WarWeapon.WeaponMesh'",
                "SkeletalMeshComponent'WarfareGame.Default__Pawn_COGMarcus.WarPawnMesh'",
                "WarCamMod_ScreenShake'WarfareGame.Default__WarCamMod_ScreenShake'")
            if not camera_anim and not selected:
                continue
            try:
                before, old_tail = properties('original', old)
                after, new_tail = properties('mod', new)
                if camera_anim:
                    report['camera_animations'].append(dict(path=path, name=after.get('SequenceName'),
                        same_bytes=old['SHA256'] == new['SHA256'], native_tail_same=old_tail == new_tail,
                        native_tail_sha256=hashlib.sha256(new_tail).hexdigest(),
                        original={k: v for k, v in before.items() if k != 'CompressedTrackOffsets'},
                        mod={k: v for k, v in after.items() if k != 'CompressedTrackOffsets'}))
                else:
                    keys = sorted(before.keys() | after.keys())
                    report['selected_defaults'].append(dict(path=path,
                        changed={k: dict(original=before.get(k), mod=after.get(k)) for k in keys if before.get(k) != after.get(k)},
                        recoil_values={k: dict(original=before.get(k), mod=after.get(k), same=before.get(k) == after.get(k))
                            for k in keys if re.search('Recoil|Shake|AimError|Spread|IgnoreControllers', k, re.I)}))
            except (ValueError, KeyError) as error:
                report['errors'].append(dict(path=path, error=str(error)))
        # Identify reused animation data by exact native-tail equality. This
        # establishes byte reuse without claiming a particular motion curve.
        animations = report['camera_animations']
        for item in animations:
            if not item['same_bytes']:
                item['shares_mod_native_tail_with'] = [other['name'] for other in animations
                    if other['path'] != item['path'] and other['native_tail_sha256'] == item['native_tail_sha256']]
    finally:
        for stream in streams.values():
            stream.close()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    for label, group in report['groups'].items():
        print(label, dict(total=len(group), unchanged=sum(item['same_bytes'] for item in group),
                          changed=[item['path'] for item in group if not item['same_bytes']]))
    for item in report['camera_animations']:
        print(item['name'], 'UNCHANGED' if item['same_bytes'] else 'CHANGED', item.get('shares_mod_native_tail_with', []))
    for item in report['selected_defaults']:
        print(item['path'], json.dumps(item['recoil_values']))
    print('parse_errors', report['errors'])


if __name__ == '__main__':
    main()
