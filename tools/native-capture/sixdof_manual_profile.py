"""Explicit owner-started first-person/free-hands profile. No test runner."""

# first_person_preview is emitted once by start_capture.hud_size_ini, including
# the manual profile's required True value. Keep only body-specific keys here.
CAPTURE = ('first_person_initial_requested=1\n'
           'first_person_body_basis=1\nfirst_person_near_clip_mm=120\nsixdof_unified=1\n')

SECTIONS = '''
[RigTracking]
enabled=1
[WeaponHandParticipant]
enabled=1
[WeaponActionGate]
engine_restore=0
[CoverPosePhaseObserver]
enabled=1
[CoverControlObserver]
enabled=1
[ContinuousHandOwner]
enabled=1
length_mode=preserve
[FreshPoseProducerObserver]
enabled=1
experimental_sequence_overwrite=1
experimental_per_bone_overwrite=1
[FullBodyC0Native]
enabled=1
hands=1
arm_control_ownership=1
held_main_mount=1
diagnostic_first_failure=0
diagnostic_held_main_mount=0
diagnostic_ready_wrist=0
[NativeAtomOverwriteReceipt]
enabled=1
[BodyC0GoalPolicy]
enabled=1
[BodyTickInput]
enabled=1
[BodyMainMountSource]
enabled=1
'''

def validate(args):
    if not getattr(args, 'sixdof_manual', False):
        return False
    if getattr(args, 'simulator_operator', False):
        raise ValueError('Manual 6DoF candidate requires physical runtime tracking')
    if not all(getattr(args, key, False) for key in
               ('continuous_session', 'use_reserved', 'require_target_prime',
                'anchor_observer', 'cinematic_camera', 'crosshair_assignment_observer')):
        raise ValueError('Manual 6DoF candidate requires the normal owned guarded startup')
    return True
