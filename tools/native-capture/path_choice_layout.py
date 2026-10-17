"""Pure bounded translation from authenticated PathChoice CPU layout evidence.

This validates the producer's consistent/fresh-proof contract, not wall-clock age
or GPU visibility. Runtime source admission still belongs to the native caller.
"""
import math
import re
import struct

from path_choice_trial_config import GAME_SHA, LINKS, ROLE_CLASSES

CONTENT = {'TopBackground':'UIImage', 'BottomLeftBackground':'UIImage', 'BottomRightBackground':'UIImage',
    'Title':'UILabel', 'LblLeftDesc':'UILabel', 'LblRightDesc':'UILabel', 'LblLeftButton':'UILabel',
    'LblRightButton':'UILabel', 'imgLeftArrow':'UIImage', 'imgRightArrow':'UIImage'}
SAFE = 'UIWarSafeRegionPanel'


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def _address(value):
    if isinstance(value, str) and re.fullmatch(r'0x[0-9a-fA-F]+', value):
        value = int(value, 16)
    _require(type(value) is int and 0 < value < 1 << 64, 'invalid layout address')
    return value


def _integer(value, low=0, high=(1 << 64)-1):
    _require(type(value) is int and low <= value <= high, 'invalid layout integer')
    return value


def _owner(sample):
    owner = sample['scene_owner']
    _require(owner['schema'] == 1 and type(owner['schema']) is int and owner['kind'] == 'scene_owner_cpu_snapshot' and
        owner['read_only'] is True and owner['consistent_reread'] is True and owner['profile_game_sha256'] == GAME_SHA and
        owner['evidence']['method'] == 'fresh_relevant_fields_and_registry_slots_with_final_reread', 'unverified owner proof')
    _require(type(sample['pid']) is int and sample['pid'] > 0 and sample['pid'] == owner['pid'] and
             type(owner['pid']) is int, 'owner PID mismatch')
    identities = owner['identities']
    _require(type(identities) is list and 0 < len(identities) <= 4096, 'invalid owner inventory')
    by_address = {_address(value['address']):value for value in identities}
    _require(len(by_address) == len(identities), 'duplicate owner identity')
    classes = owner['classes']
    _require(type(classes) is list and 0 < len(classes) <= 4096, 'invalid owner class inventory')
    by_class = {}
    meta_addresses = set()
    for row in classes:
        identity = row['identity'];address = _address(identity['address'])
        _require(address not in by_class and by_address.get(address) == identity and identity['number'] == 0,
                 'owner class association mismatch')
        meta = by_address.get(identity['cls'], {})
        _require(meta.get('name') == 'Class' and meta.get('number') == 0 and meta.get('cls') == meta.get('address'),
                 'invalid owner metaclass')
        meta_addresses.add(meta['address']);_integer(row['super'])
        by_class[address] = row
    _require(len(meta_addresses) == 1, 'ambiguous owner metaclass')
    def chain(identity):
        address, seen = identity['cls'], set()
        names = []
        while address:
            _require(address in by_class and address not in seen and len(seen) < 64, 'unknown/cyclic owner ancestry')
            seen.add(address);names.append(by_class[address]['identity']['name'])
            address = by_class[address]['super']
        return names
    roles = owner['roles']
    _require(type(roles) is dict and set(roles) == set(ROLE_CLASSES) and
             len({_address(value['address']) for value in roles.values()}) == 7, 'invalid owner roles')
    for role, names in ROLE_CLASSES.items():
        identity = roles[role]
        _require(by_address.get(identity['address']) == identity and all(name in chain(identity) for name in names),
                 'owner role/class mismatch')
    _require(by_class[roles['scene']['cls']]['identity']['name'] == 'UIScene_PathChoice' and
             by_class[roles['client']['cls']]['identity']['name'] == 'GameUISceneClient', 'nonexact scene/client class')
    for role, parent in (('client','interaction'),('interaction','viewport'),('viewport','engine'),('local_player','engine')):
        _require(roles[role]['outer'] == roles[parent]['address'], 'owner outer mismatch')
    classification = owner['owner_classification']
    _require(classification in ('local_player','unowned_single_local_player'), 'unsupported owner classification')
    player_owner = roles['local_player']['address'] if classification == 'local_player' else 0
    _require(owner['classification_basis'] == ('explicit_local_player' if player_owner else 'null_owner_with_exact_single_player_graph'),
             'owner classification mismatch')
    _require(set(owner['links']) == set(LINKS), 'missing owner links')
    for label, (role, name, declaring, target) in LINKS.items():
        link = owner['links'][label]
        expected = roles[target]['address'] if target else player_owner
        _require(link['object'] == roles[role]['address'] and link['property'] == name and
                 type(link['value']) is int and link['value'] == expected, 'owner link mismatch')
    for label, maximum in (('game_players',16),('active_scenes',128)):
        array = owner['arrays'][label];members = array['members']
        _require(type(members) is list and len(members) <= maximum and len(set(members)) == len(members) and
                 _integer(array['count'],0,maximum) == len(members) and
                 len(members) <= _integer(array['capacity'],0,4096) and all(value in by_address for value in members),
                 'invalid owner array')
    _require(owner['arrays']['game_players']['members'] == [roles['local_player']['address']], 'not single local player')
    _require(owner['arrays']['active_scenes']['members'].count(roles['scene']['address']) == 1, 'scene not uniquely active')
    return owner, next(iter(meta_addresses))


