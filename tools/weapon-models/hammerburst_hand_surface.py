"""Offline Marcus115 skin domain for HB fitting; never grants runtime authority.

Supply the complete original four-influence skin and complete bone-to-model
palette. Mixed palm/forearm triangles retain ALL influences, including bones
10 and29. A reference-parent palette remains reference-only evidence.
"""
from dataclasses import dataclass
import numpy as np


@dataclass(frozen=True)
class Surface:
    vertices: np.ndarray
    triangles: np.ndarray
    original_triangle_indices: np.ndarray
    strict_hand_triangle_count: int
    mixed_triangle_count: int


def evaluate(vertices, indices, weights, triangles, inverse_bind, model_palette):
    """Skin all vertices, selecting every triangle touched by left-hand skin.

    Palettes use column-vector4x4 matrices in original model units. No missing
    forearm/twist matrix is substituted with reference data. This calculation
    does not establish the provenance of a caller's palette or verify sleeves,
    collision, acquisition, live C0 consumption or headset comfort.
    """
    vertices = np.asarray(vertices, dtype=np.float64)
    indices = np.asarray(indices)
    weights = np.asarray(weights, dtype=np.float64)
    triangles = np.asarray(triangles)
    inverse_bind = np.asarray(inverse_bind, dtype=np.float64)
    model_palette = np.asarray(model_palette, dtype=np.float64)
    if vertices.ndim != 2 or vertices.shape[1] not in (3, 4):
        raise ValueError("Expected original Nx3/Nx4 vertices")
    count = len(vertices)
    if indices.shape != (count, 4) or weights.shape != (count, 4):
        raise ValueError("Expected all four original bone influences")
    if not np.issubdtype(indices.dtype, np.integer) or np.any((indices < 0) | (indices >= 115)):
        raise ValueError("Expected original Marcus115 bone indices")
    if triangles.ndim != 2 or triangles.shape[1] != 3 or not np.issubdtype(triangles.dtype, np.integer):
        raise ValueError("Expected original triangle indices")
    if np.any((triangles < 0) | (triangles >= count)):
        raise ValueError("Triangle outside original vertex array")
    if inverse_bind.shape != (115, 4, 4) or model_palette.shape != (115, 4, 4):
        raise ValueError("Complete115 inverse-bind and posed matrices are required")
    if not all(np.isfinite(x).all() for x in (vertices, weights, inverse_bind, model_palette)):
        raise ValueError("Nonfinite skin data")
    if np.any(weights < 0) or not np.allclose(weights.sum(axis=1), 1., atol=1e-12, rtol=0):
        raise ValueError("Original normalized skin weights required; no renormalization")
    affine = np.asarray([0., 0., 0., 1.])
    if not np.allclose(inverse_bind[:, 3], affine, atol=1e-10, rtol=0) or not np.allclose(model_palette[:, 3], affine, atol=1e-10, rtol=0):
        raise ValueError("Expected affine matrices")
    if vertices.shape[1] == 3:
        vertices = np.column_stack([vertices, np.ones(count)])
    elif not np.allclose(vertices[:, 3], 1., atol=1e-12, rtol=0):
        raise ValueError("Expected homogeneous position w=1")
    hand_weight = np.sum(weights * ((indices >= 11) & (indices <= 26)), axis=1)
    touched = np.any(hand_weight[triangles] > 0, axis=1)
    strict = np.all(hand_weight[triangles] > .999, axis=1)
    delta = model_palette @ inverse_bind
    skinned = np.zeros_like(vertices)
    for slot in range(4):
        skinned += np.einsum("nij,nj->ni", delta[indices[:, slot]], vertices) * weights[:, slot, None]
    selected = np.flatnonzero(touched)
    return Surface(skinned[:, :3], triangles[selected].copy(), selected,
                   int(strict.sum()), int((touched & ~strict).sum()))
