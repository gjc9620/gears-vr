# 掩体修改位置索引

2026-10-28 离线快照。行号用于跳转，开工时按符号重定位并比较 SHA256；主线后续修改不得被旧快照覆盖。完整哈希见 [JSON](code-locations.json)。下面均为已有位置；拟新增文件见 [实施交接](implementation-handoff.md)。

| 文件 | 符号与快照行号 |
| --- | --- |
| [tools/vr-common/cover_interaction.h](../../tools/vr-common/cover_interaction.h) | `struct CoverRange` L15；`struct CoverEpochs` L23；`struct CoverEdgeObservation` L26；`class CoverInteraction` L85；`CoverResult suspend` L96；`CoverResult update` L112 |
| [tools/vr-common/cover_input_owner.h](../../tools/vr-common/cover_input_owner.h) | `class CoverInputOwner` L36；`CoverInputOwnerDecoded begin` L88；`CoverInputOwnerCandidate update_with_decoded` L132；`CoverInputOwnerResult complete` L178；`void abort_current` L226；`void build_view` L267 |
| [tools/vr-common/cover_controller_adapter.h](../../tools/vr-common/cover_controller_adapter.h) | `class CoverControllerAdapter` L34 |
| [tools/vr-common/cover_view.h](../../tools/vr-common/cover_view.h) | `struct CoverViewAnchor` L11；`prepare_cover_view(` L36；`cover_physical_displacement(` L57；`cover_tracking_basis(` L69；`cover_tracking_point(` L79 |
| [tools/vr-common/cover_body_target.h](../../tools/vr-common/cover_body_target.h) | `struct CoverBodyTarget` L8；`prepare_cover_body_target(` L46 |
| [tools/vr-common/left_hand_interaction_owner.h](../../tools/vr-common/left_hand_interaction_owner.h) | `class LeftHandInteractionOwner` L64；`LeftHandInteractionDecision update` L74 |
| [tools/native-capture/cover_shared_input_live.inl](../../tools/native-capture/cover_shared_input_live.inl) | `bool prepare(` L120；`frame.cover.range=` L222；`participant.update_with_decoded` L260；`broker->commit` L265；`participant.complete` L271 |
| [tools/native-capture/cover_camera_live.inl](../../tools/native-capture/cover_camera_live.inl) | `Reason initial_facing(` L55；`bool prepare_with(` L82；`bool configure(` L115 |
| [tools/native-capture/cover_full_goal_live.inl](../../tools/native-capture/cover_full_goal_live.inl) | `void prepare(` L9 |
| [tools/native-capture/cover_geometry_live.inl](../../tools/native-capture/cover_geometry_live.inl) | `struct Geometry` L41 |
| [tools/native-capture/cover_neutral_eye_live.inl](../../tools/native-capture/cover_neutral_eye_live.inl) | `bool sample(` L286 |
| [tools/native-capture/continuous_hand_owner_live.inl](../../tools/native-capture/continuous_hand_owner_live.inl) | `bool passive()` L63；`bool owner_current()` L142；`void tick_begin(` L219；`void compose_begin(` L248 |
| [tools/native-capture/body_c0_goal_policy_live.inl](../../tools/native-capture/body_c0_goal_policy_live.inl) | `bool no_cover(` L145；`void pose_begin(` L192；`void pose_end(` L249；`bool read_view(` L278 |
| [tools/native-capture/full_body_c0_transaction.h](../../tools/native-capture/full_body_c0_transaction.h) | `Result prepare(` L181；`Result commit(` L200；`Result rollback(` L219 |
| [tools/native-capture/capture.cpp](../../tools/native-capture/capture.cpp) | `first_person_live::prepare(anchor,view)` L3370；`cover_shared_input_live::prepare(anchor,view,result)` L3374；`body_c0_goal_policy_live::read_view` L3378；`gears_vr::stereo_matrices(view,projection,result.reference` L3396 |
| [tools/native-capture/run_camera_live.inl](../../tools/native-capture/run_camera_live.inl) | `bool prepare(` L93；`input.standing_sm0_verified=` L119 |
| [tools/native-capture/run_camera_stabilizer.h](../../tools/native-capture/run_camera_stabilizer.h) | `class Kernel` L37；`Status update(` L62 |
| [tools/native-capture/cover_call_config.py](../../tools/native-capture/cover_call_config.py) | `def prepare(` L298 |
| [tools/native-capture/cover_call_trace.h](../../tools/native-capture/cover_call_trace.h) | `struct Function` L31；`Result capture(` L84 |
| [tools/native-capture/cover_call_live.inl](../../tools/native-capture/cover_call_live.inl) | `bool configure(` L74 |
| [tools/game-playtest/cover_trace_query.py](../../tools/game-playtest/cover_trace_query.py) | `def _parameters(` L140；`def _certificate(` L171；`def capture(` L198 |
| [tools/game-playtest/cover_surface_query.py](../../tools/game-playtest/cover_surface_query.py) | `TRACE_RVA, TRACE_INDEX, GNATIVES_RVA` L23；`def _function(` L200 |
| [tools/game-playtest/cover_state.py](../../tools/game-playtest/cover_state.py) | `def slot_snapshot(` L65；`def capture(` L120 |
