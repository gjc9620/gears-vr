"""Bounded reflected navigation hints and paths; never invoke game AI/native code."""
import argparse
import hashlib
import heapq
import json
import math
from pathlib import Path
import struct
import time

from core import atomic_json, gameplay


class RouteProgress:
    """Expire only when the current waypoint has no meaningful approach."""
    def __init__(self, *, seconds=30, minimum_progress=12):
        self.seconds = seconds
        self.minimum_progress = minimum_progress
        self.reset()

    def reset(self):
        self.waypoint = None
        self.best_distance = None
        self.last_progress = None

    def stalled(self, waypoints, position, now):
        if not waypoints:
            self.reset()
            return False
        waypoint = (len(waypoints), tuple(waypoints[0]))
        distance = math.dist(position, waypoints[0])
        if (waypoint != self.waypoint or self.best_distance is None or
                distance <= self.best_distance - self.minimum_progress):
            self.waypoint = waypoint
            self.best_distance = distance
            self.last_progress = now
        return now - self.last_progress >= self.seconds


def walking_requirements(fields, radius, height):
    """Conservative input-policy admission, never a physics clearance proof."""
    flags, clearance_radius, clearance_height = (fields.get(name) for name in
        ('reachFlags', 'CollisionRadius', 'CollisionHeight'))
    return (type(flags) is int and flags == 1 and
            type(clearance_radius) is int and type(clearance_height) is int and
            0 < clearance_radius <= 10000 and 0 < clearance_height <= 10000 and
            type(radius) in (int, float) and type(height) in (int, float) and
            math.isfinite(radius) and math.isfinite(height) and
            0 < radius <= clearance_radius and 0 < height <= clearance_height)


def walking_edge(edge, *, allow_forced=True):
    return (edge.get('walking_hint_eligible') is True and
            (edge.get('class') == 'ReachSpec' or
             (allow_forced and edge.get('class') == 'ForcedReachSpec' and
              edge.get('ground_hint_eligible') is True)))


def shortest_path(nodes, edges, start, goal, *, allow_forced=True):
    if start not in nodes or goal not in nodes:
        raise ValueError('Route endpoints are outside the observed graph')
    adjacency = {}
    for edge in edges:
        allowed = walking_edge(edge, allow_forced=allow_forced)
        if (not allowed or edge['start'] not in nodes or edge['end'] not in nodes
                or not isinstance(edge.get('distance'), (int, float))
                or not math.isfinite(edge['distance']) or edge['distance'] <= 0):
            continue
        adjacency.setdefault(edge['start'], []).append((edge['end'], edge['distance']))
    queue, cost, previous = [(0, start)], {start: 0}, {}
    while queue:
        distance, point = heapq.heappop(queue)
        if distance != cost[point]:
            continue
        if point == goal:
            route = [goal]
            while route[-1] != start:
                route.append(previous[route[-1]])
            return list(reversed(route))
        for target, length in adjacency.get(point, []):
            candidate = distance + length
            if candidate < cost.get(target, float('inf')):
                cost[target], previous[target] = candidate, point
                heapq.heappush(queue, (candidate, target))
    return None


