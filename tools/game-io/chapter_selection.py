"""Diagnostic DeltaSelectMap selection. No process launch, native call or save writes.

Caller must already be at the authenticated chapter UI, own verified backups,
review overwrites, and supply a session-bound ordinary PostMessage key sender.
Confirmation/difficulty screens are outside this module's permission boundary.
"""
from pathlib import Path
import copy
import struct
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/game-io'))
sys.path.insert(0, str(ROOT / 'tools/package-inspector'))
from menu_control import fingerprint, reader_process_identity
from inspect_ui_state import registered

FIELDS = ('FriendlyName', 'MapName', 'ChapterPointID')
HAMMER = dict(FriendlyName='HAMMER', MapName='sp_hos_p', ChapterPointID='LoadChapter=5')


class ChapterSelectionRejected(RuntimeError):
    pass


def require(ok, reason):
    if not ok:
        raise ChapterSelectionRejected(reason)


def _chapter(sample, pid):
    top, leaf = sample.get('top') or {}, sample.get('focused_leaf') or {}
    require(sample.get('pid') == pid and sample.get('read_only') is True and
            sample.get('consistent_reread') is True and sample.get('player_index') == 0 and
            sample.get('input_block_count') == 0, 'Chapter UI gates rejected')
    require((top.get('tag'), top.get('cls'), top.get('input_mode')) == ('DeltaSelectMap', 'WarUIScene', 1) and
            (leaf.get('name'), leaf.get('cls')) == ('lstMaps', 'UIList') and
            {'UIState_Enabled', 'UIState_Focused'} <= set(leaf.get('states', [])) and
            sample.get('focus_chain') and sample['focus_chain'][-1] == leaf,
            'Expected focused DeltaSelectMap/lstMaps')
    return top, leaf


def capture_chapters(reader, expected_identity, verify, deadline, *, now=time.monotonic,
                     identity=reader_process_identity):
    """Fresh reflected provider inventory with exact same-read ledger closure."""
    def check():
        require(now() < deadline, 'Chapter selection deadline')
        verify()
        require(identity(reader) == tuple(expected_identity), 'Chapter reader process identity changed')
    check()
    before = reader.sample()
    top, leaf = _chapter(before, expected_identity[0])
    r = reader.r
    original = r.read
    reads, total = {}, 0
    def observed(address, size):
        nonlocal total
        require(now() < deadline, 'Chapter selection deadline')
        require(type(address) is int and type(size) is int and 0 < size <= 4*1024*1024,
                'Invalid bounded chapter read')
        raw = original(address, size)
        require(type(raw) is bytes and len(raw) == size, 'Short chapter read')
        key = address, size
        require(key not in reads or reads[key] == raw, 'Chapter source changed during acquisition')
        total += size
        require(len(reads) < 32768 and total <= 8*1024*1024, 'Chapter read budget')
        reads[key] = raw
        return raw
    r.read = observed
    try:
        r.ident_cache.clear(); r.names_cache.clear(); reader.properties.clear()
        scene = registered(r, int(top['address'], 0))
        obj = registered(r, int(leaf['address'], 0))
        require(r.class_name(scene) == top['cls'] and r.class_name(obj) == 'UIList', 'Chapter class changed')
        for state, current in ((top, scene), (leaf, obj)):
            require(current['name'] == state['name'] and current['number'] == state['number'] and
                    current['outer'] == int(state['outer'], 0), 'Chapter object identity changed')
        for prefix in ('objects', 'names'):r.read(getattr(r, prefix+'_header'), 16)
        index = reader.integer(obj, 'Index')
        cp = reader.prop(obj, 'CellDataComponent', 'ComponentProperty')
        comp = registered(r, r.u64(obj['address']+cp['offset']))
        data, count, inner = reader.array(comp, 'ListItems', 'StructProperty', 128)
        dim, stride = struct.unpack('<II', r.read(inner['address']+0x70, 8))
        require(dim == 1 and 16 <= stride <= 2048 and 0 <= index < count, 'Chapter list bounds')
        row_type = r.struct_type(dict(address=inner['address'], type='StructProperty'))
        source = r.property(row_type, 'DataSource')
        require(source['type'] == 'StructProperty', 'Unexpected chapter DataSource type')
        binding = r.struct_type(source)
        pp = r.property(binding, 'DataSourceProvider')
        require(pp['type'] == 'InterfaceProperty' and 0 <= source['offset'] <= stride-8 and
                0 <= pp['offset'] <= stride-source['offset']-8, 'Chapter provider slot outside row')
        rows = []
        for ordinal in range(count):
            slot = data+ordinal*stride+source['offset']+pp['offset']
            provider = registered(r, r.u64(slot))
            require(r.class_name(provider) == 'UIDataProvider_WarfareMaps', 'Unexpected chapter provider class')
            values = {}
            for name in FIELDS:
                prop = reader.prop(provider, name, 'StrProperty')
                pointer, length, capacity = struct.unpack('<Qii', r.read(provider['address']+prop['offset'], 16))
                require(pointer and 0 < length <= capacity <= 2048, 'Chapter string bounds')
                text = r.read(pointer, length*2).decode('utf-16-le')
                require(text.endswith('\0') and '\0' not in text[:-1], 'Chapter string terminator')
                values[name] = text[:-1]
            rows.append(dict(index=ordinal, slot=slot, provider=copy.deepcopy(provider), values=values))
        require(len({row['provider']['address'] for row in rows}) == count, 'Duplicate chapter provider')
        for (address, size), raw in reads.items():
            require(original(address, size) == raw, 'Chapter source changed at final reread')
    finally:
        r.read = original
    after = reader.sample()
    _chapter(after, expected_identity[0])
    require(fingerprint(before) == fingerprint(after) and reader.integer(obj, 'Index') == index,
            'Chapter UI/index changed after provider capture')
    check()
    return dict(fingerprint=fingerprint(after), scene=scene, list=obj, index=index, rows=rows)


