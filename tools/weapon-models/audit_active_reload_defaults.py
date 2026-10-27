"""Read exact tagged active-reload defaults from a verified local UE3 package.

Only explicit tags are reported. An omitted tag may inherit from a parent and
does not mean zero. These values do not by themselves prove the native perfect
reload window or a live instance's effective configuration.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'package-inspector'))
from compare_default_properties import locate_defaults, table


WEAPONS = (
    'WarWeapon', 'Weap_AssaultRifleBase', 'Weap_MeleeComboBase',
    'Weap_PistolBase', 'Weap_AssaultRifle', 'Weap_LocustAssaultRifle',
    'Weap_Shotgun', 'Weap_SniperRifle', 'Weap_COGPistol',
    'Weap_LocustPistol', 'Weap_Boomer_base', 'Weap_Bow_base',
    'Weap_Troika_base',
)
FIELDS = (
    'AR_PossibleSuccessStartPoint', 'AR_SweetSpotDuration',
    'AR_SuperSweetSpotDuration', 'AR_PreReactionWindowDuration',
    'AR_MagicBulletsTimeoutDuration', 'ReloadDuration',
)


def decode_super_sweet_durations(tag):
    """Decode the explicit default struct's bounded float array, if present."""
    if tag is None:
        return None
    if tag['type'] != 'StructProperty':
        raise ValueError('Unexpected super-sweet duration property type')
    value = tag['value']
    if not isinstance(value, dict) or not isinstance(value.get('Value'), dict):
        raise ValueError('Unexpected super-sweet duration struct')
    payload = bytes.fromhex(value['Value']['hex'])
    if len(payload) < 8:
        raise ValueError('Truncated super-sweet duration array')
    count = struct.unpack_from('<I', payload)[0]
    if not 1 <= count <= 8 or len(payload) != 4 + count * 4:
        raise ValueError('Invalid super-sweet duration array length')
    durations = list(struct.unpack_from('<' + 'f' * count, payload, 4))
    if any(not math.isfinite(duration) or duration <= 0 for duration in durations):
        raise ValueError('Invalid super-sweet duration value')
    return durations


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def audit(package, inventory, names_file):
    rows = table(inventory)
    names = {int(row['Index']): row['Name'] for row in table(names_file)}
    exports = {int(row['Index']): row['Path'] for row in rows}
    by_path = {row['Path']: row for row in rows}
    result = {'schema': 1, 'source_sha256': sha256_file(package),
              'explicit_tags_only': True, 'weapons': {}}
    with package.open('rb') as source:
        for weapon in WEAPONS:
            path = f"{weapon}'WarfareGame.Default__{weapon}'"
            row = by_path.get(path)
            if row is None:
                result['weapons'][weapon] = {'found': False}
                continue
            offset, size = int(row['Offset']), int(row['Size'])
            if offset < 0 or size <= 0 or offset + size > package.stat().st_size:
                raise ValueError(f'Invalid export bounds for {path}')
            source.seek(offset)
            raw = source.read(size)
            digest = hashlib.sha256(raw).hexdigest()
            if digest != row['SHA256']:
                raise ValueError(f'Inventory hash mismatch for {path}')
            if len(raw) == 12 and names.get(struct.unpack_from('<i', raw, 4)[0]) == 'None' \
                    and struct.unpack_from('<i', raw, 8)[0] == 0:
                tags = {}  # Only the property terminator follows the object prefix.
            else:
                try:
                    tags, _, _, _ = locate_defaults(raw, names, exports)
                except ValueError as error:
                    raise ValueError(f'{path}: {error}; prefix={raw[:16].hex()}') from error
            values = {}
            for field in FIELDS:
                tag = tags.get(field)
                if tag is not None:
                    if tag['type'] != 'FloatProperty' or not isinstance(tag['value'], float):
                        raise ValueError(f'Unexpected type for {path}.{field}')
                    values[field] = tag['value']
            result['weapons'][weapon] = {
                'found': True, 'source_export_sha256': digest,
                'explicit_values': values,
                'explicit_super_sweet_durations': decode_super_sweet_durations(
                    tags.get('ActiveReload_SuperSweetSpotDurations')),
            }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', required=True, type=Path)
    parser.add_argument('--inventory', required=True, type=Path)
    parser.add_argument('--names', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    ignored = (Path.cwd() / 'analysis').resolve()
    if not output.is_relative_to(ignored):
        parser.error('Report must stay under the checkout analysis/ directory')
    report = audit(args.package, args.inventory, args.names)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for weapon, data in report['weapons'].items():
        print(weapon, data.get('explicit_values', 'not found'))


if __name__ == '__main__':
    main()