def choose_route(graph, position, *, avoid=(), target=None, goal_node=None):
    """Choose a bounded exploration route in this exact observed graph only."""
    nodes = graph['nodes']
    if graph.get('consistent_reread') is not True or not nodes:
        return None
    nearby = [(math.dist(position, n['position']), key) for key, n in nodes.items()
              if abs(position[2] - n['position'][2]) <= 180]
    if not nearby:
        return None
    distance, start = min(nearby)
    if distance > 400:
        return None
    # Exploration retires reached goals. A specific trigger must keep its best
    # endpoint when a later graph is acquired during the same approach.
    avoided = set(avoid) if target is None else set()
    candidates = sorted(((math.dist(nodes[start]['position'], n['position']), key)
                         for key, n in nodes.items() if key != start and key not in avoided), reverse=True)
    if target is not None:
        candidates.sort(key=lambda pair: math.dist(nodes[pair[1]]['position'], target))
    if goal_node is not None:
        if goal_node not in nodes:
            return None
        candidates = [(math.dist(nodes[start]['position'], nodes[goal_node]['position']), goal_node)]
    for allow_forced in (False, True):
        for length, goal in candidates:
            if goal_node is None and (length < 150 or length > (30000 if target is not None else 3000)):
                continue
            # A known ordinary route to this trigger takes precedence over
            # forced links that may cross a door whose state is still unknown.
            if (target is not None and not allow_forced and
                    (math.dist(nodes[goal]['position'], target) > 400 or
                     abs(nodes[goal]['position'][2] - target[2]) > 180)):
                continue
            path = shortest_path(nodes, graph['edges'], start, goal, allow_forced=allow_forced)
            if path and (len(path) >= 2 or goal_node is not None):
                waypoints = [nodes[key]['position'] for key in path]
                arrival_radii = [8 for key in path]
                # A nearby cover-slot start is a graph attachment hint. Its
                # authored center can sit inside cover collision; do not turn
                # back into that cover merely to begin the outgoing route.
                if (len(path) > 1 and nodes[path[0]].get('identity', {}).get('name') == 'CoverSlotMarker'
                        and math.hypot(position[0] - waypoints[0][0], position[1] - waypoints[0][1]) <= 50):
                    arrival_radii[0] = 50
                final_gap = math.dist(waypoints[-1], target) if target is not None else None
                approach_target = bool(target is not None and final_gap <= 400 and
                                       abs(waypoints[-1][2] - target[2]) <= 180)
                if approach_target:
                    # This short final approach is still an unverified walking hint;
                    # collisions and the current interaction prompt remain decisive.
                    waypoints.append(list(target))
                    arrival_radii.append(50)
                return dict(start=start, goal=goal, nodes=path,
                            waypoints=waypoints, waypoint_arrival_radii=arrival_radii,
                            precision_is_clearance_proof=False, approach_target=approach_target,
                            final_approach_distance=final_gap if approach_target else None,
                            final_approach_walkability_verified=False,
                            forced_hints_allowed=allow_forced,
                            committed_destination=goal_node is not None,
                            nearest_node_distance=distance, route_is_hint=True)
    return None