def wait_arrow_ack(snapshot, baseline, expected_index, bound_window, window, deadline, evidence, *,
                   now=time.monotonic, sleep=time.sleep):
    """Read-only acknowledgement; no dispatch/retry and no source relaxation."""
    until=min(deadline,now()+5.)
    old_index=baseline['index']
    evidence.update(from_index=old_index,expected_index=expected_index,observed_index=None,reads=0)
    try:
        while now()<until:
            require(window()==bound_window,'Chapter window changed during arrow acknowledgement')
            observed=snapshot()
            evidence['observed_index']=observed['index'];evidence['reads']+=1
            # Only Index may change. Scene/list/provider/fingerprint proof is identical.
            invariant=dict(observed,index=old_index)
            require(invariant==baseline,'Chapter acknowledgement source identity/provider/fingerprint changed')
            require(window()==bound_window,'Chapter window changed during arrow acknowledgement')
            require(now()<until,'Chapter arrow acknowledgement timed out; key not retried')
            require(observed['index']in (old_index,expected_index),'Chapter arrow acknowledgement skipped or reversed')
            if observed['index']==expected_index:
                evidence['acknowledged']=True
                return observed
            sleep(min(.15,max(0,until-now())))
        raise ChapterSelectionRejected('Chapter arrow acknowledgement timed out; key not retried')
    except Exception as error:
        prior=getattr(error,'detail',None)
        error.detail=dict(arrow_ack=copy.deepcopy(evidence),source_detail=prior)
        if isinstance(error,ChapterSelectionRejected):
            error.args=(str(error)+f" (from_index={old_index}, expected_index={expected_index}, observed_index={evidence['observed_index']})",)
        raise


def select_chapter(reader, expected_identity, values, *, verify, window, send, deadline,
                   now=time.monotonic, sleep=time.sleep, identity=reader_process_identity,
                   max_actions=32):
    """Select by current provider triple; dispatch up/down/confirm key pairs only.

    send(action, (pid, hwnd)) must synchronously send exactly one ordinary key
    down/up pair, release in finally, and raise on uncertain delivery. No retries.
    verify() must bind current game creation, owner/session and cancellation.
    This returns after chapter Enter; it never confirms difficulty/save dialogs.
    """
    require(type(values) is dict and set(values) == set(FIELDS) and
            all(type(values[k]) is str and values[k] for k in FIELDS), 'Exact chapter triple required')
    require(type(max_actions) is int and 1 <= max_actions <= 32 and now() < deadline, 'Invalid chapter action bound')
    deadline = min(deadline, now()+60)
    actions, expected_index = [], None
    bound_window = None
    def snapshot():
        return capture_chapters(reader, expected_identity, verify, deadline, now=now, identity=identity)
    for _ in range(max_actions):
        first = snapshot()
        if expected_index is not None:require(first['index'] == expected_index, 'Chapter arrow not acknowledged exactly once')
        matches = [row for row in first['rows'] if row['values'] == values]
        require(len(matches) == 1, 'Expected exactly one matching current chapter provider')
        selected = matches[0]
        second = snapshot()
        require(first == second, 'Chapter identity/provider/fingerprint changed before dispatch')
        current_window = window()
        require(type(current_window) is tuple and len(current_window) == 2 and
                current_window[0] == expected_identity[0] and current_window[1], 'Chapter HWND/PID mismatch')
        if bound_window is None:bound_window = current_window
        require(current_window == bound_window, 'Chapter window changed')
        verify()
        require(identity(reader) == tuple(expected_identity) and now() < deadline and window() == bound_window,
                'Chapter dispatch identity/deadline/window changed')
        action = 'confirm' if first['index'] == selected['index'] else ('down' if first['index'] < selected['index'] else 'up')
        actions.append(dict(action=action, from_index=first['index'], target_provider=copy.deepcopy(selected), reserved=True))
        send(action, bound_window)
        actions[-1]['dispatched'] = True
        if action == 'confirm':
            return dict(chapter_enter_dispatched=True, chapter_load_completed=False,
                        expected_identity=tuple(expected_identity), window=bound_window,
                        selected_provider=selected, actions=actions,
                        next_screen_requires_independent_review=True)
        expected_index = first['index']+(1 if action == 'down' else -1)
        actions[-1]['arrow_ack']={}
        wait_arrow_ack(snapshot,first,expected_index,bound_window,window,deadline,actions[-1]['arrow_ack'],now=now,sleep=sleep)
    raise ChapterSelectionRejected('Chapter action budget exhausted; no further input')
