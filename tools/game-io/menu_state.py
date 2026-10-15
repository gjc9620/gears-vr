"""Read the current UE3 menu and player-zero focus chain without screenshots.

All addresses are discovered anew from version-gated registries. This module
does not inject code, write game memory, or send input.
"""
import argparse
import json
from pathlib import Path
import struct
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/package-inspector'))
from resolve_camera import Resolver
from inspect_ui_state import registered, describe

def compact_state(value):
    top, leaf = value.get('top') or {}, value.get('focused_leaf') or {}
    return dict(pid=value['pid'], scene=top.get('tag'), scene_class=top.get('cls'),
                focused=leaf.get('name'), input_block_count=value.get('input_block_count'),
                dialog_title=(value.get('dialog_text') or {}).get('Title'),
                player_index=value.get('player_index'), consistent_reread=value.get('consistent_reread'),
                sample_ms=value.get('sample_ms'))

class DialogStorageChanged(RuntimeError):
    """A valid source-string acquisition changed while it was being read."""


class SnapshotChanged(RuntimeError):
    """An observed ledger read changed; discard this entire acquisition."""


def read_root_source_text(read, nodes_address, base, attempts=3):
    """Acquire one root/source string with a short consistent storage bracket.

    UIString.Nodes and its native root are regenerated during line wrapping.
    No storage pointer survives this acquisition; callers compare text semantics
    at the final snapshot boundary while retaining all UObject/scene checks.
    """
    for attempt in range(attempts):
        header = read(nodes_address, 16)
        data, count, capacity = struct.unpack('<Qii', header)
        if not 0 <= count <= capacity <= 64 or (data and data % 8) or (count and not data):
            raise RuntimeError('Invalid bounded array: Nodes')
        if not count:
            # A well-formed empty array is a wrapping rebuild, not usable text.
            if attempt + 1 == attempts:
                raise DialogStorageChanged('Invalid bounded array: Nodes stayed empty within bounded acquisition')
            continue
        root = read(data, 8)
        node = struct.unpack('<Q', root)[0]
        if not node or node % 8:
            raise RuntimeError('Invalid dialog root pointer')
        vtable, parent = read(node, 8), read(node + 0x10, 8)
        if struct.unpack('<Q', vtable)[0] - base not in (0x1348780, 0x13a4398) or parent != bytes(8):
            raise RuntimeError('Unsupported dialog node vtable/parent')
        string_header = read(node + 0x18, 16)
        pointer, length, string_capacity = struct.unpack('<Qii', string_header)
        if not pointer or pointer % 2 or not 1 <= length <= string_capacity <= 1024:
            raise RuntimeError('Invalid dialog text bounds')
        raw = read(pointer, length * 2)
        if (read(pointer, length * 2) != raw or
                read(node + 0x18, 16) != string_header or
                read(node + 0x10, 8) != parent or read(node, 8) != vtable or
                read(data, 8) != root or read(nodes_address, 16) != header):
            if attempt + 1 == attempts:
                raise DialogStorageChanged('Dialog source storage did not settle within bounded acquisition')
            continue
        text = raw.decode('utf-16-le')
        if not text.endswith('\0') or '\0' in text[:-1]:
            raise RuntimeError('Invalid dialog text terminator')
        return text[:-1]
    raise DialogStorageChanged('No dialog source acquisition attempts')