def capture(observer, *, max_nodes=2048, max_scan=8192, seconds=12):
    observation = observer.sample()
    if observation['errors'] or not gameplay(observation.get('menu') or {}) or not observation.get('pose'):
        raise RuntimeError('Navigation discovery requires fresh playable state')
    r, sampler = observer.resolver, observer.sampler
    properties = {}
    identities, reads = {}, {}
    started = time.monotonic()
    deadline = started + seconds
    byte_count = 0

    def read(address, size):
        nonlocal byte_count
        if time.monotonic() > deadline or len(reads) >= 65536 or byte_count + size > 8 * 1024 * 1024:
            raise RuntimeError('Navigation acquisition budget exceeded: ' + repr(dict(
                elapsed=round(time.monotonic() - started, 3), seconds=seconds,
                unique_reads=len(reads), maximum_reads=65536, bytes=byte_count)))
        value = r.read(address, size)
        key = (address, size)
        if key in reads and reads[key] != value:
            raise RuntimeError('Navigation data changed during acquisition')
        reads[key] = value
        byte_count += size
        return value

    def identity(address):
        obj = sampler.fresh(address)
        if address in identities and obj != identities[address]:
            raise RuntimeError('Navigation object identity changed')
        identities[address] = obj
        return obj

    def field(obj, name, kind, *, structure=False):
        owner = obj['address'] if structure else obj['cls']
        key = (owner, name)
        if key in properties:
            prop, size = properties[key]
            if prop['type'] != kind:
                raise RuntimeError('Unexpected cached navigation property type: ' + name)
            return prop, size
        prop = r.property(owner, name, not structure)
        if prop['type'] != kind:
            raise RuntimeError('Unexpected navigation property type: ' + name)
        identity(prop['address'])
        metadata = read(prop['address'] + 0x70, 0x28)
        dim, size = struct.unpack_from('<II', metadata)
        offset = struct.unpack_from('<I', metadata, 0x24)[0]
        if dim != 1 or not 0 < size <= 512 or offset != prop['offset']:
            raise RuntimeError('Navigation property layout changed')
        properties[key] = (prop, size)
        return prop, size

    def pointer(obj, name):
        prop, size = field(obj, name, 'ObjectProperty')
        if size != 8:
            raise RuntimeError('Navigation object pointer width mismatch')
        value = struct.unpack('<Q', read(obj['address'] + prop['offset'], 8))[0]
        if value:
            identity(value)
        return value

    def vector(obj):
        prop, size = field(obj, 'Location', 'StructProperty')
        typ = r.struct_type(prop)
        if identity(typ)['name'] != 'Vector' or size != 12:
            raise RuntimeError('Navigation Location is not Vector')
        for name, offset in (('X', 0), ('Y', 4), ('Z', 8)):
            component, width = field(identity(typ), name, 'FloatProperty', structure=True)
            if width != 4 or component['offset'] != offset:
                raise RuntimeError('Navigation Vector layout mismatch')
        value = struct.unpack('<3f', read(obj['address'] + prop['offset'], 12))
        if not all(math.isfinite(v) for v in value):
            raise RuntimeError('Nonfinite navigation location')
        return list(value)

    def scalar(obj, name, kind, fmt):
        prop, width = field(obj, name, kind)
        if width != 4:
            raise RuntimeError('Navigation scalar width mismatch: ' + name)
        return struct.unpack(fmt, read(obj['address'] + prop['offset'], 4))[0]

    registry = [(r.objects_header, r.read(r.objects_header, 16)),
                (r.names_header, r.read(r.names_header, 16))]
    pawn = identity(sampler.pawn)
    world = identity(pointer(pawn, 'WorldInfo'))
    collision, width = field(pawn, 'CollisionComponent', 'ComponentProperty')
    if width != 8:
        raise RuntimeError('Navigation pawn collision pointer width mismatch')
    cylinder = identity(struct.unpack('<Q', read(pawn['address'] + collision['offset'], 8))[0])
    if cylinder['outer'] != pawn['address'] or not sampler.derives(cylinder['cls'], 'CylinderComponent'):
        raise RuntimeError('Navigation requires the current pawn collision cylinder')
    pawn_radius = scalar(cylinder, 'CollisionRadius', 'FloatProperty', '<f')
    pawn_height = scalar(cylinder, 'CollisionHeight', 'FloatProperty', '<f')
    if not all(math.isfinite(v) and 0 < v <= 300 for v in (pawn_radius, pawn_height)):
        raise RuntimeError('Navigation pawn collision dimensions rejected')
    current = pointer(world, 'NavigationPointList')
    scanned, edges, path_objects = {}, [], []
    while current and len(scanned) < max_scan:
        key = hex(current)
        if key in scanned:
            raise RuntimeError('Navigation list cycle')
        node = identity(current)
        if not sampler.derives(node['cls'], 'NavigationPoint') or pointer(node, 'WorldInfo') != world['address']:
            raise RuntimeError('NavigationPoint ancestry/world mismatch')
        scanned[key] = dict(identity=node, position=vector(node))
        current = pointer(node, 'nextNavigationPoint')
    chain_truncated = bool(current)
    origin = observation['pose']['position']
    selected = sorted(scanned, key=lambda key: math.dist(origin, scanned[key]['position']))[:max_nodes]
    nodes = {key: scanned[key] for key in selected}
    for key, record in nodes.items():
        node = record['identity']
        current = node['address']
        prop, size = field(node, 'PathList', 'ArrayProperty')
        if size != 16:
            raise RuntimeError('Navigation PathList header size mismatch')
        header = read(current + prop['offset'], 16)
        data, count, capacity = struct.unpack('<Qii', header)
        if not 0 <= count <= capacity <= 256 or (count and not data):
            raise RuntimeError('Navigation PathList bounds rejected')
        inner = identity(r.u64(prop['address'] + r.profile['struct_property_struct']))
        if inner['outer'] != prop['address'] or r.class_name(inner) != 'ObjectProperty':
            raise RuntimeError('Navigation PathList element metadata rejected')
        raw = read(data, count * 8) if count else b''
        path_objects.extend((current, value) for value in struct.unpack('<' + 'Q' * count, raw) if value)
    truncated = chain_truncated or len(nodes) < len(scanned)
    unsupported = {}
    for start, address in path_objects:
        reach = identity(address)
        class_name = identity(reach['cls'])['name']
        # Special climb/mantle/door/lift edges require different inputs and are
        # retained as unsupported instead of becoming ordinary walking routes.
        if class_name not in ('ReachSpec', 'ForcedReachSpec'):
            record = unsupported.setdefault(class_name, dict(references=0, examples=[]))
            record['references'] += 1
            if len(record['examples']) < 3:
                record['examples'].append(dict(start=hex(start), start_position=nodes[hex(start)]['position'],
                                               identity=reach))
            continue
        if class_name == 'ForcedReachSpec' and not sampler.derives(reach['cls'], 'ReachSpec'):
            raise RuntimeError('ForcedReachSpec ancestry mismatch')
        if pointer(reach, 'Start') != start:
            raise RuntimeError('ReachSpec start backlink mismatch')
        end, width = field(reach, 'End', 'StructProperty')
        typ = r.struct_type(end)
        if identity(typ)['name'] != 'NavReference':
            raise RuntimeError('ReachSpec.End is not NavReference')
        nav, nav_width = field(identity(typ), 'Nav', 'ObjectProperty', structure=True)
        if nav_width != 8 or nav['offset'] + 8 > width:
            raise RuntimeError('ReachSpec.End.Nav layout rejected')
        target = struct.unpack('<Q', read(address + end['offset'] + nav['offset'], 8))[0]
        if not target or hex(target) not in nodes:
            continue
        identity(target)
        requirements = {name: scalar(reach, name, 'IntProperty', '<i') for name in
                        ('reachFlags', 'CollisionRadius', 'CollisionHeight')}
        # Only the exact supported mask is admitted. Extra bits may require
        # traversal inputs which this walking controller does not implement.
        walking_hint = walking_requirements(requirements, pawn_radius, pawn_height)
        distance = math.dist(nodes[hex(start)]['position'], nodes[hex(target)]['position'])
        forced_ground = class_name == 'ForcedReachSpec'
        if forced_ground and (distance > 1200 or abs(nodes[hex(start)]['position'][2] - nodes[hex(target)]['position'][2]) > 180):
            record = unsupported.setdefault('ForcedReachSpec-outside-ground-bound', dict(references=0, examples=[]))
            record['references'] += 1
            continue
        edges.append(dict(start=hex(start), end=hex(target), distance=distance,
                          identity=reach, ground_hint_eligible=forced_ground,
                          requirements=requirements, walking_hint_eligible=walking_hint,
                          **{'class': class_name}))
    observer.verify()
    changed_identity = next((hex(address) for address, value in identities.items()
                             if sampler.fresh(address) != value), None)
    changed_read = next(((hex(address), size) for (address, size), value in reads.items()
                         if r.read(address, size) != value), None)
    registry_changes = []
    for address, value in registry:
        current = r.read(address, len(value))
        if current != value:
            registry_changes.append(dict(address=hex(address), before=struct.unpack('<Qii', value),
                                         after=struct.unpack('<Qii', current)))
    if changed_identity or changed_read or registry_changes:
        raise RuntimeError('Navigation graph changed at verification boundary: ' +
                           repr(dict(identity=changed_identity, read=changed_read, registry=registry_changes)))
    source_digest = hashlib.sha256()
    for (address, size), raw in sorted(reads.items()):
        source_digest.update(struct.pack('<QI', address, size) + raw)
    return dict(schema=1, game_identity=observer.identities['game_identity'],
                disk_game_sha256=r.profile['sha256'], module_base=hex(r.base),
                source_sample=observation, world_identity=world, nodes=nodes, edges=edges,
                player_collision=dict(identity=cylinder, radius=pawn_radius, height=pawn_height),
                walking_policy='exact-reach-mask-1-and-current-cylinder-dimensions',
                truncated=truncated, chain_truncated=chain_truncated, scanned_nodes=len(scanned),
                selection='nearest max_nodes to current observed pawn position',
                consistent_reread=True, atomic_snapshot=False,
                source_bytes_sha256=source_digest.hexdigest(), selected_read_bytes=byte_count,
                selected_read_calls=len(reads), acquisition_seconds=time.monotonic() - started,
                unsupported_reachspec_classes=unsupported,
                route_is_hint=True, actual_walkability_verified=False,
                limits='Typed reach requirements are input-policy hints; current collision, special traversal and mission completion remain unverified.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    from live import Observer
    observer = None
    try:
        observer = Observer(args.session)
        graph = capture(observer)
        atomic_json(args.output, graph)
        print(json.dumps(dict(nodes=len(graph['nodes']), edges=len(graph['edges']),
                              truncated=graph['truncated'], output=str(args.output))))
    except Exception as error:
        value = dict(schema=1, status='rejected', session_path=str(args.session.resolve()),
                     error=dict(type=type(error).__name__, message=str(error)),
                     consistent_reread=False, route_is_hint=True, actual_walkability_verified=False)
        atomic_json(args.output, value)
        print(json.dumps(value))
        return 1
    finally:
        if observer:
            observer.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
