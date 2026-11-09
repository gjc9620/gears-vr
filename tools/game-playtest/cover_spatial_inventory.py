"""Bounded read-only inventory of ALL records reachable from one tree root.

This visits every reachable child, without spatial pruning or engine stamps.
It is not all of UWorld: overflow/other hash storage, BSP, collision filters,
shape coverage and the physics epoch are separate, unverified sources. Even an
empty inventory cannot authorize a no-hit result, occupancy or movement.
"""
import struct

import cover_spatial_root as root_source


# Observation limits, not claimed native maxima. Reaching a limit rejects the
# whole observation; it never returns an accepted truncated inventory.
MAX_NODES = 4096
MAX_DEPTH = 64
MAX_COMPONENT_SLOTS = 16384
MAX_COMPONENTS = 4096
MAX_REVERSE_MEMBERS = 4096
REVERSE_MEMBERS_OFFSET = 0x164


def _pointer_array(q, address, count, *, label):
    if not count:
        return []
    root_source._pointer(address, count * 8, label=label)
    raw = q.read(address, count * 8)
    values = list(struct.unpack('<' + str(count) + 'Q', raw))
    for pointer in values:
        root_source._pointer(pointer, 1, label=label + ' element')
    if len(set(values)) != len(values):
        raise RuntimeError('Spatial inventory duplicate within ' + label)
    return values


def observe(q, module, world_info):
    """Append root, whole reachable tree and bidirectional members to q.

    Caller must finish that SAME query. No new observation/session lifecycle,
    process write, native call or input is created. All result permissions stay
    false even when every reachable record and reverse association agrees.
    """
    origin = root_source.observe(q, module, world_info)
    first = origin['root']
    q.remember(first['address'], bytes.fromhex(first['header_bytes']))
    for child in origin['child_headers']:
        q.remember(child['address'], bytes.fromhex(child['header_bytes']))
    stack = [(first['address'], None, None, 0)]
    # Record allocations, including cookie. Aliased child ownership would make
    # traversal and the native recursive destructor ambiguous; reject it.
    allocations = [(first['address'], first['address'] + root_source.NODE_BYTES)]
    queued = {first['address']}
    nodes, components, slots = [], {}, 0
    while stack:
        address, parent, child_index, depth = stack.pop()
        row = root_source._node(q.read(address, root_source.NODE_BYTES), address)
        if slots + row['count'] > MAX_COMPONENT_SLOTS:
            raise RuntimeError('Spatial inventory total component slot bound exceeded')
        pointers = _pointer_array(q, row['data'], row['count'], label='node components')
        slots += len(pointers)
        for component in pointers:
            if component not in components:
                if len(components) >= MAX_COMPONENTS:
                    raise RuntimeError('Spatial inventory unique component bound exceeded')
                components[component] = []
            components[component].append(address)
        row.update(parent=parent, child_index=child_index, depth=depth)
        nodes.append(row)
        if not row['children']:
            continue
        if depth >= MAX_DEPTH:
            raise RuntimeError('Spatial inventory depth bound exceeded')
        if len(queued) + root_source.CHILDREN > MAX_NODES:
            raise RuntimeError('Spatial inventory node bound exceeded')
        start = row['children']
        low, high = start - root_source.COOKIE_BYTES, start + root_source.CHILDREN * root_source.NODE_BYTES
        if any(low < end and begin < high for begin, end in allocations):
            raise RuntimeError('Spatial inventory child allocation alias/cycle/overlap rejected')
        cookie = struct.unpack('<Q', q.read(low, root_source.COOKIE_BYTES))[0]
        if cookie != root_source.CHILDREN:
            raise RuntimeError('Spatial inventory child allocation cookie rejected')
        raw = q.read(start, root_source.CHILDREN * root_source.NODE_BYTES)
        allocations.append((low, high))
        children = []
        for index in range(root_source.CHILDREN):
            child_address = start + index * root_source.NODE_BYTES
            child_raw = raw[index * root_source.NODE_BYTES:(index + 1) * root_source.NODE_BYTES]
            root_source._node(child_raw, child_address)
            q.remember(child_address, child_raw)
            if child_address in queued:
                raise RuntimeError('Spatial inventory repeated node rejected')
            queued.add(child_address)
            children.append((child_address, address, index, depth + 1))
        stack.extend(reversed(children))
    if len(nodes) != len(queued):
        raise RuntimeError('Spatial inventory traversal did not consume every queued node')

    records = []
    for address, forward in components.items():
        component = root_source._instance(q, address, 'PrimitiveComponent')
        # Native field starts at +164, i.e. 4-byte alignment. Do not round it
        # to +168 or use an 8-aligned generic TArray header assumption.
        header_address = address + REVERSE_MEMBERS_OFFSET
        root_source._pointer(header_address, 16, alignment=4, label='reverse membership header')
        header = q.read(header_address, 16)
        data, count, capacity = struct.unpack('<Qii', header)
        if not 0 <= count <= capacity <= root_source.MAX_NODE_CAPACITY or count > MAX_REVERSE_MEMBERS:
            raise RuntimeError('Spatial inventory reverse membership count/capacity bound rejected')
        if capacity:
            root_source._pointer(data, capacity * 8, label='reverse membership storage')
        elif data:
            raise RuntimeError('Spatial inventory reverse zero capacity has storage')
        reverse = _pointer_array(q, data, count, label='reverse membership')
        if set(reverse) != set(forward):
            raise RuntimeError('Spatial inventory forward/reverse membership differs')
        records.append(dict(identity=component, node_addresses=forward,
                            reverse_header_address=header_address,
                            reverse_header_bytes=header.hex(), reverse_nodes=reverse))
    return dict(origin=origin, nodes=nodes, components=records,
                node_count=len(nodes), component_count=len(records), component_slots=slots,
                read_only=True, all_reachable_tree_records_observed=True,
                forward_reverse_membership_observed=True, closing_required=True,
                native_stamp_written=False, engine_calls=False, virtual_calls=False,
                empty_reachable_tree=not components, valid_empty_collision_result=False,
                overflow_or_other_hash_storage_observed=False, bsp_observed=False,
                owner_world_association_verified=False, shape_or_filter_coverage_verified=False,
                complete_scene_verified=False, atomic_snapshot_verified=False,
                world_synchronization_verified=False, path_or_occupancy_verified=False,
                activation_allowed=False)
