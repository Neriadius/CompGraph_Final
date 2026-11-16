"""Pose the Typhoeus model sitting (forward kinematics on its skeleton) and export two
key poses as OBJ files with identical topology, for a GPU morph animation.

Bones (MMD-style rig, names were lost in the file, found by position):
  10 lower-body pivot, 11/12/13 spine, 58 neck, 59 head (60 = head tip)
  14/183 shoulder L/R, 15/184 upper arm, 24/193 elbow, 27/196 wrist
  345/353 thigh L/R, 348/356 knee, 351/359 ankle, 352/360 toe

usage: python3 tools/pose_typhoeus.py <typhoeus scene.gltf> <output folder>
"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gltf_io import Gltf, axis_angle
from to_obj import convert

SRC, OUT = sys.argv[1], sys.argv[2]
STANDING_HEIGHT = 0.19          # globe units

# bone: (joint, child joint used for its direction)
BONES = dict(spine=(11, 13), neck=(58, 59), head=(59, 60),
             upperL=(15, 24), foreL=(24, 27), upperR=(184, 193), foreR=(193, 196),
             thighL=(345, 348), shinL=(348, 351), thighR=(353, 356), shinR=(356, 359),
             footL=(351, 352), footR=(359, 360))


TAIL = [255, 256, 257, 258, 259, 260, 261, 262]
TAIL_AIM = [[0, -0.25, -1], [0, -0.85, -0.55], [0.15, -1, -0.15], [0.45, -0.6, -0.25],
            [0.85, -0.12, -0.15], [0.9, -0.03, 0.35], [0.45, 0.0, 0.9]]


def rot_between(a, b):
    a = a / np.linalg.norm(a)
    b = b / np.linalg.norm(b)
    v = np.cross(a, b)
    c = float(np.dot(a, b))
    if np.linalg.norm(v) < 1e-9:
        return np.eye(3)
    K = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + K + K @ K / (1 + c)


def frame(W, j):
    M = W[j][:3, :3]
    return M / np.linalg.norm(M, axis=0)            # rotation part only


def apply_world_rotation(g, j, Rw):
    """Rotate joint j (and its subtree) by Rw (world axes) about the joint itself."""
    G = frame(g.world(), j)
    g.pose[j] = g.pose.get(j, np.eye(3)) @ (G.T @ Rw @ G)


def aim(g, bone, target):
    j, child = BONES[bone]
    W = g.world()
    d = W[child][:3, 3] - W[j][:3, 3]
    apply_world_rotation(g, j, rot_between(d, np.asarray(target, float)))


def pose(g, key):
    """key = 0: watching the fire, hands held out.  key = 1: glances aside, lifts the hands."""
    g.pose = {}
    aim(g, 'spine', [0.0, 1.0, 0.16 if key == 0 else 0.10])
    aim(g, 'neck', [0.0, 1.0, 0.22])
    aim(g, 'head', [0.0, 1.0, 0.32 if key == 0 else 0.12])
    if key == 1:
        apply_world_rotation(g, 59, axis_angle([0, 1, 0], 22))          # turn the head
    # tail (hangs off the spine): back over the log, down behind it, then curled on the snow
    for (j, child), target in zip(zip(TAIL, TAIL[1:]), TAIL_AIM):
        BONES['tail'] = (j, child)
        aim(g, 'tail', target)
    for side, s in (('L', 1.0), ('R', -1.0)):
        aim(g, 'thigh' + side, [s * 0.13, -0.10, 1.0])
        aim(g, 'shin' + side, [s * 0.03, -1.0, 0.16])
        aim(g, 'foot' + side, [s * 0.05, -0.25, 1.0])
        aim(g, 'upper' + side, [s * 0.30, -0.72, 0.62])
        lift = 0.10 if key == 0 else 0.32
        aim(g, 'fore' + side, [-s * 0.30, lift, 1.0])


def main():
    g = Gltf(SRC)
    rest_height = 1.628
    s = STANDING_HEIGHT / rest_height
    keys = []
    for key in (0, 1):
        pose(g, key)
        keys.append(list(g.primitives()))
    # same placement for both poses: hips over the origin, feet of pose 0 on y = 0
    W0 = None
    pose(g, 0)
    W0 = g.world()
    P = np.concatenate([p['pos'] for p in keys[0]])
    hips = W0[10][:3, 3]
    c = np.array([hips[0], P[:, 1].min(), hips[2]])
    R = np.eye(3)
    for key, prims in enumerate(keys):
        convert(g, prims, OUT, f'typhoeus_pose{key}', R, s, c, tex_max=1024,
                header='Typhoeus / Arknights Endfield by Abcxyz51 (CC BY 4.0), posed sitting')
    hip_y = (W0[345][1, 3] - c[1]) * s
    print('hip joint height (globe units):', round(hip_y, 4), ' hips at x/z', np.round((hips - c)[[0, 2]] * s, 4))


if __name__ == '__main__':
    main()
