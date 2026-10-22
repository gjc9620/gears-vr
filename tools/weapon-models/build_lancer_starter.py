"""Build an original three-piece VR pull starter for the Lancer.

The retail chainsaw handle is retained. This small socket, T-grip and cord
form an additional controllable VR part. Local +X is the pull direction and
the origin is the intended gun-side socket; placement still needs headset QA.
"""
import argparse
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
MATERIALS = (
    ('Gunmetal socket', (0.11, 0.14, 0.16), .72, .52),
    ('Rubber grip', (0.045, 0.052, 0.058), .02, .82),
    ('Safety mark', (0.72, 0.54, 0.10), .25, .46),
    ('Starter cord', (0.025, 0.028, 0.03), .02, .92),
)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def unit(v):
    length = math.sqrt(dot(v, v))
    if length < 1e-10:
        raise ValueError('Degenerate generated face')
    return tuple(x / length for x in v)


class Primitive:
    def __init__(self):
        self.positions = []
        self.normals = []
        self.indices = []

    def triangle(self, a, b, c):
        normal = unit(cross(sub(b, a), sub(c, a)))
        start = len(self.positions)
        self.positions.extend((a, b, c))
        self.normals.extend((normal,) * 3)
        self.indices.extend((start, start + 1, start + 2))

    def quad(self, a, b, c, d):
        self.triangle(a, b, c)
        self.triangle(a, c, d)


def cylinder(mesh, a, b, radius, sides=16):
    axis = unit(sub(b, a))
    guide = (0, 0, 1) if abs(axis[2]) < .9 else (0, 1, 0)
    tangent = unit(cross(axis, guide))
    bitangent = cross(axis, tangent)

    def perimeter(center, angle):
        return tuple(center[i] + radius * (
            math.cos(angle) * tangent[i] + math.sin(angle) * bitangent[i])
            for i in range(3))

    for index in range(sides):
        angle0 = 2 * math.pi * index / sides
        angle1 = 2 * math.pi * (index + 1) / sides
        p0, p1 = perimeter(a, angle0), perimeter(a, angle1)
        q0, q1 = perimeter(b, angle0), perimeter(b, angle1)
        mesh.quad(p0, p1, q1, q0)
        mesh.triangle(a, p1, p0)
        mesh.triangle(b, q0, q1)


def geometry():
    # Three independent nodes let the eventual game adapter keep the socket
    # on the gun, place the grip under the left hand and lengthen the cord.
    socket = {0: Primitive(), 2: Primitive()}
    grip = {0: Primitive(), 1: Primitive(), 2: Primitive()}
    cord = {3: Primitive()}
    cylinder(socket[0], (-.006, 0, 0), (.004, 0, 0), .018)
    cylinder(socket[0], (.004, 0, 0), (.009, 0, 0), .011)
    cylinder(socket[2], (.004, 0, 0), (.006, 0, 0), .014)
    cylinder(socket[0], (.007, 0, 0), (.010, 0, 0), .004)
    for y in (-.012, .012):
        cylinder(socket[0], (.004, y, 0), (.0055, y, 0), .0015, 10)

    cylinder(grip[0], (.022, 0, 0), (.045, 0, 0), .004)
    cylinder(grip[1], (.045, -.034, 0), (.045, .034, 0), .007)
    for start in (-.033, .027):
        cylinder(grip[2], (.045, start, 0),
                 (.045, start + .006, 0), .0073)
    cylinder(grip[0], (.021, 0, 0), (.025, 0, 0), .005)

    cylinder(cord[3], (.010, 0, 0), (.022, 0, 0), .0018, 12)
    return (socket, grip, cord)


def pad4(raw, fill=b'\0'):
    return raw + fill * (-len(raw) % 4)