class MenuReader:
    def __init__(self, pid):
        profile = json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
        self.r = Resolver(pid, profile)
        self.properties = {}
        try:
            self.r.prepare()
            candidates = []
            for address in self.r.object_pointers:
                if not address:
                    continue
                try:
                    obj = registered(self.r, address)
                    if obj['name'] == 'GameUISceneClient' and self.r.class_name(obj) == 'GameUISceneClient':
                        candidates.append(address)
                except (RuntimeError, OSError, ValueError, UnicodeError):
                    continue
            if len(candidates) != 1:
                raise RuntimeError('Expected one registered GameUISceneClient; found '+str(len(candidates)))
            self.client = candidates[0]
            self.client_identity = dict(registered(self.r, self.client))
        except Exception:
            self.close()
            raise

    def close(self):
        self.r.p.close()

    def prop(self, obj, name, kind):
        key = (obj['cls'], name)
        if key not in self.properties:
            self.properties[key] = self.r.property(obj['cls'], name, True)
        prop = self.properties[key]
        if prop['type'] != kind:
            raise RuntimeError('Unexpected property type: '+name)
        return prop

    def pointer(self, obj, name):
        return self.r.u64(obj['address'] + self.prop(obj, name, 'ObjectProperty')['offset'])

    def integer(self, obj, name):
        return struct.unpack('<i', self.r.read(obj['address'] + self.prop(obj, name, 'IntProperty')['offset'], 4))[0]

    def array(self, obj, name, inner_kind, limit=128):
        prop = self.prop(obj, name, 'ArrayProperty')
        inner = registered(self.r, self.r.u64(prop['address'] + self.r.profile['struct_property_struct']))
        if inner['outer'] != prop['address'] or self.r.class_name(inner) != inner_kind:
            raise RuntimeError('Array inner metadata mismatch: '+name)
        data, count, capacity = struct.unpack('<Qii', self.r.read(obj['address'] + prop['offset'], 16))
        if not 0 <= count <= capacity <= limit or (count and not data):
            raise RuntimeError('Invalid bounded array: '+name)
        return data, count, inner

    def objects(self, obj, name):
        data, count, _ = self.array(obj, name, 'ObjectProperty')
        pointers = struct.unpack('<'+'Q'*count, self.r.read(data, count*8)) if count else ()
        for pointer in pointers:
            registered(self.r, pointer)
        return pointers

    def focus(self, obj):
        data, count, inner = self.array(obj, 'FocusControls', 'StructProperty', 32)
        if not count:
            return 0
        structure = self.r.struct_type({'address':inner['address'], 'type':'StructProperty'})
        if registered(self.r, structure)['name'] != 'PlayerInteractionData':
            raise RuntimeError('Unexpected focus struct')
        focused = self.r.property(structure, 'FocusedControl')
        previous = self.r.property(structure, 'LastFocusedControl')
        if focused['type'] != 'ObjectProperty' or previous['type'] != 'ObjectProperty':
            raise RuntimeError('Unexpected focus members')
        if (focused['offset'], previous['offset']) != (0, 8):
            raise RuntimeError('Unsupported reflected player focus layout')
        # Only player zero is supported; no guessed struct stride is used.
        return self.r.u64(data + focused['offset'])

    def refresh(self):
        r = self.r
        relocated = False
        for prefix, minimum in (('names', 1000), ('objects', 1000)):
            header = getattr(r, prefix+'_header')
            data, count, capacity = struct.unpack('<Qii', r.read(header, 16))
            if not minimum < count <= capacity < 4000000:
                raise RuntimeError('Invalid refreshed registry')
            singular = 'name' if prefix == 'names' else 'object'
            relocated = relocated or data != getattr(r, prefix+'_data')
            if data != getattr(r, prefix+'_data') or count != getattr(r, singular+'_count'):
                setattr(r, singular+'_pointers', struct.unpack('<'+'Q'*count, r.read(data, count*8)))
                setattr(r, prefix+'_data', data)
                setattr(r, singular+'_count', count)
                setattr(r, singular+'_capacity', capacity)
        if relocated:
            r.names_cache.clear()
            self.properties.clear()
            if r.name(0) != 'None':
                raise RuntimeError('Refreshed name registry zero entry changed')
        r.ident_cache.clear()

    def dialog_text(self, top):
        """Read two labels of the calibrated confirmation layout only.

        Native node +0x18 is an empirically validated FString for this build;
        reflected Pointer is opaque, so unknown vtables/formats are rejected.
        Object/type/ownership reads retain byte consistency. Native source storage
        gets a short bracket and a final semantic reread, because wrapping rebuilds it.
        """
        r, pending, seen, texts = self.r, [top['address']], set(), {}
        while pending:
            address = pending.pop()
            if address in seen or len(seen) >= 96:
                raise RuntimeError('Dialog child cycle/limit')
            seen.add(address)
            obj = registered(r, address)
            for child in self.objects(obj, 'Children'):
                if registered(r, child)['outer'] != address:
                    raise RuntimeError('Dialog child owner mismatch')
                pending.append(child)
            if obj['name'] not in ('Title', 'MessageText'):
                continue
            if r.class_name(obj) != 'UILabel' or obj['name'] in texts:
                raise RuntimeError('Ambiguous dialog label')
            prop = self.prop(obj, 'StringRenderComponent', 'ComponentProperty')
            component = registered(r, r.u64(address + prop['offset']))
            if component['outer'] != address or r.class_name(component) != 'UIComp_DrawString':
                raise RuntimeError('Dialog string component mismatch')
            value = registered(r, self.pointer(component, 'ValueString'))
            if value['outer'] != address or r.class_name(value) != 'UIString':
                raise RuntimeError('Dialog UIString owner mismatch')
            # Keep reflection and UObject metadata in the ordinary read ledger.
            nodes_prop = self.prop(value, 'Nodes', 'ArrayProperty')
            inner = registered(r, r.u64(nodes_prop['address'] + r.profile['struct_property_struct']))
            if inner['outer'] != nodes_prop['address'] or r.class_name(inner) != 'StructProperty':
                raise RuntimeError('Array inner metadata mismatch: Nodes')
            if registered(r, r.struct_type({'address': inner['address'], 'type': 'StructProperty'}))['name'] != 'Pointer':
                raise RuntimeError('Unsupported dialog native node layout')
            nodes_address = value['address'] + nodes_prop['offset']
            raw_read = getattr(self, '_dialog_raw_read', r.read)
            text = read_root_source_text(raw_read, nodes_address, r.base)
            texts[obj['name']] = text
            if hasattr(self, '_dialog_sources'):
                self._dialog_sources.append((nodes_address, text))
        if set(texts) != {'Title', 'MessageText'}:
            raise RuntimeError('Missing dialog labels')
        return texts

    def force_registry_refresh(self):
        """One bounded inventory refresh, retaining the originally proven client."""
        r = self.r
        headers = {prefix:r.read(getattr(r,prefix+'_header'),16) for prefix in ('names','objects')}
        inventories = {}
        for prefix,raw in headers.items():
            data,count,capacity = struct.unpack('<Qii',raw)
            if not data or not 1000<count<=capacity<4000000:
                raise RuntimeError('Invalid forced registry header')
            inventories[prefix] = (data,count,capacity,struct.unpack('<'+'Q'*count,r.read(data,count*8)))
        for prefix,(data,count,capacity,pointers) in inventories.items():
            singular = 'name' if prefix=='names' else 'object'
            setattr(r,prefix+'_data',data)
            setattr(r,singular+'_count',count)
            setattr(r,singular+'_capacity',capacity)
            setattr(r,singular+'_pointers',pointers)
        r.names_cache.clear();r.ident_cache.clear();self.properties.clear()
        if r.name(0)!='None':raise RuntimeError('Forced registry zero name changed')
        if dict(registered(r,self.client))!=self.client_identity:
            raise RuntimeError('Scene client identity changed during registry refresh')
        if any(r.read(getattr(r,prefix+'_header'),16)!=raw for prefix,raw in headers.items()):
            raise RuntimeError('Registry header changed during forced refresh')

    def sample(self, *, include_tree=False):
        try:
            return self._sample_once(include_tree=include_tree)
        except RuntimeError as error:
            registry_changed = type(error) is RuntimeError and str(error)=='Object registry identity changed'
            if not registry_changed and type(error) is not SnapshotChanged:raise
        # Both recoverable failures share exactly one retry. _sample_once has
        # restored r.read and discarded its failed ledger. No new timestamp,
        # preparation, client discovery or nested retry budget is introduced.
        if registry_changed:self.force_registry_refresh()
        return self._sample_once(include_tree=include_tree)

    def _sample_once(self, *, include_tree=False):
        self.refresh()
        r = self.r
        original_read = r.read
        reads = {}
        def observed(address, size):
            value = original_read(address, size)
            if (address, size) in reads and reads[(address, size)] != value:
                raise SnapshotChanged('UI changed during snapshot')
            reads[(address, size)] = value
            return value
        r.read = observed
        self._dialog_raw_read, self._dialog_sources = original_read, []
        try:
            registry = r.read(r.objects_header, 16)
            client = registered(r, self.client)
            if r.class_name(client) != 'GameUISceneClient':
                raise RuntimeError('Scene client replaced')
            interaction = registered(r, client['outer'])
            if r.class_name(interaction) != 'WarUIInteraction' or self.pointer(interaction, 'SceneClient') != self.client:
                raise RuntimeError('UI interaction backlink mismatch')
            blocked = self.integer(interaction, 'BlockUIInputSemaphore')
            scenes = self.objects(client, 'ActiveScenes')
            summary = [describe(r, address) for address in scenes]
            for scene in summary:
                obj = registered(r, int(scene['address'], 16))
                mode = self.prop(obj, 'SceneInputMode', 'ByteProperty')
                scene['input_mode'] = r.read(obj['address']+mode['offset'], 1)[0]
            result = dict(pid=r.pid, client=hex(self.client), active_scenes=summary,
                          input_block_count=blocked, top=None, focus_chain=[], focused_leaf=None,
                          read_only=True, player_index=None)
            if include_tree:result['tree'] = []
            if scenes:
                top = registered(r, scenes[-1])
                if self.pointer(top, 'SceneClient') != self.client:
                    raise RuntimeError('Scene client backlink mismatch')
                tag_prop = self.prop(top, 'SceneTag', 'NameProperty')
                tag, number = struct.unpack('<II', r.read(top['address']+tag_prop['offset'], 8))
                player_index = self.integer(top, 'LastPlayerIndex')
                result['top'] = dict(summary[-1], tag=r.name(tag), tag_number=number)
                result['player_index'] = player_index
                current, seen = top['address'], set()
                while current:
                    if current in seen or len(seen) >= 16:
                        raise RuntimeError('Focus cycle/depth limit')
                    seen.add(current)
                    obj = registered(r, current)
                    state_objects = self.objects(obj, 'StateStack')
                    states = []
                    for address in state_objects:
                        state = registered(r, address)
                        if state['outer'] != current:
                            raise RuntimeError('UI state owner mismatch')
                        states.append(r.class_name(state))
                    result['focus_chain'].append(dict(describe(r, current), states=states))
                    child = self.focus(obj)
                    if child:
                        if child not in self.objects(obj, 'Children') or registered(r, child)['outer'] != current:
                            raise RuntimeError('Focused child outside parent tree')
                    current = child
                result['focused_leaf'] = result['focus_chain'][-1] if result['focus_chain'] else None
                if include_tree:
                    # Optional inventory shares this snapshot's observed reads
                    # and final verification. No extra reads in the default path.
                    visited = set()
                    def visit(address, parent, depth):
                        if address in visited:raise RuntimeError('UI tree cycle/duplicate child')
                        if len(visited)>=128 or depth>16:raise RuntimeError('UI tree node/depth limit')
                        visited.add(address)
                        obj = registered(r, address)
                        if parent is not None and obj['outer']!=parent:
                            raise RuntimeError('UI tree child owner mismatch')
                        states = []
                        for state_address in self.objects(obj, 'StateStack'):
                            state = registered(r, state_address)
                            if state['outer']!=address:raise RuntimeError('UI state owner mismatch')
                            states.append(r.class_name(state))
                        children = self.objects(obj, 'Children')
                        focused = self.focus(obj)
                        if focused and (focused not in children or registered(r, focused)['outer']!=address):
                            raise RuntimeError('Focused child outside parent tree')
                        result['tree'].append(dict(describe(r, address),parent=hex(parent) if parent is not None else None,
                                                  depth=depth,states=states,focus_child=hex(focused) if focused else None))
                        for child in children:visit(child,address,depth+1)
                    visit(top['address'],None,1)
                if result['top']['tag'] == 'WarfareConfirmation' and r.class_name(top) == 'UIScene_Confirmation':
                    result['dialog_text'] = self.dialog_text(top)
            # Re-acquire current source strings; obsolete wrapping allocations are
            # not UI identity. Changed semantic text still rejects this snapshot.
            for nodes_address, text in self._dialog_sources:
                if read_root_source_text(original_read, nodes_address, r.base) != text:
                    raise RuntimeError('Dialog source text changed before snapshot verification')
            if self._dialog_sources:
                result['dialog_text_consistency'] = 'bounded_root_acquisition_and_semantic_reread'
            # Re-read every ordinary sampled value; no assertion of an atomic game lock.
            for (address, size), value in reads.items():
                if original_read(address, size) != value:
                    raise SnapshotChanged(f'UI changed before snapshot verification at {address:#x} size {size}')
            if registry != original_read(r.objects_header, 16):
                raise RuntimeError('Registry changed before snapshot verification')
            result['verified_reads'] = len(reads)
            result['consistent_reread'] = True
            return result
        finally:
            r.read = original_read
            del self._dialog_raw_read, self._dialog_sources

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--samples', type=int, default=1)
    parser.add_argument('--tree', action='store_true', help='Include bounded, consistently re-read top Children tree')
    args = parser.parse_args()
    reader = MenuReader(args.pid)
    results = []
    try:
        for _ in range(max(1, min(args.samples, 100))):
            began = time.perf_counter()
            value = reader.sample(include_tree=args.tree)
            value['sample_ms'] = round((time.perf_counter()-began)*1000, 3)
            results.append(value)
            print(json.dumps(value if args.tree else compact_state(value)), flush=True)
            if args.samples > 1:
                time.sleep(.1)
    finally:
        reader.close()
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(results, indent=2), encoding='utf-8')

if __name__ == '__main__':
    main()
