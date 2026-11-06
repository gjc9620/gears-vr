# 修改位置索引

2026-10-28 静态索引；读取时 HEAD `69a8799e8c7d9f96023593e42782483ce118a908`。主线同时推进，行号只导航，实际修改以符号及 [文件 SHA256](code-locations.json) 为准。这不是冻结主线源码或要求回退。配套 [实施交接单](implementation-handoff.md)。

| 文件 | 已定位的函数／行号 |
| --- | --- |
| [weapon_reload_model.h](../../tools/vr-common/weapon_reload_model.h) | `begin`：66；`insert`：113；`finish`：41；`cycle`：124；`cancel`：140 |
| [weapon_hammerburst_reload.h](../../tools/vr-common/weapon_hammerburst_reload.h) | `begin_r3`：141；`confirm_latch_open`：168；`insert_magazine`：199；`confirm_latch_closed`：210 |
| [weapon_gnasher_mechanics.h](../../tools/vr-common/weapon_gnasher_mechanics.h) | `begin_reload`：292；`acknowledge_inventory`：330；`observe`：382；`acknowledge_cycle`：367 |
| [weapon_longshot_reload.h](../../tools/vr-common/weapon_longshot_reload.h) | `begin_r3`：204；`complete_step`：263；`acknowledge_inventory`：284；`acknowledge_completion`：307 |
| [weapon_snub_reload.h](../../tools/vr-common/weapon_snub_reload.h) | `begin_r3`：195；`confirm_ejection`：221；`insert_magazine`：248；`move_original_parts`：257 |
| [weapon_boltok_reload.h](../../tools/vr-common/weapon_boltok_reload.h) | `begin_r3`：318；`advance`：263；`insert_ammunition`：370；`acknowledge_inventory`：383 |
| [weapon_boomshot_interaction.h](../../tools/vr-common/weapon_boomshot_interaction.h) | `begin_r3`：336；`insert_ammunition`：386；`acknowledge_inventory`：398；`acknowledge_completion`：415 |
| [weapon_torque_bow_interaction.h](../../tools/vr-common/weapon_torque_bow_interaction.h) | `begin_r3`：333；`trigger`：377；`request_abort`：216；`take_request`：279；`acknowledge`：413 |
| [weapon_hammer_of_dawn_interaction.h](../../tools/vr-common/weapon_hammer_of_dawn_interaction.h) | `observe`：288；`begin_r3`：379；`take_request`：385；`acknowledge`：397 |
| [weapon_troika_interaction.h](../../tools/vr-common/weapon_troika_interaction.h) | `edge`：392；`update_aim`：420；`take_request`：424；`acknowledge`：445 |
| [weapon_bolo_grenade_interaction.h](../../tools/vr-common/weapon_bolo_grenade_interaction.h) | `bind`：276；`take_request`：293；`cancel`：300；`acknowledge`：406 |
| [weapon_smoke_grenade_interaction.h](../../tools/vr-common/weapon_smoke_grenade_interaction.h) | `bind`：302；`take_request`：320；`cancel`：327；`acknowledge`：431 |
| [grenade_throw.h](../../tools/vr-common/grenade_throw.h) | `grenade_world_launch`：48；`solve`：115；`begin`：147 |
| [weapon_equipment_backend.inl](../../tools/playground/weapon_equipment_backend.inl) | `current`：30；`tick`：182；`reset_after_drain`：222 |
| [weapon_equipment_rig.inl](../../tools/playground/weapon_equipment_rig.inl) | `acquire`：16；`tick`：46 |
| [weapon_hand_client.inl](../../tools/playground/weapon_hand_client.inl) | `observe`：112；`equipment_hold_committed`：195 |
| [weapon_action_entry.inl](../../tools/playground/weapon_action_entry.inl) | `prepare`：118；`invoke_owned`：299；`process_event`：334 |
| [weapon_call_subscriber.inl](../../tools/playground/weapon_call_subscriber.inl) | `begin`：56；`cleanup`：94 |
| [weapon_unified_fire.inl](../../tools/playground/weapon_unified_fire.inl) | `prepare_current`：154；`typed_result`：317；`begin`：410；`normal_return`：497；`cleanup`：510 |
| [vr_lancer_reload_hold.inl](../../tools/playground/vr_lancer_reload_hold.inl) | `lancer_clear_timer`：68；`lancer_set_fallback`：74；`lancer_finish_now`：81；`lancer_refresh`：138 |
| [weapon_lancer_r2_native.inl](../../tools/playground/weapon_lancer_r2_native.inl) | `tick`：215；`yield_for_native_equipment`：359 |
| [weapon_equipment_domain.inl](../../tools/playground/weapon_equipment_domain.inl) | `domain_ready`：4 |
| [weapon_lancer_component_probe.inl](../../tools/playground/weapon_lancer_component_probe.inl) | `tick`：15 |
| [weapon_lancer_feed_anchor.h](../../tools/vr-common/weapon_lancer_feed_anchor.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [weapon_lancer_insertion.h](../../tools/vr-common/weapon_lancer_insertion.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [weapon_lancer_magazine_pose.h](../../tools/vr-common/weapon_lancer_magazine_pose.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [weapon_action_boundary.h](../../tools/vr-common/weapon_action_boundary.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [weapon_call_boundary.h](../../tools/vr-common/weapon_call_boundary.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [left_hand_interaction_owner.h](../../tools/vr-common/left_hand_interaction_owner.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [weapon_grip_damage_metadata.inl](../../tools/playground/weapon_grip_damage_metadata.inl) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [weapon_metadata_parse.h](../../tools/playground/weapon_metadata_parse.h) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [playground.cpp](../../tools/playground/playground.cpp) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
| [CMakeLists.txt](../../tools/playground/CMakeLists.txt) | 公共类型／主线接线点，见实施交接单；哈希已记录 |