def _ancestry(proof, cls, meta_address, base_name):
    chain = proof['class_chain']
    _require(type(chain) is list and 0 < len(chain) <= 40, 'missing layout ancestry')
    addresses, names = [], []
    for index, row in enumerate(chain):
        identity, meta = row['identity'], row['metaclass']
        address = _address(identity['address'])
        _require(address not in addresses and identity['number'] == 0 and identity['cls'] == meta_address and
                 meta['address'] == meta_address and meta['name'] == 'Class' and meta['number'] == 0 and meta['cls'] == meta_address,
                 'invalid layout class evidence')
        expected_super = _address(chain[index+1]['identity']['address']) if index+1 < len(chain) else 0
        _require(row['super'] == expected_super, 'broken layout ancestry')
        addresses.append(address);names.append(identity['name'])
    _require(names[0] == cls and names.count(base_name) == 1 and
             ('UIScene' if base_name == 'UIObject' else 'UIObject') not in names, 'wrong layout class branch')
    base = proof['base']
    _require(base['identity'] == chain[names.index(base_name)]['identity'] and base['package']['name'] == 'Engine' and
             base['package']['number'] == 0 and base['package']['outer'] == 0 and
             base['package_type']['name'] == 'Package' and base['package_type']['number'] == 0 and
             base['package_type']['cls'] == meta_address, 'invalid layout base owner')
    return addresses[0]


def _bounds(node, meta_address):
    bounds = node['render_bounds']
    _require(type(bounds) is dict and bounds['order'] == ['left','top','right','bottom'], 'missing directed bounds')
    values, ordered = bounds['values'], bounds['ordered_values']
    for sequence in (values, ordered):
        _require(type(sequence) is list and len(sequence) == 4 and
                 all(type(value) in (int,float) and math.isfinite(value) for value in sequence), 'nonfinite/invalid bounds')
    raw = bytes.fromhex(bounds['raw'])
    _require(len(raw) == 16 and list(struct.unpack('<4f',raw)) == values, 'raw bounds mismatch')
    normalized = [min(values[0],values[2]), min(values[1],values[3]), max(values[0],values[2]), max(values[1],values[3])]
    _require(ordered == normalized and type(bounds['x_reversed']) is bool and type(bounds['y_reversed']) is bool and
             bounds['x_reversed'] == (values[0] > values[2]) and bounds['y_reversed'] == (values[1] > values[3]),
             'directed/ordered bounds mismatch')
    reflected = bounds['reflected']
    _address(reflected['property_address'])
    _require(reflected['type'] == 'FloatProperty' and type(reflected['array_dim']) is int and reflected['array_dim'] == 4 and
             type(reflected['element_size']) is int and reflected['element_size'] == 4 and
             bytes.fromhex(reflected['array_layout_raw']) == struct.pack('<II',4,4), 'unsupported bounds metadata')
    _integer(reflected['offset'],0,0x10000-16)
    _ancestry(bounds['class_ancestry'], node['cls'], meta_address, 'UIObject')
    return normalized


def centered_translation(sample):
    """Nearest-integer union-center shift; no hardcoded resolution or displacement."""
    try:
        return _centered_translation(sample)
    except (KeyError, TypeError, AttributeError, OverflowError, struct.error) as error:
        raise ValueError('malformed PathChoice layout evidence') from error


