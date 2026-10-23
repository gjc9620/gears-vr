"""Encode private passive-observer metadata. No files, processes, hooks or writes.

CRC32 detects accidental truncation/damage; it is not authentication. The
canonical config SHA256 is provenance only. This packet never grants authority.
All current source checks must still be validated by the live observer owner.
"""
import hashlib
import json
import struct
import zlib

MAGIC = b'GCVRCALL'
VERSION = 1
MAX_PACKET = 2 * 1024 * 1024
MAX_CHECKS = 16384
MAX_SOURCE = 131072
MAX_LOCALS = 4096
MAX_U64 = (1 << 64) - 1
GAME_SHA = 'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4'
DISPATCHER_RVA = 0x1d6f30
# Mechanical copy of the existing ui_call_boundary.h certificate. The native
# parser uses that header directly; cross-language tests detect certificate drift.
DISPATCHER_BYTES = bytes.fromhex('405553565741544155415641574881ec38050000488d6c2430c5f829b5f00400')
FUNCTION_KEYS = ('PlayerTakingCover.PlayerMove', 'PlayerTakingCover.UpdatePlayerPosture',
                 'PlayerTakingCover_Stationary.PlayerMove', 'PlayerTakingCover_Stationary.UpdatePlayerPosture',
                 'SetPawnCoverAction', 'SetCoverDirection')
HEADER = struct.Struct('<8sHHIIIIIQQQQ32s')  # 96 bytes; no native padding.
FUNCTION = struct.Struct('<IIQQ8I')          # 56 bytes; order tag, zero flags.
CHECK = struct.Struct('<QII')               # Address, length, zero flags.


class WireRejected(ValueError):
    pass


def _require(ok, reason):
    if not ok:
        raise WireRejected(reason)


def _uint(value, bits, label, positive=False):
    _require(type(value) is int and int(positive) <= value < (1 << bits), 'Invalid wire integer: ' + label)
    return value


def _span(address, size, label):
    _uint(address, 64, label, True)
    _uint(size, 32, label + ' size', True)
    _require(size <= MAX_SOURCE and address <= MAX_U64 - size, 'Wire source range overflow/bound: ' + label)


def _hex(value, size, label):
    _require(type(value) is str and len(value) == size * 2, 'Invalid wire hex length: ' + label)
    try:
        raw = bytes.fromhex(value)
    except ValueError as error:
        raise WireRejected('Invalid wire hex: ' + label) from error
    _require(len(raw) == size, 'Invalid wire hex bytes: ' + label)
    return raw


def encode(config):
    """Return a bounded LE v1 packet; no I/O or authority is conferred."""
    try:
        return _encode(config)
    except WireRejected:
        raise
    except (KeyError, TypeError, ValueError, OverflowError, struct.error, UnicodeError) as error:
        raise WireRejected('Cover-call wire shape rejected: ' + str(error)) from error