def glb_bytes(nodes):
    binary = bytearray()
    views, accessors, meshes = [], [], []

    def append_view(raw, target):
        offset = len(binary)
        binary.extend(pad4(raw))
        views.append({'buffer': 0, 'byteOffset': offset,
                      'byteLength': len(raw), 'target': target})
        return len(views) - 1

    for node in nodes:
        primitives = []
        for material, mesh in sorted(node.items()):
            if not mesh.indices or len(mesh.indices) != len(mesh.positions):
                raise ValueError('Malformed starter primitive')
            if len(mesh.positions) >= 65536 or max(mesh.indices) >= len(mesh.positions):
                raise ValueError('Starter primitive exceeds index range')
            positions = [value for vertex in mesh.positions for value in vertex]
            normals = [value for vertex in mesh.normals for value in vertex]
            p_view = append_view(struct.pack('<' + 'f' * len(positions), *positions), 34962)
            n_view = append_view(struct.pack('<' + 'f' * len(normals), *normals), 34962)
            i_view = append_view(struct.pack('<' + 'H' * len(mesh.indices), *mesh.indices),
                                 34963)
            p_accessor = len(accessors)
            accessors.append({
                'bufferView': p_view, 'componentType': 5126,
                'count': len(mesh.positions), 'type': 'VEC3',
                'min': [min(v[i] for v in mesh.positions) for i in range(3)],
                'max': [max(v[i] for v in mesh.positions) for i in range(3)]})
            n_accessor = len(accessors)
            accessors.append({'bufferView': n_view, 'componentType': 5126,
                              'count': len(mesh.normals), 'type': 'VEC3'})
            i_accessor = len(accessors)
            accessors.append({'bufferView': i_view, 'componentType': 5123,
                              'count': len(mesh.indices), 'type': 'SCALAR'})
            primitives.append({'attributes': {'POSITION': p_accessor,
                                               'NORMAL': n_accessor},
                               'indices': i_accessor, 'material': material})
        meshes.append({'primitives': primitives})
    gltf = {
        'asset': {'version': '2.0', 'generator': 'GearsVR original pull starter'},
        'scene': 0, 'scenes': [{'nodes': [0, 1, 2]}],
        'nodes': [
            {'name': 'StarterSocket', 'mesh': 0},
            {'name': 'StarterPullGrip', 'mesh': 1},
            {'name': 'StarterCord', 'mesh': 2}],
        'meshes': meshes,
        'materials': [{'name': name, 'pbrMetallicRoughness': {
            'baseColorFactor': [*color, 1.0], 'metallicFactor': metal,
            'roughnessFactor': rough}} for name, color, metal, rough in MATERIALS],
        'buffers': [{'byteLength': len(binary)}],
        'bufferViews': views, 'accessors': accessors,
        'extras': {
            'units': 'metres', 'pullAxis': '+X', 'sourceMeshCopied': False,
            'attachmentVerifiedInHeadset': False,
            'parts': ['fixed socket', 'tracked pull grip', 'stretchable cord']}}
    json_chunk = pad4(json.dumps(gltf, sort_keys=True, separators=(',', ':')).encode(), b' ')
    bin_chunk = pad4(bytes(binary))
    total = 28 + len(json_chunk) + len(bin_chunk)
    return (struct.pack('<4sII', b'glTF', 2, total) +
            struct.pack('<I4s', len(json_chunk), b'JSON') + json_chunk +
            struct.pack('<I4s', len(bin_chunk), b'BIN\0') + bin_chunk)


def preview(nodes, path):
    from PIL import Image, ImageDraw
    if not path.resolve().is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Preview must stay under ignored analysis/')
    canvas = Image.new('RGB', (960, 540), (20, 26, 32))
    draw = ImageDraw.Draw(canvas)
    eye = unit((.9, -1.4, 1.0))
    right = unit(cross((0, 0, 1), eye))
    up = cross(eye, right)
    faces = []
    for node in nodes:
        for material, mesh in node.items():
            color = MATERIALS[material][1]
            for start in range(0, len(mesh.positions), 3):
                normal = mesh.normals[start]
                if dot(normal, eye) <= 0:
                    continue
                triangle = mesh.positions[start:start+3]
                center = tuple(sum(v[i] for v in triangle)/3 for i in range(3))
                points = [(480 + dot(v, right)*4400, 270 - dot(v, up)*4400)
                          for v in triangle]
                light = .40 + .60*max(0., dot(normal, unit((.35, -.4, .85))))
                rgb = tuple(round(max(0., min(1., channel*light))*255)
                            for channel in color)
                faces.append((dot(center, eye), points, rgb))
    for _, points, color in sorted(faces):
        draw.polygon(points, fill=color)
    draw.text((20, 18), 'Lancer VR pull starter / procedural prototype', fill=(220, 230, 236))
    draw.text((20, 508), 'Socket fixed to gun  |  grip follows left hand  |  cord extends',
              fill=(175, 188, 200))
    path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'assets/weapon-6dof/lancer-starter.glb')
    parser.add_argument('--preview', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    nodes = geometry()
    raw = glb_bytes(nodes)
    if args.check:
        if args.output.read_bytes() != raw:
            raise ValueError('Lancer starter asset differs from procedural source')
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(raw)
    if args.preview:
        preview(nodes, args.preview)
    print(f'{len(raw)} bytes, {sum(len(m.indices)//3 for n in nodes for m in n.values())} triangles')


if __name__ == '__main__':
    main()
