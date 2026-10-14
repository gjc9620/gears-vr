"""Pure launch-time resolution policy; no native rendering or allocation.

Capability declarations are supplied by a verified build manifest. This module
does not establish that a build actually renders independent VR targets.
"""
from dataclasses import dataclass
from typing import Mapping

DEFAULT_QUALITY = 'low'
MAX_SOURCE_EXTENT = 8192


@dataclass(frozen=True)
class VrQualityProfile:
    quality: str

    def __post_init__(self):
        if type(self.quality) is not str or self.quality not in ('low', 'high'):
            raise ValueError("vr_quality must be exactly 'low' or 'high'")

    @property
    def eye_width(self):
        return 2048 if self.quality == 'low' else 4096

    @property
    def eye_height(self):
        return self.eye_width

    @property
    def source_width(self):
        return 2 * self.eye_width

    @property
    def source_height(self):
        return self.eye_height

    @property
    def source_pixels(self):
        return self.source_width * self.source_height


def resolve_quality(config: Mapping, manifest: Mapping):
    """Return an immutable profile, or None for unrequested legacy research.

    A capable release defaults to Low. Existing research manifests with no
    capability keep their existing behavior unless the user requests a tier.
    """
    capable = manifest.get('fixed_vr_resolution', False)
    if type(capable) is not bool:
        raise ValueError('fixed_vr_resolution capability must be a boolean')
    requested = 'vr_quality' in config
    profile = VrQualityProfile(config['vr_quality'] if requested else DEFAULT_QUALITY)
    if not capable:
        if requested:
            raise ValueError('This build does not support independent VR resolution. '
                             'Select a verified native build with fixed_vr_resolution support before setting vr_quality.')
        return None
    return profile


@dataclass(frozen=True)
class VrRuntimeLimits:
    system_width: int
    system_height: int
    view_widths: tuple
    view_heights: tuple

    def __post_init__(self):
        if type(self.view_widths) is not tuple or type(self.view_heights) is not tuple or \
                len(self.view_widths) != 2 or len(self.view_heights) != 2:
            raise ValueError('Exactly two immutable per-view size limits are required')
        for value in (self.system_width, self.system_height, *self.view_widths, *self.view_heights):
            if type(value) is not int or not 0 < value <= 0xffffffff:
                raise ValueError('Runtime size limits must be positive unsigned 32-bit integers')


def require_size_support(profile: VrQualityProfile, limits: VrRuntimeLimits):
    """Validate fixed extents; do not clamp, resample, or use recommendations.

    System/view maxima apply to each of two XR swapchains. The independent
    shared SBS texture has its own 8192 bound. GPU resource/format/budget and
    real engine target admission remain the integration's responsibility.
    """
    if not isinstance(profile, VrQualityProfile) or not isinstance(limits, VrRuntimeLimits):
        raise ValueError('A resolved quality profile and validated runtime limits are required')
    if profile.source_width > MAX_SOURCE_EXTENT or profile.source_height > MAX_SOURCE_EXTENT:
        raise ValueError('Requested SBS source exceeds the shared texture size limit')
    for eye in range(2):
        if profile.eye_width > min(limits.system_width, limits.view_widths[eye]) or \
                profile.eye_height > min(limits.system_height, limits.view_heights[eye]):
            raise ValueError(f'{profile.quality} exceeds advertised XR size limits for eye {eye}; '
                             'select a supported tier before launch')


def require_same_launch_profile(active, requested):
    """Reject a resolution change after launch, including legacy/profile changes."""
    if any(value is not None and not isinstance(value, VrQualityProfile) for value in (active, requested)):
        raise ValueError('Launch profiles must be resolved before comparison')
    if active != requested:
        raise ValueError('VR quality is fixed for this launch; stop and restart before changing it')