def _centered_translation(sample):
    _require(type(sample) is dict and sample['read_only'] is True and sample['consistent_reread'] is True and
             sample['input_block_count'] == 0 and type(sample['input_block_count']) is int and
             sample['layout_evidence'] == 'consistent CPU layout snapshot; not actual draw/scissor proof', 'unverified layout sample')
    owner, meta = _owner(sample)
    top, scene = sample['top'], owner['roles']['scene']
    _require(top['cls'] == 'UIScene_PathChoice' and top['name'] == 'DeltaChoosePath' and top['tag'] == 'DeltaChoosePath' and
             top['tag_number'] == 0 and top['number'] == scene['number'] and scene['name'] == 'DeltaChoosePath' and
             _address(top['address']) == scene['address'] and _address(sample['client']) == owner['roles']['client']['address'],
             'top/owner scene mismatch')
    active = sample['active_scenes']
    _require(type(active) is list and active and [_address(value['address']) for value in active] ==
             owner['arrays']['active_scenes']['members'] and _address(active[-1]['address']) == scene['address'], 'active scene mismatch')
    tree = sample['tree']
    _require(type(tree) is list and 1 < len(tree) <= 128, 'invalid layout tree')
    by_address, by_name = {}, {}
    for node in tree:
        address = _address(node['address']);name = node['name']
        _require(type(name) is str and name and name not in by_name and address not in by_address, 'duplicate layout name/address')
        _integer(node['depth'],1,16)
        by_address[address] = node;by_name[name] = node
    roots = [node for node in tree if node['parent'] is None]
    _require(len(roots) == 1 and _address(roots[0]['address']) == scene['address'], 'wrong tree root')
    root = roots[0]
    _require(root['depth'] == 1 and root['name'] == top['name'] and root['cls'] == top['cls'] and
             root['number'] == top['number'] and _address(root['outer']) == scene['outer'] and
             _address(top['outer']) == scene['outer'] and root['render_bounds'] is None, 'root identity/layout mismatch')
    _require(_ancestry(root['render_bounds_absence']['proof'], root['cls'], meta, 'UIScene') == scene['cls'],
             'root ancestry mismatch')
    for node in tree:
        if node is root:continue
        parent = _address(node['parent'])
        _require(parent in by_address and by_address[parent]['depth'] + 1 == node['depth'] and
                 _address(node['outer']) == parent, 'broken layout parent/depth/outer')
    _require(SAFE in by_name and by_name[SAFE]['cls'] == SAFE, 'missing unique SafeRegionPanel')
    safe_node = by_name[SAFE]
    _require(_address(safe_node['parent']) == scene['address'] and safe_node['depth'] == 2, 'safe panel hierarchy mismatch')
    safe = _bounds(safe_node, meta)
    rectangles = []
    for name, cls in CONTENT.items():
        _require(name in by_name, 'missing observed content node')
        node = by_name[name]
        _require(node['cls'] == cls and _address(node['parent']) == _address(safe_node['address']) and node['depth'] == 3,
                 'content class/hierarchy mismatch')
        rectangle = _bounds(node, meta)
        _require(rectangle[0] < rectangle[2] and rectangle[1] < rectangle[3], 'empty content bounds')
        rectangles.append(rectangle)
    for node in tree:
        if node is root or node is safe_node or node['name'] in CONTENT:continue
        _require(node['cls'] == 'UIImage', 'unrecognized non-image content')
        _bounds(node, meta)  # Validate it, but never include unidentified imagery in the union.
    union = [min(r[0] for r in rectangles), min(r[1] for r in rectangles),
             max(r[2] for r in rectangles), max(r[3] for r in rectangles)]
    def inside(rectangle):
        return safe[0] <= rectangle[0] <= rectangle[2] <= safe[2] and safe[1] <= rectangle[1] <= rectangle[3] <= safe[3]
    _require(safe[0] < safe[2] and safe[1] < safe[3] and inside(union), 'content outside safe domain')
    x = int(round((safe[0]+safe[2]-union[0]-union[2])/2))
    y = int(round((safe[1]+safe[3]-union[1]-union[3])/2))
    _require(abs(x) <= 512 and abs(y) <= 512 and inside([union[0]+x,union[1]+y,union[2]+x,union[3]+y]),
             'translation exceeds safe domain/budget')
    return x,y