def _encode(c):
    _require(type(c) is dict and type(c.get('schema')) is int and c['schema'] == 1 and c.get('sha256') == GAME_SHA,
             'Unsupported cover-call config schema/build')
    _require(c.get('mutation_authorized') is False and c.get('callsite_aliasing_verified') is False
             and c.get('runtime_observation') is False, 'Wire flags must remain passive/unverified')
    pid = _uint(c['pid'], 32, 'pid', True)
    base = _uint(c['base'], 64, 'base', True)
    _require(base % 8 == 0 and base <= MAX_U64 - DISPATCHER_RVA - 32, 'Invalid wire module base')
    ids = c['identities']
    _require(type(ids) is list and 1 <= len(ids) <= 1024, 'Invalid wire identity count')
    addresses, slots = {}, set()
    for item in ids:
        address = _uint(item['address'], 64, 'identity address', True)
        slot = _uint(item['internal_index'], 32, 'identity slot')
        _require(address % 8 == 0 and address not in addresses and slot not in slots, 'Duplicate/unaligned wire identity')
        _uint(item['cls'], 64, 'identity class', True)
        _uint(item['outer'], 64, 'identity outer')
        _uint(item['number'], 32, 'identity number')
        _require(type(item['name']) is str and item['name'] and not any(x in item['name'] for x in '\0\r\n'), 'Invalid wire identity name')
        addresses[address] = item
        slots.add(slot)
    _require(all(x['cls'] in addresses for x in ids), 'Wire identity class closure is incomplete')

    def identity(index):
        _uint(index, 32, 'identity reference')
        _require(index < len(ids), 'Wire identity reference outside table')
        return ids[index]

    pointers = [identity(c['roles'][name])['address'] for name in ('controller', 'pawn', 'camera')]
    _require(len(set(pointers)) == 3, 'Wire controlled roles alias')
    dispatcher = c['dispatcher']
    _require(dispatcher.get('compared_in_current_process') is True and type(dispatcher['rva']) is int
             and dispatcher['rva'] == DISPATCHER_RVA
             and _hex(dispatcher['entry'], 32, 'dispatcher') == DISPATCHER_BYTES, 'Wire dispatcher has not been verified')
    checks = c['source_checks']
    _require(type(checks) is list and 1 <= len(checks) <= MAX_CHECKS, 'Invalid wire source-check count')
    sources, records = {}, []
    total = HEADER.size + 6 * FUNCTION.size + 4
    for item in checks:
        address, size = item['address'], item['size']
        _span(address, size, 'check')
        raw = _hex(item['bytes'], size, 'source check')
        key = address, size
        _require(key not in sources, 'Duplicate wire source check')
        sources[key] = raw
        total += CHECK.size + size
        _require(total <= MAX_PACKET, 'Wire packet exceeds 2 MiB')
        records.append(CHECK.pack(address, size, 0) + raw)
    _require(sources.get((base + DISPATCHER_RVA, 32)) == DISPATCHER_BYTES,
             'Wire source checks lack the original dispatcher span')

    def source(address, expected, label):
        _require(sources.get((address, len(expected))) == expected, 'Wire source binding differs: ' + label)

    functions = c['functions']
    _require(type(functions) is list and len(functions) == 6, 'Wire requires exactly six functions')
    function_records, function_addresses = [], set()
    for index, (function, key) in enumerate(zip(functions, FUNCTION_KEYS)):
        _require(function['key'] == key, 'Wire function order/key differs')
        item, owner = identity(function['identity']), identity(function['owner'])
        expected_owner, expected_name = key.rsplit('.', 1) if '.' in key else ('WarPC', key)
        _require(item['name'] == expected_name and item['number'] == 0 and item['outer'] == owner['address']
                 and owner['name'] == expected_owner and owner['number'] == 0
                 and addresses[item['cls']]['name'] == 'Function', 'Wire exact function/owner differs')
        address = item['address']
        _require(address not in function_addresses and address <= MAX_U64 - 0x11c, 'Duplicate/overflowing wire function')
        function_addresses.add(address)
        script = function['script']
        script_address, size, capacity = script['address'], script['count'], script['capacity']
        _span(script_address, size, 'function script')
        _uint(capacity, 32, 'script capacity', True)
        _require(size <= capacity <= MAX_SOURCE, 'Invalid wire script count/capacity')
        raw = _hex(script['bytes'], size, 'script')
        _require(type(script['sha256']) is str and hashlib.sha256(raw).hexdigest() == script['sha256'], 'Wire script hash differs')
        header = struct.pack('<Qii', script_address, size, capacity)
        _require(_hex(script['header_raw'], 16, 'script header') == header, 'Wire script header differs')
        locals_size = _uint(function['locals_size'], 32, 'locals size', True)
        _require(locals_size <= MAX_LOCALS and function['entry'] == base + 0x1d6ca0
                 and type(function['entry']) is int and function.get('flags_classification_verified') is False,
                 'Unsupported wire function entry/locals/flags')
        source(address + 0x88, struct.pack('<I', locals_size), 'locals size')
        source(address + 0x8c, header, 'Script header')
        source(address + 0x114, struct.pack('<Q', function['entry']), 'function entry')
        source(script_address, raw, 'Script body')
        fields = function['fields']
        _require(type(fields) is list and 1 <= len(fields) <= 256, 'Invalid wire function field count')
        by_name = {}
        for field in fields:
            name = field['name']
            _require(type(name) is str and name and name not in by_name, 'Duplicate/invalid wire local name')
            prop = identity(field['identity'])
            _require(prop['name'] == name and prop['number'] == 0 and prop['outer'] == address
                     and field['owner'] == function['identity'] and addresses[prop['cls']]['name'] == field['type'],
                     'Wire local owner/type differs')
            offset = _uint(field['offset'], 32, 'local offset')
            width = _uint(field['width'], 32, 'local width', True)
            dimension = _uint(field['dimension'], 32, 'local dimension', True)
            _require(width <= MAX_LOCALS and dimension <= 256 and type(field['storage_width']) is int
                     and field['storage_width'] == width * dimension and offset + width * dimension <= locals_size
                     and field.get('flags_interpreted') is False, 'Wire local containment/flags differs')
            _hex(field['flags_raw'], 8, 'local flags')
            by_name[name] = field
        pairs = [0] * 6
        if index in (0, 2):
            offsets = set()
            for selected, name in enumerate(('PawnCT', 'PawnCA', 'PawnCD')):
                _require(name in by_name, 'Wire PlayerMove local is missing: ' + name)
                field = by_name[name]
                _require(field['type'] == 'ByteProperty' and field['width'] == field['dimension'] == 1
                         and field['offset'] not in offsets, 'Wire PlayerMove byte layout differs')
                offsets.add(field['offset'])
                pairs[selected * 2:selected * 2 + 2] = [field['offset'], field['width']]
                prop = identity(field['identity'])['address']
                source(prop + 0x70, struct.pack('<II', 1, 1), 'PlayerMove property width')
                source(prop + 0x94, struct.pack('<I', field['offset']), 'PlayerMove property offset')
        function_records.append(FUNCTION.pack(index, 0, address, script_address, size, locals_size, *pairs))
    provenance = hashlib.sha256(json.dumps(c, sort_keys=True, separators=(',', ':'), allow_nan=False).encode('utf-8')).digest()
    header = HEADER.pack(MAGIC, VERSION, 0, total, pid, 6, len(checks), 0, base, *pointers, provenance)
    packet = header + b''.join(function_records) + b''.join(records)
    _require(len(packet) + 4 == total <= MAX_PACKET, 'Internal wire size mismatch')
    return packet + struct.pack('<I', zlib.crc32(packet) & 0xffffffff)
