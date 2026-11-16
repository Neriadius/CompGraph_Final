"""Minimal glTF 2.0 reader (numpy): accessors, node transforms, meshes, skins, materials."""
import json
import os

import numpy as np

COMP = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
NCOMP = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def quat_to_mat(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def axis_angle(axis, deg):
    axis = np.asarray(axis, float)
    axis = axis / np.linalg.norm(axis)
    a = np.radians(deg)
    K = np.array([[0, -axis[2], axis[1]], [axis[2], 0, -axis[0]], [-axis[1], axis[0], 0]])
    return np.eye(3) + np.sin(a) * K + (1 - np.cos(a)) * K @ K


class Gltf:
    def __init__(self, path):
        self.path = path
        self.dir = os.path.dirname(path)
        with open(path, encoding='utf-8') as f:
            self.j = json.load(f)
        self.buffers = []
        for b in self.j['buffers']:
            with open(os.path.join(self.dir, b['uri']), 'rb') as f:
                self.buffers.append(f.read())
        self.nodes = self.j['nodes']
        self.parent = {}
        for i, n in enumerate(self.nodes):
            for c in n.get('children', []):
                self.parent[c] = i
        self.pose = {}          # node -> extra local 3x3 rotation (applied after the node's own rotation)

    def acc(self, i):
        a = self.j['accessors'][i]
        dt = np.dtype(COMP[a['componentType']])
        n = NCOMP[a['type']]
        if 'bufferView' not in a:
            return np.zeros((a['count'], n))
        bv = self.j['bufferViews'][a['bufferView']]
        off = bv.get('byteOffset', 0) + a.get('byteOffset', 0)
        stride = bv.get('byteStride', 0)
        buf = self.buffers[bv['buffer']]
        if stride and stride != dt.itemsize * n:
            arr = np.ndarray((a['count'], n), dtype=dt, buffer=buf, offset=off, strides=(stride, dt.itemsize))
        else:
            arr = np.frombuffer(buf, dtype=dt, count=a['count'] * n, offset=off).reshape(a['count'], n)
        arr = arr.astype(np.float64)
        if a.get('normalized'):
            arr /= float(np.iinfo(dt).max)
        return arr

    def local(self, i):
        n = self.nodes[i]
        M = np.eye(4)
        if 'matrix' in n:
            M = np.array(n['matrix'], float).reshape(4, 4).T
            if i in self.pose:
                M[:3, :3] = M[:3, :3] @ self.pose[i]
            return M
        R = quat_to_mat(n['rotation']) if 'rotation' in n else np.eye(3)
        if i in self.pose:
            R = R @ self.pose[i]
        S = np.diag(n.get('scale', [1, 1, 1]))
        M[:3, :3] = R @ S
        M[:3, 3] = n.get('translation', [0, 0, 0])
        return M

    def world(self):
        W = {}
        def walk(i, P):
            W[i] = P @ self.local(i)
            for c in self.nodes[i].get('children', []):
                walk(c, W[i])
        scene = self.j['scenes'][self.j.get('scene', 0)]
        for r in scene['nodes']:
            walk(r, np.eye(4))
        return W

    def primitives(self, W=None):
        """Yield dicts with world-space geometry for every mesh primitive in the scene."""
        if W is None:
            W = self.world()
        for ni, M in W.items():
            node = self.nodes[ni]
            if 'mesh' not in node:
                continue
            mesh = self.j['meshes'][node['mesh']]
            skin = self.j['skins'][node['skin']] if 'skin' in node else None
            if skin is not None:
                ibm = self.acc(skin['inverseBindMatrices']).reshape(-1, 4, 4).transpose(0, 2, 1)
                jm = np.stack([W[j] @ ibm[k] for k, j in enumerate(skin['joints'])])
            for p in mesh['primitives']:
                at = p['attributes']
                pos = self.acc(at['POSITION'])
                nrm = self.acc(at['NORMAL']) if 'NORMAL' in at else None
                uv = self.acc(at['TEXCOORD_0']) if 'TEXCOORD_0' in at else np.zeros((len(pos), 2))
                idx = self.acc(p['indices']).astype(np.int64).reshape(-1, 3) if 'indices' in p \
                    else np.arange(len(pos)).reshape(-1, 3)
                if skin is not None:
                    joints = self.acc(at['JOINTS_0']).astype(np.int64)
                    weights = self.acc(at['WEIGHTS_0'])
                    weights /= np.maximum(weights.sum(1, keepdims=True), 1e-9)
                    # linear blend skinning: sum_k w_k * (world[joint_k] @ inverseBind_k)
                    S = np.einsum('vk,vkij->vij', weights, jm[joints])
                    P4 = np.concatenate([pos, np.ones((len(pos), 1))], 1)
                    wpos = np.einsum('vij,vj->vi', S, P4)[:, :3]
                    wn = np.einsum('vij,vj->vi', S[:, :3, :3], nrm) if nrm is not None else None
                    extra = dict(joints=joints, weights=weights)
                else:
                    wpos = pos @ M[:3, :3].T + M[:3, 3]
                    N = np.linalg.inv(M[:3, :3]).T
                    wn = nrm @ N.T if nrm is not None else None
                    extra = {}
                if wn is not None:
                    wn /= np.maximum(np.linalg.norm(wn, axis=1, keepdims=True), 1e-12)
                yield dict(node=ni, name=node.get('name', ''), material=p.get('material'), pos=wpos,
                           nrm=wn, uv=uv, idx=idx, **extra)

    def material(self, i):
        if i is None:
            return dict(name='default', color=[0.8, 0.8, 0.8, 1.0], texture=None, alpha='OPAQUE',
                        emissive=None, emissiveFactor=[0, 0, 0])
        m = self.j['materials'][i]
        pbr = m.get('pbrMetallicRoughness', {})
        def tex_path(t):
            if t is None:
                return None
            img = self.j['images'][self.j['textures'][t['index']]['source']]
            return os.path.join(self.dir, img['uri'])
        return dict(name=m.get('name', f'mat{i}'), color=pbr.get('baseColorFactor', [1, 1, 1, 1]),
                    texture=tex_path(pbr.get('baseColorTexture')), alpha=m.get('alphaMode', 'OPAQUE'),
                    emissive=tex_path(m.get('emissiveTexture')), emissiveFactor=m.get('emissiveFactor', [0, 0, 0]))
