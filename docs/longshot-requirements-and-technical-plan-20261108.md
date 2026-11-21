# Longshot sniper rifle demand and technical programme

2026-11-08 Drafting,2026-11-10 Update continuously. The target working area is `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`Branch `codex/6dof-single`. Users have requested to enter development; this document keeps the demand, technical options and subsequent realization/certification records.

This paper sets out the reviewable play, scope, posture and sight requirements, and then lists technology and authentication programs. The components are the starting values for physical space realization on this project; final exposure and head comfort still need to be tested.

## One, play needs.

### 1 You're feeling a gun and second hand.

- The main handler controls the position and complete rotation of the gun, with a real shot in the direction of the original.
- Second hold point by user chart 1 The picture supports the diagnosis of the area of exposure, which is to be obtained by changing the three-dimensional coordinates from the model to the actual gun.
- The deputy is close to holding the point of attachment, and his hand is tied to the gun and moves with it.
- The deputy moves to the existing hands and solves them.
- Grip is loose and still within range, returning to sorbent. Physical hands are removed from the range. Range judgement continues using the original tracker, avoiding showing that they cannot leave after adsorbing.
- The overlap is based on the current single interactive arbitration, with clear ownership of each of the carrying, bolting and ranger control.

Second touch point status is `自由 → 接近吸附 → Grip 控枪 → 松 Grip 后吸附 → 离开自由`. If Grip enters from outside the range, you follow the existing input border and reverse re-alignment protocol and include it in the test.

### 2 Scope and position of all interactive components

Snap Volume defines the area close to adsorbing and alignment. Grab Volume defines a real grab to an area where control can be obtained. GripPoint defines final contact transformation, GrabPose defines hand-style. All components use the marked local coordinates of the weapon in a uniform manner and change with the current weapon.

| Part | Snap and Grab Behaviour | Conditions for completion and feedback |
| --- | --- | --- |
| Master's handle. | Approaching alignment. Take ownership of the gun by the established rules. | The master is holding a gun steady, his fingers and his hand are tied. |
| Figure 1 Second hold point. | Approach suction, Grip. | Snap phase no side-arming, Grab phase hand solver effective. |
| Left shoulder-loading area. | Reuse current shoulder-take range and input | One effective capture of one live ammunition and maintenance of the only item during hold |
| Single live ammunition | The live ammunition has a capture range and a posture. Hold the rear body behind your left hand. | The direction of the warhead, the identity of the item and the loading process |
| The chamber entrance. | The zoom-in adsorbent entrance is holding live ammunition and internal alignment is being directed to the chamber. | Ammunition is seized, the chamber opened, warheads are directed and inserted after delivery |
| Gun-knocking handle. | Approach adsorbing, real capture and then binding on the institutional path | Complete opening and closing end points confirmed, indicating current direction required |

#### 2.1 First Round Range Configuration

The following table shows the parameters that can be directly used for the first round, with all new dimensions being the design start. A line on the left shoulder comes from existing public delivery. The length is expressed as a game of interactive space. The box is filled in wide and large, and the sphere radius is completed. The model local coordinates are converted by currently valid world variations and unit ratios, and currently tested scale is processed separately. The lenses and bolt centres write the same configuration after the model has been marked, and the range shares the configuration with visible guidance.

| Part | Close to snap volume | Grab Volume allowed to start scratching | Withdrawal and conditions of validity |
| --- | --- | --- | --- |
| Master's handle. | Ball with a point of contact, radius. 7 cm | Same center ball, radius. 7 cm | Uncapture More 9 And, uh, quit Snap and hold the gun and obey the rules of release. |
| Figure 1 Second point. | The box along the wood shaft, long 16 × Wide 12 x High 12 cm | Grip in the same box. | Snap Keep Box As 20 × 16 × 16 cm, over and out; Grab beyond box decontrol and recovery requires new effective capture |
| Gun-knocking handle. | The ball with the hand, radius. 5 cm | Same center ball, radius. 5 cm | Uncapture More 7 cm exit;the vertical deviation between the grab and the bound path exceeds 7 c. Uncontrol |
| Single live ammunition | The central line length of the capsule along the bullet axis is equal to the actual length of the live ammunition, radius. 3 cm | Same capsule. | Quite beyond without holding radius 5 cm Exit; follow release rules when held |
| Contact of the chamber with the holder. | Circumpolar column, radius, out of the chamber. 6 cm, axis length 8 Cm, the warhead is in full range. | The capture of live ammunition is in the same range | Keep alignment radius 8 cm, outward length 10 cm;gun open and axle angles 30° I'm just adsorbing. |
| The chamber is finally in the seat. | Converse Lead Segment, Horizontal Error after Alignment 1.5 Cm, axle angle.10° | Use current live ammunition lease | The original warhead is at least in front of the chamber. 2.5 Cm, send it real to the mouth axis  ⁇6 mm;continuing 100 ms, speed 3 m/s, up to one submission |
| Left shoulder-loading area. | Reuse public ellipse, default left-right half axle 20.45 Cm, top-down and back-and-back axis. 24 cm | Reuse same common-mission center and shoulder-take input | The horizontal half axle is configured along the body 16–22 cm; retaining middle line exclusion and original left-hand selection |

Default left shoulder ellipse full size 40.9 × 48 × 48 cm. Half axle is from `body_slots.h` It's... `min(22, max(16, armspan_cm × 0.135 − 2.5))`Default arm layout as 170 cm. Its center is already in use `shoulder_ammo_center` The shoulder area provides the location where the items are generated and applies live ammunition to the GabPose immediately after the live ammunition is removed.[Public body slot code](../tools/vr-common/body_slots.h)

The master handle, the wood and the bolt are defined by contact points with unsorbed physical hands. Live rounds grab anchors with thumbs and index fingers, and load warheads and axes. The change of controller Grip Pose to these anchors is provided by current hand models. The original left hand that continues to use public paths Grip Pose.

The main handle, the wood and the bolt suggest a difference in approach. 60°. Sniff to display public C 0 It's...120 ms transition, valid Grab to access clearance; early draft 80 ms recommends harmonization as a result. Transition only shows that exposure judgement continues to use original tracking; continuous sources are interrupted more than 100 After ms, old adsorption is maintained as invalid.

#### 2.2 Overlap and continuity of action

The lease for the valid holding of items or components remains in priority. The loaded chamber is currently inserted, the bolts during the opening and locking stages are now empty-handed targets, and the wood protection during the ordinary gun bearing phase is second-hand targets. The remaining priority candidate is selected at a uniform distance to his point of contact, which is determined by the same order of stabilization parts. Selected Snap targets are switched after exit from the preservation area or change of steps.

When you grab the bolt, you move along the path of the original institution. The exterior deviations exceed the surface difference, release or failure to pursue the capture, and the actual mechanical state is left for re-certification. The rotational and straight segments each have a point of marking, and current mechanical confirmation requires a modulation error in the relative bone target.2 mm, corner error 2°, complete the required trajectory to confirm the endpoint. Physical hand-frame jump and old attitude cannot skip process validation.

The arm ownership of the item is released after completion, subject to existing retake rules. The empty chamber surface maintains interactive target and state feedback, with loading operations driven by live ammunition in hand. Current conditions are shown when closing, reverse bullets or approaching the gun body.

#### 2.3 GrabPose for each entity widget

All the parts of an entity involved in interaction need to be independently exposed to anchors, hand-directional direction, five-finger posture and forearms. The armbone follows a fixed bone length and has its only consumer on hand. Snap shows exposure preview, Grab shows a hold position, both using the same real surface benchmark. After capturing the extended range, the final point of contact remains on the model surface.

| Part or stage | We have to make and check the match. |
| --- | --- |
| Master's handle. | The palms are covered in the back, the middle finger, the no-named finger, the little finger around the handle, the thumb on the side, and the index finger falls near the trigger with the real trigger. |
| Second point. | By Chart 1 Hold the front, hold the bottom of the palm, wrap the side, and the thumbs are down; Snap and Grip have to be checked for stress posture. |
| The bolts are open. | The thumbs and the finger wrap their hands, the other fingers avoid the receiver, and the wrists follow the original bolts and rotate peacefully. |
| The gun's locked. | Use a hand grab that fits in the closed direction, check the wrists, fingers and sleeves of the full roll-back to the end. |
| Take live bullets out of the left shoulder. | When successful, grab the shell with a thumb and an index finger or middle finger, with the warhead pointing in the direction that naturally leads to the chamber; shift to shoulder area using the same live-fired hand type |
| Carry live ammunition | Hold the point on the actual surface of the shell, rotate it with the left hand and keep your fingers in contact with the same bullet. |
| Port alignment and insertion | Continuous change from a launch posture to a delivery posture, with fingertips tied to cartridge casings; release the items and return to freedom or gun-breeding candidate status upon arrival |

First round of visual acceptance targets are the surface gap at the main point of contact.3 mm, local minor penetration depth 2 mm. The threshold is the intended target, which needs to be reviewed in conjunction with actual triangles and visible skin deformations. The rest of the fingers must be bent naturally, maintaining a reasonable space relationship between the butt, lenses, receivers and sleeves.

Each set of GrabPose checks the view from the palm, back, thumb, front and back, covering adsorbing, grab, maximum bolt, locking and release. The hand position is continuously transitioned, with a consistent length, wrist and forearms. All components are checked through a physical visual match before recording the full receipt of the gun.

Handheld configuration must be maintained between Snap, Grip Control, Pine Grip back and Snap. Hands-suggestion can update the world change of the current grip, but cannot recreate a target with only a shake point and discard the five-finger attitude of the original widget. The hand-carrying style of the second hand is used only on the left hand; the hand-hand continues to use the hand-share changes from the real trigger. Undo the display when custom hand-style is missing or bound to fail, keeping the original tracker and the established body state.

#### 2.4 The same results as the mouth, the same effect of carrying a bullet and hand.

Retrieving cylinder radius from the outer vent 6 Cm deep. 8 cm, adsorbed to keep the cylinder radius 8 Cm deep. 10 cm; warheads above the level of the mouth 1.5 Cm, original bullet axle angle.30°. The above is a physical dimension, and the model does not expand or narrow the actual interaction. Only when open, the only live ammunition in this round and the original lease for carrying a bomb are valid.

Match 120 ms transition helps to move horizontally and axially, keeping the current player's original warhead in depth. Show the bullet, fine grip, Grip target and loaded judgement share the same calculation of the current real frame. Hand positions are through the only C available 0 Consumers publish that the original controller/original hand position continues to be used for range and real motion determination; “player has delivered a bomb” cannot be read from the paste showing hand inverse reading.

The loading must be accompanied by a degree of acquisition/maintenance, real proximity to re-entry, eventual alignment and original inventory service inspection. Insorption transition or retreat is not self-fulfilled; the warheads are static at the entry point and do not generate loads. One bullet actually delivered is submitted for one transaction only; it shows that the revocation and return of stocks will be handled by their respective owners and the hidden model does not represent the recovery of stockpiles.

Unlock, source lapse, reference space/entry generation/arms/goods/lease change to withdraw current support; old near progress cannot be followed over generations. 100 Leave the area and show up gradually, but without authorization to load.

#### 2.5 Player-by-play and feedback details

The following table specifies the player's actual operation. The range acquisition, input edge, current item identification and stage conditions are valid before entering the next state; adsorption preview itself does not submit to institutional or inventory actions.

| Player Actions | Allowed Changes | Current Visible Feedback and Next |
| --- | --- | --- |
| Empty left hand to second shake point for box | Left hand to the right, GrabPose. Against the logs, right hand to the gun. | Displays current holding; light movement left controller does not change the gun point |
| Hold on second touch, Grip. | Keep the same left hand type, access both hands and handle guns. | The gun's attitude is continuing to fit. The left hand moves and starts working on the gun. |
| Pine, Grip, the original left hand is still holding the box. | Unload the gun, keep close to sorbent. | The gun is back in control of the right hand, with the left arm still attached to the wood; move out and keep the box and retreat to free hands. |
| Move your left hand to the handle after shooting. | Only adsorb to current handle position, show Ripper type | The direction of the opening remains visible; the bolt does not move when it is uncaptured because it shows a close hand |
| Press Grip and pull it in the handle range. | Control the bolts along the original model tracks under existing parts leases | Show the direction of the rotation/diverse that has not been completed; actual tipping after opening end |
| Left hand to public left shoulder firing area, carrying out the same recovery as Lancer/Veterinary rifle. | Get a live sniper and use the live bullet to hold your position immediately. | Show the port and direction of the delivery; constant holding or repeat reading of a similar pressure at less than multiple bullets |
| Hold the live bullet into the chamber for the cylinder. | Show live ammunition aligned with left hand and maintain original axis depth | The bullet axes did not show the correct direction in due course; stock was not submitted before arrival |
| Actual forward delivery, completion of the required approach and arrival trip | The same live ammunition was loaded up to once, with one pint on it and the articles released from hand constraints. | The vent is over, switch to lock-in; the gun remains unfired. |
| Grab the handle and push back and close the lock. | Authenticate full mechanical endpoint, determine the result by the time this material is actually fired to lock | `<4.000 s` Perfect.`≥4.000 s` Normal; re-fired after confirmation of original results |
| Unhandled, missed, cut or suspended. | Stop this component control, save a certified mechanical/stocking result and release the current display target | Show actual steps missing after recovery, re-engage and press effective grab; no old adsorption or old pressure skip step |

The range edge test must cover the exact entry, the maintenance of the boundary, re-entry after crossing the border and the movement from the outside of old Grip. All display ranges are valid for physical size configurations. The first round can be adjusted to update the document, configuration and corresponding border examples.

### 3 Manual Loading Process

1. When loaded and locked, you can normally fire. Once the original confirmation is made that a shot actually came out, the record book will be marked as the only identity and time point to be changed manually.
2. Shows how the bolt is opened. Left hand leaves the sub-share, grabs the bolt and opens it on the actual institutional tracks.
3. The chamber is in place to perform the visual steps for the shell-release, showing a direction for left shoulder taking.
4. The left shoulder removes a full live bullet, using the current shoulder extraction rule, showing the contours of the chamber and the direction to be inserted.
5. Holds live ammunition into the enlarged chamber area, binds and completes its loading.
6. Displays the lock-lock guide. The left hand snap is pushed to the lock action required by the model before completion, confirming when the machine is completed.
7. The original version is completed with a confirmation of the firing readiness and the result of the re-loading, exits the guidance and can be recaptured to the second shake point.

The rotation, tectonics and locking sequence of the bolt is approved in original models and animation tracks. `b_LS_Bolt` There is a rotation trajectory, which requires complex action marking to be completed when developing.

The rules for hand-held refilling are maintained at the entrances after firing, empty triggers and retakers. The door is kept open when stocks are loaded but the bolts are not locked.

### 4 Perfectly swapped.

You're... `t0` The video shows the time of the game to confirm that the weapon was actually fired.`t1` The game time completed for the full-filled post-lock lock of this round.`Δt = t1 − t0`。

| Situation | Result |
| --- | --- |
| The entire mechanical process and inventory are valid.`0 ≤ Δt < 4.000 s` | Request a perfect reload incentive to confirm the results. |
| The entire mechanical process and inventory are valid.`Δt ≥ 4.000 s` | Regular switch. |
| Lack of start of fire for Ben. | Normal Load |
| The plug was successful, but the bolts were not closed. | Keep locking the lead. Keep it unfired. |
| Repeat key buttons, repeat lock notifications or repeat returns | Maintain submission of results and incentives for the current round at most once |

Time-end completion of real machinery is delayed by a different time. 4 The re-embolition, re-extract and re-enter the interactive range will continue to be filled after seconds.

The timeout is to be used in the original game clock syntax, and the real time off is to be frozen. Failure to pursue or cut a gun interrupts the current interactive lease, re-certifies machinery and stock when it is restored, and maintains an authentic starting point for the original fire. Recovers normal load if the original record cannot reliably relate. This breakup strategy is a program proposal that needs to be verified with actual suitability.

### 5 GearVR General Interactive Guide

The rules apply to the interaction of GearVR weapons, which are connected to the existing guide route.

| Current Status | Guide content |
| --- | --- |
| - Fire the air. | The bolts are high and open. |
| Opened | Left shoulder bullet. |
| armed | Port profile, live bullet insertion axis |
| The live-fire angle is not right. | Align the directional hints |
| Loaded | The locks are closed and the end. |
| We're out of ammo. | We'll use the real one-time feedback rule. |
| Switching. | Short ordinary or perfect results, closing the current cycle of step guidance |

Guides the movement of target components, with readable eyes, to avoid covering all information by hand and gun. Common syntax is re-used from existing Lancer, with specific contours, locations and paths provided by Longshot geometry.

## II. Sniper-scopy programme evaluation

### 1 The target experience of the magnification in the mirror.

- The magnification is limited to the actual lens aperture, and the mirror remains normal VR World Vision.
- The mirror retains the original magnification syntax, partition and corresponding aim state. The multiplier value is approved from the certified original configuration or actual status.
- The eyes are in the mirror when they are located behind the eyeglass, at a distance appropriate to the horizontal position.
- The eye and the left calculate visibility separately, supporting natural selection of an eye.
- The protostar checks with the original authoritative ray, covering close cover, long-range targets and movement of hands.

OpenXR provides the view, position and layer submission engagements for which lens images and weapons are used to interact. Two eyes rendering is matched with presentation of view positions and spaces.[Khronos frame submission guide](https://github.com/KhronosGroup/OpenXR-Guide/blob/main/chapters/frame_submission.md)

### 2 A path.

The priority goal is to use an independent lens scenario image, which is then synthesized by the lens aperture, depth and current eye. First, check whether the existing original frame opener scene will obtain a separate replicable texture within the same frame while retaining the normal two-eye view. If only one set of camera results are presented at present, it would be necessary to add controlled inner mirror scenes pass to the existing Native rendering owner, and to reuse the original version multiplication and partition logic.

Existing `weapon_longshot_scope.h` The basis for calculation is provided for eye relief, pupils overlay, eye observation, independent camera view cones, mask proof and proof of cosming. The output is a rendering program that still needs access to original game scene drawing, lens surfaces, GPU resources and back-up.

Key tasks include physical eye-scope changes, lens aperture marking, mirror view and depth resources, multipliers and original sight mapping, subtracting synthesis, left-eye masking, and historical segregation of views. The engineering risks are concentrated on the integration of extra scenery with old eyes' history, resource life cycle, and frame budgets.

The acceptance is expected to see both normal scenarios in the mirror and real-time magnification within the lens, correct edges of two glasses, separate head and gun responses, and acoustic results consistent with actual firing.

### 3 B, right-eye to target.

This is a user-allowed delivery exit program, with the current code being more commonly reused. When eyes enter an effective observation area behind eyescopes, they trigger the original opening path and keep the original magnification image and frame. Eyes leave, drop their weapons or go back to the first person who calls VR images.

The first round of entry parameters is eye distance. 4–10 cm, horizontal deviation 2.5 Cm, look at the angles.15°, continuous satisfaction 80 ms. Exit parameters are less than eye range 3 cm or greater 13 cm, horizontal deviation greater than 4 And the cm, the angle is bigger than 22°Either of these values is the default, suspension or failure to identify. The distance is calculated by the relative eye lens of the actual observation eye, and the right eye is tested separately, with the current observation eye being maintained until exit. These values are reversible prototype parameters that need to be adjusted according to models, control proximity space and headline feedback.

Switches to only processing observation mode. The gun control and firing still uses the same weapon posture and current authoritative gun openings. The original full screen opener path requires additional verification of head-motion scaling, double-scopes, acoustic direction and instantaneous view jumps. When an uncomfortable fullscreen zoom occurs, evaluates the form in which the original target image is carried in the existing fixed space displaying the route and clearly records the actual delivery pattern.

Existing 2026-10-22 Fixing records contains original mirror-frame eye drawings and `bZoomed` Camera selection, suitable for reuse. The head magnification effect of the record and continuity of the switch mirror remain to be verified by this gun.[Frame repair log](sniper-scope-vr-exit-20261022.md) [First person claims to repair the record](sniper-first-person-20261022.md)

### 4 R on the eye.3 Switch original multiplier

User specifies the right handle clicker to press R in-side 3 Maps the zoom operation after the original opening. Each real-time application is a multiple of the original at the bottom line, and the specific multiplication slot, cycling order and multiplier retention after the lens follows the authentic original logic.

The path also needs to be mirrored and the path of the original version aimed at the context and image confirmed. Both multipliers read the actual zoom status before and after a multiple switch. The gun is valid, eye and controller tracker is effective, no currently paused menu, no interactive loading.

| Input Situation | Route rules |
| --- | --- |
| We're in a good eye-marking position. Press R.3 | This edge is dominated by multiple consumers, distributed once to the original scaling entrance |
| I've got eyes on it. The original opening or mirror image is still in preparation. | This time, R.3 Consumable in the context of the target, re-activated after active image |
| Hold on to R.3 | Keep this one down, no repetition factor for the frame. |
| Hold it when you're away from your eyes.3 And closer to the eyeglasses. | Keep your original attribution at pressure and then re-set it in a valid eyelid. |
| Scaled by id, cut-gun, missed or suspended during pressure | Rescission of enforcement permit, this time under pressure to remain consumed until release; new perimeter is required after recovery |
| Common outside mirror R 3 Operation | Using existing input maps; Longshot fill is driven by physical action entry |
| Pauses the R in menu 3 | Processed by menu input context |

The multiple is next to the single consumer in the existing integrated input path. When the eyebrow is taken over, the old R-R-Previous 3 The same consumption record is identified for the bullet switch, the angle switch and the combination key test.3 The function is activated during the holddown.

With two-showbar menu combination key, keep menu priority: the hit combination key or the non-trigger rate when entering the menu. Only close to the eye but not yet completed a valid eyebox stop and cannot hold out an R that is already pressed outside the mirror 3 It becomes a multiple operation.

Short hint shows HUD in the vicinity of the target or inside the original mirror, and " right-hander switch rate " is shown when the eye is valid. Multiplier turns to show the actual multiplier briefly after success. Only action tips are shown when reading without a certified multiple. Switch result confirmed by readback through the original state, and the multiplier after reading the same confirmation is rendered within the mirror.

### 5 Recommended decision-making

The application begins with the existing rendering code and preserved evidence to approve the internal image entry, resource attribution and frame budget. If an independent magnification image requires a wide re-linking, this round uses the B path to complete eye openings and complete manual closing rings, recording the remaining works of the A path.

The main gap in A is judged by current source code to be actually rendered into the connection, and B already has the original display chain basis. The exact workload of both paths remains dependent on the current original open-scope camera and resource flow verification. Comfortableness requires attention to natural header response, information masking and reasonable spatial position, with specific eye box parameters being validated by this project.[Meta Comfortable Guide](https://developers.meta.com/vr/design/comfort/)

## III. Existing foundations and realization of changes

### 1 Bases for checking

| Location | Current content | This time, fit. |
| --- | --- | --- |
| `tools/vr-common/weapon_longshot_reload.h` | Single-handed, step back, right and left hand identification, completion of submission and direction description | When actually firing, use stricter than 4 Seconds, real mechanical steps. |
| `tools/vr-common/weapon_longshot_reload_test.cpp` | Existing Pure Status Machine Test | Add current time, recovery, inventory boundary |
| `tools/vr-common/weapon_longshot_scope.h` | ISP Optical Planning for Independent Mirror Insight | Actual Longshot parameters, product rendering access or eye-detecting reuse |
| `tools/vr-common/weapon_longshot_scope_test.cpp` | Optical testing using composite parameters | Actual tag configuration, mode switching and renderingend authentication |
| `tools/playground/weapon_lancer_manual_reload.inl` | Original auto-refill block and call permission framework | Verify Longshot sub-entry and access specific fit |
| `tools/playground/weapon_unified_fire.inl` | Unified real gun opening. | Verify Longshot class wired to asset, confirmed shooting incident |
| `tools/playground/weapon_native_lancer_follow.inl` | Current gun and second-hand access point. | Provides a touch through the dedicated Longshot configuration |
| `tools/vr-common/weapon_lancer_interaction_guide.h` | Step-by-step guide geometry and status patterns | Reword, provide the outline and action arrow of this gun |
| `assets/weapon-6dof/offline-v2/longshot-round.glb` | Full live ammunition assets are available | Inspection units, grab points, load the objects in the direction and to take them into operation |

This round is already in place. `begin_r3` The bullet-replacement is now moving to `begin_manual`, use physics to start the edges and start with a game time for authentication of actual events, strict `< 4 s`. The current machine requires a complete five-step semantic sequence and final locking, with the original test call moving. The actual shooting event provider, auto-loading blockage and reloading have been reached; the bolts, single-shot rounds, plugs and closed lock adapt to existing source code and offline testing have not yet passed.

R 3 The multiplier is suitable to obtain a real right-hand click edge by the existing input route, and preserves the consumer ' s affiliation at that pressure until it is released. The original multiple entry, slot reading and callback were approved when implementation was under way. A Multiplication configuration of the camera inside the path update mirror, actual multiplication of the B path using the original target; both share eye status and enter consumption records.

Original model sheet log Root 9962、Bolt 894 Triangle, Scop/Lenscap at the Lod 0 There is no independent whole. Development should be based on the actual geometrically bound lens area and bolts, using the existing content of the asset to determine the configuration of exposure.[Model worksheet](weapon-specs-20261028/model-work-orders.md) [There are Longshot Specifications](weapon-specs-20261028/weapon-cards.md)

### 2 Run-time data stream

`当前跟踪手与眼位 → 武器局部几何候选 → 现有交互仲裁 → Snap 显示或真实 Grab → 枪栓及实弹状态机 → 原版库存与完成事务 → 引导及手姿显示`

The actual fire echo is also recorded in the switch to this weapon at the launch point. The mirror view reads the same final weapon changes and current eye position, showing a common verification of identity and frame origin with the shooter consumer.

The public consumer will only increase the certified distribution required, and the handbone and weapon postures will be consumed through the only existing implementation point.

### 3 Auto-replacement of bullet blockages and inventory services

- Approved Longshot instance actually owned by the current player `ShouldAutoReload`、`CanReload`、`Reloading.ForceReload`, original bomb patches and exit routes covering equipment, weapons collections and re-equipment.
- The manual switch strategy is applied within the existing Native calls owner.
- Make an appointment for a bullet-capture phase. `loaded 0 → 1`、`reserve R → R−1`The identity of the item, the inventory version and the submission of the token correspond to the same operation.
- The original ATM will still have to meet manual transaction requirements when it arrives, preventing re-loading. The original perfect reward also requires the only token completed and confirmation of a return.
- Releases live ammunition before submission are released.
- The long-term holddown, repeated calls and delayed receipt are all tested.

### 4 Development order and checkpoint

1. The geometric configuration of the gun is frozen by approving original model grips, vents, bolt tracks, live ammunition assets and existing prototype sniper mirror resource portals.
2. The main hand and second shake, Snap/Grab, co-sourced firing and live incident timing were completed.
3. Auto-refill blockage, single-shot left shoulder, loaded, bolted, locked.4 Second results and full-step guidance.
4. Implements A or B based on internal output assessment, completing the calibration of a protometric and actual ray.
5. (c) Perform new and affected offline testing, compile and freeze private candidates.
6. Perform the targeted simulator validation in existing controlled operating windows, check actual two-way images, preserve results and clean up records.
7. Delivery of identified candidate, up to three header operations and log positions, with user-verified comfort, eye experience and natural change.

## IV Test and Controlled Simulator Certification Scheme

### 1 Autotest

| Test Team | Must Overwrite |
| --- | --- |
| Second point. | The deputy has no gun effects when approaching sorbent, the master moves without Grip, Grip controls, unhandled back to Snap, original hands out, hold down entry, miss and change guns. |
| Widget Geometry | World and weapons coordinates converted, fully rotated, physical model scale, boundaries inside and outside the range and precise borders, area demurrage, wide entrances to the mouth, overlapping arbitration, wrong side approach |
| GrabPose | Main handle, second shake point, bolt opening and locking, live round-up and carrying, load-to-load examination; verification of contact modification, fixed bone length, continuous arm wrist alignment and actual multiple angles |
| Single-shot Load | One shoulder grab, weight hold, live ammunition is in reverse, chamber locks refuse penetration, holding continuous loading, repeat insertion, early closing, real and complete process |
| Inventory | R=0、R=1, multiple rounds, cancellation after taking, failure before insertion, failure when inserting, failure to repeat, retake and collect, total constant |
| Perfect timing. | 3.999 S. Perfect,4.000 s Normal,4.001 s Normal, no-firing incidents, aerial fire, repeated shooting notifications, timing backsliding, repeat bolts, cross-barrel and recapitulation of the shotgun |
| Auto-removation | After firing, static beyond the original change of fire time, continuous unloaded gun, automatic entry, timer during receipt, old recall after manual completion |
| Aim. | Eyeglasses, distance and horizontal boundaries, right-and-right eyes, threshold vibrating, missed track, original open mirror failure, retrospect clearance, axle and lens map |
| R 3 Rates of multiplication and conflict | One eye click, long presses, continuous hits, outside clicks, external pressing, off-scope clicks, open-up clicks, menu and combination keys, missed recovery, multiple failure and repeat reply |
| Render | Mirror mask and shield, left-eye shift, near target, resource recovery, view history, time out or invalid frame exit |
| Guide | Each filling phase immediately gives the right target, steps to switch, bullet missing, pause, cut a gun and exit after completion |

The test entry must be clearly registered and run, the assertion maintained and the output failed. The integration example is validated by normal calls to the product access point for input, inventory and original events.

### 2 Actual simulator check

Use the currently controlled program, back-office window, observe-only, test silent and physical input isolation using purchased games and existing private packagings. Checking the location of running windows before startup with tools for identity, freshness, configuration restoration and normal closure. All actions are performed using the same product.

| scene | Operation and practical evidence |
| --- | --- |
| Hold and shoot. | Grab a gun, the deputy approaches but does not press Grip, move your hands and observe separately, then Grip, aim and actually shoot one hand, check the gun's mouth, trace, ammunition and visible hand positions |
| Auto-refill block | After firing, wait for the original reload to be completed, pull the trigger and check the empty guns, check the loaded continued 0 |
| Regular switch. | Waiting over 4 Seconds, open, left shoulder one, hold the chamber, lock it, confirm normal results and actually re-fire. |
| Perfectly swapped. | After the new one, 4 Complete the entire set of actions in seconds, check the release and locking times, the original reward and the next actual result |
| Stock and interruption | Last bullet with limited readiness, zero-premise, de-capable, unlocked attempted shooting, stop and resume the reloading, check reconciliations and guide |
| Aim. | Periscope, lenses, transverse deviations, left-eye observations, head and gun moves, close target fire to check actual images of selected A or B |
| Original Multiplication | Press R after the eye.3 Cross-repeated multiples, check images, status and single responses; long-stamped, over-screened, zoom in, double-stamped, cross-checking of input attributions and conflicts |
| All GrabPose | Maintaining multiple angles of the master handle, second shake point, taking bombs, carrying bombs, delivering bombs, opening and closing stages to check contact, interpolation, cuffs and finger changes |
| End | Pause recovery, normal end, check for Native, entrance, XR, game, encoded status and configuration original bytes |

4 The strict boundary is covered by offline certainty tests. The actual simulator performs actions that are clearly on either side of the border and records the real game time, avoiding having scripts waiting for hours to be used as evidence of perfectness. Only repaires and reviews affected movements when specific failures occur.

The visual inspection requires examination of the entry into the chamber, locking of the gun, hand positions and mirrors, and following existing media retention strategies.

### 3 Completion Conditions

The user will have a physical record of the ease of head-eye, naturality in hand and operationality of physical controls near the head.

## V Status of the programme

The following table in this section is the current status; the subsequent “History realization and certification process” and appendix keep a record of their respective points of time, where the words “the latest” are not reached by “the window to be submitted” do not cover this table.

| Requirements | Current Source/Product Status | Current Actual Authentication |
| --- | --- | --- |
| Main hand feeling with a gun and a shot. | Quite a way to get the consumer and start the original. | 37448 Actual shoulder grab and initial pass |
| After the shot, change of rounds manually. No automatic reloads. | Auto-refilled blocker completed | 37448 After shooting, continuous.6 The second is empty. |
| Gun bolt Snap/Grab and real trip | Has been compiled;5 And get,7 Cms hold. Original path and bone endpoint confirmed. | 37448 The first embolism was rejected by the historical trajectory and the opening has not been completed. |
| Take a live bullet from the left shoulder. | Reused public shoulder area and existing bullet-taking input, sole item lease | Resource ready, not actually single-shot |
| # Magnify the mouth,120 Ms adsorb with real delivery. | Source code and complete freeze; acquisition radius 6 cm/deep 8 Cm, keep the radius.8 cm/deep 10 cm; showing live bullets using the same current frame as handGrip | Full product offline production function/status machine checked; game loaded is not completed |
| Inventory submission, locking and perfect reloading. | One round of original stocking services; from actual fire to real lock-down, less than 4 The second is perfect.4 sills;ascorted boundaries | Full normal reloading, perfect reward and backfire are not actually working. |
| Figure 1 Second point with all GrabPose | Snap uncontrolled, Grip-controlled and spare parts hand positions are taken; natural contact is still pending/not used | Snap/Grip/Leave/Escape and Uncovered Contact Unaccepted |
| Sniper mirror and right swinger double. | Quick-Eye full screen option and R 3 Attribution; not taking the input from the eye. PiP in the lens is not yet available | The original low/high multiplier and access lenses are not yet running. |
| Change the steps to guide. | Gun-stroke target/direction, shoulder area single-shot line frame and exit arrow, bullet missing physical rejection mark, chamber outline and reboot after firing/completed hidden life cycle | Complete, stage-by-stage visibility and comprehensibility not yet accepted |

The port connection has been re-checked:`product-guide-lifecycle-r1`It's...1123 Freezing Source and PG`0f04387ef560eb159002548b7c155ae9d0e9cb805aacf2dfb5d2dcb76c11b53a`The load, displaying caches, hold-ups, geometry and reloading machines continue to follow this offline line; they lead other new genesis below. There is no need for repeated realization or rerun without modification testing; this cannot replace actual loading and inventory verification. The old appendix records on “not yet connected” refer only to their candidates at the time.

37448 Normal closing and final handover of originals`134c7369…`Release to the only GN-v 4. Next candidate for position is actually frozen as`longshot-prepared-20261110-050140-manual-open`：1597 Source,6 Products, original 18 Action Drive, prepared SHA`f0c263ed7e5dc7f66f02e99aea074f293f70a158b8a1b49712cda24ea8f5f52b`，PG`e2fc7447…`relative.37448 Only three Longshot files with failed initial input limits are added, and 5 Products and 18 The driver remains unchanged. The intention is added after Bow through the established GN route; it has not been accepted, no actual predecessor bound/implemented entry/claim/ticket/window and is not allowed to be implemented. Next step is to obtain precise historical evidence of rejection, retain all original prescriptions and identity restrictions, and complete normal round-up rounds and backfires.

GN subsequently confirmed that the intention of the gun had been recorded but not accepted; it had previously been pre-sequently issued as Troika 045716/core 2 This is after Bow, so the gun is behind Troika.

Full product guide now added`product-guide-single-round-r2`，PG`c3e572b7f0ba4dd0bbc074e5a6e79d4c2b4d32afe737fb65149545daecf71292`, complete compilation of links 0. Left shoulder bullet hint added to the same 15 And the two-faced frame and the live bullet.8 cm takes out the arrow, using the same certified body/reference transformation; a line frame is not an ammunition resource. The contour of the chamber is changed to a direct reference to actual adsorbing radius/deep constant.1 Change the guide to include only files, geometry and corresponding tests, and 1120 Document consistency, existing frame life/life 8 The budget is maintained.18 Group ratio, unit and under rotation, chamber 31 The end points of the original line are exactly the same as the old function. The first edition is independently compiled and found to lack a visible hosteo_frame.h dependent,r 2 Complete and reconstruct, initial record maintained.`single-round-guide-integration-result.json`(a) Recording source codes, construction and testing;`guide-single-round-r2/guide-design-preview.png`Created by the freezing function 's physical segment and examined in person, only for design previews, not game pixel acceptances. The complete product is not ready to run entry/queuing/deployment/run, frozen 050140 The race to the position remains unchanged.

The whole product is also filled with a negative feedback from the missing bullet.`product-guide-no-ammo-r1` PG`482d45ee1f0d6e5f61e4c135978334a6cd5b7fa3a23ae29ec2ed902eb5ef0514`Fully compiled links 0. Only existing stocks were read successfully and the same valid bolting attempt was confirmed as capacity 1 Air, limited.0, this time the refusal is recorded and the bullet is crossed red on the left shoulder; no more arrows are shown.750 ms, same intent continues to hold on for a continuous period, and identity/reference/entry context or in-generation changes do not show that subsequent valid positive inventory reads the clear hint. Unlimited bomb preparation 0, loaded or read failed to trigger.750 ms is the duration of the event that has occurred; each current drawing and action continues 100 ms check to provide feedback that does not update the old input time, do not obtain a lease, do not trigger original loading or write-in. Root and frozen sources are actually reloading/geometrically tested, keeping normal/perfect round in its entirety, just as it is.4 The second, inventory list and re-test.1118(a) The document remains unchanged;050140 Keeps the record of the candidate ' s place.`no-ammo-feedback-integration-result.json`,Design Preview`guide-no-ammo-r1/feedback-design-preview.png`The actual pixels have not yet been deployed/queued/run or validated.

Camera control supplements a re-emerging cache sample exit defect: old Control returns to its previous near status early when the serial number is not advanced, skips the current pause/ball change/eyebox exit condition; all four of the actual old function's target clips return near=1 The probe exits.1. The initial course is then repeated before the entry 80 The cache sample is out of sight during ms stay, but the old stop was used when returning.`product-scope-cached-exit-r2` PG`0e4ff8178892be42dfab3776401ea7718bd108399eeee353b4d40403fbfe0514`Compile Full Link 0: The sample does not advance or multiply, but the current suspension/reload/eye box lapse will cancel old near and candidate stops; the input of high water level and consumption pressure is retained when serial numbers are reversed or competing with the same source count, and re-postering must be re-stopped and adhere to real release/new pressure.100 Time limit, ms 80 The ms enter the stop and the eye box is not the same size.

Root and freeze control functions are passed 80 Check; also freeze the actual output of Control to original scope owner production containing documents, with four exits confirming OFF, R 3 Disable occupancy, re-observation of visibility and verify that old source reading back is not re-opening, and that real new edges can restore low/high multiplier rates. The original version of the survey is still called, state read and Native Bridge are injected into the boundary, not through actual game images or real key conflicts. Relatively missing complete products are replaced by only two header/test files.1121 The file remains unchanged; the Native/Pause/Input author does not change the record.`scope-cached-exit-integration-result.json`, two failed probes and intermediate versions are retained. The product is not ready to run the entry/queuing/deployment/run;050140 The candidate for the opening of the lock will remain as it is and cannot claim to have been either the bolted or the whole gun passed by this offline amendment.

Complete validation scripts supplementing original firing links, product`product-original-shot-binding-r1` PG`e18d9b0d6a2e66e72dedf1b52ac404456a21e6652bde9f8c388b69c9608d7557`Compile Full Link 0. The production logic only adds confirmed firearms/world/possessing the number of a sub- and original fire event after two established logs, does not add engines to read, clock or write, or alter actual change in the firing decision. The new certifier requires that the lock-in gun, serial number, original event, time stamping, and initial real fire back check be accurate, then calculates the duration of the lock-down less firing. The original script only uses two times inside the lock-up, with the binder having recovered its starting point for acceptance of later; the new check rejects the counter and checks that normal rounds do not add perfect rewards.19 Include Punctuation 3999999／4000000／4000001 Microsecond boundary, all through; these are the certifier kits, not the game pass.

The normal bullet-replacement branch, before any agency enters it, will be clearly completed with the TimeSeconds read in the original read-only inventory snapshot.4 Seconds, not extrapolate normal results by waiting for a wall clock or slow running script. Actual script injection tests show old versions of the clock at source 1.1 Seconds start moving. New editions wait.5 Second (original fire)1(sec) only moves; missing/distant fire echo,30 The following normal input is stopped when the clock does not advance and identity is lost in the second wall budget. The perfect branch retains a new real shooting and real opening/take-off/dropping/locking action, which must be measured less than 4 The root and the C++ original firing and reloading machine test pass, and a new log is actually checked;36 Entron 65 mm approach path, zero input after identity loss and 6 The entry refuses to stop returning to pass for the first time. Full preparation scripts in the future have been explicitly added to the new only-reading list of helper, and old full candidates are not posing as ready;050140 Positioning candidate is fully maintained.`original-shot-binding-integration-result.json`The product and the new complete script are not queued, deployed or run, and normal/perfect round and back-fired fire is still not physically verified.

After the user restores the target, then replace 050140 Prepare stationary source offline counter.`history-replay-050140-r2/FrozenReplay.exe`，SHA`490738695f9c632fd53e48a3a70a56b96ccd3ab4c74b1741148ca66d5663002c`. Compiler relies on report confirmation 7 The header of the project is drawn from the candidate ' s frozen directory, matching each item to the list of prepared sources without citing subsequent variable root codes. The program must provide an expected PID and creation time, with no match for identity, missing/invalid parameters, cut-off of original records, magic or layout not matched;8 The package is checked. The device is known to be manually imported and cannot be impersonated 37448 Original history or new game evidence. Actual next-run original window is still missing,gate 7 The specific case of failure is still unknown.18 The drive, access rights and queue order have not changed; offline recovery is not a record.`history-replay-050140-r2/result.json`。

## Historical realization and validation process (retains original point statement)

The latest actual game is 37448（creation 134387277995045930，v 5-Repair has been consumed and does not try again: real shoulder-take, authority-issue, post-launch 6 The second is empty, the original fire clock echoes, and one-shot resource ready five checks pass. The first segment is manually plugged in the frame.746 Interrupted, actual/waned progress still 0, original sample spacing 112.2413 ms, current source age 91.4049 ms;historyselect togate 7, which is rejected by relative.build, does not yet know the specific conditions of the inner layer.6 Sample function completes the check point, not rejecting number; count 0 is the output.count, which does not mean that the original input is running empty. Not reached actualSeek, old 25820 seek 13 Not a single hitch. No loose ends.100 ms/disarmament/lease/epoch.1、wrapper 0 Separate accounts are maintained.

Normal current round Native/input/entry/drain, XR 0／launcher 0 The /gamegone and ten original bytes of the C-C-X are back to pass.2 Page Full Double and 6 The map of Zhang has been examined by the police.803game／1128 Sim, double.52.8 Seconds, endend 17 ms;capable gaps 2199.9711／316.6612 ms retained, not judged by the engine Cardon. The gun, the aimer and the original HUD are visible but the wrist/forearms clearly shield contact and bolts, all natural hand types are unacceptable.48 h Latest 3 Actual retention 0 Stay.37448／13712／49912, delete 1722 Media 267299124 Bytes, errors 0；7 Saves a numerical file. Full embolism, shoulder-picking live ammunition, lock-off delivery, normal/perfect reward, re-entry and near eye multiplication are still missing.37448-closeout-review.json（6910024a）。GN SG 35-v 4 Only metadata uni-nuclei, actual preparation 0 and accept the only follow-up; eventually, fresh idle/xraw review and submission of a separate window.4／v 5 The preparatory text is historical and does not cover this actual result.

The latest actual attempt is already consumed 011500-v 4, stage failed to start game: new Echeck as main script exec was not set up for O search path, line 9 The import of retain_test_media failed, and the old HB doe time stamp was subsequently rejected at the entrance. Actual process/idle, worker/done/launcher.log and stage pre-byte, launcher-session only stdlib reports; ten originals, five stage archive values and original stop tags were restored, claim/ticket retained as history, not used or forged 0 Or the media is passing. The latest real Longshot game evidence remains 47996 Get the gun failed.

Only one intention is prepared for this moment.v 5-repair, no new consume/sage/run: the new check only adds the original O path and the actual E role is oldly failed 1 Newly adopted 0, two rely on the original freeze path, the original bytes and 18 Drivers remain unchanged. The new authorization must be directly bound to the current consumptionv 4 claim、24590337 End of failure, f 62d 36♪ I'm gonna be a piece of paper and 18dd 28d 4 Restoration baseline;old 539／HB 13712 History only, not considered as currently unconsumable.26 The actual frozen patch check is passed, including the old normal window submission independent rejection, the only single/scope/active parent process/original/raw text and new claim review.0：1596 Source/6 Products/18 Driver/1078 current and archive/13435host／7 Source; no C++, original input or normal cleanup process is changed. New clear repair authorization remains pending, the current window is kept under this LS and no parallel execution permission is granted for GN and its rear rows.5-repair/tail-packet.json，prepared eb 7d 8863／plan 05a 7ab 2c／ready 3ac 593af／entry 76833894. Below v 2／v 3／v 4 The preparation records are kept in their current state, and the actual failure and new preparations above do not represent complete target completion of the gun.

Current unrun candidate for update as the same intention 011500-manual-open-admission-v 4 Wait for HBv 6 The actual combination check found v 3 The original executing body will cover load/sha as the only lmbda accepted from Path, with a new claim binding and triminald Hash into the string; the AST that executed the former body two helper has already read_text/read_bytes AttributeError, which was previously isolated by fixture using external Path to force-to-helper, leaving out this layer of coverage.4 The only thing that matters is the fact that the two former bodies are body helper plus Path (p).14 AST, source code/six bin/old 1075 Python pins hold, add the entry to 1076. Namespace running with actual body 38 Passes of the gate/coding/inde/storage originals/restoration/terminal inspection, hand-over to receive scripts 6 Set and set Jvenv readiness 0；1596 Source/6 Products/18 Driver/13435host／7 Source matches.3 Non-consumer eligibility for decommissioning, no actual new claim/ticket/stig/game;47996 The game failed and the whole gun was accepted unchanged. Package is acquire-current-admission-v 4/tail-packet.json，prepared 0e 3888c 8／plan 3a 8eba 2d／ready 13e 61093／entry 4ecde 59f。

Below v 2／v 3 Records are kept as at the time and their eligibility for execution is already in the v above.4 Alternative.

**The current complete delivery has not yet been adopted.**Updated engineering development/deliverable self-assessment by harmonized A 20%／B 30%／C 20%／D 15%／E 15%Weighted to 41.25%The pieces are a base A.0.5, B complete proprietary process 0.5, C components adsorb/GrabPose/Result 0.25 D. Interrupted/stocked 0.5 E-Current deliverable returns 0.25. The score is not working hours, old maturity points or head-over acceptance rates; old 25820 The first release and empty slice passed does not automatically inherit to the current unrun composition. Detailed seven-step player results, evidence, risks and remaining items are written in this piece of gun. `analysis/coordinator-plan-review-20261109-2243/longshot.json`。

Current 47996 Early VR shoulder acquisition failed, and this basic link must be restored; earlier 25820 The actual bolting is still unpassed, with the whole of the back being removed, loaded, locked, re-fired and strictly less than 4 The second reward is still available. The sequence of implementation is to position the first original rejection with a narrow-set off-the-shelf segment, immediately after repair, and to promote the full normal filling cycle of the same normal composition, then concentrate on verifying the incentive, close eye multipliers, associated interruptions and all hand models; not “take one bullet” as the end point, nor continue to list hand-held candidates for unconnected products.

Current Follow-up Candidates 011500-manual-open，PG be655f557b03e2ba4a8d40346048032f89cfecbb19e99f920d7655cc50c9efc8 It's actually...47996 Use 08537 The only thing that can be used is to change the private equipment consumer code.23 Original current check location and 9 The original word plus the threaded partial POD rejected the number, including the original second clock check in the outer layer; the reverse eraser could be restored to the original byte without a new engine reading/clock/entry writing/Native ABI change,100 ms and ownership checks maintained.160 Backend/640 Outer layer original functions cross-checked, original/new production Runtime 120 Example 1548 Check pass; observe expenses not measured,47996 No new numbering is available. The cause of the explosion or repair cannot be declared offline.

The same successor candidate also connects to three E-owned assistants, e.g., with a track and key, who read the current round after the first Grip explicitly obtained the rejection and stopped following ordinary actions;8 Old/New Actual Core Separation Passed without Modification 18 Reference drive, and no holding results. The actual initial archiving of the HB has been nucleone-built: repeated binding with the installation of slash aliases, repairing this gun's own config generator is only one setup, and conflict setting aliases are still rejected;11 The item is not written in the unmodified form_policy.locate passes/rejecteds do as expected, without a real configuration/Steam/registration form/play.47996 Hold on to the reason for failure.

It was the same.011500 Intent bits replaced with unrun 011500-manual-open-admission-v 2 and accepted by HB for future HB 010515/v 5 Only follow-up after physical completeness; original receipt accepted SHA is 3c 55d 24 a Make it clear that there are two practical reverses to the original entry: old HB is acceptable after new identity.005428⁇ Frag original interface and the environment clean-up missing unlined GEORS/OPENXR variable.v 2 Only fix access/contract: fix the current five HB identities and real claire, terminal and final transfer routes, single-nucle physical cleanup/two-way personal review/coding/retention original evidence; fsync with the same ten originals and raw handover to the first O_EXCL consumption record, stage and recheck before recovery, remove the previous second capture. Case does not involve clearance of GEARS*/PYTHON*/XR*/OPENXR*, prepare/idle/normal stop/subprocesses, all applied, OS variables remain. Normalstop works with cleaning conditions, gun movements and all C+/6bin/original C+/VIbin/s.1073 Python pins do not change, only add one actual entry to 1074 The actual AST entrance.37 The item fixixture is passed, including the recurrence of old defects, new door rejections, process/microproof evidence/original and inode or replacement inode/four prefix isolation; no touch on real future claim or game.1596 Compile source/6 Products/18 Original driver/1074 current and independent archive Python/13435host／7 Source's fixed Jvenv readiness mode 0. Real claim/ticket does not exist, no stage or running. Details are acquire-current-admission-v 2/tail-packet.json; old v 1 Retain as unconsumption history of withdrawal from operating eligibility, old Cover 58df、002143／47996、3 bfc is not allowed to return windows. Full general filling/backblowing priority, after which rewards, near-eye multipliers and all natural GrabPose acceptances are still required and cannot be prepared for inspection through closure.

The current binding is based on the same intent.011500-manual-open-admission-v 3: HB permutated HOD 49912 Recorded start-up cancellation occurred and HB changed to execution v for this only 6 So this gun is only for HB-precision.93c 9130f／readyf 7ee 0986／entryb 85850 bf, fixed claim and future financial path, and its own alias/metadata; HB preprepareda 186 No change. This gun.14 Functions AST and v 2 All equal, reset the constant replacement with reverse recoveryv 2 Source byte precision, regular Native/XR 0 The exception is not to access the HOD without change in the contract for the complete media.1596 Compile source/6 Products/18 Original driver/old 1074 Python pins hold, add v 3 The entry is 1075, fixes the actual preparation of Jvenv 0，13435host／7 Source is not updated, original 37 Lines don't run again.3 prepared e 47c 9833／plan 7c 7edaa 7／ready 80cf 212a／entryd 433e 1 f, package acquire-current-admission-v 3/tail-packet.json, awaiting HB Unique;v 2 Unrun eligibility withdrawn, v 3 The real claim/ticket does not exist, is not stage/run.4 The second prize, the peri-eye multiplier and all of the GrabPose acceptances did not change.

v 3 Final handover of receiving scripts has been completed and the actual delivery window is not yet accepted: only fixed HBv 6 Exact original final SHA, re-enact frozen entry 14 A practical semantic function; the only instrument issued is after verifying the true advance, executing the existing read-only preparation mode and confirming the idle, then preserving the same ten original bytes, dropping the original sole nuclear record, releasing the original complete verifier and actual entry gate. The script does not consume running the claim, stage, do not start the game or send input.6 The new target check in entry is passed, including actual missing/mistake/format error that the channel refused before copying any original, document or queue; the first fixture error requires argparse to show internal ValueError details amended and recorded, and the script received was not altered by the original package.37 The items and gun packages are not duplicated.

Recent Actual 47996（creation 134387152660895255) Normal end, but no play: The current weapon is recorded in Longshot after original X has been picked up and then the VR shoulder fails to claim that core_checks was empty and did not reach the initial or untie.0♪ And the imprato is 1, no outer exit code to claim success. Actual 389 Recording native_equipment status=unaviable, input_status=event; the line is not sufficient to prove timeout, spatial area or identity root cause, old 25820 The opening of theSeek 13 and its core 5 The two pages of a complete double-road overview and six original maps are already in hand. The images do not create VRs;45.18 Two seconds, game.630／sim 965 Frame, maximum catch gap.3066.6112／349.9965 ms, all over the end 500 Within ms, the gap is maintained and does not vary from engine to engine.0／launcher 0/gamegone and 10 baseline original bytes restored, encode 0／retention 0 Latest reservation 3 Session 47996／9764／34568. Resh@elder_scene 225349-V 2 Actual preparation 0, identity/baseline/idle and formally turn over window 3bfc 6a 24, the LS no longer holds windows or writes about shared runtime.47996-Closout-review.json and Run 47996-final-handoff-boom 225349-v 2.json。

Can not open message 002143-manual-open-admission-v 4, the only follow-up to the same sequence without adding a new position. Cover finds old before turning over the window 235126(a) The use of the term "mummerburg_reload_chain.py" in a conservative list by 7c 210b 0 For d 4c 106a 6, the old package is rightly rejected. Only three new lines of HB have been added to the kernel and branch stopped; the LS still chooses its own core/open drive. The smallest derivative only updates the snapshot and V 4 Entry identity, the rest of the old 1067 Quickshot,1596 Compile source/6 Products/18 LAS Drive/13435 All the host lists remain unchanged, and add new entry to 1069 Crurent/archive. Actual fixation of Jvenv readiness mode 0 Source code/product/driving/independent archive/host/7 Source is passed; exclusive consumption, non-authorized flags, environmental cleanup and the original operating body are not changed, but are replaced by reverse constants.4 The old V-V is not a good idea.3 Unrun withdrawal, real V 3／V 4 Neither claim nor ticket exist; Cover has been sent in a single copy of the same order, which still requires a new final window and remains accepted for the front line 144 Pregame Total.

Previous Source Candidate `235126-manual-open-admission-v3`- I'm sorry.231059-v 2 The running qualification has not been run because Python optimized parameters remove the asserted verification. Same to V 3 The reverse source code changes the two behaviour and self-identification constants; PG 08537ef 8／Nativec 98568f 0／XR 16840b 27、1596 Frozen source/6 Products/18 Action Drive/13435 Most and old 1067 The project snapshots are unchanged, only V is added 3 The entry snapshot is 1068. Physical preparation mode 0、7 Category whole entrance failed to retry,8 Cover has been independently checked and formally accepted V in the same single sequence.3, Records `cover235126-formal-acceptance.json`. Old f 10 The pre-reform is now precisely bound because of the same claim gap 20261110 Cover admission-v 3／prepared b 14404 ca. The actual call factor for this pre-sum is inconsistent with the slash parameter string, atentry 39, exit before reading plan and stage 1; the gun has been independently checked for five identities, de facto exit logs, retention of claim and PID non-existent, Bol 66b 239 Intersection, 10 current/store baselines and independent idle, accept this pregame complete, new_game_identity is nulll. No fictional games/records/coding/reserved jobs or old 41460 Cleaning through. Recording. `cover-b144-pregame-closure-acceptance.json`. Cover fresh is still required to send the only formal final handover; acceptance of the prior act itself does not teach windows.10、41460 Or an old gun.25820 None of the windows can be taught. The real LS claire, the final ticket, stage and games have not happened. Only necessary checks are made before actual changes or window submissions, without repeatedly expanding the envelope in waiting.

The gun's only follow-up has been independently processed in the same order of procedure as Boom.225349 admission-v 2 Short replacement, V.1 Operational withdrawal: physical preparation model 0 Five identities.3955 Source/6 Products/86 Actual dependency/7737 Most and 7 The complete entry is rejected by non-acclaim parameters, PYTHON environmental clean-up and its own path are not changed. The recovery is stopped normally by the word. The optimisation of the entire copying entrance/non-retest check is performed in person. Lately HB and Frag only follow a genuine new tail and no second team has been registered. `successor-boom225349-v2-admission-check.json`Old Cover 231059 Record of admissions kept as history, not V 3 The window.

A specific cessation condition gap has been filled by a complete re-load driver in the future: waiting for bullet phase 3 Canceled to idle 0 or any current reloading of the same gun into recovering 8 immediate cessation of subsequent normal bomb-taking input; regular completion phase 7 Quit To 0 The records of the other guns are not miscalculated. Directly executes the old/new actual driver six mainframe separation contrasts, while the old code still attempts to move a shoulder after two bullets have been interrupted, with the new code being zero; only one zero key release of final posturing is retained. `full-driver-waiting-round-stop-r2/result.json`. This is not a real game that goes through and is not repackaged 235126; first locate and repair the original bolt rejection, then advance the general whole-turner and counterattack.

2026-11-10 The new live-fire is only possible if the original frame is called in and the idle is used to draw a yellow-head cue, which leads to a bolt being pulled after having received the loaded gun or full reload.4 The second, inventory, incentive, interruption and old lease clearance checks were still in place. `guide-lifecycle-r1/result.json`. Only three documents were then incorporated on the original complete private composition, the rest 1120 Keep files in original bytes and complete translation links through PG SHA 256 `0f04387ef560eb159002548b7c155ae9d0e9cb805aacf2dfb5d2dcb76c11b53a`Native is still ced 416 Be, no reconstruction.1123 Freezing source production test runtime test compiled again as 0,Prettyback/R 3, the original single-shot inventory and parts hand-wiring are still recorded at the real back end. `guide-lifecycle-build-result.json`. No new prepared packages, queues, stage or deployments are generated;235126 The diagnostic version remains unchanged and the actual guide image is still unverified.

Before 230242 Preparation mode was rightly rejected because of a new HB common driver; at HB 230535 The source version of the change was only made once, necessary for non-change.231059 Only the document is updated with its own entry constant, and the remaining source/product/host list of compiled documents remains unchanged.230242 and 222840 Unrun withdrawal records and a lead phase matching Windows path strings have been declared unsuccessful; no real one-time instrument or other assignment has been generated.

Workspace and branch checked to `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`、`codex/6dof-single`. The HEAD at the drafting time `6269cc9` Keeps as historical location; the source code is maintained separately. The workspace also contains other changes to the source code for sessions, and subsequent operations and public access require compliance with current windows and shared file attributions.

### Second handshake correction and surface inspection (P.S.)2026-11-09,up to the newest)

Actual frozen C 0 Consumers have a re-emerged defect: effective Grip, after entering the hands to control the gun, two branches replace left hand targets with new aggregate targets, only hold points and weights, and lose their original preset/15 Section authored position. Privately corrects the original target by updating only the shake points and weights that are available; original hands solver, original controller, right hand type, identity/lease, clock and only C 0 All writing points remain on the same path.

Group C 0 Test old Marcus 115 Reference/local binder to reproduce old codes 11,105 Checking 128 Entry failed, private correction 11,105 Checks all pass. Cover effective hand-to-hand solver, non-mounted back and only close to adsorption three paths, four hand-to-hand choices, customization 15 The custom table intentionally uses a different staggered posture as a test mark, not as a rifle-bearer type. The first test packaging failed to assert before the original document CRLF matches the difference between LF, failed script retention and subsequent amendment kept the original line-relay byte.

Complete private Native `native-support-style-v4` Compiled Link Successful, SHA 256 `600fc2f982a6a80f488670f83dc20dc13fac32af9c29313d964266f19e965f02`；1582 Freezing Source Only `body_c0_hands.h` Change. No share of Native is shared, no version prepared, deployed, queued or run, nor is it replaced for locating the actual embossed rejection 201902. Record `support-authored-c0-regression.json` and `native-support-style-v4/build-result.json`。

The surface patch of the second shaker is still unattended: after the actual generic rifle-bearer model was rebuilt,180 Triangular line of rivals/guns; two complete subsequent rounds of each 345 and 369 Yes, both maintain failed and refuse to take over the product. The original lower track is open at different heights, as previously selected 3.6 cm height falls on the side wall of the sink; amended to be the actual outer orbit and full handspace binding as 112 Yeah. And then we'll split the five-finger point of contact/method availability from the cost of an discontinuity collision, latest fit 5 Yes 5 Counterparties/guns, but still 116 The rivals themselves, of which 66 Cross-finger branch; index finger contact line point only 0.881 The thumb joints are also subject to natural review.4／fit 5 Three views, all unconnected. The count here is a cross-section over the established threshold of the line, not penetration depth. No range hits, single grips close to or five fingertips are allowed to replace full GrabPose; the cuffs, whole arm and actual game skin must still be independently verified.

201902 The conservative Python operation boundary was changed and the candidate for new assignment is withdrawn from historical non-operational status, as shown below; the original fixed Hash check is not relaxed. Complete reloading, original multiplication image and natural fit of all components remain unaccepted.

### Purchased version of path-based interface with unimplemented empty windows (in thousands of United States dollars)2026-11-09）

**The latest user selection already covers the then Steam path limit of this section:**User then assigns directly `J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64` Used for this gun verification. ExE SHA has been physically checked 256 Yes `bdc6705d…`and approved from unmodifiable private configuration `76561197960271872EngineUserSettings.ini` Set binding; no longer require this validation to restart Steam. Below 215840／4380 Maintains a record of historical failures and non-implementation.

New `222840-manual-open-owner-copy` The post is available on the website of the Ministry of Foreign Affairs.1596 Compile archive/6 Products/18 Original driver/1 Read-only Dependence/1059 There are currently no bad entries in Python and the independent archive, and EXE, specified path, private settings and interpreter identity pass.08537ef 8／Nativec 98568f 0／XR 16840b 27, package the current Python boundary by this user only; no changes to Steam, registration form, game assets, public entrance or backstage isolation protection.8 Prefixes, prefixes, entry 3 The error path/Hashi/set denial and accurate recovery test were re-approved. This is a new back-to-back window application, which has been turned over to the original only subsequent track; no return of the empty window that was handed over will be required, not in second position.

Original 201902 The old plan still chooses the history game directory. New unchangeable `215840-manual-open-retail` Select explicitly existing Steam Purchase `J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe`The actual EXE Hashi is still supported `bdc6705d…`。1596 Compilation and archiving,6 products,18 Basic drivers and 201902 It's consistent, PG.08537ef 8／Nativec 98568f 0／XR 16840b 27 No change. Current 1054 Individual archive of project Python by SHA/AST, lined 13 add or float items, where 3 Portable of this gun path/baseline package, the rest being other unused branches and offline tools; this is not an assertion 1054 All of this is being implemented or static analysis attests to all the dynamic dependency.

New packagings only obtain a 10-document baseline after the actual final handover and independent idle, bind purchase-end path to private settings, and maintain 0.3, backstage, silent, physical input quarantine and original action drive. Actual original NORMAL-stop, Native/entry/pad/weaponsdrain/XR/launcher cleanup reply and new baseline return only after adoption by fresh idle.360 The second boundary only requests a normal stop without killing the game.8 Prefixes, prefixes,3 The error path/Hashi/set-up rejection, original configuration of other fields to be retained, BOM/CRLF and missing files to be restored, and write a refusal before the open status change is executed.

215840 The files were identical at the time of production, and subsequent actual pre-tests correctly rejected and modified `solve_foreend_staged_grip.py`；1596 Archives,6 products,18 Driver and Archive 1054 The copy is still consistent. Failed prechecks are saved in `retail-215840-refused-preflight.json`The candidate is not deployable and the current document is not silently covered by the old candidate.

Drive on Boom 214704 Actual 4380 The same buyback portal has been proven to be blocked by the original "Steam needs to play" dialogue before injection; Native, operation drive and game recording are not started. This is a front-runner start failure that does not count as a gun game validation. It has been verified that it is normal WM_CLOSE exit, 10 file restoration, retention finality 0 Read-only observation also shows that Steam registers PID.51320 No actual Steam process is 50140, because the root is not proven; users are asked to restart and log on to Steam at their convenience without changing the registration form, account number, license, Steam or game file. The existing Steam client start-up branch explicitly requires manual front office, does not circumvent current backstage and isolation requirements, and does not reverse history directories.

This gun is the only window management that's been received.215840 Execute permission; period zerostage, zero game, zero input and zero recording not applicable. This gun review/coding/retention is then uni-nuclei for Frag 200930 Full Current Precheck 0 The exact prepared/plan/ready and the current baseline of the 10 documents, which are the only subsequent final surrender window. The authorization for historical certification given to its owner alone belongs only to that task and does not extend to this firearm. The true tail order of the gun is maintained; the gun is re-applied to the real end when the conditions for the purchase of the version are in place and the new exact candidate is ready, without a second team or an automatic return request. `retail-no-run-final-handoff-frag200930.json`。

### Follow-up to the original bone length and handheld (infra).2026-11-09）

Reversing, splitting and bending the root of original local axis control and changing the hand to horizontal support, fit 7／8 The cross-finger line is down to 0 But the gun is not in contact with the thumb; it's all denied. The hand and the five fingers are set to fit.9 The whole line of guns is 0 But the actual palms are hanging.12.22 Mm, can't count on a gun.

Check the palm dots before you find them.10462 Actually, there's only...48.2%The rest of you follow the thumb bone.25／24. Replace 100%The point of skin that has the weight of the bones.1036 Zizen front.4744 It's...1.2 mm external contact target, fit 10 Handheld error.0.95 mm, legal point accumulation 0.994; thumbs are moving naturally, not overturning the side wall. There's still a version of this.4• The line of the pointer/sun gun,4 Hand-over to no name/nominal root. Hand-over to hand-out.66 Yes, the product has not yet been accessed.

Both follow-up reviews were rejected in their entirety: only anonymous/finger and the near-distant binding of actual intersections, fit 11 And become 11- Yes.6 cross-finger; push whole front by back-com AABB 1.914♪ And the fit ♪12 I'm not gonna let it go.4 The front, but the front of the gun.34 For new lines. No approximate loss or partial avoidance was used to replace the original whole gun/hand triangle review.9／fit 10 The tri-view has been shown; the current production of wood handlers is not replaced by these unsuccessful candidates. `foregrip-reference-progress-review.json`。

Then fix the real hand pointer fly-flit.13 Although cross-referenced 0 But the whole gun line.46 Yes;fit slides within the original limited triangle 14 Yes 19 Guns/80 Self/11 cross-finger; moving forward and eliminating the four-finger axis 15 Yes 43 Guns/90 Self/45 Cross-touch. All rejected, no product configuration of the wood. The continuation of the offline variant has been stopped; remaining contact is centralized after normal filling can be played without a full action that the player can observe instead of the proposed scoring.

### The current frame results are shared with the mouth adsorbing and hand positions (infra).2026-11-09）

New whole quarantine combination. `product-full-chamber-presentation-r2` Compiled Link Successful, PG SHA 256 Yes `9d2e0afad1d3b5d870677707d8106e57a5973eabde8af50d1f800cf2de9d3664`。1115 Freezing source-by-source check; six affected modules compiled from the actual frozen source to execute all 0 Shows/hand position cache, original shoulder grabs, manufactures for loading, manufacture of reloaders, parts positions and physical size guidance. Covering stationary adsorption, real axle delivery, alignment with frame display/Grip, invalid withdrawal, re-close after reference/entry changes, and inventory and strictness less than 4 Second border. Each time you are prepared to read the original Rig only, show no repetition with Grip getter; this is offline check, not time-consuming proof of the game.

This is the one that keeps the close-eyes back.3 Enter Attribution, V 3 Part hand position targets, original launch entrances, UI frames and existing lead lines, adding physical scope of the vent,120 ms transition to real delivery constraints. No changes have been made to the read-in fields of Main, Backend, Native, Refire; Main/backend only combines to private frozen sources. See results `analysis/longshot-development-20261108/chamber-presentation-integration-result.json`。

The complete combination is now static ready to be `182222-full-reload`，3199 Archive Source/6 Products/18 Base drive/7 Additional access and read-only precheck passed; no queue, deployment or operation, nor replacement of accepted `172406-manual-open` narrow version. Old front-runner HODcore 2 withdrawn, replace core 3 later withdrawn; latest core 4- Query, re-admissed by Boom.1247 Source/6 Products/81 Reliance on passing still has to wait for the final complete window. There is no running window, only follow-up 172809。Public v 71 and the existing manual entrance remains as it is.

The future full re-load drive is also moving to fixed physics. 6.5 cm: actually implement old drive injection paths in 0.5 Multiplication Model Re-emergence 3.25 Cm's error, fix it.36 Group/model ratio/gun rotation/real-fired load rebuilt from independent four-digits, all passed with maximum error 2.55e-16 m Seven institutions refuse to stop the inspection or change of identity. These are offline proofs that are non-playing, uninvolved; and the failure of the first version of test kit labels without geometry is retained.

A follow-up original surface review of a single GabPose found clear defects: output with the same frozen C++ finger algorithm 15 Section 4D. Rebuild original 1395 Handsome and practical 1248 The bullet face, the original V.3 Hold on to it.148 Over and above 1 The original figure III looks at each other, and the body goes through the index finger/middle finger/ thumb, refusing to call it natural. One time the stand-alone candidate for a border gun falls down.64 Yes, there are still piercings and unnatural fingers, which also refuse to connect the product; the original bone length/input/team is intact. The indicator is not penetrating depth or actual game skin proof. The next point is used in lineline skin-tape lines to cross the finger to reach a target, keeping non-contact finger natural scrolls and then documenting them. `round-skin-v3-review.json`;full candidate still contains old V 3, only as preparation for unrun mechanical validation and not final hand-delivered.

Next single gesture 5 Entered private future product source code/compilation: a bellow selected from the original hand-held line, side-to-side weapon, rear leg held and bone length unchanged; firsthand look, hands-shot 148 The crossline is down to 0. The index finger/ thumb contact from 172 Yeah, down to the...10 No new pair; still original index finger 12／13 The joint wrinkles, which cannot pass as a complete natural skin or sleeve.3 As a result 33 Rejected for puncture and contact error, fit 4 As a result 14 In the middle, contact is denied and the original result is retained.

Reuse existing V 4 Complete 32 Byte Model/15 Sections reference binding, former Broker and the only C 0, only private ff 1 Frozen narrow source combination; retaining original target R 3/view and V 2／V 3, does not share the Native or copy Lancer Shape Constant. New Native `ced416be`PG complete `d254c5ac` Compiled 0; actually freezing left C 0 Consumers use original reference/local folders 815 Item 0，V 4 owner 44/Real provider injection 67 Item 0，V 3/V 4 Transition with three freeze PG module checks 0 And there's something else.24 Group skin mounted/units/rotational contact check. The same live count and watch, V, is locked when the bullet is taken.4 Not valid and not back to match old V 3 Finger shape; original Rig still determines the real warhead and agency/stocking authority is not manually readable from display.

The group is still prepared to be `190957-full-reload`，3208 Archive Source/6 Products, not queued, deployed or operated; no replacement 172406. The three test packagings failed and the two pre-package paths/cache types failed, so they cannot be erased after success.4 Artisanal/skin following, other parts, complete filling and multiplier images are still not recorded. `authored-round-integration-result.json`。

Latest 172406 Actual 25820 Complete and end with only Frag 172809, this session no longer has windows: gun pick/authority launch/continuation 6 Second empty/real-actuarylock/one-shot resource 5 Adopted;46 Current constact, original UI starting pointback,33 Point handle paths and actual Seek.764 First refusal to drop on a private term "seek_gate"13- That's... `prepare_controls` Return false before SetPosition calls; reconcile_gate 8 Overrided, pre-call source age 87.2834 ms, grip andlease are valid. It does not prove that the function is still active when it returns, nor can it locate internal terms or attribute them to a pure time limit. The actual opening is still incomplete, no single hair has been removed and no declaration is made 17072 The contact failed to be repaired by the cause and effect of idle scope.

Wrapper, Bureau 50208 Final 0／operator 1／launcher 0，Native／entry／pad／drained TRUE、XR 0/game exits; early finally still live to hint for retention without mandatory termination.8 The first of these is the one on the Internet, which is a series of articles.3 original bytes of privately owned products; first-time addition of defaults to discovery `lancer_interaction_ranges:false` Unlike Settings BOM, the original save bytes are restored after confirmation that there is no unknown configuration change, and late Writer has not proven. Original working/done/log and new values return not covered by the old slotting.

2 Page Full Double and 6 Zhangharatu is a witness to the trial.843game／1163 Sim, Range 54.539／54.522 Seconds and double tails covered; original scene sandbag, forearm/cuff and gun angle to block institutional contact, complete natural positions, embolism visible completion and multiplier not accepted. Maximum game capture gap 2.073 Seconds, not FPS/Cardongenian. Encode 37067／retention 36113 Actual 0 Deleting 1090 Media/176836700 Bytes/errors 0,Reserve 25820／11536／49864 Latest 3 Bureau 48 Hours, source/product/save/log. Last complete follow-up 1080 Source/6bin／19root＋27frozen／187PG＋381 Native and 289 Python SHA/AST does not differ, and the only window is clear after the fresh idle. Close. `idle-validation-first-run-25820-review.json` SHA `e2889d9935c557e8b1d7ac993668ebdbb1497012b3dc0009e882acc9205efe28` Keep it that way; the next point is the same. `prepare_controls` Internal position denied, no reruns unaltered 172406 Don't relax.100 ms/identity/Lease.

Previous Actual Window 17072 Historical conclusion: original fit trigger, authoritative launch and continuity 6 Seconds keep empty through, then fresh. `longshot_reload_contact_current` Stop, not entering the current round. The Bureau has been properly finished and returned only. Actual embolism is completed, single-shot from left shoulder, loaded/locked/ordinary and perfected, original low-high-rate images, all hand/finger/cuff surfaces fit and physical head recognition still pending. PiP in lenses is not yet achieved; no display of targets or CPU tests can be recorded as skin-coated pass.

### 25820 Targeted candidate for pre-checking for bolting (in thousands of US dollars)2026-11-09）

`195551-manual-open` Ready, private PG. `08537ef8` It's a complete translation. It's a direct inheritance of the actual 25820 It's...1096(b) A copy of the PG freeze source;1596 Run archive source/6 The Queen of the West is a very important part of the world.18 Input driver and read-only reader with 25820 Same bytes, Native `c98568f0`、XR `16840b27` Reused. Only two private files have been changed: the original animation pre-screening path adds a POD number, which is the largest 12 This is the additional number of this line of the gun.`prepare_controls`、`observe_expected`、`observe_kind` and `selected_node` Maintain original conditions, short-circuit sequences and read times; accurately restore the entire animation file after observation. No engine is added to read, clock or validity, nor will it be used `enclosed_observation` is replaced by skipping the original read.

Actual old `prepare_controls` Function extract and compile 104 Group Injection Contrast, Include 25 Number of rejection points, two peripheral observation models and observation switches with full consistency in return values, call sequences and output, failure to produce zero; all 25 The rejection point is actually triggered in normal mode. The package does not have a dispatch API, which does not prove that the game has been successful with the original getter, skin or bolt. See the results. `analysis/longshot-development-20261108/controls-trace-test-result.json`;for plans, see table of contents `controls-trace-runtime-plan.json`。

This candidate is used to find 25820 Medium `seek_gate13` The first internal rejection condition is not functional. Additional requests have been made for unaccepted, non-deployed, inactive and no running window at the end of the existing queue;25820 The final handover is closed to the immutable retrospection. Next time, only a short film will be made after the clear and sole window turn-over and independent pre-screening, with the first clearly interrupting normal movements and cleaning properly.100 ms, fresh identity/lease agreement, reverse re-location and delivery to a lock is less than 4 Keeps all the seconds.

### 2026-11-09 Run boundary and gun-string update

Current only active identity accepted is `201902-manual-open-wrapper`PG is still `08537ef8`♪ Native still ♪ `c98568f0`XR is still `16840b27`I'm sorry.195551 not running; pre-generated to supplement operational dependency 201149 And then, the wrapper wrongly asked for a subsequent preparation.201902. Two old files remain unchanged and the application for running is withdrawn. New candidates are in conservative archiving 1045 One project, Python and one SHA/AST-by-one verification, containing unexecuted tests and other branches; physical entrances, exec/runpy, launcher subprocesses and sys.path map separate, not eligible 1045 A fully run or complete dynamic import closed package has been proven.18 Original Input Driver/Reader,1596 Compilation source and 6(c) The product is retained; both pre- and post-checks are carried out simultaneously with the current root and independent snapshot.

Wrapper only removes the "subccessor/subc accessor_thread ready" prefix, retaining front-runner final turnout and its review SHA, current state/candidate/ready/prepared, dle and pre-check.8 Group cross-check: old version no subsequent real rejection, new version is available when the window is correctly driven; wrong front-runner Hash, state, financial, Candidate and both product records Hash are still rejected, testing stopped before any preflight/subprocess/stage.191450 Independently fully certified and officially accepted 201902 This session is not stage/run, no windows. Later on to Lancer 200139 Revert explicitly due to its own visual impairment, Frag 200930 Independently complete and registered as the sole follow-on to the LS; subsequent preparations will not block the process.

There's new evidence on the embolism surface: re-use the proven format decoder and check the original Longshot export. `741d9653`，9786 Top Point/10856 Delta and vertex-to-capitulate CPU/GPU/BoneMap consistency.356 The anchor is on the upper end of the emboss.3.926 cm. New anchor points from 50 Proposed combination of an independent outer cap top, with the mesh local coordinates approximately `(-4.81249,13.94165,5.35609)`;Snap/Grab/Guidance and Hand positions share the current embolism point by currently mesh-closed bot conversion to agency coordinates. This is not explained 25820 It's... `prepare_controls` Rejected and not altered.201902 The original geometry.

The old general cylinder hand is in 12 At least one roller sample still exists.292 Counterpart - The gun is on the surface, and a parental review refuses to be final.65 Yeah, relaxing the non-contact finger.28 Yeah, at least when you're running small on the y-string.19 Yes, both are retained and rejected. Finally, a small extension is made in the direction of the deviated finger, so that it can be combined.4 The gun is intertwined with different fingers.0. Six three views have been examined; there are still 55 The strict surface line of the joint or thumb handle cannot be considered innocent or natural hand-passed, and the complete journey, forearms/cuffs and actual skin consumers are still under examination.

Original embolden anchor and independence 15 Section V 4 The bolts are in the private group. `product-authored-bolt-v4`，PG `7c05ef9d` Full Compiled 0，1123 Freezing PG source-by-source verification, reuse of Native `ced416be`。24 Group Quarreau - original embolism triangle contact cross-unit and world rotation check 0, freeze production release module check 0;full cycle of the actual freezing function for the production of reloading of live ammunition, strict 4 Second-second boundary, break incentives/leases and clearance checks 0. The runtime test was first uncompiled because the command path is reserved to force a path quotation, corrects the empty path and passes without failure as product or game failure. These are offline evidence. New postures and wrist mounted into pairs, V 4 The missing unpublished does not match the old fistshake; the original tracker still determines the action, showing that the hand does not give authority to the institution. The proposed co-opt only the original physical dimensions and other physical dimensions cancel the display alignment.

The complete combination is only source code and compilation, but has not yet been staticly prepared for new operating packages, queues, deployment or run-in, without replacement 201902；Publicv 71 and fixed manual access is unchanged. `authored-bolt-integration-result.json`、`bolt-grasp-surface-review.json` and `authored-bolt-frozen-checks.json`The next step is to check the hands-gun contact of the entire process of authentic animation; after obtaining a new window for Boom, we will be able to get a full-time job.201902 The internal headline of the pre-positioning machine was refused only in its original short section, and the complete reloading, multiplier image and headlight continued to be unattended.

### Arranged wrist position after sampling of original animation process (2026-11-09）

The match.4 Not final: in original `LS_Reload` It's...896 Bytes/50 key, decode/native-relative conversion/ BoltPath/sample/grip function actually frozen, and four vertex skin, check 33 When the opening point is up,0.5625 Progress has found a thumb handle and a bone-weighted mirror area with an outer shell.5 The original “receiver” has been corrected by the actual triangulation coordinates: the area raw is approximately X-rays.6.06..−5.39、Y 20.11..21.59、Z 16.46..18.02, adjacent to the original Spope reference point. Failed diagram with 5 I'll see you at the original triangle. `bolt-grasp-stroke-audit-r2`No other.32 Point or end of point failed to overwrite this.

The offline check misreads the original reference skeleton and reloading frame as a first-time result.31.6 The micrometres are identical, with a difference in actual data 69.49 Micro/0.00276 degrees;R 1 The claim failed. After amending the check method, the frame and actual 25820／frame 757 The only error in stopping the bone back is that 0.000562 mm, rotation matrix error approximately 1.31e−7. The original world foundation was checked with the Bureau ' s two non-common points, which were wrongly identified 1.94e−8m／3.15e−8 m; no constant animated root tracks of non-participating institutions were replaced with current world roots, nor were product coordinates converted or valid.2 The script is left unsolved by a single error of resolution caused by an extra brackets, not as a game or product failure.

Proposed 5 As a result 15 Refusal to qualify for the still hand-gun line and thumb line.6 Local contact point for wrists by `(10,−5,−5.5)cm` Reassigned `(8.5,−4.5,−6)cm`Reformulate the index finger/ thumb, relax the remaining three fingers and expand them in small amounts.18.398 cm-opening path unchanged;33 The same standard points of progress are completely unarmed - the gun lines, including the original failure.0／0.5625／1 Progress 3 view; this is not proof of continuous collisions between sampling points. The line between fingers is 0 There's still one.49 Strict surface line between the same joint and thumb handles, continued to be marked as unaccepted; full forearm/cuffs, adsorbing transition and actual skin consumers have not yet been accepted.

Proposed 6 Private complete combination accessed `product-authored-bolt-balanced-v4`，V 4 Update to `0x4c53424f4c543032`New PG `dbe9a415` Full Build 0、1123 Freezing source verification 0; three de facto freezing inspections -24 Group fingers abdomen/preparatory embolism triangle contact, production release, production change runtime - all 0. Re-enact Native `ced416be`, no re-numbering or adding writing; its actual left 12..26 The bone consumer versus the original.115 Bone Reference/local kit 815 Item 0 Actual V 4 The adapter has a pre-existing Broker/CoreSnapshot to verify the gun's authenticity.15 Festivals, active magazine and passive socket releases 87 Item 0 The latter is still not real-time/Scene or skin evidence.

The new complete configuration is not prepared for running packages, queues, deployments or actual runs; accepted 201902 The mechanical locator candidate and geometry remain unchanged, waiting for the only window that will finally be handed over by Boom. Strictly delivered to lock. `<4s`/ Achieved 4 Seconds normal,100 ms, fresh identity/lease agreement, real edge and single C 0 Keeps records. `authored-bolt-balanced-integration-result.json`、`bolt-stroke-surface-review.json` and `native-bolt-v4-checks.json`. The next offline work is the second holding point and front arm/cuff contact, with actual mechanical embolisms, single-shot rounds, complete loading/locking rewards and multiplier images still missing.

### First round of implementation checkpoints

- Longshot Reload core has achieved actual firing time start-up and strict. 4 The second boundary, lock end, and the same live record retry clock are maintained and then replayed. `/W4 /WX /UNDEBUG` Compile and run 231 Check through.
- Add `weapon_longshot_scope_control.h`and achieve an eye-to-eye range,80 ms. Stable period, exit delay, R 3 The door control is restored to the single-pressure attribution, menu and non-tracking.40 The offline inspection passed and the test target was registered as CMake.
- The current mirror interior planner. 136 Check passed. 407 Item is a offline code check.
- The first round of tests is still expected because of the old assertion that R.3 The initial moment fails, the correction is said to be passed after the actual shooting. The record is kept in `analysis/longshot-development-20261108`。
- Original saved `Weap_SniperRifle.HandleButtonPress` Export appears `R3`、`bIsTargeting`、`SetZoomed` . The text SHA 256 Yes `6af91f648e34f5698bf72e77aa25a435be7a24b9e6949f4a87b30f0c4663ec16`, with undeconstructed instructions, only serves as evidence of positioning for the subsequent check functions and parameters. The scaling switch is required to change the context of the aim to ensure that the original low multiple rate can be used again.3。
- Consumers, real multiplier calls, automatic bullet-replacement blockage, model geometry, GrabPose, mirror images or regression paths, guide and simulator are still in the pipeline. Target remains active.

The user-identified rules are single-shot rounds of left shoulder, a second handshake close to the sorbent with Grip control gun, a GrabPose sticker for all interactive physical components, step-by-step reloading guide, manual recharge, actual shooting until lock is less than 4 Perfect determination of seconds and only eye-pointing performance.3 The original multiplication is to be replaced by an interactive dimension, attitude differentials, eye-point thresholds, a break recovery strategy and mirror path selection. This option proposes implementation parameters and decision points, subject to modelling and controlled validation.

### Product assets and secondary points of access check points

This round is progress, work position verified as `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`Branch `codex/6dof-single`。

- Public asset readers have added Longshot class, grid names and root package identification to support current asset queries with co-gun users.51 A practical reader memory device was passed, containing a wrong package refusal and an existing return of Lancer, Frag etc.
- `weapon_longshot_foregrip.h` Give a stand-alone provisional protected wood surface point from the original model zero base vertebrae 6423, the original coordinates are `(0.7677327, 43.251, 3.37458)`. The original model has been viewed with a side and overlooking view, and the hand-to-hand transformation reconstruction test has passed. This coordinates still need to be calibrated from many angles.
- `weapon_native_lancer_follow.inl` The gun has been accessed and the hands under a valid deputy lease have been released.`weapon_lancer_grasp.inl` Add a new independent Longshot passive branch, using current tracking, arbitration and geometry of the gun, real Grab still awarded by the existing owner.
- Freeze 1051 Part-source independent Playground Release full compilation link successfully, DLL SHA 256 Yes `b8ffad36d145ee728b758833b1d17c4e21000d4a932e62400f08807900ea288f`. The source Hash has been reviewed individually and the evidence is based on `analysis/longshot-development-20261108/product-first-hold/build-result.json`。
- Reader's folder is used for the first time to run a stack overflow with default stacking 32 MiB test inn passed, and the link parameters and failure were kept on dedicated JSON. The packaging for the first successful product was also marked by log decode error and actually compiled and linked back 0;the successor has been clearly processed log coded.

The exact box size of Snap/Grab, full GrabPose, bolting, single-shot and automatic bullet exchange blockage, real multiplier call, target output and step-by-step direction are not yet deployed or start the sniper verification. The running window is held by HB sessions and returned to the editing window after the independent Longshot share editor.

### Live ammunition resources and live clock provider check points

This round continues.

- Longshot is connected to the existing manual `CanReload` Access strategy and public inventory reader. Original save script `WarWeapon.ForceReload` Query first `CanReload`A gun. `ShouldAutoReload` You need to be physically verified after firing, shooting and collecting guns.
- `weapon_longshot_round.inl` The original material that shows the current weapon is re-used. The shell casing, warhead and base fire still need to be independently calibrated. The capture and loading movement continues.
- Two live LODs entered through geometric tests, 3744 Top Point/1248 Triangular and 1248 Top Point/416 Triangular, verify the axis conversion, indexing and three-part material indexes to engine local +Y. Test for first-time error in unit ratio 1♪ Got it already ♪ 10–500 Scope rejected, changed to effective ratio 100 benchmark comparison.
- `weapon_longshot_fire_receipt.inl` In existing authentication `CalcWeaponFire` Call the observation point to record the time of the selection and follow up normal GT confirmation that one-off ammunition is consumed before it is sent back. Directly includes a detached package with no consumption, repeat events, wrong capacity, identity change, failure source and bad clocks. The actual game cover of the route remains to be verified, with the original floating point `TimeSeconds` The precision of this is still limited to operational border observation.
- New Candidates for Independent Directory `analysis/longshot-development-20261108/product-round-fire-receipt` The full Release link was compiled successfully.1054 A source-by-source review of the freeze was approved.256 Yes `0ad0a781c7139d177491dd95ce2e0b46cc81aea4855b6b56e71cd797c28c6a19`, a binary named after Hash is saved. Build and test logs are recorded as this directory `build-result.json`。
- The first gun builder catalogue continued to be used in an iterative format, earlier `b8ffad36` The middle DLL is overwritten and the old result has been marked with its original path to no longer provide the version. `0d72be32` With this one. `0ad0a781` DLL Saved to `analysis/longshot-development-20261108/binaries`。

The current running window has been handed over to the grenade session, and the gun is still being prepared independently. Complete mechanical filling, original reward, target output/multiplier route, guide, GrabPose and controlled simulator acceptance are not complete and remain active.

### Read and Initial Base Validation Preparation for Original Widgets

This round is progress.`weapon_longshot_parts.inl` Validate the original model with an existing famous bone reader, Root 0、Bolt 3、Scope 2 Original bone name and index from saved `core-bone-audit.json` . The live ammunition is now to be read through the original part of this gun, and failure gives a message from available resources that reads the date. The bolt stops/positioning and physical processes are yet to be accessed.

Newly Independent Playground Release Compiled Link, DLL SHA 256 Yes `7ebfff3e46f08d2ab25020c524c2908a5df3b862deaa84550ae7e09ec92990ad`. Freezing the previously compiled source code base, which covers only the connection to this firearm and is maintained in parallel with the working area. `analysis/solo-m1-20261028/longshot-prepared-20261108-212838` Save 1552 Source/drive document and full original version of the binary, Hashi review passed.

First base drive reuses existing controlled start-up, real walk/original X pick-up and shoulder access routes, by practical `CharacterSlot` Select the shoulder position, which will be checked six seconds in a row after one shot, and the live clock echoes and hides the live ammunition resource. Driver AST and the only injection entry check pass without stage or execution. The batch covers only the base line, and complete manual filling is still to be performed and verified separately.

The running window is now a Snub session and has requested subsequent handover. Two original grenade sessions are active without authentication, Native/XR is not reached and the user has been asked to confirm Steam's startup status. This gun does not retry the start-up problem and continues offline development. Based on existing source code, it is still moving forward and remains active.

### Gun-string composite animation reading checkpoint

This round is subject. `weapon_longshot_animation_tracks.h` And corresponding tests, decode the constant of four bones, Float.96 NoW data, while preserving the offset and rotation. Decoding to verify all ranges, complete data coverage, key frame numbers, rotation range, four-digits and non-limited values failed to maintain output values. Synthetic tracks, time sampling, damage data and release checks have been used `/W4 /WX /UNDEBUG` Compile and implement adoption.

`weapon_longshot_animation.inl` Compiled original curve reading interface to access this gun. It requires the current `LS_Reload`, original resource package, AnimSet type and Root/ LensCap/Scop/ Bolt track sequence, and repeat checkes the properties, tables, contents and animation nodes before and after reading. The first round of full compilation failed because dynamic string fingers were passed to an existing word-volume template support function, and the full translation link was corrected. The original curve reading branch in the actual game has not been reached, and stop/positioning is still on line with the hand-drive drive.

Freeze 1058 DLL SHA for source code 256 Yes `840608d5f8522f7537b36ee865ae6c98c8c01ce0952584d229f237a02b16c283`Save the binary on Hashi. `analysis/longshot-development-20261108/product-bolt-track-reader/build-result.json`. Ready `7ebfff3e` The base batch continues to be kept independently.

Received new environmental path from host integrated transmission and read only SHA to check EXE under this path 256, matching the existing support configuration `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`. Follow-up check to follow the project game direct control/simulator script and double-track video, following the owner's clear command Computer Use disabled. See the following record for the actual status of subsequent implementation.

### Progress in the compilation of original ripple animation and hand track mapping

Add `weapon_longshot_clip.inl`, follow the only existing animation ownship journal, pre-check original `PlayWeaponAnim` and `StopAnim` Only the body is shown behind the interface. `LS_Reload` This is a line interface, which the physical hand condition machine has not been called and cannot be counted as a bolt pass.

`weapon_longshot_bolt_path.h` The horizontal and rotational calibration of the ripple tracks of the original Bolt, broken down by eight segments between each of the original key frames, is arc long to map the consecutive hand positions into the original animation time. It refuses to jump, exits from the trajectory, non-closed endpoints and abnormal root bone movements. Complementing the reversion of the thin rotation track with the four-digit reverse mark, two target tests are frozen. 1061 Playground Release build success for source files, DLL SHA 256 Yes `9039a77b7c745750c543ef8c8310372fe1fa10d5ff10a48904648f99918deaed`- The evidence is... `analysis/longshot-development-20261108/product-bolt-clip/build-result.json`. This candidate is not in the status, this base run still uses frozen `7ebfff3e`。

### Base run. 36596 ♪ First failed

Snub expressly gives this session the sole-occupancy window after normal close. This is an accurate stage `longshot-prepared-20261108-212838`，PG `7ebfff3e`、Native `c98568f0`Simulation of XR `16840b27`,Present V 71 No change. Actual game and XR projection entered, but blocked by scene barrier when moving from birth point to ground Longshot. `(-3900,0)` Present. `(-3918,-154)` And then I'm not close. `(-3920,-335)` The ground pick-up, shoulder removal, start-up, automatic loading of the prohibited and resource preparation have not arrived.

Original is not running, Native cleanup validation, exit recovery and XR exit succeeded, launcher returns 0, the game is out. A double-track video is encoded and looks at a full saver overview and end game/simulator original image that can still be visible empty-handed and stop on the edge of the scene. 19.47 Seconds, double-way end covered in 500 ms inside; maximum game capture interval 2.38 seconds, not as frame-by-speech evidence. Failed file is kept in `analysis/solo-m1-20261028/menu-firstperson-36596-220104`Two-way video with PID `startup-20261108-220011/dual-output`. The next step is to fix the normal swing line of this gun detector and re-route it with existing scenes without changing product access or role position.

### Entity bolt driver compilation and window return

This Media Cleanup has been completed 0, delete 2874 - A document,493109926 bytes, errors 0 Keep the latest 36596／49012／16292 Media and media 48 The configuration, simulator setting and head command are rechecked. The original head command does not exist. Verify that the only post-idle window is clearly given to HB, and can then be retrieved again by Queue, without self-starting for short free time.

The independent inspectioner of this gun has been changed to a normal swing around the existing rear route, which enters from outside the barrier when approaching Longshot. `run_longshot_operator.py` The memory selection does not change the public checker, role location, pick-up original call or product access. AST and the only replacement point pass, and the revised route has yet to run.36596 Failure to retain script amendment cannot be accepted as a gun.

Supplementary `weapon_longshot_bolt_stroke.h` And target test. Real new Grab, current lease, effective hands tracking, same binding and 100 ms is a prerequisite for the process. Enter the ripping handle 5 cm After you have the ball, use this real hand position as the starting point for the physical process and solve the original composite trajectory. The proposed endpoint must also see the original Bolt position on the frame below the requested curve by error. 2 mm, rotation error not exceeding 2 . Animation cannot be completed by the end, holding an old lease, leaving a trajectory, jumping and missing track. The millimeters and angles here are the threshold for achieving a neutral bone consistency, not the appearance of a GrabPose acceptance.

`weapon_longshot_bolt.inl` Signed into Playground, which provides the actual body Root/ Bolt matrix reading, current model and rip point mapping between closed bones, current rig controller conversion, original curve content checking and existing unnotified ook call. The handle point remains the original LOD model 0 Top Point 356 The initial selection, the actual alignment and the full angle look have not yet been validated. The general reloading has not been activated as this drive is still being used, the delivery, loading, reward and step-by-step guidance are still to be linked and cannot be compiled for actual bolting.

Shares in this round with additional modifications `observe_kind` Yes, Longshot has allowed the type but still accepts it. `AR_Reload/HB_Reload` Add a gun to the question. `LS_Reload` The remaining guns remain the same. The amendment is coordinated with HB for independent editing. 1064 Source file with complete compilation links with identical GERS build options, DLL SHA 256 Yes `900d0049f075b0f8780c559041e7f29b87fbdf886ba034b69e8f8b4f5313f75e`Save it on Hashi. The evidence is that `analysis/longshot-development-20261108/product-physical-bolt/build-result.json`, no stage/unrun. The test failed initially because of the spilling of large stacks and was then distributed by default; the local name mask warning for the first compilation of the product has been amended and the failure log is maintained. Target continues active.

### Hold and display interface for single-shot live ammunition

This round is progress, continuing the same checkout/branch without starting a new game or changing the open candidate. `weapon_longshot_round_hold.inl`, follow the available resources owner and actual `ammo` The lease does not create a second inventory. Only if the current frame is real, new ammunition, Grab, original left shoulder supply area hit, object serial numbers are consistent with input token, current binding and resource ready are established, will one live bullet be bound. Read later the rig handshake matching the current source, and install transform drive items in fixed hands; no sorbent visual hand to prove the actual loading process.

Read actual read LoD 0 Toppoint data, bullet length 15 Cm, radius about 9.37 mm, original base fire ends in +Z=0+Z =15 cm, long axis after resource conversion is a local +Y model. So the bullet in demand can be specified as a live-up capsule 15 cm. Bullet axis, radius 3 cm to get the capsule, keep radius. 5 cm. This is the area design size; this round achieves a holding display interface after the bomb lease, and fall-back re-poaching capsules are not yet connected.

The original hand-held marking was marked with the original Marcus left hand-hand autopsy, and the bullet line turned to the wrists and held at the bottom of the shell. It remains an initial pose that requires physical visual examination, not a complete natural GrabPose pass. The limits on copper shells/heads/floor fire are also maintained for the current re-use of the original gun material.

The production source target test with a holding function is passed, and the boundary is injected into the resource, Rig and current source. The test covers original shoulder area, non-new acquisition, wrong lease type/life cycle/token, missing track, current Rig mismatch, hand-to-hand move, duplicate frame, drop failed retention object, and normal recovery after the source fails to remove displays. Showing the source failure will remove old visible objects while retaining the identity of the item that the inventory owner still needs to process; visible normal Hide is removed only after success.

Freeze 1066 Full compilation link for Part Source 256 Yes `c1491b3739f74373a331dc5599c194184b950cd2bc412b83ed7e25bcad27e838`We'll save it for Hashi. `analysis/longshot-development-20261108/product-round-hold/build-result.json`. No stage/unrun yet. The switchboard, single-shot inventory call, loading geometry, perfect reward, step-by-step guidance and complete GrabPose are still waiting to be connected and cannot be tested for closure with this interface.

### The switchboard is operational, not yet running.

This round is subject. `weapon_longshot_runtime.inl`, the current manual program/committee echoes and normal GT recoiltick, fire block, switch notification is distributed. Longshot's candidate is no longer in Lancer widget proposal. Coordinated with HB, shares insert points, only add a gun condition and call to build a previous frozen source plus an overlay without mixing other sessions.

The current source-code phase chain secures the original version of the lock for real capture and immediately ceases the unlocked body clip to record the same gun's Chambers and prevent firing; suspends/receipts from the original recovery and stock reconciliation, without automatic time overrun or reloading.

Perfect time to receive a game clock with actual fire back, and the microseconds are tight. `<4000000`. The original interface for the shared ledger contains a cut-off end, so this gun is passed `4000000-1`, do not modify the boundaries of other guns. The initial empty gun load that does not actually fire back to fill a zero window can only be done normally. The new target test is verified by one shot of genuine shared ledgers 3.999999 The second is perfect.4 sec/4.000001 sec/8 Seconds are normal and one-shot stockpiles constant, executed. First clip wrongly added one to the deliberate-zero 's original, not exceeding before.revision, claiming failure; changed to provide real observation syntax 1 The production books were not relaxed.

The loading test is taken from the source code. `weapon_longshot_insertion.inl`, which is included in the original form for operational time and target packaging. 8 Cm deep,6 cm radius,30 Range, then to the end of the seat  ⁇6 mm, horizontal 1.5cm、10 range. The input interval does not exceed 100 ms, relative speed does not exceed 3 m/s. Target tests cover the first approach afterseat, directly generate a rejection at the end of the seat, angle/horizontal boundaries, cross-border, speed jump and spacing, and execute passage. The binder is only injected into the original bone and live bullet posture to read borders without any game process or inventory.

`weapon_longshot_guide.inl` A path has been drawn through the original Actor frame, sending a spherical hint of the next component for the capture, left shoulder, loading and locking phases; actual visibility, step-by-step text/sound or final guidance performance has not yet been verified. The chamber site is currently the original model `(0,26,10.5)` The initial surface position of the project, and then the direction of the loading forward, will also require physical modelling and operational image confirmation that it cannot be calibrated in geometrics at the end.

Freeze 1071 Release DLL is the latest source code `fc4ecc33906204c1d8f84e89121da91a575b8580d504d8af201882f6d28751b5`. The evidence is in the `analysis/longshot-development-20261108/product-manual-runtime/build-result.json`. The initial construction of the local name mask and the missing namespacelias has been modified, and the failure log is maintained. The current candidate has not yet stage/run, nor has the complete status machine itself been injected into the boundary; it cannot be used to reload a local loading/time test as a whole game.

Still clear: actual shell case ejection visualization, component neutrality Snap and integrity GrabPose, chamber adsorption display, stage text/sound, close eyeglass magnification and R 3 Product route, complete normal/interrupted/cleaning validation. The independent re-lock path that is interrupted after insertion is now completed as usual and needs to continue refining the original perfect time and breakover of the incentive. The running window continues with the only switchup for other sessions, and this round does not start a new game.

### Basic links 21724 Practically narrow through

HB 56220 The only window that will be identified after normal closing, double-circuit review and retentation technical is the base section of the previous queue, which does not fold up. Still, stage is still the one that has been selected for this session. `longshot-prepared-20261108-212838`／PG `7ebfff3e`、Native `c98568f0`、XR `16840b27`. Physical check at the preparatory stage 1552 Freezing source code, six construction products,12 An active HUS/AST driver, and EXE HASH for the owner-specified copy of the game. v Open 71 Keep the control unchanged, using only the existing game direct/simulator script, without Computer Use.

Actual Game 21724／creation `134386234086007894` Finish the original floor by a modified regular rocker route Longshot pick-up, CharacterSlot 2 Actual left shoulder to take a single shot with the same source. The only actual shot 1 It's... `trace_valid/write_source_consistent/trace_source_consistent` Both. 1, Trace Start is the same as current muzzle origin. Actual firing back to callback game clock `39033260us`capacity 1／AmmoUsedCount 1 And then... 6 seconds,24 All actual reading samples maintained AmmoUsedCount 1, no automatic recharge. One live bullet hides resources in frame 612 Reached ready, inventory written 0. Pause/ B returns through. This is proof that the limited waiting period after this launch does not cover dry triggers, retake guns or complete manual change of bullets.

Game and XR double-track recording is coded and you can see for yourself two pages of a complete saver overview, as well as key originals of shoulder grab, hair start, empty waiting. 957 Frame, Covenant 55.234 sec, simulator 1180 Frame, Covenant 55.217 Seconds, double-crossing at the end. 500 ms inside. Maximum game capture interval 1.833 sec, not as proof of the rendering frame/FPS. The image is visible in actual holding and paused return but natural master/submandate/ parts GrabPose continues to be unaccepted with complete alignment and cannot be closed because this automatic inspection passed.

Native clean-up, portal restoration, virtual input recovery, XR exit 0、launcher exit 0 Confirms with game exit. Configure and simulator settings are restored to the original Hashi check, head command is returned to existing status, after running 12 The drive-up Hash hasn't changed. The evidence is... `analysis/longshot-development-20261108/basic-21724-review.json`;the video is on `analysis/solo-m1-20261028/menu-firstperson-21724-startup-20261108-225640/dual-output/paired-output.mp4`. The final state of this media clean-up and only window is recorded in the JSON, without a process being empty or starting the next section.

This request 20600 _Other Organiser 0, actually deleted 3704 Documents/660032393 bytes, error 0 Keep the latest 21724／56220／54740 Media and media 48 Hour limit. Check the idle and have identified the only window to be delivered, Lancer 212510, and forward follow-up of the Frag effect to Snub original box HB C 0 Visual order.`basic-prepared.json` Synchronized real status/recorded state and two results, original 36596 Failed to retain. The gun continues to complete the general switch offline, not re-entries the running window, target active.

### Implementation of the full production status machine at the boundary and three restoration amendments

This round is progress, no new game has been started. `weapon_longshot_runtime_test.cpp`, original contains production `weapon_longshot_runtime.inl` , actually run the shared view inventory with the real Frame/Decision/Lase type. The injection is a Native bone and bolt endpoint echo, original engine call, resources, inventory read/setter, GT border with source validity. It therefore proves state conversion, accountkeeping and restoration control, does not prove actual animation, hand-routing, incentive engine effects or images.

Full execution covered normal/perfect round, just as it happened 4 The initial normal load of seconds, without actual shot, drop retake after delivery, recapturing after insertion, limited zero-load rejection, unlimited-loading zero reserve and current production loading functions. The live-fired bullet insert always produces only one inventory written in, the loaded re-link is not a single button, and only once the confirmed perfect completion has been completed calls for the reward boundary.

The three failed assertions were first actually reproduced and then the targeted amendments were made, all of which were retained in the original RED text.

- Closed, recorded and original exit failed temporarily, with the original code entering recovering but not yet issued. Perfect receipt prevents resumption. Retention of the same completing, drop/exit/clip etc., is now maintained without reward or re-insertion until exit has been successful.
- The stop request is received after the physical lock has been closed, and the sequence is restored, and it will be blocked until completion. Now finish the existing closing/reward that has been submitted to the lock before stopping new action. No new input or action permissions are added.
- Insert a text that is explicitly rejected and the actual inventory remains pre-reserved. Check the original counts and have the book book cashledge reject, and then clear this unloaded reservation debt. Unsolved, semi-written or inventory inconsistencies remain unchanged and remain uncommitted.

The three amendments and all current target scenarios have been implemented. 1072 Source code, Playground Release full compiled link, DLL SHA 256 Yes `e104456b681643b3f091f8a5e2f2d98b27eb32411051d2188f11d9a6b38bcc34`Save it on Hashi. The evidence is that `analysis/longshot-development-20261108/product-runtime-recovery/build-result.json`;RED files are of the same level `runtime-exit-retry-red.log`、`runtime-stop-completion-red.log`、`runtime-rejected-insert-debt-red.log`。

This candidate is not stage/unrun, and the full reloading game has yet to be completed. The break retention of original perfect clocks/rewards, actual original curves and action validation, chamber calibration, all GrabPose/Snap, shell case vision, complete guide and near-eye sight mirror are still pending. 21724 The old base segment or the current injection end point passes when the bullet is replaced.

### Original award for eligibility and cut-off original clock connection

This round is progress, continue to specify checkout/branch, no new game. Check for reality `lancerreload_state::perfect_reward` The interface must be available before it is called. `capture_reward_eligibility` Qualification confirmation; the previous package was only a simulation of the award, which did not reflect this prefix. After filling in the real interface, actually testing RED is now read and retained before normal loading starts, and currently re-entering. The previously overbroad incentive border simulation cannot be considered as an actual engine reward.

The interruption after loading is now retained as the same transfer, observer review, opening inventory before insertion, original shot time and incentive eligibility. Re-extension already exists `ObservedInventory::rebind_hold` and `ReloadTransaction::rebind_suspended/resume`: The same world/player/gun/game must still match the identity of Native, input/space, and the actual inventory must be identical to the one submitted, and a new real bolt is needed. It does not reset ForceReload or submit second rounds of inventory, nor brings back old handheld leases or gestures. Original eligibility confirmation goes through both the same example and holds over to the original reward interface.

The true test before the weapon is loaded has been added, and the original code is lost because of a change in the hold identity. Only the same example that matches fully preserves this unchangeable shooting time fact, and all actual actions still need to be from current sources/leases/geometry. The old clock or book is not renewed when the source does not match.

Tests performed in the form of a production status machine have covered pre-loading hard, post-loading with hold/over, repeat interruption, original clock below `<4s` Perfect and `4s` Normal, single inventory submission/one incentive and entry or stock change refusal to restore. Original RED reserved in `runtime-reward-eligibility-red.log`、`runtime-interrupted-clock-red.log` and `runtime-preinsert-hold-clock-red.log`These are evidence of control streams injected into the Native/engine boundary, and no actual receipt of guns/images/rewards has been proved.

Freezing of independence 1072 Full Release Compiled Link for Part Source, latest DLL SHA 256 Yes `52d05fbe6a1dc380c4e5de718771531c00042204bbb29b02923285e57a03925e`, save on Hashi; evidence is `analysis/longshot-development-20261108/product-interrupted-cycle/build-result.json`This candidate is not stage/ unrun. This section replaces the previously mentioned source limit of "unusual after interruption" for independent bolting, and the game effects remain to be verified; the current retake of the gun uses a new complete physical embolism/locking process without claiming that part of the process and appearance have been restored. The rest of the mirror magnify/R 3, the complete GrabPose/Snap, shell casings and guidance, chamber calibration and whole-gun game acceptance remain open.

### Actual eyeglass alignment with current frame close-eye observation

Original neutral Lod 0 The glass in the back of the model is in the original model Y =20.38986 - We got it. 18 Queue vertex, the center of circle is `(-2.581341104,20.38986,18.036765533)`Radius `1.853067493` Original unit, the default 100 Unit/mec 1.85 cm. Maximum round-up is about `3.5e-6` original unit. This point replaces the temporary selection that was taken earlier only from one of the vertex on the lens surface. Model Obj SHA 256 Yes `bc019162…95c7`full coordinates/face/toppoint recorded in `analysis/longshot-development-20261108/ocular-disk-fit.json`. It proves geometrical plane, not pupil, eye range or actual optical markers.

Add `weapon_longshot_eye_contact.h` Actual `weapon_longshot_eyebox.inl`. Map the centre to world rice coordinates along the current gun matrix, convert two physical eye positions with the same ricket 's rich base/reference. The UE front/ right/ top/ back axis is separated from the XR right/ upper/ back axis and cannot send the UE's +X front to the XR-Z front assistant directly. The close-eye threshold is set 80 ms stop/ keep the band to follow demand, menu, switch, miss or source not matching unear-eye. Observe back check binding on current Framekey, interactivity, reference/profile generation 100 ms statute of limitations.

The POSIS is used to select the wide range of non-released phases by using a fixed mountain, with the normal bearing of the gun away from the eye. The full range still requires real mesh/rig frames. The free-freezing is done independently after having given priority fire/follow/reload jobs. 1076 Releaase Build Success, DLL `717567008cbe39e2c5293531e4fd72d38d039b3ed47ce1c297baf1077886d623` Keep it on Hash. Evidence. `product-eyebox/build-result.json`. The candidate has not actually run, has not changed its profile/zoom or consumed the original version R 3, cannot be counted as a visible sight.

### Rendering constraints of original multiplication with R 3 Consumption readiness

Original Saved `SetZoomed` The group will change bZoomed, hide the gun and notify the server;`GetTargetingFOV` Return at high rate ZoomFOV, R 3 Handler switch and process sound/ExtraRot only when PC.bIsTarging. Original export undefunct token still requires real interfaces/effect validation. Existing Native `stereo_matrices` It'll cover the projection with a head-opening FOV. 0/5/8/9 coefficient, so only the original SetZoomed cannot prove that XR images are actually magnified.

The independent scene rendering in the preferred lens still requires a completer of controlled capture/ texture/scope mask. There is no current pure optical plan with GPU channels; original zoom does not automatically generate images for entity mirrors. A near-eye image retreat allowed by the owner also requires final view selection and maintains current co-source orientation and normal exit, which cannot be imposted as an image effect. This is a source assessment, and no one has been declared operational.

To avoid close eyes, R.3 The project is being repeated by the module and original version.`vr_gamepad_input.h` Add visible `consume_buttons(mask)`Clears the processed short-press pulse and suppresses the still held original virtual key to real release. Recording the physical level before converting the combination, keeping double-stumping Menu and avoiding late replaying R when exiting the mix 3. The original route is followed by a new pulse/hold by a combination/disable and then the target test. The original behaviour remains unconnected when no call is made, but is still not connected to Native export or gun R 3 owner, does not change the original controller, posture, time stamp or other gun act.

The waiting rules have been communicated to the owner in an integrated manner: a session is held when the shared window is lined up, there is a waiting circle and continues to work independently, without waiting for or marking the end of normal occupation.

### Native R 3 The quarantine bridge has been compiled and the failure automatically released.

Add `GearsCaptureLongshotScopeInputV1`, requests to carry the original full source only for current GT with Broker, original defined frame, same identity/content and 100 ms accept after the time limit. The original version of XInput is still available as the only Route; this bridge consumes R 3, do not generate keys, do not post gestures and do not provide hand-action privileges. Stop the engine to clear the isolation. The new source code is plugged into the working tree `capture.cpp` and `vr_gamepad_live.inl` , without replacing an open or current running package.

Checks the exit conditions and finds that only a single equilibrium diarm will leave its source isolated. The modified version is original `controllers.sync_ns` Limit maximum 100 ms, key sampling cannot be renewed; expired or clock is automatically lifted and consumption holds are still real until the next R 3. Actual production `.inl` Border test extension shape with actual Route, Broker rejection, stop/disable, queued short, held free, real uncut,100 ms endpoint, timeout and clock backward to execute pass. It is not a real game hook or multiple switcher validation.

Authenticated Native `c98568f0` Freezing source and all original build switches as baselines, covering only the bridge and Route changes, Release complete compilation link successfully, DLL SHA 256 `ae3bf39e1cefb85413fb54816c3224e53b8395035faee96e73f5af35f1971d83`, save by Hash. The export table confirms the existence of a new entry. The independent test source completed by CMake is used only to generate projects; `GearsCapture.vcxproj` , and then enter the C++ compilation only to freeze `capture.cpp`, the original production compilation and MinHook source have other Hash records. The initial missing archive built-in configure failed and the first untimed DLL was retained as final adoption. `analysis/longshot-development-20261108/native-scope-input/build-result.json` and `exports-r2.txt`。

The current Playground close-eye observation has not yet reached the bridge, nor is it connected to the original version of the towering/zoom owner or eventually visible zoom. This section updates the previously mentioned "Native Export" source state without changing the conclusion that the sightings and whole guns are still incomplete. The DLL does not stagger, no game started. `git diff --check` Shared `weapon_lancer_grasp.inl` The end-of-the-line report was blocked and the document was not rewrited without permission; the changes made by this bridge itself were verified separately.

### Keep the side-knit and release connected (2026-11-09）

Public `SupportBox` Only targeted box contacts centred on current points of appreciation are calculated, independent of engine objects, input release, resources and leases.`GripGameState` Adds the box field that is closed by default; only enablers use box areas, and other guns continue spherical branches. This structure is Playground's internal observation/readiness, which is not within Native's Fram/Proposal/Desion or export parameters; Native does not depend on this structure, and no hook or camera/handwriter has been added for this size change.

This gun is independent foregrip configuration with the original model +Y as a long-shot axis to capture the box wide and high 16×12×12 Cm, keep the box as 20×16×16 cm, the center follows the installation transformation of the protected wood surface to the hand/controler's handle. The box axis changes with the current gun body rotated against the master hand and its dimensions are in the form of interactive space rice, not in the form of a model original unit or test retrofit resolution. The UE/XR vector base variant contains a manual change, so the axis is accepted as being positively reflected; it is not mistook for a four-digit attitude number.

By Table 2.1 The established demand, when only one gun is activated, that the real co-handheld exceeds the maintenance box and that holding Grip back does not retake the lease, requires real release and press. When you leave your hand in the box, you save the selected socket identity, revalidate the current original hand contact with the current box and release it to the hands instead of using the old gestures. The actual Grip authority continues to be created by the original shared arbitration.

Longshot branch is now calling weapons has nothing to do with it. `gears_weapon_support_socket::mode`; HB module is a thin-repeated version of the original interface from which it has sessions, and this gun no longer relies on HB proprietary mode. The Longshot geometry is not quoted in the public domain, Longshot sizes and handheld are left in the gun configuration. Shared gasp files cover only the branch of the gun and the public include, without bringing other Lancer editors with them into isolation construction.

Geometric tests cover the acquisition/maintenance of boundaries, rotations, invalid scales and positive intersections on three axes. 69 Adopted, with original 65 Example and add box agular points, current rotation, maintaining area across borders, real release before scratching and selected socket changes; complete manufacture of this gun is also passed. Initial geometric tests wrongly require UE ⁇ XR box base-lines to be +1, the new default parameter for the first product construction conflicted with the old dichotomy statement; the original log is kept. Pure calculation does not match local host tests to a real hand surface/fifth finger.

Freeze 1078 Source Release `37d936b5…609f` Compiled link passes; only one read-only status conversion/institution geometry checkback is added to the same source, rebuilding DLL `601511a52b624204770699806661d692a03ad929289dfcdd519bded81c679259`The evidence is... `product-support-handoff` and `product-open-observation`. These returns do not add game operations or endpoint privileges. Ready `longshot-prepared-20261109-005036-manual-open`（1577 Group Source), reuse validated Native `c98568f0`／XR `16840b27`, status/run not yet.

The next physical narrow section only validates the real opening, single-shot removal and release of the left shoulder, driving only the hand gesture and button of the existing simulator. Original inventory type/identity double reading follows public reading, geometric log is planned for input only, and completion must be supported by a separate production status machine and original bone endpoint. Full loading/locking/reward, visible scope, R-ray 3 The product takes over and all components of natural GrabPose continue to be unaccepted. The window has been run in the order of current consolidated applications, and continues to await a clear and unique handover.

### Final projection of the close-eye full screen retreat has been compiled (still default not enabled)

The original status change does not automatically bypass the current eyeFOV overlay, so add independent pure calculation `weapon_longshot_scope_projection.h`, accept the target level of the original getter FOV and the certified axis direction for this narrow-eye projection. Keep two original view, point spacing, depth and other projection factors adjusted only 0/5/8/9. This is the exit route for close eyes allowed by the owner, not the PiP in the lens, and does not change ballistics, body bases, Rig or HMD sources.

Add `GearsCaptureLongshotScopeViewV1` Fit to independent Native modules. Access follows the current Broker 's original defined Frame; original objects and attributes are used through public `weapon_hand_scene_binding` The model +Y, which is a prototype axis, needs to be reviewed in the actual image and authoritative ballistic trajectory. It cannot be single-pointed. Failures keep the clock as frame normal, without newer displaying of old Frame. The menu shows an independent current screen veto; body and body selection is completed first, and existing shades and their sources continue to work.

Production pure and actual Native `.inl` The injection test is passed: the original FOV 2-set, the center of the gun axis, maintenance of the eye view/deep, rejection of the whole group of single-eye failures, change in the original object chain, current matrix away from the eye, rejection of sources, invalid metadata, world/reference changes and visible closure. Broker injects the engine object metadata/reading boundary into a bind, which does not equal to real object metadata or rendering pass. The initial target class address of the package has not been aligned and the overall name has been covered, and product identification checks have not been modified to remove the folder.

Freezing of independence 1574 Source, previously validated, Native source plus a local cover of the gun. First DLL `56c5ed7c…12f3` Saves its exact capture source; release relink successful after veto is added, DLL `cf72bc5fdd26987c910580101c410e27463b2db5b32d6b70cd53d4e221175fee` Press Hashi to keep the full evidence in. `native-scope-view/build-result-r2.json`. The source insert point has been clearly confirmed with the bunker, and no current Native changes are copied.

Playground originals still pending calls from the campaign/zoom owner and this export, so the current functionality is not enabled by default and no actual mirror amplification or R 3 Multiply to change evidence. Ben Native does not stape/run, the queued Manual-open still uses old Native `c98568f0`. Uncodified confirmation of current running order as Cover r 7⁇ HB original box Snub ⁇  Boltok ⁇ Longshot; normal waiting does not constitute completion or blocked.

### Finish the reload by original serial number, so as not to prejudice subsequent leases

The follow-up line has been divided into Snub ⁇  Boltok ⁇ Frag 011230(old)004429♪ The same point of failure ♪005036, the only window that is turned over by all Frag. The gun is not set/started earlier; the previous queue text is recorded at that time and does not cover this definite position.

Public original status module provided `recover_serial(serial)` The interface returns the "less-and-out" serial number to the "lease-free" and does not exit the new line that was later acquired; it does not prove that the old parts are back on track. The gun continues to reconcile its clip, live ammunition resources, inventory and reward.

Adding a pre-production kit to the clip that could temporarily fail to restore borders, actually implement old ones `601511a5` Freeze runtime: original 100 crip still pending, please 777 It's all clear.777, trigger the protection claim, RED saves in `runtime-scoped-cleanup-red.log`. Current source code restores scene through the same interruption, and is also completed by physically closed but crip restores pending; both are maintained new 777, the actual exit count of the old case always is 1. The second completed scene is currently passed by the current source code and does not declare separately that its old source code RED has been reproduced.

The complete machine is still in its form.1078 Source: Playground Release DLL `7319548e1e7075ffe38b4d31ea394f1c075ed91a2ed8584e6e5e34146354de4e` Compile links are enabled. Public status source covers only the exact `retains_serial`／`recover_serial` Two interfaces, without bringing in other HUD or weapon modifications under development. `product-scoped-cleanup/build-result.json`. The candidate is not stage/ running; currently queued 005036 Keep the original frozen version and do not replace its game results with a new source test.

This gun is in public. `pending_work()`, the module itself will determine the non-idle phase for a clustering life cycle and cannot be used as a new action for other guns. The original line exit is tested but crip still pending must report true, false after the end of the previous period, while retaining 777. Hide the reserve resource and pure logic device using its own resource Drag/Show to stop responsibility. The interface and return to join independent DLL `b4dd64e71393729d790872472995a5e55a735fae916747b512108543a6eb8a26` Compiled links pass. Evidence. `product-lifecycle-pending/build-result.json`, no stage/run. Shared outstanding combinations are still independently developed by their own modules and cannot be described as having been completed as having been organized throughout the life cycle.

Actual bolt drive pre-screening to see outer layers `run_longshot_check.py` The past will `checks` Clear it out. No new one. `--longshot-open` To the inner layer, an imperator will only repeat old base segments if it is directly run. The gun packaging unit has been repaired; it is actually implemented to convert, intercept and activate exec, check the Popen parameters that generate AST, confirm that the default still runs the base, visible parameters enter the bolt segment, and no game starts. The original checker only checked for a top-coding constant, leaving out the parameters in the embedded function, so its assertion failed; instead, it checked the actual generator and AST, and then it was changed to check the actual generation source and the AST, which were not started.`manual-open-wrapper-preflight.json` Two cases were adopted.

Modify drive to the same candidate refreeze as `longshot-prepared-20261109-012911-manual-open`（1577) still is PG 601511a 5／Nativec 985／XR 16840, product binary unchanged and certification scope increased.012911 Alternative 005036 As the only recipient after Frag, Frag has clearly confirmed. This paragraph is not yet stage/scramble and does not pass as a physical embolden with the correction of parameters or pre-checking.

### 4632 The actual opening narrow zone failed and the normal closing was completed.

Frag 2044 The only window is clear when all are properly closed.1577 Source:6 products,18 Exec SHA after an active driver and owner-assigned game, precise temporary saving 012911, use PG 601511a 5／Nativec 985／XR 16840、0.3 Play games with direct control paths in place 4632／creation 134386334627412787 The originals are a ground pick-up, real shoulder grab, authority one-off,6 sec 24 The sample is empty and the hidden resource is stored in one place.1 Game clocks are 37851505us。

The new manual segment has been actually implemented to "slid the bolt and press the Grip" and then wait `longshot_reload_geometry` Timeout, impraator exit 1. No actual bolting or left shoulder taking through. launcher exit 0 The current log does not distinguish between denial of access, non-availability of sources, shared occupation and “recovered idle when the frame is acquired”; it cannot be directly stated that no end-state change log has been called without its original call.

Complete 66.85 Two-way overview of three pages and 48.52／50.98／55.97 Three original maps of the second are already in hand, and the left hand is indeed close to the gun body, without accepting mechanical travel or natural gestures.999 Frames, Simulators 1429 Frame, double-track tail over.500 ms;most capture interval for games 1.867 Seconds, still not actually rendering FPS evidence. No new sighting mirrors or all GrabPose passes.

Native clean-up, portal restoration, virtual input recovery, hand_stop 5／drained、XR 0、launcher 0, the game exit is confirmed. Sets the original SHA, configures the original content and head commands before they do not exist;18 The original configuration check automatically merges the line with read_text, and compares with the backup string that maintains CRLF, which leads to false failure, instead of maintaining the original byte decoder after changing the line, re-writing the check without rewriteing the configuration to move the checker.31013 terminal 0，retention 77892 terminal 0, delete 1808 Documents/326585823 Bytes, errors empty, last three times/48 The only window that can be given to Cover reader is later specified.

The evidence is... `manual-open-4632-review.json`、`manual-open-4632-contact-selection.json`The video is here. `analysis/solo-m1-20261028/menu-firstperson-4632-startup-20261109-014413/dual-output/paired-output.mp4`. This failure is complete and maintained, target-active. The subsequent only real input/proposal/decision, entry threshold, shared occupancy and missing Frame will be read back only, locating the undivided branch above; no change in input, source time, range, access conditions or 100 ms limits and does not rerun the same candidate for which no new information is available.

### The gun model close-eye coordinates returned with scope view V 2(offline, non-deployment)

The current authorized gun range will map the original generic component to a frame-held gun. `plan.target`. The immediate view will determine the location of the previous direct use of the general assembly matrix, which may not be consistent with the weapon-held display plan. The actual production of the eyebox returns to the device, keeping the same gun-carrying plan unchanged from both eyes and returning only to a remote location: old realization error exits the near eye, RED exits -1073740791 Saved. The modified calculation of the location of the lens from a frame, weapon/component/assembly/hold_general is calculated for valid gun-holding plans; original components are still used to authenticate real objects, rigids and actual scaling. Expiry plans, invalid schemes, holding identity mismatches remain rejected.

Native Show Package Upgrade to `GearsCaptureLongshotScopeViewV2`, the existing gun-held model posture and realistic zooming that carries this Frame. Native preserves the current Broker access, actual object chain, original matrix double reading, scaling and eye distance check, and from there the armed model calculates the eyescope and axes. It no longer uses the restored normal component position as its current gun hold location or adds or renews the tracking time. Final projection selects a location, head mask effective inspection and flat-menu veto maintenance; does not write C 0, Rig, body or view matrix. Model axes still need to be physically targeted for image verification.

The injection test for the production of eyebox and Native Acapter is passed, covering normal components returning to immovable eyescopes, real help-model rejection after removal, source/reference/holder identity change, expiry, menu/ball exchange, eyes and disarm. Full isolation construction is done through Playground `4089bd5ca930a67efbb57c560df9bb649a889e16388901e93e42c79d04c5a1c7`，Native `664cf7e7d4b85a328208aef77233881e896124b2dfbae9053b40b615538688cb`；V 2 Export verified, source archive and unmodifiable DLL check SHA. Record is as `held-scope-result.json`。

It's not... 4632 The causal conclusion of the failure of the bolt entrance is not replaying.3 The originals are not yet connected to the PG release, and the current version of the video is still available.`scope_ready=false` Keep. Line up. `021930 / PG3cabf19e + Nativec98568f0` Continues to locate only the original failed and does not mix with this coordinates/opener.

### Original open mirror to adapt layer (offline, no life cycle caller available)

Add Independent `weapon_longshot_scope_original.inl`: Only original version is called through the existing GT Dispatcher `WarPC.SetTargetingMode`、`Weap_SniperRifle.SetZoomed`、`SetOwnerNoSee` and `GetTargetingFOV`. The original declaration class/package, functional identity, script Hash, the executive entrance, parameter name/type/flags/scope/no overlap and closed search; no guess of the layout of the Bool parameters, no writing frequency, no use of other guns. Review the original Frame relationship with Actor before execution; clean-up interface only allows open mirrors or multiples from old sources. The caller must register recovery obligations before any seetter, and does not own input or camera.

Full isolation Playground compiled and approved by SHA `362b98a00a01e3e0a8e8cef0331057fb4c2c4ff361449a493749c584ba93a470`. The injection boundary test for the production leg is passed: three types of bool call, parameter/mask/cross-border/circle/external declaration/script change rejection, invalid current source, old lifetime only OFF, real getter return value check. The test does not prove retail metta accepted or did not execute original game calls.

Original `SetZoomed` It's gonna affect both the body, the body and the head. `SetOwnerNoSee`;The method changed `bIgnoreControllersWhenNotRendered`. The complete owner must measure the actual state of the original, clearly visible attribution and accurate recovery before access, avoiding a low-multiplier gun that covers the full screen backsliding or changes the body/head display after exit. Original sighting mode, real multiple switch, R 3 The quarantine, actual FOV release and complete exit path are still not connected to the declared sniper mirror completion.

### Source process (undeployed/unrealized) for full screen exit is connected

This section updates the source state of the previous section, "No life-cycle callers available". `scope_state` Read-only fit and stand alone `scope_owner`: Measurement controller/pawn control, target key, bZoomed, original DefaultFOV, gun body/body/two recapable head components and bowwnerNosee, body bIgnoreControllers WhenNotRendered; complete double-reading inconsistent or unknown of any required field. Release 1 is only fully neutral and has no other L 2 Downer enters; different visible attributions are rejected before any matter is resolved, leaving open the possibility of a reduction in arbitrary non-neutral status.

After the immediate eye stabilizes, register to close the debt and isolate the original version of R by currently running the Frame 3, call original versions of the datating and visuality; low multipliers also hide the body so that physical lenses do not block full screen. The original gets the returned FOV to Native V 2 displaying packages;R 3 Only a new button confirmed by the close-eye controller is used to recreate visibility at low/high multiple rates after calling the original SetZoomed switch and retake real FOV. The same Frame re-entry cannot be switched again, unsure that the call failure will only be restored without replaying the key.

Eye contact, menu/alteration, loss of source, change of identity, cutting of a gun and exit at stop: old action lifetime allows only OFF; debt is closed until physical contact/key/zoom/ignore/identity and the same component has been restored. Native rejects the old display package when the current source expires; R 3 The quarantine still keeps the original. 100 ms timeout and holding to real release rules. Stop service, Frame unaviilable, original enabling/drain conditions and normal stop request are partially accessed. Manual embolism only starts with real Grant before exiting the gun scope without restarting the original reload, without changing the count or inventory.

L coordinated with Lancer 2 Inserts only the current search for a valid stocking debt matching Longshot hold; no private stage of other guns is considered to be a block.2 Same-source authoritative firing path maintained, with no full fire exit mode for Frag. Source document continues to function: eyebox is responsible for near-eye and key edges, scope_original is responsible for original typed call, scope_state is responsible for reading only measurements, scope_owner is responsible for the gun debt, Native is only responsible for consumption of original R 3/ Select Projection. No new input, C 0 The camera, the skeleton or the rig writer.

Playground `b071234705d1c65b86f854944bf9224e9c85f5c4acdd9e6ebd16fa2bc908e66e` compiled;with scope native `664cf7e7…688cb` Neither stage/play is running. Two earlier compilations failed, respectively, for Windows near macro and /WX global module, with original archive preserved and changed name. Original call, status read, scope journal, production eyebox and production reload passed; over-covered already visible/L 2 Attribution denied, Native history failed, once R 3/Synthesis does not reset, partial setter failure, delayed closure of debts, source/lifetime lapse, hold, stop/yield, and scope does not start manual reloading until they have left. Real environmental reflection/hook/image is not covered by these tests.

Records:`scope-composition-result.json`, the test EXE has been pressed to save unmovable copies. The actual original interface is still not complete in this version, the close-eye low/high-multiplier image and acoustics, body/head recovery, key conflict and exit comfort are still subject to control. The physical lens PiPiP is not yet available. 4632 The reason for the failure of the bolting entrance is not proven;021930 The original diagnostic candidate remains unchanged and continues to be delivered on the sole running window.

### 49736: The actual bolt entrance is at 12.01 c. Contact refused, not leased.

Cover 49584／65700 Finish the whole and make sure that you have a single window. First, review your share-only booking `world_state.py` . Prop Cache Key Adds the stringure to avoid mixing the search results into this type of statement query.021930 The source code archive did not include this external reader, preserving the fact of reliance on the gap; derived from the original freeze `021930-manual-open-reader-r1`, add a new document SHA/Source log and end check. Guns 985／PG 3cab／XR 1684、18 The order of the driver and the original action has not been changed;1578 Source/6 Products/18 Driver+1 Read only pre-screened. No new scope is deployed.

Actual 49736／creation 134386398105656748: original pick-up, shoulder acquisition, authoritative launch,6 sec 24 Sample empty is not automatically filled, round ready passes. Real original shot timed 51917518 Microseconds. Then real left hand squeeze to 0.896484375 Time, Frame 745 Tracked/known/down average 1，busy 0、animation 0、main 1;current root local position is (0.210708539,0.0949293835,0.179750655 I'm going to the handle.0.120099638 m, greater than original 5 cm radius. Proposal 0, arbitration no_candidate 4, no case/new_grant; entrygate 1 Reject before the original begin. This round still does not exit because it is waiting for opening office 1 No bolts or bullets passed.

This is the evidence of this real push. It can't be determined by itself.12 cm from controller offset, mathematical conversion or a bone. The driver is Planning marker for Frame 584, pre-actual shooting and Frame 745; the old tags only output the first two, and no comparison of source time or current root/original or left Grip, so that deeper deviations cannot still be described as confirmed. The next narrow change is to fill only the currently empty chamber contact with read-only data, source time and actual Grip/planning end contrast, and input needs to be real close and pressed without expansion 5 Cm or any access.

83.95 Second Game/84.00 Second simulator double-way 3-page total span overview and 64.47／66.53／71.48 Three original maps of the second are available; the approaching handle and natural hand positions remain unaccepted.1098 Frame, maximum capture interval 2.317 seconds;simulators 1838 Frame, Max 0.567 seconds; double-track tail over 500 In ms, capture statistics are not a rendering of FPS or headline validation. Start waiting cards, blank spaces and failed images.

Native/entry/virtual input/hand dry normal, XR 0、launcher 0, game exit; set the original SHA, configure the original content and restore the original head;18 Drive + new Reader does not drift at the end. Encoding 65148 terminal 0，retention 12945 terminal 0 Delete 3018 Documentation/471140855 Bytes, errors empty, last three games/48 Hours. Only HB after the absence of nuclear object/XR/recorder/encode/retention 023513/ Later on, it's to Lancer.023554, this gun does not continue in the window. Record:`manual-entry-49736-review.json`、`manual-entry-49736-observations.json`and the integrity of saving `playground-49736.log`. Target remains active, complete manual/perfect reloading, scope image and all GrabPose failed.

### Subsequent offline source code: scope attribution and alternative parts positions

Same-hold further binds the full constrol/weapon registration, Nature Cookie, Lifecycle and World Registry; new frames can continue, and change sources cannot continue to use old targeting attribution. Prototype counter-example has failed to assert that EXE and logs remain. If the original object relationship is closed, it will be restored only if the relevant mark of all original recipients is certified or the precise old registered object is cleared when the existing read-only retirement certificate confirms that no debt exists; unknown obligations are maintained without a new pawn or expired input being used as a backup for action. `6e02e03d30d3564290a8c019cd1f2179a81558bbbec42dae5fdf0795ad14ee0e` Compile and produce owner injection tests passed at the border, undeployed/play validation. `scope-cohort-result.json`。

Increased optional generics for bullets and bolt handles `GearsVrPublishPartGraspV3`: outer selection/cylinder, inner layer original V 2 Request and current Broker authentication;old V 2 Export, layout, default hand. Native continues only through the single C 0 Treat skin targets and existing 120 ms transition, without generating controllers, guns, leases or movement rights. Original input/100 ms/current object/linework/release review retention. `ff1c8126be0c4fc506c5de9709388c2b2dc6d624d73d71892348b6068bbd4b03` COMPILATION COMPLETE, V 2/V 3 Access tests passed; fixed bone length/original input/downed-buffer geometry clip 8090 Inspection 0 Failed. These are not actual skin co-acceptances.

The Longshot component layer is being connected to the above-mentioned general interface: the bullet is carrying a shell casing with precision, showing that it shares the only hang point at the tip of the actual chamber; the embolism sets in the direction of the cylinder package from the bone stem point currently read to the spinal loop. Pure geometric tests have examined the points of the abdomen at different world rotation/unit scales and the points of contact with the cartridge's shell, hand-held contacts and the embolism axis are rejected without effect; the actual production of the shoulder, lease, follow-up and withdrawal test for the plate has also passed. The new parts publisher is yet to be compiled and validated by a game; no natural fit, no penetration or full angle is claimed.

The new scope/spank is frozen. `034956-manual-open`. The controlled candidate remains Nativec 985、PGf 14695a 1、XR 1684 read-only POD and the same planned source time for only empty chamber contact; no action added, no expansion 5 cm, does not adjust the arbitration or loading mechanism.1578 Source/6 Product/18 Driver/1 Read-only precheck passed, Cover 23420 The only clear window that is left after the complete closure starts running and results are recorded separately.

### 65996: Real contact and new lease pass, failure to change the status of the bomb

`034956` Actual 65996／creation 134386447353782239: normal pick-up, shoulder acquisition, authoritative shooting,6 sec 24 Sample empty and single resource ready to go through, actual shot game_tick_us 35802471. Using Frame 740 It's a sync.979359634294200 Current empty geometry, time planning.979359723589200, mark the age of 89.3 ms;real request peer is kept while actually entering. Actual Frame 801 Left and right original Grip reaches target, tracked/known/down average 1、raw squeeze 1 Local spin button distance 6.39003719e-7m；request_magazine 1、decision granted 2、magazine lease 3／id 1／new_grant 1 I'm sorry.5 The cm contact has been approved, not anymore.49736 It's...12 Cm deviation.

Then runtime date 4、serial 0, no original Reloading state, and eventually no opening receipt/bomb taking, impraator 1. The file contains both transfer.bind and original begin, and the log does not record more subconditions, so it cannot simply confirm that the original function has been called by the date.6 And the return of the next production proposal supports the remnant source deficiencies of the “same-proposal change”; the actual repairs are still subject to controlled review.1036／1037 The no-current contact/tracking available data occurred after the first Grant and was not the reason for the rejection at the first entry point.

Double 67.88 sec/67.87 seconds,3 Full Overview and Game 00757／00816／00818 The original picture is on the look.912 Frame, maximum capture interval 9.050 seconds;simulators 1256 Frame, Max 9.150 Seconds, double end.500 ms inside. Large interval and start-up waiting card retention cannot be considered as rendering of FPS or proof of cause for the spacing; actual hand/part alignment is not accepted.

Native/entry/pad/hand dry/XR/launcher is normal, game exits; config original content, settings former SHA, head lack of precision recovery 18 Drive and end of reader. Encoding 68660 terminal 0，retention 4187 terminal 0 Delete 2951 Media/509266573 Bytes, empty Urors, last three games/48 Hours;onlyHB after nuidle 044514. The script was initially reviewed for an additional layer of the dual-output path, no code started and the original FileNootFound log was kept separately; the records were properly completed after correction of the parent directory. `current-contact-65996-review.json`、`manual-entry-65996-observations.json` And complete. `playground-65996.log`。

### Intersection of proposal restoration and 050328(not running)

Add a new return to the Longshot proposal, real`same_pose_consumer_observation`Comparing rules and original begin boundary injection: the old code changed from request_magazine to help incument in the same Frame, which did produce a game_difference 6 And reject, RED EXE/Logs remain. Fixing the four categories of roote that have been successfully submitted is only available in Longshot program; current observation still authenticates the real Actor, input, identity, prescription, master, busy, animation, mount. Next Frame calculates incument from a new state; external incument or baseline changes will not be covered.100 ms, do not retain expired action permissions.

Same return after repair 0, the subsequent Frame release incument; busy/animation/master lost/input or change of reference/external indent still refuses. Complete manual cycle, strict 4 Seconds, terminate/replace hold/restore the debt test. New segregation PG `3133acaf541d984a8782e5e38c802e2a7f4f86e105a17ae40b00c4a4df6538cd` Full 0 From original f 146 The observation freeze only extracts the restoration, without mixing scope or new positions. The archive fixure first brought unassembled scope ' s exclusive type of yield assertion that it was retained separately failed and passed after a copy of the test excluded this exclusion; the actual archiving production rantime was not modified.`050328-manual-open`It's...1578 Source/6 Product/18 Driver/1 Reader checks are cleared, and the only subsequent window to Cover is turned over to HB after completion.050438 It's just a state of readiness, not the game fixes through.

### One-shot/staggered GrabPose compiled (undeployed)

Follow-up configuration PG `981e6c7f70e032edb0a3d54bcdeb3f1d895d8bd9a6db6a19a8c4cea47608ad98`，1091 Source, full text 0;with Native `ff1c8126…b4b03` V options 3 Head file is identical. Longshot parts layer select geometry and hand shape only: the handle first snuffs the ball 5 cm, adsorbed/captured to hold 7 The real Grip lease is the actionr; release to Snap and re-enter after leaving.5 cm, hold down Grip and return to the new lease without skin adsorbing. The exact current bone POD of Frame/source sync can only be targeted with the right hand using a real Grip that has not been diverted from the same plan. V is not supported.3 The old Native did not accept this optional skin request and did not change the course of action.

Actual compilation and execution from the combined archive: geometrical transformation, production of parts publisher, production of bomb laminates and production of runtime; relevant tests from scope owner are retained in this combination version.0 Only solver, fixed bone length, input protection and default V 2 The path is maintained. The current contact with the abdominal and embolism hand is the geometric target defined by the author, which still requires original maps to check for puncture, point towards and fit the whole surface; this combination has not been discharged into operation and is not aligned with 050328 Mechanical restoration review mixed.

### Subsequent adsorption time limits and directional guidance (source code/de-line, not operational)

Skin adsorbed to maintain metadata increase in sync continuous check: above last valid skin request 100 I'm not enjoying it anymore if I'm back in time.7 Cm-maintain area. We have to reenter.5 cm acquisition area; this is not a renewal lease or new action. The boundary archive publisher test is passed, combining PG `bf3318be00ba3dddf49dd856728b8e1e98f9e38706c8e99d388c7183cc7b3d32` Full 0, Records `part-freshness-result.json`。

Then add a single frame direction to the group: draw the actual Root+X-entry axis when carrying it 8 Cm deep/6 And the contour is close to the contours.1.5 cm horizontally in circle and forward arrows; an arrow opening/locking from the read-out original curve and current target progress sample is not guided to complete institutional actions. Keep a spherical hint as the current path. All lines are drawn by the former Actor. DrawDebugLine declaration class, parameter type/scope/stand alone, Vator type, Native portal authentication; no persistent line only.8 ms, check source/functional identity and original for each line 100 ms time limit; outer layer action/restoration does not wait for contours to be completed.

Final successor group PG `45359b4b1a9aa05889d7b3292dab44c677c792b64eed84df97ec175b2425b01c`，1093 Source, full compilation link 0; size, insertion direction, world rotation and unit ratio constants checked from geometry tests compiled in its actual archive, pass. Native remains ff 1c 8126,synthesis repair, scope and V 3 The position is inherited. No stage/no game running, and the mapping is still subject to physical review for visible, readable, masked or impacting operations; text/sound not recorded. `guide-shapes-result.json`Queued 050328 Still.3133acaf/c 985, does not include these subsequent changes.

### 71272: Consortment repair has passed the original entrance, follow-up to Gate 7 Failed

Cover 61492 The only window that comes out of the box is clear.050328 Again.1578 Source/6 Product/18 Driver/1 Reader checks are consistent.71272／creation 134386471850461237: basic collection of guns, shoulder acquisition, authoritative shooting,6 Second empty chamber and resource ready continues to pass.36000591；Frame 824 Left to the handle.6.02123759e-7m、request 1、decision granted 2、magazine lease 3／id 1／new_grant 1, followed by actual 0→acquiring、serial 1, prove that the original begin has acquire, old Gate 4 And indeed they have crossed.

Frame 825♪ There's a date ♪7(crip.service is not ready or Bolt.prepare refuses) No opening receipt/shot receipt. The following live video of the weapon in the original animation removes the gun from his hands and then finally releases it after acquiring idle. The current log still does not distinguish between specific sub-conditions for stopping nodes, serial authentication, control preparation, curves or bone endpoints, and does not support the "seeing an animation" as a separate node cause. The complete manual cycle is still unsuccessful.

Double 3 Full Page Range and Game 00840／00851／00861 original maps;playing games 999 Frame/65.234 Seconds/Maximum capture interval 1.567 sec, simulator 1396 Frame/65.267 sec/maximum 0.300 Seconds, double end.500 All of them are back to normal: Native/entry/pad/hand dry, XR 0、launcher 0, game exits, config/settings are original SHA/head missing, driven and reader terminated.86601 and retention 74123 Meanterminal 0 Delete 2021 Media/351190123 Bytes, errors are empty; only HB after nuclear idle 050438. Record `begin-publication-71272-review.json`、`manual-entry-71272-observations.json` And integrity `playground-71272.log`。

### Original crip/body posture recovery 060433(not running)

Use the reverse-engineer-anything workflow first to query the master share KB `J:/gearsVr/analysis/re-kb/knowledge.sqlite`, save script SHA and read the original text; no empty KB of this checkout is used, no game starts or a native analysis database is opened. PlayWeaponReloating will start animated firearms on Weapon ReloadAnim in its original version; PlayWeaponAnim also has CustomAnim Node and common security paths; FreezeWeapon Anim reads the current nodes when calling. The script contains unsolved native token, CustomAnim Node ' s actual node selection cannot be independently certified by static declarations.71272, the reason for the more detailed operation is left unknown.

The source code is also confirmed that the original body-relaying assistant had only contained other suitable types and Longshot returned directly.This time, the editors were confirmed as not overlapping by both Lancer/Boltok parties, adding only two rows of Longshot type and white list conditions;The original StopWeapon condition, all typed/identity/downed_state, serial number restoration and other details of the ditty are retained.The normal Longshot process takes over the original version of the already initiated LS_Reload, and is certified by the existing animation service acquire_prepared and stopped accurate current nodes and no more double Play.The lack of reading before the action is only expected to be the next real new Frame, which does not repeat the attempt;Original status, real Magazine lease, current object and original 100 ms is still required. Auto-propelled in the original version after cessation, as needed, by the same existing banes seek; real opening/locking remains determined by actual bone and physical hand processes.

clip recovery/rejection results are now immediately transferred to clean-up; the unknown status in preparation remains pending.20 The following tips are not read. After moving from the action path to current fololow and aim processing, avoid splitting a genuine time-use drawing first, or using this frame plan with a left shoulder hint; their role is still a reminder, without permission. The independent gun-only Play branch of the loaded live ammunition after interruption is not certified by normal process fixes but remains closed independently.

Segregation of PG `6814108b9626c6680c629a79ffa9745a0b27f42a1ae0ae64e6b68ff20c60fbc0` Full Compiled 0; the actual archive doesopt adapt, runtime back passes, covering no second Play entrance, accurate readnodes, one original stop takeover, new Frame reading retest, unknown/external owner reject, restore debt and prepare for failure cleanup. Engine/metadata border is injected, not original call for running evidence. The first buildup failed when the old freeze-off animation interface did not accept the addition of a fourth diagnostic parameter and failed to file; now it has been called using compatible three parameters instead of copying the current animation service of another gun.

`060433-manual-open` 1580 Source/6 Product/18 Driver/1 Reader Freezing Pre-Censorship, Nativec 985／XR 1684 No change, not stage/not running, does not contain scope, V 3 A figure of the position or direction. The order of registration is Frag 053206→HB 055335→Frag 053956→Longshot 060433→Cover r 12, final handover is a requirement; Cover accurate plan is b 5e 097b 018a 98bd 1d 576d 088e 60cb 80a 778ebd 8a 4a 4853ca 3035dc 370c 42373 a, old ddc 91b 7 a Not available. `original-clip-result.json`、`original-clip-queue.json`。


### 063907: Takeover stoptime validation and this narrow segment verification

It is found that the original capture updates the cache before checking stop/seek, which may obscure unexpected changes in time or rate. The action now reads the same freeze on public reconnectile_seek and only updates it after success; the freeze version accepts only the requested time for a pending seek, while the old time/unknown result continues to reject. Only restores the path allows Stop-only to read without reading the submission of the seek, mechanical endpoint, ammunition or reward.

PG `d0cd6edee9214534fc4fa9aa7066409cb2280009382481f607b0a5319798ae77` Full Compiled 0; actual block-off support and original public reconcile extract test, and complete runtime return 0. Tests cover stoptime/rate drift, unknown Seek, refusal at an old time to be determined and clean-up no-go proof that the engine boundary is still offline evidence.060433 not running;new 063907 Yes 1581 Source/6 Products/18 Driver plus 1reader，Nativec 985／XR 1684。Frag 063214 Actual 68512 When you close the window in its entirety, clear the only window and then check all freezes and rely on passes and stage. Only run existing opening/shoulder take single-shot/release segments; actual results are recorded separately. The only subsequent Cover reader-r 12+eye-r 13，prepared cover-r 13，plan 6838d 7b 93804c 01aac 3e 101d 8d 20be 602bcead 458714d 02459067faae 1b 7e 274。


### 8092: original crip stopped passing and the handle was still failing

Exact 063907／PGd 0cd，game 8092／creation 134386514191954964 Regularly pick up a gun, get on the shoulder, shoot with authority,6 Two seconds to empty and single-shot resource ready over, shot game_tick_us=36306198。Frame 811 To the handle.3.05e-7 m and get a new Magazine lease. Original crip report for ready 2／adopt 0／capture 0／service 0、playing 0／stopped 1／time 0 Sequence length 3.33333325。Frame 812 Still get 7, no embolden path/opening approach. Clip reports only recording changes in key values and no subsequent crip status; the range is reduced to preparare of the embolism but more detailed reasons for a curve reading, actual closed bones, paths or prescriptions are still not recorded and do not state the causal repair.

3 Page Complete 63.85 Two-Site Summary and Game 00824／00830／00837 The original picture shows that the gun is no longer re-loading from hand to hand, and the pistol is clearly incomplete, without a physical bolt or single-shot left shoulder.982 Frame, maximum capture interval 1.800 seconds;simulators 1366 Frame, Max 0.333 Seconds, double end.500 ms covered. Start waiting card/gear retention, not rendering frame/headprint proof.

runner 87008 And launcher 0，operator 1 Real failure; Native/entry/pad/hand dry resumed pass, XR 0/game exits, config original content/settings former SHA/head original precise recovery,18 The driver and reader are finished in a consistent fashion.34413／retention 72543 Meanterminal 0 Delete 2718 Media/473729225 Bytes, errors empty; only clear post-nucleial idle Cover reader-r 12+eye-r 13, accepted by the other party, only using its freezecover-r 13;This session does not add case to this window. Recording 8092-review.json、manual-entry-8092-observations.json and complete playground-8092.log。


### 065312: Squeeze ready to reject subdivision candidate (not running)

Only read and prepare steps retained read PDF, failed to distinguish crip, curve, unit/bones, closed position, mesh, path and final source check; most changes are recorded 16 bar. Decoding the file also records the original frames/format/offsets/bytes values. Failed to release the prep status. `84593cc90b477ee877081f171d36c321c734a3032a35b5a737c90c04384e3b9a` Full 0，065312 Total 1581 Source/6 Products/18 driver and reader prechecks are consistent. Exactly freeze the prepare function, real BoltPath and injection engine boundary tests 0: The verification location/angular/bone anomaly/unknown source refused and was successfully submitted once; the first test compilation was rejected by WX for the mpfile disposal warning, which was retained in the original log and replaced with the checked tmpfile_s.

This same request is scheduled for all earlier Coverr 13→Snub 060545→Boltok 063023→Frag 064829 later, pending confirmation at the actual end; only subsequent HB accepted 065400-refire-supply. There is no stage/run, and there are no free windows to grab. The complete follow-up group has combined the stoptime verification with this observation, PG `90c8067ccba05f35ab1be83b692bc06c7d0778cb77bce1cbdbd7dd0843482e92`／Nativeff 1c 8126、1096 Complete source compilation 0; the public animation service and the help function of the clock are identical to the tested version. The combination is not deployed, without its compilation results statement scope/GrabPose/direction or full-turning pass. Recording of Bolt-prepare-observation-result.json.


### Retrieval of original package certainty and 070405 Cosangular Replace (not running)

Re-search the package_objects and WarfareGame artifact of main KB, re-use saved packages, full export inventory and name lists, and no longer display names as objects path.91c 0f 56 e. Normal Animation Report SHA 5db 59fb 2f 47b 8229f 7dafc 57be 9a 7b 30c 60171cc 0d 85329723915ce 13fd 4ab 4 f Both verified. Real normal LS_Reload 32NoW（3），NumFrames 100, compression embolisms horizontally/rotationally 50 key,896 bytes;failed animation is Float 96／160 Key. Original production accepts Float only 96 and 1 or NumFrames, which determines that normal animations are rejected and that all previous false curve tests cannot be overwritten as normal packages.

Old freeze decoder reads the same actual 896 Byte Stream RED Exit 1;restricted after amending format and independent compression keys 50 Keys are identical to the full number of independent Python inteval oracle, and original BonePath can be bound; former Float 96, damage to country/ span/format/range and failure not to publish check pass. Bit-level formula checked separately[Fixed version of UEVIewer definition](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMeshTypes.h). Do not change 2mm／2 Do, real hand process,100 ms or any original owner/stock/reward conditions.96/ Keys equals frame” assumption corrected by actual data.

Segregation of PG `90b5e763cb849ab20dc8f483f90d7b4d3cd28dff726c3ad81a58bc912c7408f6` Full 0, actually freezes both source back execution 0，070405 Still 1581 Source/6 Products/18 driver+reader all-nuclear. Not operational 065312 Only last position replacement accepted, old freeze retained; eventually Frag forward 064829 HB updates, followed by HB updates 070119／PG 30239f 21. No stage/play running, no proof of decoded actual run time, coordinates stored to the bone agreement or original plug-in time; the rest of the physical bones/path check continues. Recording of the original-interval-candidate-result.json and the real package return log.


### Input planning for subsequent complete cycle (not connected)070405）

Add the same setem number POD (ceiling) to the subsequent combination of successfully shouldered live ammunition 16 The original hold/lease/source/resource cancellation test is compiled from the actual archive and is performed 0. The first complete construction was rejected by the WX because of the local reported masked resource symmetrical variable, which failed to be retained and then separated from the full PG after the count_report_rows `1f427be04960125a6ae23d0a5fd28b27750f29ebb4ab02cf9dc0339663d5af17`／1096 Source compiled 0 Native stillff 1c 8126. No stage/ queue.

Independent offline Bolt input planner from actual 33 A path point generates a continuous trajectory, maintains the initial raw hand contact deviation, reverse paths when closed, fixed-gen controller is directed and includes speed limits; does not write at product or give mechanical completion.50 A planned sample check was passed. Previously, the warhead planners were re-covered with an independent four-digit figure 96 The group is not yet connected to the real driver, but it is a very important task for the team.070405 It's...18 Drivers are unchanged; physical physical pathways, plug-in and incentives must be separately validated.


### Follow-up lockdown time checkback (not deployed)

Record the original shot of this round after physical locking and transfer has been confirmed; no extra reading games, no change in rigour 4 Second condition, inventory or reward. Follow-up PG `dd559233d8e43cb6d35fb4f56c57ec6abe8a0e8885d6931ac8337ce7a89f4366`／1096 Complete source compilation 0, actually archive runtime test execution 0, overwrite the original full cycle,4 Second boundary, old starting point/hold/cut off incentive and debt recovery; Nativeff 1c 8126 No change. Deployment and no access to queues 070405; actual lock-down and perfect reward still to be shown. Recording


### 50728: metadata contract rejected, properly closed

Frag 071258／15196 Keep the other native cleanup timeout/drain while finally closing the window 0／entries 0／launcher 1 Failed; its games, XRs, video, code and retention are all final, configured accurately and this session did not change the failure to normal. Only window is received.070405 It's...1581 Source/6 Products/18 Driver+reader is once again fully unified and stage.

Actual 50728／creation 134386549558313796，shot_tick_us 37689041;base shoulder-taking, authoritative shooting,6 Second empty and single-shot resource ready over.818 Real new lease versus original crip ready/stopped 1／time 0 Over.819 bolt gate 3／curve_gate 3, returns before field authentication without data decoded, closed bones, bolted paths or single-shot success. The static main KB complete Engineering. AnimSequence object list contains NumFrames/two CompanyFormat/two CompanyLengths/KeyEncodingFormat; the original seven fields co-opted does not exist and therefore are sufficient to reject it, but this wheel does not distinguish between six earlier respective results and cannot be overstated as actual sole caruse has been proven.

3 Page 65.450／65.500 Full-second double and game 00843／00849／00857 original maps;playing games 1000 Sample, maximum catch gap 2.133 sec, simulator 1400 Sample, Max 0.350 Second, tail is covered. The gun is stable at contact and the pistol is not ready to meet the standard, with no actual embolism/takeout. Native/entry/pad/hand dry normal, XR 0／launcher 0/game exits, config original content/settings former SHA/head original precise recovery,18 Drive and reader end without drift. Encoding 42093、retention 82021 End 0 Delete 4663 Media/915976716 Bytes, error, only HB after nuclear idle 070119. Endendreader first wideshot hoook-entry/restored sidecar rejected before writing; re-informed local report path, original error retained, unmodified or restored. Record interval-50728-review.json。


### 075334: Old AnimSequence field contract (not operational)

Only the Longshot animation reading layer is used to include accurate Engineering.AnimSequence and complete stabilization of UFild chain certification, confirming that the version does not have KeyEncodingFormat, so it reads the six actual fields in the old UNiform compression stream. The normal ookup failure without attributes does not produce compatible qualifications; all links/identity/paternity structures must be fully logged and rechecked. All new seelector, external uuter, unknown type, chain cycle/oversight, cut-off or intermediate changes are rejected.100 ms/real mechanical endpoints are not required.0 is an old UNiform contract that is certified and not read to a field where it does not exist.

Actual archiving of the helper by regular and 15 Dissociate the PG. `ed9a5bbad953b6b1e4266167d2af72bca80c4e4d73f1ccbfe80a8985ad1365e3` Full 0，075334 Total 1583 Source/6 Products/18 Driver+reader pre-test consistent, Nativec 985／XR 1684 reuse;old 896 Bytes/50 Key decode and Float 96 Returns to a fully used source. You have not yet started the file or played. Only all more processed HBs are requested 070119→Coverr 16→Snub 071103→Boltok 072100 The record is the only one that follows.


### Actual read_curve field contract RED/GREEEN returns

Separate old 070405 And new 075334 Freezing of the full production function of the sourceread_curve, given real 896 Bytes/50 Key Animation and Original Field Interface: The old function exits when requesting that no statement be made about KeyEncodingFormat 1,sstream not read yet; new function GREEN execute 0, only six actual fields are requested and issued after the old schema border 50 key.8 Fields are missing/unsupported format/framework inconsistent/source or data, nodal changes. Class/metadata/engine call borders remain injected and full chain Helper is produced in another actual production 15 Rejects branch testing; does not claim that the game field has passed. Recording curve-reader-fixure-source.json and curve-reader-red/green logs.

Follow-up complete group of PGs `8abb1f9f8a0f67351fa942f0e471e5de6f17d96ba43f9c606b217cc2f4f1ca22`／1098 Source Part 0,use and 075334 The same two current changes sources, including pre-scope, V 3 Shape, directional direction, positionm track and locking time; unstage/ queue. Run the tail chain has been directly reconfirmed as HB ⁇ Coverr 16→Snub 071103→Boltok 072100→Frag 074333→Longshot 075334. Frag has been approved 075334 prepared 5eb 6dc 7c／plan db 546879／1583 Source 6 Products 18 Helper+reader is the same; this is only a final position, not a current window. This session has not been followed up.


### Follow-up complete cycle and close eye check entrance (source code only, not running)

(b) Reuse the original wrapper/oporator/core gun and firing process at the independent full entry, choosing either a single ordinary complete cycle or an additional exercise full loop only for its own copy;18 The link is based on a physical new line/physical path/s shoulder to retrieve thetem track/original warhead insertion/locking time/real count and reward field determination, strictly.4 Seconds are not relaxed by slow input. Enter the owner's invalid injection certificate after a subsequent input has been sent and no further entry is made, pose; pure bootstrap anchor/AST and original file unfixed check passed. The current only source code does not provide for group running packages, unstage/queuing or running, which does not constitute complete loop pass.

The close-eye input plan uses a real lens relative to the Grip conversion and eye counter-resolver target.64 The group of eye deflections/head rotations/polar and vision targets are re-established by an independent four-dollar number.1.5e-8 Rad is falsely tilted; instead of full matrix line atan 2 Pitch, uncommitted oracle tolerance, scope 64 and feed 96+2 Pass. The plan is still not called or entered; actual mirrormount/scope status and image are otherwise confirmed.

Also search for the main KB to review each of the four scope functions and parameter names, as well as eight current status fields, with a declaration of preservation; this is a static inventory check, not actually memory layouts, call success or multiple image pass. Recording the fuute-test-registration-result.json and scope-saved-contract-udit.json.


### 65324 Closed-Step Rotate Agreement Refuses and 085615 Candidates

Actual 65324／creation 134386585341708092 Passed six fields and old schema authentication, Curve_gate 0；NumFrames 100, format 3/0/0、16offsets、896 Bytes, complete original trajectory 50 key. Close Bone Actual P ()17.9751892,-0.0772024846,10.5666047)，Q(0.00848905012,0.00651679957,-0.707132657,0.70699991); XYZ, the opposite of the original track closed to Q, same W, Gate 6 Angular Error 179.965367°Location 5.899e-7 m. Operator ultimately lacks a flash response 1, the reservation actually fails.

Fixed co-location of four-digits of the original trajectory, at the Native relative transformation adaptation boundary for production of read_curve; original decoders, locations, keys, time series, inventory/input/ownership and 2mm／2°The verification is not changed. The entire sample is validated and converted, and the original package is not partially modified.1 Revert 179.965367°，GREEN 0 Got it.8.5377e-6°／5.8984e-7 m and can be bound.50 key path;extracting the true of new frozen production 896 Bytes Formal and 8 Deny execution 0. Only Bolt is physically measured, and other bones and dynamic plugs are still to be proven by the game.

New narrow version of the PG `92eb8f719caf2518edd14bb8e006c8dd7e1d0bd2a9007eefc599423bc48ca778`／1084 Source, Full Group PG `a09cd8325d1f0bf3857a6035ad02d7a7e8475f87cdae222e056e96aecfcda16c`／1099 Source All 0。085615 narrow-endped f 92db 796／1584 Source 6 Products 18 All checkes for help+reader have been checked and are now cleared by Bol 085057 The only follow-up to the same end position was clearly accepted, which has not yet been finalized for window/stage/run.085657 Full combination with Nativeff 1／3183 Source,6 products,18 Base+7 Follow-up Helper+reader check passed, not requested to run; old 083748 Keeps unrun history. Current queue chainCoverr 19→Snub 082247→Bol 085057→Longshot 085615；085057 Replace original only 084022 Same slot.

This time.3 Double-trip Full Range and 3 The map is already on the table.1010 Game/1424 Simulator frame, range 66.58／66.60 Seconds, take the biggest gap.2.033／0.267 Seconds, keep the third person ' s name/wait card to start; inappropriate rendering frame or natural hand-to-hand certificate. Self-normal cleanup/restoration of all original bytes through, code 0、retention 0／errors 0 Deleting 3912 Media/760141638 bytes;only after independenceidle to Coverr 19. See also Legacy-65324-review.json、native-convention-regression.json。


### Independent reference check for the time series of original orbital sampling

Fixed-commit.[UEVIewer sample achieved](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/Mesh/SkeletalMesh.cpp)Use even-time, consistent with the current time/SequenceLength xNumKeys sampling position. Real 50 Maximum rotation of the adjacent key 62.99°;each paragraph 99 At a fraction point, the current linearly integrated interpolation value is the maximum difference between the sphere interpolation value 0.310°. This is an external realization/static mathematical reference that does not support the active time series of this game, so no changes to sampling times or 2°Practical consistency threshold. See stock-interval-interpolation-check.json for further details.


### Next close eye R 3 Validation readiness (not running)

Eyescope planning records only repeat the currently bound handheld matrix and already-enroled Pose, most of which 8 bars; multiple status records are only used to repeat and transmit original copies of the request to the zoom/FOV/body and head visibility, most often 32 Article, expressly image_verified=0. No new Engineering query, Hook, Native or Bones writer. Full PG 4702a 3ee／1099 Source compiled 0, original eyebox/scope ownership test executed 0;restore text of GetTargingFOV 997be 5 Consistent with the knowledge base, the zoom branch returns to its former ZoomFOV and the ordinary branch calls the father method, which still requires physical metadata and video validation.

The future scope-only snapshot tool uses existing readers, all type/bit mask/biread/component link/descript/object identity closed for review.11 The rejection and neutral/low/high/fail/missive head component is offline.3♪ Hold on to the outside, put your eyes on the back, let go of the new R ♪3, keep holding, hold on to the lens/re-entry and final state accurately restored; only normal controller input is sent, and no original status better.6 Document AST, two original Runners Replace/4 Zero subsequent input offline after denial and loss of ownership; first offline check misdirected CRLF byte instead of producing read_text swaps, anchor refused to be retained, amended for execution 0 No game influence.

092652-Scope's frozen.3184 Source/6 Products/18 Base+8 Specially helper+reader and checked, PG 4702a 3 I'm with Nativeff.1;not queued/stage/playing, done first 085615 The actual opening narrow.085615 Binary and 18 Drive unchanged. Record scope-check-prepared.json, scope-snapshot-test-result.json, scope-driver-preparation-result.json.


### 16872 Actual closed-space correction, HUD starting point and animation receipt offline

085615／PG 92eb 8 f Actual 16872／creation 134386612967401307 Crossed gate 0／curve 0 And publish 33 A real handle path. Actual error 1.27348e-6m／1.02453e-5°, this closed rotation is passed; has not yet constituted a dynamic embolism.813 HUD configuration successfully, Frame 814 capture 1／service 2 Failed, operator eventually exits without condition 1. Two controller syncs are separated 110.4793 ms, but the capture front is owned/GT/clock review, and the log is not enough to attribute it to input spacing; no relaxation 100ms。

Share HUD issue explicitly sends the RedloadStartTime writing costs to reward_start and updates its exact journal. The original animation read_down still requires old reload_start, which is checked with clock and refreshed by Cache.1 Revert the change to stoptime 0 Animated rejections;newlock GREEEN 0 The government has been able to provide a comprehensive response to the problem of HIV/AIDS, and it is not possible to accept a single successful HUD issue in a visible response.11 Programer admission and 10 The clock/node/service/object changes are rejected and the atom rolls are cached. No new starting point, seek_pending or self-observation changes can be automatically accepted.16872 No starting point values are individually exported, so that conflicts confirmed by source code and offline cannot be recorded as having been proven only for reasons of the Bureau.

A narrow PGed 239336／1084 Source, follow-up complete PG 5ae 303f 8／1099 Source Part 0;freezing narrow and complete runtime (including Back failed cleanup/ strict 4 s/Recovery/New lease protection)0。094103 Freezing 1584 Source/6 Products/18 Former Helper+reader and checked, prepared fca 1f 094／plan b 0d 150f 1;has been actual Snub 093107 The complete combination has not been reprepare. There is no sared animation source code, Native, hook, input, inventory or threshold change.

- The Bureau.3 Double-Specific Page Range +3 The map is very much in the past.1009／1426 Frames, frames.66.667／66.650 Seconds, maximum collection gap 1.917／0.267 sec; HUD has appeared, forearms shielding handles, full hand not accepted. Normal Native/entry/pad/drain/XR 0／launcher 0/game exit, configuration/settings/head original byte restoration and 18 driver+reader without drift;coding 0／retention 0 errors 0 Delete 884 Media/136577426 Bytes, the only one that will be given to Frag after independence 090321 And Coverr.24 And Snub 093107. See also the text of the document.16872-review.json、owned-ui-start-result.json。


### Follow-up complete process completed unlocked blockage and real re-entry (source code not operational)

"Practical R in future regular process for independentlongshot_full_chain entry "2 Check that no new return was issued, used still 0 and the ammunition is not changed; actual re-entry after completion of process requires new original confirmation from 0 Change 1 Each movement and key is confirmed by reader as still the same Longshot; no later than zero subsequent input is determined to be re-executed.0. Only complete process source changes will require new freeze before entering its future window;094103 Original 18 Help and scope 092652 Special 8 Helper has not changed, no action added in the current narrow segment.


### 095812 Update and limit local logs for the same team

Rejected to distinguish successful producer back from the previous node, only added most to crip_clock 8 Local journal/cache values are available; log clearly notde_values_are_cached=1, no additional node/Engine read, call or permission.913725d 8 Full 0, freezing the actual clocks and 21 Deny execution 0，095812／prepared 9f 772390／plan 50ed 9829 Nuclear 1584 Source 6 Products 18 Help+reader passed, unstigma. Snub has expressly accepted it to replace the old 094103 Same slot, front and back.090321→Coverr 24→Snub 093107→Longshot 095812, no second application or forward transfer; only one follow-up to the gun 095943, its final plan-v 2 95b 0dc 72／21driver+74 Support relies on the session to be maintained and no windows are currently available.


### Follow-up guide drawing order prioritize direction and seating

The original chamber.31 A line of arrows at the end.8 ms draws budget that can cut it off. Only adjust the pure geometry queue: first three delivery arrows, then 8 A small ring is eventually entered, with a contour; all dimensions, totals, original DrawDebugLine calls and budgets are unchanged without any new Engineering function/call or mechanical permission. Geometric dimension and rotation/unit constantity test is performed 0, the latest complete combination of PGs 37095c 9b／1099 Source compiled 0, is not prepare/queuing/run;095812 The narrow candidate remains unchanged. See more on the guide-priority-result.json.


## Appendix: Existing operational and candidate records

The following record is maintained in its entirety at the original point, where the expression “current” “unaccessed” is considered to be “unacceptable”. The latest combination/scheduling and acceptance conclusion is based on section V of this update; historical failure does not take off or pretend to have been repaired.

172406 The successor candidate has been compiled, offline checks and static preparation, not deployed/run: only Main joined the optional first/two pre-declarations in private freeze, Backend uses Scope for currently selected and entry idle Longshot with no available access to all active institutions and other guns. Former web code of observer/proof/handClit is fully maintained, evidence changes are still completely rereading and failure to read is denied;100 The following is a list of the items that are to be found in the table: ms/Frame/identity/Lease/Input/ Geometrical Relaxation, no extra Engineering query or clock.26 Manual memory check for items 0（2418(a) A full byte change of evidence, manual 1000- It's the one.116.44ms→6.70 ms is not proof of the cost of the game.1096, Archive 1596／6 Products/18 Driver +reader pre-check 0，PG `1b27c30383f0f8f32e51832fe8966fe2e931688dc5df3ee3152c62bd4c82d65c`，Nativec 985／XR 1684 No change; succession 17072 The actual passing of the original event is appropriate and cannot be recorded as a new binary.`idle-validation-candidate-result.json`、`idle-validation-runtime-plan.json`. The application has not been accepted or finally submitted to the window since it was submitted to the candidate, after a longer period of time.r 1 Failed to compile front-to-face statements,r 2 Failure to establish anchor and initial test package quotation/decode error retained, ultimatelyr 3 Compile 0/ Test 0;unrun failed prototype.

Latest 160159 Actual 17072 Complete and end with only Frag 161511, this session no longer has a window. Add the initial branch to actually trigger:1 Parameters 8／synchronous_press 0 It's old.1 Invoked 1 In real Frame 591 Execute, return to old source age 88.1618ms；source_ready 2／packet_ready 10／stale_sample 0 Authority_valid 1 Take the gun, start it, go on.6 Seconds do not automatically ready-by-dot; cancellation/state handover of undispatched events is not yet a game and cannot expand on this conclusion. The manual embolism planning stopped because “No fresh longshot_reload_contact_current recapt” and no new bult Grip/UI/Trapping/Blast was available.599／source 2254; follow-up records include stale_before and stale_or_changed_after, but there is insufficient evidence to confirm the first bind_contact sub-gate, the only cause of a change in the hold-gendering or the pure prescription for an extenuating circumstance, without which identity may be relaxed or 100ms。

NHB Native/entry/pat/drained/XR 0／launcher 0 The withdrawal of the /game and the original config/settings/head recovery were checked, but the exact bytes were not recovered.18 Drive +reader no drift.791game／1146 Sim capture samples, range 60.400／60.434 seconds;complete 3 Page Double-Segment 4 Zhangharatu, with original HUD, TPS/waiting card, forearms and sleeves reserved, complete natural attachment not accepted; maximum capture interval game 2050.0143ms／sim 1083.331 ms, not a rendering frame or FPS certificate. Encoding and only retention average 0(Deleting)830 Media/132477029 Bytes/errors 0- The latest.3 Bureau 48 The only window that will be turned over after the final independent idle and once again full-nuclear Frag depends.`first-event-17072-review.json`Save read-only follow-up analysis`current-contact-17072-source-audit.json`;no reruns unchanged 160159. Follow up scope/V 3/Guidance+same initial fit-in group 1111 Source compiled 0，PG `28d33e35dd008e0d4a81a78ff663af53fb3b4fb6300b95d4dd4e1e4bde4f42a3`No preparation/teaming/deployment, and no product for the geometry of vent magnetic inhaler.

Latest 144539 Actual 35152♪ All done and only handed to Bol 144535, the current window is not part of this session. The shoulder takes a gun through, but the first gun only has ammunition 11→10 The shooting sample pixels, authoritative trackory, did not produce a time-out of the firing and drove it to stop at core, no access.6 Second empty/UI/locking. Review the old PG log line 137–138 Confirm that both the actual InstantFire portals are rejecting:1、held_current 1, the controller age 104.107ms、stereo 104.1143ms，frame_active 0;tracking position/rotation differential to 0. This evidence demonstrates that access expired, does not prove a specific time-consuming source or is attributable to the preparation of changes in the UI frame. The original POD did not reject source Frame/sequence and prohibits subsequent spelling times.0／launcher 0/game exits, former config/settings bytes and head 18 Driver +reader non-dispersion checked; complete 2 Double Page and 4 The original map is read, coded.58517／retention 88464 Actual 0. Official end-review maintains the original SHA and adds a read-only analysis to the`first-shot-35152-source-admission-audit.json`。

160159 Candidates have only completed source code, compilation, offline check and static preparation but not deployed/run: from accuracy 144539 The original source stack and sniper entry is frozen.41 Byte BeginSate Packaging (suber.BeginSate)5 Remittance timers) and 96 BytesShouldRefire overwrite; the latter is only for real R 2 The original version after the-up is returned without firing. The entry/father ' s first hair/original timmer/identity must be passed, and during the waiting period the packaging and the parent function are rechecked to maintain a true pressurized cohort,250 ms event duration, current Frame 100ms、20 ms sends the surplus and pre-claim reset rules. No second queue/input writer/Hook or automatic refile path, public root refire files are not edited. Original bytes match entry 1321 Former first-body/timer/once-only queue 145 Checks 0, complete PG compilation 0; original release metadata are compiled, not yet independently executed and do not declare cancellation/repatriation passed. `bfeac3a9e89e99f8fcd9f85f9a58e9e60c3ce0424cf73e0f1c4b0e37d0b38db4`，1596 Archive Source/6 Products/18 Driver +reader pre-check passed, Native stillc 985、XR 1684. No prototype a 091 As a result of the source code review, the old handover path was found to exclude Should Refire 2 replace;do not treat the prototype as proof of failure.`initial-event-candidate-result.json`and`initial-event-runtime-plan.json`. Real start-up repair, bolt completion, single load/locks/rewards, multiple image and natural hand positions remain open.

160159 Additional certification: production freeze`longshot_should_proof`And then there's something else.28 Manual metadata check compilation execution 0, including normal original package body,24 Signing/gun type/state/key fields/bytes/reading rejection and denial of access 3 Fieldactor parameter mismatches. It validates the offline branch of the production fit function, which does not represent the original status return to game pass; candidate, prepared/ready/plan SHA and 18 Drive to keep the same as you do.`initial-event-release-adapter-checks.json`. This candidate is now a real team tail HB 153818 Direct identification is processed pending its final delivery in full and has not yet been stage/run.

The following candidates and active entries keep their historical points, which do not represent the current queue or current acceptance status.

Latest 133536 Actual 61036 Ended, unreached on current cycle view location: normal start-up and 6 Second-time empty, Frame.756 Curve/Closed Bonesgategate 0、33 Point path ready, then return to the frame UI Timing question before hacking 1→idle 0;no UI back, Bolt begin/motion, history or seek sub-door reply.28872 The reasons for the Seek inner layer are still not located.61036 The cause of the problem is not proven. The driver refuses to stop.2 Page Full Double-Segment 4(a) Personal review of the original map;59.6 seconds,800game／1253 The sim sampling, end-covering is passed and the forearm embolden and natural adhesion are still not accepted.0／launcher 0/game exit and original configuration, settings bytes/head restoration,18 driver+reader without drift all pass;coding 61338/ Unique 69785 Actual 0 Deleting 789 Media/119742590 Bytes/errors 0. Last independent idle and only to Coverr 39(Corrected)plan 2b 96e 8d 9 HB again 135655, no old window records added.`bolt-seek-61036-review.json`。

142004 The frame changes that have been compiled for this failed preparation period have not yet been deployed/run: after the first real curve/bones are ready, UI settings can only happen after subsequent real Frame; repeated call-out with Frame does not move, next Frame re-scrip checks the original source/clip and adds current real held lease. Release or lose lease is done as per original cleanup process; no Eganine query/hour clock added, no Frande/renewal/release time frame is created, perfect replacement firing starting point and 4 Second boundary maintained. Production operators inject "preparation for depletion" to obtain the expected RED, new root future and precise narrow version of freeze test GREEEN; four regression cover pre-recovered sources lapsed, repeat with Frame, follow up without source/dismissal, lose the held lease, maintain full cycle, strict 4 The full PG compiles the example of seconds, inventory and interruption cleanup.0、1592 Archive Source/6 Products/18 Unmodified and pre-screened by Reader SHA, PG`35d01bf5be1363be58f31125eb3e8da64b817b5b0be0b4eb9ebe3d736257b26c`Private sources select only the change in their gun schedule, do not copy yet/lock back to root future, and do not use offline batches or a new API attachment. This change is not proof of cause and effect in the game; application has been made for single copy with scope manual-open, currently without running window.

Replace the same unrun slot now with 144539 Old 142004 Retain and explicitly not deploy: received ahead 9896 After the failure of Permission Error, who had failed to carry out his weapon operation, was only in this gun after he was rewritten as a self-employed JSON.`run_longshot_operator.py`Loaded memory template to replace common script line reading; in case Permission Error or JSONDecodeError waits 20 ms and go on.45 Second cycle, original live/error/ready check all reservations, and the other I/Os still fail immediately. Common templates and others 17(a) The base drive remains unchanged;8 The actual CTPU checks were passed, including instantaneous recovery, permanent occupancy/semi-writing timeout, loss of identity, recorded error, insufficient readiness fields, other I/O and normal ready. The PG/Native/XR and the pre-defined frame code remained completely unchanged;1592 Source/6 Products/18 Driver +reader re-screened. New prepared SHA`ce9281403fc54546f122fb4e8fb08d419256ef59478ac188531c7c82685f6916`, checking the original only forward to confirm siphon replacement, not yet sage; no forward XR/launcher exits 1 Claims of operational pass, separate verification of resource destruction/restoration and independent idle.

Latest 125546 Actual 28872 Ended: original 8 The frame production history channel is actually available, first callbackgate 9／6 Point/Sequence 3026 to 3031,override 111.1077 ms;this dot is static, then real Frame 808 Implementation of Seek was successful. Actual institutional observations were achieved 0.498613169；Frame 861 I'm hoping for progress.0.592625852 timecase 4／action_gate 5 Rejected on`longshot_clip::seek`,ping cache 0 Current sampling age 85.4656 ms, the first internal short-circuit condition is not proven. Curve/content check has passed and cannot be described as the only root cause of this Seek interval or subsequent Native withdrawal; no bolting completed or bullet removed.3 Double Page and 4(a) The physical map of Zhang is under examination, forearms shield/HUD/natural contact remains unaccepted;62.2 seconds,940game／1328 Sim sampling, end-covering is passed but not rendered frame/headprint.0／launcher 0/game exit, former config/settings bytes and head recovery,18 Driver + read-only checkers without drift; encoding 46594 and only 13920 Actual 0 Deleting 2582 Media/419166808 Bytes/errors 0. Eventually independent idle and only to Bol 125154, this session has no running window.`analysis/longshot-development-20261108/bolt-history-28872-review.json`. Next offline position is the sek sub-door, old 125546 Do not run again.

133536 The candidate for independent observation has been refused by this Seek, but is not deployed/run: only in the original private freeze`seek_impl`and`reconcile_seek`Local phase number for the word prefix, up to output after this gun is set 12 The login is not used to save the memory; no Engineering reading, clock or SetPositation calls are added. The two functions remove the PDF account by word and equal the old freeze. The shared area public animation file is not written and does not include the completeness of other current firearms. History successfully returns is also weighed separately by waiting/moved/interrupted.12 Heads/27. The ceiling of the point remains unchanged.64 The mathematical/supreme source SHA for item and bolt return is the same as for full PG compilation 0，1592 Source/6 Products/18 Unmodified and read-only SSA precheck passed, PG`c64af0ea463065ca6714d849a10734b9f9ecf4e36c09d46b24d0c3abfca59e66`. This candidate is used for positioning, not to call fixes; a running window has been requested with the same manual-open narrow section along the real tail and has not yet been obtained. The first form of script was rejected before translation because it was mistakenly assumed that the old freeze contained a new HB special seek branch, amended by genuine old code, failed to archive and did not blend into the new HB branch.

And only offline make up reading batches of the PG.`bf39b1e76d573399614b0420c30b90ac05f714833412b90366f2b6d7554e7b70`No prepare/teaming/deployment/operational, not replacement 133536. It keeps the freeze from a precise consumption freeze, generic header files and matching two files; calls scope, HandClit, offline fixture and all weapons logic, with all evidence bytes rereading and masking.4 KiB page, per batch 1 KiB, gap 32 B. Extra filling per round 256 KiB, any reading failure does not break down the re-test and is held back in range.27 Inspection of production monitors,679 The test for the byte of evidence, a failure of real PAGE_GUARD and the Franme/identity back check and complete PG compilation were passed manually.30000 Scope/84 Round-time original 54.1075 ms, batch 59.0244 ms, this time slower; no accelerated or Seek cause-effect restoration is claimed.`read-batch-preparation-result.json`Wait.133536 The specific refusal is decided upon.

Current state: Regular taking of guns, authoritative shootings, empty chamber non-automatic refilling, embolism/new capture lease, original reload acquisition, old animation decoded and closed bones verified with the latest available evidence.28872 Continue to confirm that HUD starts are compatible and go beyond historical continuity and actual Seek, but then follow-upSeek is rejected in the process. The physical embolism is complete, a single shoulder ejection, insertion/locking/perfect reward, close eye multiplier image and all natural positions are not yet passed.3 The spare parts configuration has been compiled and targeted offline testing, but is not deployed. The current narrow version of the guide remains a spherical hint, with subsequent source code added to the contours of the chamber and the emboldens direction arrow; text/sound and final visibility are pending acceptance. The following dimensions are the minimum values that demand and currently achieve, which cannot be a substitute for actual contact and head-on experience acceptance.

Latest 095812 Actual 20908 Complete end: original clip closed bonegate 0／curve 0, position error approximately 0.000417 mm, rotation error about 0.0000146°，33 Pointing hands path release;actual UI back case 0 And follow-up Capture case 2 Averaged 1, precise starting point 52.6677742→40.1346207,Cachetime 0，frame 719 Enter opening. Drivers are not read to openup, eventually overtime; the Bureau lacks process-by-process rejection of a request for reply and cannot be judged as one of the reasons in bones, successive input or seek.3 Page Double-Segment 4 A map of the action has been shown, with a front arm covering the bolt and no visible arm moving in lieu of completion certificates. Native/entry/Input/XR/Launcher exits normally, config, settings and original head recovery; code and actual retention 0 The only thing after independence is Bol.095943. This round does not touch the bullet, inserts, locks, rewards or multiplier images.

104905 Actual 35040 Completed first rejection and complete closure: actual UI back/capture again, frame 810 Getting a physical embolism;812 First tiny movement reviewing curves/seek branchphase 4 Quite rejected. This time sample date 6、capture gate 0, grip/lease is effective, source continues to be poor 90.1495 ms, sampling age 80.305 ms, position error approximately 0.000465 mm, rotation error about 0.00000724°;failed. The old Native retraction age 100.4974 ms, but there is no precise rejection within the branch at this time and it cannot be described as the only root cause. Driver immediately stops the remaining movement after the initial refusal and clears up with the final launch position of the gun. Two pages are fully saved for a two-way overview and 3 Zhanggame/1 The front arm is sealed, HUD and the start-up is reserved.53.184 sec,game 856／sim 1136 Sample, not per cent of the frame or headline.0／launcher 0 The following is a list of the names of the people who have been arrested and killed in the past few years:18 driver + read-only dependent on no drift;coding 9333 and retention 43977 Actual 0 Keep up to date.3 Bureau 48 Hours. Only Bol after independence 105222;the current window is no longer part of this session.

Latest 085615 Actual 16872: Old AnimSequence/Interval decoded and closed bones verified, Gate 0／curve 0,Press 33 Pointing to real handle tracks; closing position error 0.00127 mm, rotation error 0.0000102°. Next frame after HUD setup succeeds 1／service 2 Refuses, still untackled/out of single-shot. The source code and offline regression reveal a conflict between the original receipt and the original reloading after HUD was written to the starting point of the firing;095812 New candidate (replace only unrun)094103 The same team) only accepts the exact starting point for a successful writing and rereading of the same HUD service, continues to call unmodified actual node clock/identity verification, compiled/offline pass, and has not yet been validated again.16872 The starting words were not exported separately and cannot be declared as the only proven reason for the Bureau.3 Double Page and 3 Original image, self-normal cleanup and original byte restoration pass, encode/retention actual 0 The only thing that has been handed over to Frag since independence.090321 Old 65324／50728 Failed to retain. Follow-up complete combination and scope candidate unrun; full reload, multiple image and natural grip still not accepted.

User confirmed that from actual shooting to the bolt closing lock, less than 4 The second is perfect. 4 The second is the normal round. The bullet follows the left shoulder of Lancer and the veterinary rifle, interacting with the existing key button rules, each time a full sniper is removed.

112230 Actual 12980 Complete end, untamed: gun withdrawal/start/6 Second empty chamber/UI/Closed bones and 33 Point path/begin pass;frame 793→794 First time observe in case 3 Continuous inspection denied. Two admissions to the institution for a period of synchronization 111.6819 ms, current sampling age 65.6107ms。sample gate 6, grip/lease is valid but does not enter Curve/seek and has no actual past-proof usage/cost PoD, so cannot be claimed 35040 The failure has been repaired. Driver refuses to stop and clear the gun. Two pages complete save the distance of two paths with 3 Zhanggame/1 Zhang Sim's original map is read by his own person;52.800318 seconds,821game／1127 The sim sample, embolden, HUD, start-up waiting and complete natural adhesion is not accepted.0／launcher 0 The following is a list of the names of the people who have been arrested and killed in the past few years:18 driver + read-only dependent on no drift;coding 74268／retention 7279 Actual 0 The only thing that ever happened to Coverr since independence was Idle.32。

112230 Universal reading and verification is kept in the same order.100 ms/identity/re-play and all action doors;26 Block freeze manual memory checks and 5 The true HandClit time/identity/readout check and complete PG translation is passed. The evidence remains offline/compilation only.12980 The existing XR CSV does not contain the original hand tracks and tracking sequences, and cannot be ex post facto proven to have failed to track the left-hand sample or controller.

125546 New Quest of Continuous Candidates frozen, not deployed/run to replace unrun in the same queue position 124434. Reuse already certified 8 Frame controller production history, which only proves the original hand movement when GT consumers are missing the middle frame: all original serial numbers and synchronous times must be preserved, hands tracked and entered into a generation, same real pressure, stabilization of Native reference and rigid master installation; current Frame, lease and current sampling 100 ms is still independent. Every jump stays valid.100 ms, retick and whole bit shift 1.5m/s＋2 mm limit, path deviation from upper limit 15 mm. History points can only complement the continuity of movement, not generate world frame, renew/renewal or back-up as bone end; seek works for real flame and completes the native bones that still need to follow up on true frama 2mm／2°Confirms that any refusal to clear the cache does not replace the accepted anchor. Public input backend, bridge protocol, Native and read-validation source remain frozen.

The candidate is actually completed 22 Original relative trajectory of the item,19"The Zen-Luct,23 Production adapter inspection and re-entry of original bolts, all compiled and executed 0;including original 110 The ms consumer interval refuses to reproduce, the frame/right/left sub-key/current expired rejection, the sector-by-section margin does not accumulate a free pass and the hand is still inactive and animation to endpoint is incomplete.50 Key Root Static Check passed, but must be reproven from the actual curve when running. Full PG construction 0，1092 Compile Source and 1592 Archive Source/6 Products/18 Unmodified and read-only check is made using SHA.`1a2ccf6ef86a73e11e3023872edcacfe9e26a5f8d4cd7eb60decd82516cb349d`; evidence is available`analysis/longshot-development-20261108/producer-history-test-result.json`. These are offline/compile evidence that does not prove history, real embolism, loaded or hand-held in the game. Registered at Cover r 34 After not yet received the final only running window, Public v 71 Keep it as it is.

User Supplement requests to switch the multiplier from original opener to right swinger and press R 3, only active observation of sniper mirrors close to the eye. All interactive physical components must be matched with a corresponding GrabPose patch that includes adsorption preview, actual capture and motion processes.

The poaching of the vents has added an independent geometry.`weapon_longshot_chamber_snap.h`，20 Targeted check for compilation and execution 0 Entry/maintenance of borders, reverse warhead and illegal transformation refusal, overall typographical rotation/unit-to-model ratio of the gun, maintenance of original axle depth, and installation of an anti-resolvation precisionr Grip target by the same live ammunition. It calculates only alignment targets without generating load or inventory endorsement; no access to products or re-numbering 125546. The subsequent connection must show live ammunition, C 0 A fine hand holding and loading judgement shares the same current frame alignment results, close/departure, real depth of delivery and speed of withdrawal of unsorted original warheads. The current opening, live ammunition and lease agreement is required to be valid and steps/lease agreements/source changes are immediately withdrawn and retained 120 ms shows transition. Geometrical adoption does not represent actual magnetic traction interaction or natural exposure acceptance.

Subsequent geometrical amendments (unconnected product): original aids explain thresholds to model uniform distance, Scale 2 It'll make it real.8 And horizontal.4 And acceptance, Scale.5 And then you'll refuse to be real.3 Cm deep/4 Cm horizontal. New physics is back in the original file.0 Implementation 3 Recurring; the amendment only changes depth/horizontal indicators back to real physical rice, and the chamber continues to retain model coordinates with local points of original warheads, while adsorption targets remain in current original axis direction.72 Rotating root/non-zero chamber/Scale.5、1、2 and 50、100、200 Checks and originals 20 Geometric Check Completing Execution 0 Actual 28872／61036／35152 The record is close.1, does not consider this offline problem as a cause of failure in the old game;160159 and follow-up not prepared 28d 33 The new header is not included in the e-group.`chamber-physical-range-result.json`; actual magnetic insulation/hand Pose/loading connection still to be achieved.

Specific interactive engagements for the inhalation of the chamber (to be achieved by product): Only when actually opened, holds the only live LIVE ammunition on this round and the original lease for left shoulder delivery remains valid.6 Cm, axle forward.8 cm;sorted to maintain body radius 8 Cm, forward.10 cm; most reverses across the vent 1.5 Cm, the axes and the pins are not exceeding the angles.30°. Leave body maintenance, release, lost track/current 100 Dissociate when the source, item/lease/gun/reference changes; withdraws the assistance to remove inventory matters that are not cleared.100 ms does not inherit larger preservation bodies, and re-entry to the acquiring body is necessary for re-adsorption.

Insorption only supports horizontal positions and axis direction; the actual delivery depth takes a projection of uninsorted warheads in the same current root coordinates and cannot automatically advance into the chamber.120 Each intermediate posture of the ms transition also retains this original axis depth: first, horizontally, with a value warhead inserted in the same depth plane, then by moving the axis at the shortest possible turn and then by real.15 cm long back-default fire position to avoid a direct insertion of the bottom fire and rotation leading to a false axis movement.0; it is forbidden to add hand, controller or gunwriter. The fine gesture showing live ammunition and the request must come from the same current Frame result. Native's actual consumption and skin show that contact with the frame remains separate and cannot be used as a hand in which the request is made.

120 The net geometry support needed for the transition has been added, and it is not possible to get a better understanding of the situation.54 Check for compilation execution 0: In 5 Transitional weights,3 Units,3 The warhead remains at its current true axis depth at the model scale and common root rotation; live ammunition corresponds to the position of the reverse grip. The auxiliary is only kept with horizontal/rotational modifications, which can be shown gradually after leaving the contact body but cannot be used as a permit for exposure or loading.72＋20 Geometrically removes the new function from the geometric version.`chamber-transition-geometry-result.json`; has not entered any product, currently Frame displays cache or inventory determination.

Logically loaded retention close to before delivery: original warheads at least prior to loading 2.5 cm, then move real to axis  ⁇6 mm seating area; retaining original 100* The present document was not edited before being sent to the United Nations translation services.3 m/s speed limit. Horizontally 1.5 cm/flip 10°The final in situ inspection is applied to this valid aid result, and the primary input of the support must remain in the body above-mentioned acquisition/maintenance; it cannot be shown that the match has been made but still requires the player to satisfy the original without assistance 1.5 cm. The transition is not in place, the original warhead does not really move forward, the static hand simply moves to the end, the inventory is not matched or the Frame expired. The loading creates only one original stocking service and locks up one single round of the current round; maintenance or withdrawal of adsorbance itself without detonating, creating new items, or opening locks ahead of time.160159 Freezing of products.

For this load line, the relative trajectory of the original helper is attached only to the API, the former project/build subject and all existing signatures. The new API first verifys the full continuity conditions of the original production trajectory, then rotates the actual left hand and mounts the same live ammunition into the warhead position.16 Check in 16 Freezing source compilation of files 0, with current Marcus 115 Squeeze the mount in finely with the original Native Rig 50／100／200 Subunit alignment, source missing/substitute/fine refusal and output of atomicity; evidence`attached-motion-geometry-result.json`I'm sorry.20 The item is not yet connected to the chamber geometry.133536 Run the candidate to remain frozen; geometrical matches are not submitted for approval as skin, real loading or stockpiles.

Follow-up complete combination 113340 Compiled, frozen and static relying check, unteamed/undeployed/not operational, still requires the adoption of a physical opening narrow section. It maintains the full screen option for the close-eye version.3 Door control and V 3 The re-load drive stops when the first trip is rejected or loaded/locks are returned, seven genuine drives are routinely confirmed as stopping and clearing with the same gun in situ, and zero input after identity lapses.31 Field checked source code SHA; complete PG construction 0、3187 Archive Source/6 Products/18 Base drive/7 Additional entrance and read-only pre-screening pass. Port magnetic insulation alignment, complete natural contact, multiple image and physical validation of normal/perfect reloading continue to be open.

The complete follow-up combination is now updated to 130712(Replaces unrun)113340, still not queued/deployed/run: only superseding 125546 The historical continuity of production of the same six documents is achieved, with the existing scope/V in full combinations retained 3/Guidance and seven do not stop driving. Full PG compile 0，PG`e848129146de70f0edaa949b9070f9eaf07c05f31c54ee5de6ad6e2acafeac06`，3191 Archive Source/6 Products/18 Base drive/7 Additional entrance and read-only SHA precheck; Native remains ff 1c 8126. This combination does not contain new calibration geometry lines and cannot be used as complete evidence of magnetic insulation, reloading or the passage of a target.125546 The narrow end of the bolt remains the only operational application that will be made for the next time.

| Delivery | Source & & Compilation Status | Current factual evidence |
| --- | --- | --- |
| I'm not gonna shoot you with a gun and an authority. | Presented for UCP | Actual gun and initial launch. |
| Keep the empty chamber after firing. | Auto-refilled blocker completed | Actual continuity 6 Seconds not automatically completed |
| Gunpowder. | Original crip, composite paths, historical continuity of original production and physical process adaptation compiled | Actual closed bones and 33 Evidence is available for point paths, original hestory and Seek implementation, and institutional observations are moving to 0.4986; follow-up Seek refused, the embolism is still unattended |
| One live bullet on the left shoulder. | Existing shoulder area/input, sole item lease and live ammunition resources are connected | Resource ready; actual single-shot not reached |
| Plug, lock and perfect. | Continuous delivery of ammunition, stocking services, real lock-out and strict 4 Second-second boundary compiled/delineated test; port magnetic insulation aids not yet connected | Not actually completed; Normal/Perfect Incentives and Retroactivity to be validated |
| Second point and all GrabPose | This gun handler configuration, Snap/Grip state and V 3 Fine/cylinder grip compiled | The gun is completely four-dimensional and has not been verified with all visible surfaces. |
| The short-eyed original image and the R.3 Multiplier | Fullscreen alternative paths and input attribution compiled, independent validation portal prepared | Low/high multiplier images of originals not yet running; PiP in lenses not achieved |
| Interactive guidance per step | Current narrow spherical hint, subsequent contour/direction arrow compiled | Complete phase-by-stage visibility, text/sound still to be completed |

The following dimensions and exposure thresholds are the project ' s initial value achievement and acceptance targets; source-code tests, compilations, simulators images and physical experience are accounted for separately.

2026-11-10 Actual round 37448 Supplement: P by user 2 P-director active. Real left shoulder, initial launch authority, continuous firing.6 Seconds keep empty, raw fire timed back and single resource ready five passes. First manual embolden in the frame 746 Interrupted: Source end 2870→2875 Separate 112.2413 ms, current sample age 91.4049ms；sample_gate_cached=6 It's a sample successful check point, history gate 7 This is the original relative: :build rejection. Log count=0 The output is the number of outputs after failure and cannot be inferred from the original input window as empty; the exact sub-judgment remains unproven. Failure to complete a bolt, single-shot removal from the left shoulder, loading, locking, reward, retrospect or internal mirror multiplier verification.0.4986 The evidence of the journey is running earlier, not counting.37448 Already got to the action.

37448 Normal exit, recovery of 10 original configurations, 2 full pages of time line and 6 original maps for personal review, double-circuit code and one retention cleanup are completed. Gun body and bolt contact is masked by wrist/forearms, complete natural alignment with physical head experience still pending. Window has been handed over to GN-v through the only final primary handover 4, Original filerun 37448-final-handoff-GN-v 4 The sha of the json.256 Yes 134c 73699fb 7667dfe 609e 077f 9d 1e 53a 3de 40923c 09de 5ca 4a 57cbacab 1cb 19; this gun no longer writes shared operational status and does not repeat a consumed operating licence.

Add a private project-history-vidence-r to this rejection 1, only first time 7 After a failed action determination, the limit records the original two ends that are read, all entries in the window, holding controls and fixed root changes. The original,100 ms-jumping/current prescription, identity, lease, single input source and bone backfill; no replicating frame or extending input history. The offline extracter rejects missing blocks/incomplete records, the original production of parameters in the direct relative:build log. Five endpoints are separated but different from intermediate facts, with root code passed and freezing source code; contains complete chain passage, original window cut-off, left handtracking change, prediction time repetition and holding margin change rejection.37448 The original input is not the basis for claiming that the actual root cause was found.

The private candidate completes the PG compilation link 0，PG SHA 256 For e 2fc 74470ed 9ec 3e 4d 5780077a 9cf 0d 61a 61271b 42b 3561c 667ace 282932bf 2 f Relatively realistic.37448 Only two Longshot files will be modified to match the frozen product, adding one Longshot header, and the rest 1094 The document is unchanged; no additional emboldens in the root code are used to geometry of exposure. The initial records are not ready for entry, queue, deployment or run-off; the latest status of independent access is shown below.

### 2026-11-10 Independent run and sequence update

The development sequence, as the owner latest requests, is: first to strike through a gun, shoot at it, physically bolt to the left shoulder to take a live bullet, physically load, lock and re-fire, and then verify that normal and strict are less than 4 The second is perfectly re-loaded; 2 complete the adsorption, acquisition/maintenance/exit range, tolerance and interactive guidance; 3 lastly, part by component and natural GrabPose is perfected. Existing experiences and hand-held results are maintained, basic grab points continue to be used for processes that do not match unaccepted fingers to prevent process development. Real institutional endpoints, identities, input time limits and stock retention remain constant.

Sniper is used independently.`run_longshot_independent.py`and`independent-history-plan.json`。050140 The source and the six products remain unchanged.18 The original action drive is unchanged; the new entry re-enacts the existing private start-up mutually exclusive, pre-starting checkes the actual game/XR/record occupancy, preserves 10 original targets and private Config/SaveData, accurately restored and released after normal completion. The separate code, review and media retention are no longer a condition for capturing the game. Old front chain, step-by-step receipt and one-time final window transfer require history and no more binding on new independent access points; old consumer entrances are not used.

New portal completed read-only source, product and performance pre-check; normal stop limit is 360 In seconds, request for normal stop only after checking this real launcher identity. Current location target is still actual 37448 First time heistory date 7 Refuse, the exact failure is not known. The new entrance is ready to complete a new game validation, full reloading, multiple image or physical header acceptance.

Actual 39920 P has been specified by the owner 2 The P directory completes short and normal end. Five basic checks are passed again; the first opening action is in Africa 976 Progress 0 When refused.7576 Full original byte record bound by the actual process and creation time, frozen 050140 Codec direct to original`relative::build`Recurring denial: previous admission 2974 This time, this time 2979, original returns window only 2976～2979;lack 2975, the first consecutive pair is a failure. Enter peer interval 111.7769 ms, current age 91.8691 ms. The record demonstrates that the necessary original middle is missing; no complete bottom production package is recorded, and it is not possible to distinguish short-ring coverage from the point where it was not successfully released and does not repair 2975 Nor is the expansion of the cache considered as a physical rehabilitation that has been adopted.

39920 The government has been able to provide a comprehensive and transparent response to the crisis, which is why it is not possible to stop the violence.18 The shared environment is released as soon as the original and private configuration files are accurately restored. Three pages of complete two-way overview and six original maps are viewed, coded properly; wrists sleeve blockage contact, full process and natural fit unaccepted.9.33 The maximum interval of seconds is reserved for evidence collection and cannot be diagnosed directly as an engine in Cardon.

Private History-retention-r 2 Keep the bottom production ring by 8 Increase to 16 Still the highest output.8 Original sample; current sample and right hand press record still limited to original 8 Scope, Current 100 Ms, jump.100 The agreement uses an independent v.4 Maps and versions, mixing old components will be rejected without prejudice to public v 71. The old/new actual InputBridge returns show the old loop cover binder rejected, the new complete chain passed; the genuine leaking, left-hand tracking change, loose hands, loss of focus and flow switching still refused, with the original identity, cross-rest, life cycle test passed.39920 The reconstruction of the missing sample.

Matches private Native/PG/SimulationXR compiled.`native-part-grasp`Full freeze source plus one historical header, with an optional V 3 Export, cannot claim and actual 39920 c/985 Baselines only one document variance; PG relative 050140 Only historical bridge changes. Simulate XR to complete two additional missing tests/recording sources only for the configuration of registered, unencrypted targets. First attempt to reconstruct Native directly from old delivery sources failed because archive omitted testing/build files and retain currentory-retention-r 1, inactivity is built. The new candidate only serves a controlled simulator and the old physics XR has not risen to v 4, this combination cannot be delivered to the head.

Actual 55436 Failed before authority is first released, with a gun taken through the left shoulder only and not at the historically preserved cover point; this can not be used as a bolt repair. Native/entries/pad/XR/Launcher has withdrawn but Hand Drain returned to false, leaving the entire tailings uncompleted. The outer layer was initially unrecovered until the end of the end of the party, supplemented by a unified RuntimeGuard and its original internal lock.18 The two pages of complete double-circuit and two original maps are read-and-coded; the near end is waiting for VR frame cards, complete match and no institutional conclusion.

New 152926 Candidates for withdrawal of the entire Native base change to original c 985 Archived.1332 List of entries; where the C++ and header files are kept in their original language, the historical bridge is the only change that is made within the list. Missing compilation/construction inputs are completed from reserved private sources, recording each source with SHA;`cover_body_thunks.asm`And before that, Git...90f 3a 131 Normalize content. It does not claim to match original c 985 Binary bytes can be repeated.612ac 98e、PG 6aab 5f 2 b. Simulation XR 137c 147 e Both are approved for compilation/pre-test, and the next physical validation is still pending.

The new entrance is unified.`RuntimeGuard('solo-m1-1028d')`Overwrite backup stage to start recovery process, maintain the original internal Owner lock and wait for the end of the side 30 Seconds to judge the safe recovery. The retrace only excludes independent offline encoding, not real games, XRs, recording or action processes; codes and reviews are still used in their respective directories.55436 and 39920 The occupancy records are not used again.

### 2026-11-10 Real progress after the restoration of the original historical chain

Actual 5212 Adopt original c 985 Native, archive reconstruction, all five basic actions passed; the original production history continuous check passed and the bolt actually advanced 0.0926963, and then refuse to enter the current time limit check at the animation control preparation stage.18 The full set of two pages has been shown in person; six original maps are cleared by rolling retention strategy before viewing, and are clearly not counted as considered. Full bolting and reloading are not working.

Actual 49240 Add existing mass reading: original witness bytes are still itemized, double-renewed, no authority or time limit is available, and no failure to split the retest.679 The physical witness bytes, PAGE_GUARD and the original observer exit check passes. Five basic actions pass again, with a bolt thrusting 0.0764822；frame 800 Final pre-implementation inspection refused, and the frame was subsequently withdrawn to confirm the age of the sample 100.521 ms. Early cache of rame 794- It's not rame.800 Direct cause evidence. Normal closing,18 The item was accurately restored and released; the overview was generated but not seen, not counted as receiving or inspection.

New private 164405 Candidate for Reinstatement`read_curve`Animated nodes that already exist at the end of a successful event will only skip the original enclosed-observation path once when complete input, frame, role, weapon, node and sequence are fully matched to current operation. The current input is eventually repeated.100 ms, ownership, functional metadata, parameters, unnotified location and post-implementation checks remain; no enforcement clearance is provided and no frame cache is available for reply.57 Entries Matches and 33 The actual function of the item is used as a model; both narrow sections and complete patch PGs are compiled. The shared animation functions only add optional parameter forwarding for default closure, while other weapons are called to keep their path.

The candidate is yet to be physically activated; complete recovery, loading, locking, ordinary/perfect reward, re-entry, periphery multiplier and all GrabPose have not been completed.4 Group not deliverable stillv 3 The latest entry is directly from the existing unified interrespondence.30 Waits for seconds, does not consume a single entry during the occupation period or modify the running of other sessions.

Actual 54064 The short run of the candidate was completed: five basic checks passed, original animation preparation, initial embolism and production history chain passage;526 The first current inspection of the bolt tic refused, but the frame sample was not obtained or is not being executed. So the new conic curve circuit route is still not covered by this bureau and is not called repair passage. Full observation device produces and seals original testimony, but there are no precise failure sub-rules or age input for this record that allows the cache to be saved.6 or grip_down in emptysample 0 The results of the survey are as follows:18 Released after the original document was accurately restored.

54064 Two pages of complete overview and six original maps have been reviewed by the parent, and the double-circuit code has been completed.49240 The two pages and six original maps that have been kept have been supplemented with a parental review and code; the status of “unreading” was previously updated by this article without refilling previous records.54064 Start still containing third person and waiting card; capture maximum spacing game app 2 sec, simulator approx.267- I'm just collecting evidence. Checking out the evidence.54064 And the private entrances that are inherited.0.3 render ratio;new 172501 Candidates for reinstatement 0.4, does not treat the previous result as 0.4 Or paint quality check.

172501 Further advance the existing synchronous original byte verification scope of Longshot until exposure binding after success of the former copy_frame; the bolt is only used in fully matched and unproscribed frame verification ranges, and independent calls or mismatches remain within the original range/original observer.100 ms, double reading per original byte, change retreat, failure to read and all actions are called to be maintained. The actual freeze of the pass-through of Spope/observe and new borrowed function offline checks covers changes, reading failures, disablement/framework/weapons/module mismatches, recovery of software status and range destruction. The narrow section PG is compiled; the real cost improvement and complete opening of this adjustment remains subject to a new short test.

Actual 40964 Press 0.4 Proportional implementation 172501: five basic checks passed; bolting in the original 8 Second waiting is not accepted 100 The location log only produces a log that does not contain the actual bolt loan path of the new Spope.27 Article 48 cap; last rule is frame 665 The life cycle changes cannot be considered as the root cause of proof.18 Exact restoration and release completed; two pages of complete double-tracking and six original maps of hand-reviewed, double-circuit code 0 The right and left hand are in the rest of the room, and there is no conclusion on the bolt or loading.

New 174832 Candidates only adjust the match between their location output and historical fire-fire facts: using already existing same_internce_across_hold, allowing life cycle and intergenerational movement; world, role, weapons registration status, game/native, registration cookies, world generation, session, space and input subrogation remain the same. Only output of current bind_contact is verified, no restoration of old Frame, lease, pressure or execution rights, and no reseting of the original firing clock at most.8 The record indicates the actual identity of the person for verification in the next line; this change cannot be passed as a practical reason or repair. The Identity Function is effectively frozen and is checked offline, including 12 The change in identity/session/input and the step backwards are rejected. The narrow section PG is compiled and passed; the complete bomb mix continues to retain the same modification.

### 2026-11-10 Bottom of the full-scope set.

To avoid following 55436 Unreceived Native-part-grass Base Package, independent construction of Nature-c 985-scope-retention-r 3: action actually passed by base 612ac 98 e Archive bottom, only with reserved close-eye selection and R 3 Consumer interface. Capture.cpp contains display options for the new original interface, two export and existing projection, which are fully consistent with the original text after reverse removal; the rest is R from the original virtual handle 3 Consumption, associated head and test. Control historyv 4, original body/hand C++ and compilations are not replaced; the original build-up selected MinHook relies on a complete archive and checks that overlapping items are identical.0，SHA 256 e9cda3ae714422c92d62da9f4c682271f3035cd209f1b92bf60677c4aa59e04b. The version is not deployed or operational,174832 The switch is still in use.612ac 98e。

For the new frozen Native physical interfaces and functions, four offline checks are available: R 3 Access/consumption and prevention of re-playing, current object and eye/model validation, maintenance of original FOV projection and eyes view and depth, original handle map and release re-press. The border test double does not constitute a multiplication image or header acceptance.1/r 2;testr 1 Failed to compile because old folder is not declared scope time stamped, change to current complete clipr 2 Adopted, no production interfaces were changed to adapt tests.

New Native is consistent with the full PG ' s Frame, controller input/history, Rig and scope input/view header by bytes; V 3/V 4 The fine hand-held interface is still not integrated into the Native.2 The lack of a precise interface cannot be considered as the entire GrabPose. PiP in the original lens is still not achieved, and what is currently pending is an open-end version of the camera path that the owner permits.

Base V 2 The mastering protocol has been supplemented with an independent translation module check: compile new Native and complete PG actual headers, compare request/Frame sizes, alignment, field deviation, countdown, version and export name; request 1496 Bytes, Frame 1416 bytes. This conclusion covers only the line agreement and does not prove to be optional V 3/V 4 Or a visible hand-plug.r 1 The folder failed because the C link could not return std:array;r 2 The C++ link and the separated headname space were used to pass without change in the production head.

### 2026-11-10 Independent examples of sniper movements and 55280 Actual results

(a) Deposition/restoration of the case of solo as per the latest independent operating engagement;174832 The shared access is clearly decommissioned. New prepare_longshot_isolated.py sets maintenance based on the only example of a prepared candidate cost gun and a privately owned copy of Native/PG/XR 0.4; preparate_solated_longshot_operat.py only changes examples, logs, input and output paths of action scripts that already exist. The game is bound by this instance launcher-session, XR by the PID in the same session_report and at the creation time, enters its own command file without using an example-located MCP or the sole process for the whole machine.40964 Shared Run 18 The restoration of responsibility is closed.100 ms, inventory and institutional endpoint rules remain unchanged.

First longshot-1110-180729 Render size query before game creation failed: the original save directory only XR EXE, and the private directory omitted openxr_loader.dll of the same construction. No game created; original failure and stopback 1 Keep. New instance longshot-1110-181402 Copy complete EXE construction output from the same verified build-up library, start and real source 1 Two-eye projection passes. No duplicate failed example name, recast EXE or write shared PG/Input/Agriculture.

Actual 55280／XR 34612 The process of obtaining fresh contact_current and entering the real grip and production history cycle is carried out through five basic checks.710 First clear record of Curve_receipt_supplid=1 And original Seek succeeded. Spope reads it.39 Number of times; frame 716 Actual progress 0.0175842 After that, at prepare_controls date 20. The reply is wthdrawal_frame 715 old cache;same later withdrawn from frame 716 Recording 5、age 100.5397 ms, reservation 100 ms rejects. Relative exercise 155.3118 ms does not replace input through the original historical chain. There is no complete bolting, single-shot removal from the left shoulder, loaded, locked, rewarded or re-fired.40964 The specific causes of the identity mismatch and the effects of the rehabilitation on the  Normalization have not been established.

The Bureau's normal stop order.0，launcher 0, Native/entries/Pad Recovery and XR 0, your own game/XR has been checked for non-existent again; but hand drain=false, fire_disable_code 2／hand_stop_code 5, launcher passed=false, complete. No cases of killing or operating someone else. Two pages of full-scale cross-references and six original maps with a double-crossing code 0; wristslide shielding of gun bolts, full GrabPose is not allowed through. Capture begins at the action entrance and does not count as full title/start screen coverage.

The master then reports that the front window is re-emerging, and the latest requirement is to stop the new game/XR start, to close the existing side of the line and continue offline.55280 The recovery of the backstage restoration shortness was subsequently withdrawn by the owner reporting again to the front window: a clear resumption order must be awaited from the owner and cannot be restored by self-recovering only for start-up code fixation or another round of shortness. Runtime-start-paused.json and the entire unit of gunrunner are checked before any start-up preparation takes place; no detours or uses old access points may be used. Target continues to develop, actual rear stations and silent performance have not yet been received, and no initiation or progress notifications are sent to integrated delivery.

### 2026-11-10 Animated control queries and offline repair of normal stop calls

Targeted 55280 The first of these is the prepare_controls date 20, the existing stopAnim and SetPosition independent metadata are used for the same local query; only established paths that fully match Longshot with frame curve backslides are enabled. Repeating names but not matching types return to original Function type queries, receiving object registration and class maintenance, function identity, parameter size/type/diversion/mask, no notification settings and final 100 ms time-use check is kept. There are no cross-frame caches, input privileges or new Engineering calls. The additional parameters for the shared animation function are defaulted to close and other calls keep the original query path.

narrow-section-control-funct-pair-r 2 Compile 0，PG f 2d 673a 5;completefix-full-control-faction-pair-r 1 Compile 0，PG 70944e 8d。r 1 Failed to maintain original translation because the old freezer used a reference to a computer_names when the name function was closed; freezing group only adds the same functional switch border without changing it. The actual freeze of Args/field/fields/prepare_controls functions is on average and restricted names path 66(a) The equivalent price and refusal of inspection;800 Members' folders read 1605→804, just offline reading counts and not term the game as time-consuming improvement. Shareseek and Longshot real functions 33 Passed; original 57 Curve back check is still valid, the result is used and no double counting is new.

55280 Fire_disable at Stop 2 It's still stopping, hand_stop.5 is allowed to stop/unactive result, cannot 5 Separately called errors. The original session.close did not read the pending fire back after it stopped; this round returned only as it was.2, read the same stopback again when the hand has stopped and the original period is set to two seconds. No new deadline, startup, inventory or input operation; second still 2, other errors, loss of identity and timeout continue to fail.14 Entries offline check passed, including original 55280 Failure to return mode;r 1 The game is closed and the end-of-the-playing has not yet been proven.55280 The original was not changed.

The list of two compilations and source codes is documented.json will verify the products, full source code Hashi and original Native in future prepare 612ac 98e／XR 137c 147 e, create new independent examples without replacing old files. No new example is created at this time, no game/XR/simulator has been started; complete bolting, single-shot, loading, locking, normal/perfect reward, retroactivity, near-eye multiplier and full GrabPose are still to be physically verified. First complete logic, then adsorb/Volume experience, and lastly fine GrabPose remains unchanged in order.

Next R/EX check closed: prepare checks the actual sixdof_unified_runtime.py path and tested Hashi before any new instance is created; runner checks the selected records Hashi, Native/PG/XR, PG source code basis and the same exit code before startup. Sharing exit files are modified after preparation will be explicitly rejected and another version of withdrawal cannot count as this round fixation. The actual preflight function executes the private temporary file 9 Check passed; the pause still returns before reading the pre-recipient record and version check. This round does not create new examples or start a game. The first test failed only when checking the AST statement position (multiple statements are generated by branch lines), after correction of the folder location, without modifications to the production function.

### 2026-11-10 Combination check for close-eye control and original status management

scope-control-owner-tests-r 2& Complete PG 70944e 8 dCope Control connects with user in actual order: reader ready and then see the frame/R 3, last owner tick. The original function, status reading and Native Bridge use the existing test boundary; the bridge is not considered a successful image result.42 The claim is adopted, covering low/high/low multiple rate, holding down non-relay, holding up closeness at a distance, holding off eye entry, Menu/altering/entry failure and recovery, prohibiting new activation upon cessation, and returning failed restoration and re-observed access after setter has had side effects.r Early r 1 It's...37 Item retained with result,r 2 Expand 42 Line, no impostor 79 item.

Two actual production documents SHA are for control 41300582 With owner 04474a 9 c. The combination does not modify the production code, nor does it start the game/XR; actual behaviour of original boundary, R 3 The actual isolation of other functions, the lens image or the close-eye full screen option are still subject to controlled measurements. The PiP in the lens is not achieved; it is currently being achieved as a near-eye target view option that is still allowed by the owner.

### 2026-11-10 Full recharge driver migration to stand-alone instance (source code preparation only)

Prepare_longshot_isolated Add visible mode: opening default still narrow PGf 2d 673a 5;full-ordinary and full-factured selection of the PG in the record 70944e 8 d. Unspecified mode does not provide for the complete process to be started in secret. Operator is prepared to copy four full-fledged reloadherper into this private drivers directory and record the original text and generate Hash, replace only the open distribution boundary with a core end, take the gun/actual launch/six seconds manual empty space confirmation of the original unchanged. Normal mode only performs a round of general exchange and re-launch; practiced mode starts second round after normal wheel with an actual new start, with a full drive and requires strictness less than 4 seconds. Enter the command file that is still sent to you by Context from the same example, and the original gesture, inventory and physical body reply request remains unchanged.

Mode tools are pure transformations without startup, input, file or game access. The three modes generate code syntax, actually generate distribution calls, four illegal models reject and the movement boundary changes refuse to be offline; candidate version check expanded to 13 entry, the full mode mismatched narrow segment role is expressly rejected and the same as the prepared model must be applied.9 The result of the entry is kept at the previous source check point.13 This round does not call the prepare creation example, does not deploy/start, and does not execute a full drive input. Next time you actually resolve the record opening failure before pushing through the whole process; the automatic start-up of the suspension rules remains unchanged.

### 2026-11-10 When the master is clear,32612 First defeat.

The owner has clearly responded in this session to "Recover the controlled sniper gun." This time only returns to Longshot's controlled area, and the global stop signage and other weapons are not deleted. Add a new one by example, manise/config Hasi and the current global stop sign of anner-runtime-resume that Hashi bound; only the prefix and precise role of longshot is acceptable, and the new stop tag renders old callback invalid.11 The physical function offline check is passed. This gun runs a separate preparation and two definitive suspensions of the gun/global; old examples are not used.

New instancelongshot-1110-191858 Use current offscreen 1110 Start component and simulator background, original Native 612ac 98e、PGf 2d 673a 5、XR 137c 147 e, percentage 0.4. Actual game 32612／XR 32948 Into the room.1 And epoch.1 The projection was submitted for a letter; the gun was taken but the first shot was empty, the actual shooting inspection failed and no bolt entered.626 Back to the post 101.2497ms；frame 627 Yes 89.7644ms、preparation_budget 0 The trigger count is presmed 1／released 1. Related 'prepare status'8／follow 6"is the state of preparation that was successful in the past, not a subjugation where the frame is failing to prepare.100 ms and 20 ms perform residual; the exact cause of the initial expiry is not proven and cannot be attributed to the new paired lookup (the Bureau did not enter the route).

Operation Script 1♪ Stop normal 0、worker launcher 0, his own game/XR disappeared and was not killed; Native/entries/pad resumed, XR 0. But the new exit readout actually works without fire_disable 2／hand_stop 5, dry=false, launcher passed=false. Two pages full-scale pairing and six original maps are examined by the person, complete double-circuit code 0; the game side has a gun-held image, ending with WAITING FOR VR FRAME, and the simulator mirror continues to be white. It cannot be accepted by submitting a successful claim of actual two-way images or background focus.32612-review.json. There is no running back; the next point is to accurately match the current input access and keep the stand-alone issue of closing the session pending and the white mirror.

### 2026-11-10 55924 The first defeat of the actual initial process and history geometry

First, in the original f.2d 673a 5 The most significant addition to the freeze on PG 16 Line Longshot's existing tick POD record, reverse to remove include 2 function is the same word by word; no new game read/clock/input or retry.2-source-trace-r 1 Full Compiled 0、PG 7af 56 eba. New Independence instancelongshot-1110-192925／game 55924／XR 56344 Still used 0.4 And the original Native 612ac 98e、XR 137c 147 e; last recorded of the old offscreen strategy, which cannot be reused.

The five basic tests are passed.586 Return to current 1、pressed 1／released 0、preparation_budget 1、muzzle_ready 1 It's not a good idea.1／game_tick 37527317 Success.32612 The failure of the initial launch did not recur, and it did not claim that its cause had been repaired. Real bolting, historical chain and co-synthesis passed through, and the actual bolt pushed to 0.200844716；frame 787 PHARE IN STOKE OBSERVATION 3 Aborted, instead of seek/prepare_controls. Current sample failed 65025400 The lease is valid for ns, grip and the lease.110585700 ns, history Context both valid, same_context passed but self-set date 3 A comparison of scale/root positions/ angles has been rejected and is not yet readable by historical transfer. The record contains only the latter Context, which cannot infer a specific sub-point from the latter scale value, let alone simply relax the threshold.

Followup 1 Only in Gate 3 After the decision to reject is taken, output has a ratio, position and angle difference compared with the three original thresholds; no changes have been made to the select, stoke or action logic, no object query or clock has been added. The reverse removal certificate is passed and complete PG compiles 0、4d 3cd 00 d; recorded as the next candidate, not deployed for operation.2 Both the records and historical comparisons were unsuccessful, not counting the mechanisms for repair or acceptance.

55924 Two pages full-scale tectonic and six original maps hand-reviewed, double-track coded 0 The real eyes and hands are visible in this simulator mirror.32612 The white mirror is not reproduced but the root is unproven. The wrist sleeves are still blocking the bolt contact, and the whole GrabPose is not passing.0／worker 0,Native/entries/Pad recovery, XR 0♪ Game gone, no killing; firing stopped reading 2、hand 5, drain=false/launcher passed=false, no history failure.55924-review.json。

### Current back-office entrance update (overriding old pauses and offscreen reuse rules)

The latest owner requests to continue running after the back-office problem is resolved; the unified entrance has archived central stop tags and switchbuild-instance-claok 1110 The gun is marked as runtime-start-passed-archived-cloak, according to the latest agreement.1110...json, does not remove any identity/input/machine/candidature protection or change the status of other guns. The current candidate, Manifest/config, then reprepare is replaced with a new name instead of using the above example of offscreen and not impersonating re-prepared by changing the old proof path. Keep CLOAK and silent, use existing private projection recording if HWND is not used, and do not undiscover the user desktop. Public backstage tools are updated by their maintainer, this gun is no longer shared and is changed to an exception; original examples authorize record keeping as history.

### 2026-11-10 The first CLOAK pre-work process identity denied.

longshot-1110-193813 Reprepare, selection PG, as the latest clak product 4d 3cd 00 d, but not yet when a launcher or game has been created, the working script is retained at "On resource provision service industries".120 Second timeout and attempt stop return 1 No launcher-session, no completed or run-through success.15872 No, the log is not available, the log is explicitly rejected before launcher was created, no launcher log/session/bird, and the offline new classification function checks the real directory to confirm that it failed only before startup; no example of a gun bolt is used.

Independent contrasts to not start the game are repeated from Windows Virtual Environment Python: vvenv Popen PID 43852, the actual code PID 56688;direct base interpreter Popen/code 51268, exit average 0. Not delete identity protection, but then use it as a common C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe直接启动, Numpy/PIL has approved. This gun runs a refusal to execute plusvenv and waits only for “exact worker identity lost” + explicitly above-mentioned pre-start rejection + no evidence of launcher/bird”;9 The offline examination of the real function of a given item is passed without judgement being terminated solely on account of missing logs, missing session, overtime or failure to observe identity. Any launcher evidence continues to be properly closed.193813 The result is not rewritten retroactively. Evidence python-child-education-check.json, prelaunch 193813-review.json and prelaunch-worker-failure-tests.json.

New Longshot-1110-194333 Use the same clak and PG 4d 3cd 00 d, Action script is ready. The direct interpreter portal resource check twice found two actual automatic instances and therefore returned before startup; no new game/XR/entry action for the gun was available and not reused 193813 The current candidate remains ready to continue with the same date after resources are actually released.3 Compare short-term measurements, without resources waiting for markers blocked, without the introduction of back-to-back/sign or fixed scheduling.

The independent case video has also been entered into the gun directory: retained_isolated_media.py only includes a review, exact game identity, terminated exection, code 0 Keep the latest information.3 and 48 Media within an hour, only jpg/png/mp in overtime session 4 List as candidates; json/jsonl/csv/bi-digit and source code are not deleted, or closed or unread directory is not touched. The absolute path of the review for de-checking before deletion is not a symbol link to the current gun catalogue, file is not marked and size/mtime remains unchanged. The temporary directory check is passed and actual dry-run is kept.55280／32612／55924 Three, delete.0; this tool is called after the new video coding has been completed, without accounting for game running resources. The resource check continues to find two real cases occupied, this gun 194333 The offline retention rule is not used to check the progress of counterfeit bolts.

### 2026-11-10 Example of CLOAK 46196 Actual sample first loss and double bone reading

After resource release, longshot-1110-194333 Start successfully with direct base interpreter;game 46196／XR 38852、PG 4d 3cd 00d、Native 612ac 98e／XR 137c 147e、0.4 And clak 1110. Five basic checks passed and the historical raw input chain multiple times 9 Passed, identical framework successfully seen several times.805 Rejected at sample stage, sample_valid 0／sample_gate_cached 2, the last wanted 0.0623586792, not yet valid BoltSample; specific track/matrix/current sub-point is not recorded and cannot be used in empty sample 0／lease 0 Explains that you let go, and you can't fabricated the frame.3 Or a new pair output, so...55924 Geometric items are still unknown. The true whole bolt/load/lock is not passed.

Normal stop 0／worker 0, Native/entries/pat recovery, XR 0,game disappears; fire_disable supplementary reading still 2、hand 5,drain=false/launcher passed=false. Two pages of complete pairing with six original maps for personal review and double-circuit encoding 0 The real eye view is visible; the wrists are blocked and GrabPose does not pass. Four currently reviewed media remain in accordance with the original rules 46196／55924／32612 Cleaning the old ones up 55280 From the dual-output 1543 Media documents;55280 The source code, logbook, original values and parent records are maintained and no longer claim that their original media is still in existence.46196-review.json with Sosolated-media-retention.json.

Offline source check found that the Bolt:: bones only use Root/Bolt two World matrices, but longshot_parts: read also calls MatchRefBone/GetBoneMatrix from Scop. Add read_bolt_pair and only available by Bolt:bones; commonread_part_group default 3 Longshot only allows for display 2, other count/type is refused before reading. Root bone/stagger name, expected index 0／3, function and parameters, matrix limitations, end-of-stage asset identity, metadata and final current check for all retentions; other paths that require Scope are still three times read.6→4, without delay frames, matrices, leases or prescription results and without change to the process threshold.

narrow-band 2 Full Compiled 0，PG 5cf 214 dd;complete-full-bolt-bone-pair-r 1 Compile 0，PGd 7d 85ac 5, two actual read_part_group functions are consistent. Original/new real function 48 Entries offline check through: Default 3 The equivalent, the same results for the first two bones, Scope not involved in double observation, associated name/getter/index/non-limited matrix/altitude replacement/metadata/curent rejection and illegal mode boundary. Engineering/metadata boundary injections cannot claim improvement in actual cost or embolden repair.r In narrow segmentr 1 The combination stopped because of multiple same short anchors; the only anchor near parts_failure was used and failed to retain.1 Template commas in only asert macros lead to the folder's compilation failure, r in parentheses 2 The production code is not changed with the test.

### 2026-11-10 Double bones.57172 Actual results and normal exit sequences repaired

longshot-1110-201126／game 57172／XR 7376 Actual use of narrow segment PG 5cf 214dd、Native 612ac 98e／XR 137c 147e、0.4 Five basic examinations passed, double bones sampled and the original production history chain passed;760 Valid at sampling, Grip/lease valid, age 92261300_, history 9／count 7, and then seek gate 5 The test of the site ownership/current refused, but not yet SetPosition, and the actual bolting is still in progress.0. witdrawal_frame inSeek 759 It's an old cache, not a frame.760 Direct proof; subsequent Native withdrawal frame 760 Clear Reason 5／age 101032300ns／game_difference 0／frame_difference 0 Keep it that way.100 ms, not to say that the specific total cost has improved or that double bones have been repaired.46196 The lack of a valid sample has not been repeated.55924♪ The gometry gate ♪3 Nor has it been repeated, and the old reasons are retained.

Normal stop 0／worker 0, Native/entries/pat recovery, XR 0,game disappears;fire_disable 2／hand 5/drain=false/launcher passed=false still failed. Two pages of complete pairing and six original maps are examined by the parent, double-track code 0 The actual eye view is visible, the wrists are shielded with a bolt; no embolism, single-shot or subsequent locking has been completed.57172-review.json. Original review-select is empty when the code has not generated a page, and the script was initially pressed 2 The page is closed; only two pages of the produced and examined have been re-listed without any actual failure.57172／46196／55924，32612 Oldest Media 600 Item clean-up, source/log/value and review conclusion retained.

The end-source check found that there was a labcher.stop first-off RepositoryLauncher. Stop cancels and then calls Runner.request_stop to immediately send Capture.Stop;The latter may have withdrawn Native's registration first, while run_session finally should have stopped Native before it had been previously funded_session.close.This is the sequence of actual existence in the source code and is not yet the only proven cause of all historical drain failures.This gun is a new request_down_orderly_stop: accurate verification of your identity as index, session_id, owner/game, with launcher canceling the tunnel and then 95 Wait in seconds for the original report to stop directly and keep the original person finely released. The final failure still fails; timeout, event loss, identity change or reported change retention error, then return to existing status.stop and do not change public launcher, Native or PG.11 The offline inspection is passed (no actual sending event), and includes refusal of identity/reporting, non-transmission of Native, failure and termination of neutral path. No physical proof of end-of-pipe repair has been obtained.

Prepare new instance of clak for validation of this sequence 1110-203254 PG 5cf 214 dd, action logic remains unchanged. First, because of insufficient memory availability and then before startup due to two actual instances; no new game or input for this gun. No other example is restarted/modified; current target is advanced until actual resources are available to do a closing/opening validation with specific modifications.



### 2026-11-10 Consistency with skeletal results (not yet validated by game)

Targeted 57172 The actual Seek pre-source is exceeding limit, and the active embolism frame prefixes the original HUD to contact bone reading; then it is only in synchronized call that a single result from the stack is handed over for root/bone sampling. Original input, space, hanger, animation state or sequence mismatches are still refused re-use and removal; ultimately,100 ms and original version of the execution check reservation. There is no cross-clause cache action, extension of time limit or addition to write.

narrow candidated 9adbde 4 Full candidates 99374297 Compiled successfully. Actual helper/native_pose host boundary 73 Item passes; narrow-end status machines 40 Secondary handover, complete version 50 The secondary handover is passed, covering the failure of early HUDs without sampling/dismotion. Host testing does not prove that the game is plugged or performance improved.1110-205714 Use a private GPU projection recording without capturing the desktop or unscreening the window before it is hidden.57172 Actual failure, cleanup fire 2/drainfalse and unaccepted complete reloading/image/GrabPose remain.


### 2026-11-10 Actual 55252: Preparation failed, normal exit to fix first game validation

Independent instancelongshot-1110-205714，PG d 9adbde 4，Native 612ac 98e，XR 137c 147e，0.4、cloak 1110 The actual gun, shooting, shooting.6 The second is empty, the time-recording of the shooting and the preparation of five single-shot resources for passage.766 Left Grip has a gun-tip lease, original animation stopped and entered intoacquiring; frame 767 Control source retreats, page 100139000 ns. No subsequent longshot_reload_geometry, no actual bolting process, no physical cover of the branch that intersected with the frame bone and no claim of repair 57172 The specific sub-conditions for the lack of a follow-up record in the preparatory phase are still unknown.

Normal exit in the original starter sequence, clear the weapon before stopping Native.0、hand_stop=5♪ And the power of the power ♪0、XR 0 The game is gone and no end has been imposed; this is the actual validation, not all of the scenes have been repaired.

Private GPU Projection 410 Frame, code, read it.1 Overview of full time span and 4 A raw eye map. The last part exists.13.42 s with 6.98 S-Image intervals, which cannot be considered continuous images, do not justify the time-out of control sources. Gunshot/hand contact mask, GrabPose complete fit, full opening/bulleting/jet/suffling/reward and mirror internal scaling still pending. Media keep up to date by completed and reviewed records 3-Em,48 hours; source code, number and failure logs are kept. Next positionacquiring phase is a source time limit with the original query boundary without repeating the same whole period test.


### 2026-11-10 Actual 34640 Early shoulder acquisition failed, non-barrel repair results

PG 180 b Compiled and original status/40 The second handover is passed, with the largest increase 32 After acting, acquiring/exposure check/time-consuming logs. A reverse eraser restores the logic of the original game without new game queries, actions or time limits.9a 2 E also compiled and adopted 50 The secondary handover of the original checker has not been stored for temporary operation.

New Independent Examplelongshot-1110-211039 The actual original ground-picking was successful, but did not complete the actual shoulder holding, no firing, base 5 None of the items were adopted; they cannot be borrowed 55252. Log 26 Input baseline, no actualevent access record, and no additional bolt log branch is covered.100 ms hold. Normal cleanup is done again 0/hand 5/drainedtruue, Native/entry restored, XR 0# The game disappears, no forced ending. #

Private GPU 280 The frame code is working. Read it in person.1 Overview of the full time horizon and 4 A primary eye map with long cut-off intervals, without claims of continuous collection or hand-printing.100 ms requests a sync of the GPU intercept on xrEndFrame path; the next controlled attempt will only change the request interval for private recording of copies of this example to 500 ms, retain the same candidate, same action and all identity/refreshness rules. For recording pressure and input continuity tests, recorded relevance is not considered as a proven cause. No analogue or other example is shared.


### 2026-11-10 Actual 4440: Enter a true process, the curve is denied before reading

Independent instancelongshot-1110-212034 Keep the PG on.180b/Native 612/XR 137 Interval with original action, private GPU intercept 500 ms. Five basic actions are passed this time. Add acquiring four times to access crip, bone/curve preparation, HUD preparation and opening; frame 1489 Start the process, time-consuming 16.78 ms, enter initial age 68.30 ms/end 85.09ms。frame 1491 Original SetPosition confirmed success; subsequent physical bone process detected 0.0465875327。

First actual action failed as frame 1499 phase 4/action_gate 2/curve_gate 1，sample_valid=1 The Grip and the lease remain 1 History 9. Selfed_node refused at the first current, but has not run the getter. Failed point input age 98.81 ms, same frame Native backreason 5 Records 99.9975ms，game_difference 0。frame 1498 Old withdrawal and rame 1499 Separated from frame records. Frame.now_ns' separate age is not recorded and cannot be used 98.81 The evidence is that the border was not reached by all hours; the specific sub-condition for refusal cannot exceed the existing evidentiary declaration.12 Sub-paragraph 0 The return, unreadable failure. It cannot be proved that the bone exchange is re-used or the original exit; it is not fully opened and it does not have a bullet plug in.

Regular cleanup for third consecutive fire 0/hand 5/drainedtrue, Native/entry restored, XR 0 Starter 0, the game disappears, no forced ending. GPU projection 96 The frame code is working. Read it in person.2 Overview of the full time horizon and 4 A raw eye map; long-span still exists, arms are blocked and GrabPose is not accepted.34640 The resource conditions are different and this progress cannot be considered as a cause-effect repair of recorded frequencies.

Next step is to reduce the search function metadata that repeats within each bone reading, and keep original functions/parameters/receivers and time limits checked, and call caches without going over the original; subsequent evidence must record Frame and input age separately. The same whole segment does not run again.


### 2026-11-10 Merge candidate with skeletal getter metadata (not running)

Only longshot count 2 Bones are being read, the MatchRefBone/GetBoneMatrix metadata search for each bone is consolidated into a local field over time; no Cache Call from the original version. The original component_getter, parameter layout, MatchRefBone Index, matrix structure/value, front-end assets and current source check is retained. Other weapons and default count 3 Still following the same path. Compile narrow candidates 28fdd 5b 5 Full candidates 5bcf 3524 Success. Actual Args/field/fields each configuration 30(a) The metadata equivalent/rejection/inheritance/same name/case check, two configurations of construction are approved;71 The whole body is read and cross-checked.40/50 Handover through.800 Member Host Folder Name Read 1605 Down 804, does not mean that the game is time-consuming.

Next Actual Controlled Example Longshot-1110-215400 Ready, hold.0.4# It's a private GPU 500 ms recorded. Failed logs add the age of Frame.now_ns and Frade, which are recorded separately from input age, without adding clock or game call. Specific time/routing improvements and complete emboldens remain unverified; complete delivery ratings continue 41.25%。


### 2026-11-10 Actual 9224: getter path contacts, no fresh planning records after video failure

longshot-1110-215400 Actual PG 28f，Native 612/XR 137、0.4# It's a private GPU 500 ms request. Five base action numerical check passed; new merge count 2 Path Generation 18 Bar Contact Positioning Record, Final Frame 835. After manual process 8 No driver in seconds.100 Ms fresh contact records, unmoved left hand to bolt, no entry into acquiring, no new proof of Frame age, no claim of settlement 4440 The real trip was denied.

GPU records only 59 Frames, frames.0.05 to 29.82 Seconds, then 5 Seconds waiting for fresh images to be timed out; event duration up to 69.32 sec, the later action is not covered by a screen. Normal stop writes the wrong record without encoded and cannot be used as a complete video. Read it in person.1 Zhang recorded partial overview and 2 An original map, which does not cover the shooting/breeding, does not count the whole picture. The contact planning failed and GPU was kept separately over time without extrapolation of cause or effect.0/hand 5/drainedtruue, Native/entry restored, XR 0 Starter 0♪ Game disappears, no end to coercion. ♪

Controlled driver has now increased video error protection: stop normal movement at the next live inspection and then move on to normal clean-up; stay the same 5 The seconds are running out.100 ms input/authorization time limits and original actions. Actual generation of protection functions 4 The fact that a check-off has been made (valid/failed, failed/normal on recording, failed to record) does not mean that the GPS timeout itself is repaired. Next example of independent preparation longshot-1110-221517, not yet started. The overview tool will be more recent than the current image 1 The second event is classified as uncovered and no longer included in the contact picture.

Clock caliber correction: the page/new Frame age of the Bolt Motion log uses the clock that was acquired before the incoming, stoke.observe and read_curve, not after an independent failure.4440 It's...98.81 ms should be read as input age before action observation, Native frame retracted 99.9975 These are their own sampling moments; do not take the former as evidence that they were all within borders at any moment of failure.2mm/2 Degrees/Progress 1.5%Check to hold,4440 Dynamic angle error 7.85 It was not the first point of rejection in that period, but did not reach the end; the sampling time series followed up with a genuine results check, without attribution or relaxation.


### 2026-11-10 Actual 55560: Real Process 26%End execute precurrent refusal

longshot-1110-221517 Keep PG 28f/Native 612/XR 137、0.4# It's a private GPU 500 ms requests, add failed action protection for recording. Five base actions are now passed;acquiring 4921—4924 Finish original crip, path, HUD and begin.4926 The original SetPosition was successful, followed by actual bone process detection 0.262122011. First rejection 4958 phase 4/action_gate 5、curve_gate 0；seek_gate 13、prepare_controls gate 20 The final current failed, but has not been setPosittion. Enter age before observation 90.553 ms, Frame Age 71.6923 ms;subsequent Native withdrawal 5、input 100.5893ms、game_difference 0/frame_difference 0. Seek logs witdrawal 4807 It's an old record. Not a book.4958 Direct evidence. Old evidence.4440 Curve Readgate 1 No recurrence, but no overall cost or cause of repair is claimed.

Clean up fire.0/hand 5/drainedtrue, Native/entry restored, XR 0 Starter 0♪ Game disappears, no forced ending. ♪92 Frame, no error, code 0 All action events within image range 1 seconds;to examine in person 2 Pages complete with overview of the extent and 4 A primary eye map. Start black frames, frame spacing, arm shielding, not to mention continuous images/front protection/GrabPose complete acceptance. Record failure protection does not trigger, and does not claim that 9224 GPU timeout repair. Full opening/take/loading/locking, perfect determination and sniper mirror still unplayed, and deliver score 41.25%No change.

Next step is to evaluate the scope of the programme: before consuming this stoke.observe, if there is insufficient time left for implementation of this frame, no original sek, no consumption schedule, no change in historical anchorages and a real fresh Frame continues as a continuous/identity/lease check.100 ms, actual bone endpoint and 4 The sec determines to be maintained, without retrying from the issuer that has been sent or whose results are unknown. The program is only recorded and not achieved.

The complete NATIV interface has a static gap: freeze Nantive e 9 c source code does not contain hand_authored_grass head, Capture.cpp only see V 2 publishing mouth;full PG 5 bc Gun bolt/bold bullet use V 3/V 4 Optionally. V before 2 ♪ We're gonna have to prove it by not proving it ♪3/V 4 Deliverable. No binary export audits or new construction; in logical order, then finely positioned, subsequent access to existing General Consumers and the only C on a precise private baseline 0, re-interface and actual display validation, and not call complete PG compilations full match.


### 2026-11-10 Real fresh frame waiting for candidate (realization, compilation and offline check passed, no game validation)

The original BoltStroke/Path code remains unchanged, running the former observ/observ_continuous once on a stack copy. The original check failed and submitted the original failure; only the original results were moved, no Seek_pending, already existing Frame/entry clocks are in place 100 ms and any age 80 ms (remaining)20 ms or less) returns to the watering without a copy of the process, without updating hisstory::previous, without calling curve reading or original setter. Next real fresh frame retakes source/identity/lease/continuation check; history missing/floating/status changes are still rejected and do not create frame or patch points. The last observation anchor is reserved and cannot be re-concentrated after the later physical position has been lost.

Short candidatesa 2728d 81 Full candidates 81035ba 5 Compiled successfully. Actual old/new tick executed with unmodified BoltStroke/Path 39 Offline inspection: normal behaviour control,80/100 ms boundary, delay not to issue non-consumption anchors, subsequent real observation reservation migration, original leg checks, wrong identity/dismissal/lost history/expired input, undetermined outcome does not delay, actual endpoint 2mm/2 The limit is passed. Engineering and the historical transmission boundary use a mainframe double, not proof of repairing the game.

Disk protection is reviewed by shared maintenance source: prepare/start resource access/store until startup 4 GiB, without prejudice to the running or deleting of files. Follow-up independent preparation records and maintenance of the used soloted_sim.py/runtime_receipts.py source snapshot with Hash to check changes before start-up; no change in old candidate Hashi.106 Cleaning of copies successfully constructed 784 Rebuildable obj/pch 763163534 Byte, DLLHashi, source code, evidence and debug symbol; do not recur to the directory, or touch another directory or process. New instancelongshot-1110-224826 Ready, not yet started, delivery rating still pending 41.25%。


### 2026-11-10 Actual 56280: first bolt Grant is running late before startup, waiting for branch not covered

longshot-1110-224826，PGa 272/Native 612/XR 137，0.4# It's a private GPU 500 Ms records, with source snapshots and 4 GiB disk access. Five basic actions are passed this time. Left hand precisely reaches the bolt, frame 1083 phase 1 Decision-making 1/new_grant 1;same native reason 5 Withdraw, page 100756000 I'm not sure if you're gonna be able to do this.0 Runtime did not enter acquiring, longshot_seek_deferred or actual gun embossing, and then 1084 idle entry_gate 1 Rejects. The specific failure sub-condition of the idle bind is not recorded, and does not extend the ordinary late time limit to the root cause of all inspections.39 The offline inspection passed for physical repair.

Normal closing fire 0/hand 5/drainedtrue, Native/entry restored, XR 0 Starter 0♪ Game gone, no forced end. GPU ♪117 Frame, no error, code 0 Read it yourself.2 Pages complete with overview of the extent and 4 A primary eye map; arm-covered contact, long spaced to be considered complete or continuous. Media keep up to date 3 A copy of the successful completed/review/coded gun record is not deleted from the source numbers and evidence of failure.

Next step to review the Longshot Mechanism for processing priorities in the Backend: existing shooting readiness pre-empts the fresh follow and reload, and reflect/read expenses after first Grant may consume this frame budget. Only when the Longshot component authority is accurate and no outstanding ceasefire/release obligations have been confirmed, priority will be given to the mechanism before the mechanism; other guns and situations requiring priority release of fire remain in order. No old grent resumed, no Grip re-opened, no widening 100 ms; inspection of existing shooting ownership/release interface before implementation, compilation and validation cannot be based solely on the Bureau log 2 Preparation is the only root cause. Full delivery still 41.25%。


### 2026-11-10 Candidate for the sequence of interactive handling of bolts (actual effects to be verified)

Actual 56280 After the first bolt authorization.100.756 ms entered expired and has not yet entered acquiring; the reason cannot be attributed directly to fire preparation. Adding a strictly restricted sequence of processing candidates: only the current Longshot, precise frame valid bolt authorization, right trigger neutral, original R 2 With L 2 When the current fire/targeting/first firing/release/mine debts are known to be loose, operating mode and without any existing ones, fresh follow/apply handle them with the original reload and then perform a full-time shooting preparation and tick on the same frame. The complete functional version also requires that sniper mirrors remain unprocessed. Any unknown, pending release, stop or different weapons remain in order. All movements themselves are re-proved.100 Add a new neutral status to the targeted test of the actual dispatch code; game proceeds are subject to this candidate's physical running certificate.

Current round of validation: narrow range candidate 32 Item . Full candidature 33 entry, and extract actual neutral check and dispatch code from the current working tree 35 The latter uses a double for the movement control border where other original owners already exist, without proving that other weapons are actually being used. Both independent candidates have successfully compiled and Native/XR has not been replaced.4 GiB is not activated; the space recovery is followed by a two-way automatic instance slot.5cf 3f 70 Saved; shared HEAD/index stability claims were not passed during the storage, and after confirmation of personal checkpoint content was consistent with the file. The sharing status has not been restored or overwritten. The game proceeds from processing the sequence are still not proven.

Actual 53500（longshot-1110-232701: Five basic actions passed.875♪ The world is so full of shit ♪1♪ And round_ready ♪0, prefix the frame 1, and then record L 2 Processed, supporting the candidate schedule is executed; no separate branch tags are available to support time-consuming returns. No entry into acquiring, no actual bolt movement occurs and the late frameseek branch is not covered.876 The log input expired 163.784- I can't push you back.875 All subconditions. The bullet resources are in 628→634 Ready.849 The blogger says that the government is not a party to the law.867 Once again,868 The Queen of the Nation is a great city.873 New No.5(a) Substitutes;875 Still not in place. The question should be examined whether the bolt should not be blocked by the resources of the bullet display and the complete resources, inventory, identity and movement verification for subsequent actual recovery/entry should be retained.

Private GPU Recorder for this round 102 Frames without error, maximum spacing 0.756 sec, encode exit 0; personally view the entire process 1 Pages and 4 Original map, arms shield point of contact, no match confirmed. Normal exit: Fire_disable=0、hand_stop=5, drained=true, Native/entry/virtual handle recovery, XR exit 0 The game disappears and no end is imposed. Full reloading, open-screening, and Grab Pose are still not accepted.41.25%The total score is maintained.


### 2026-11-10: Bullet display resources checked in interactive stages

Based on 53500 It's...875♪ The world is so full of shit ♪0 The entrance failed to remove the demand for a idle switch against round::ready/current, moving the same check to the real left-hand bullet entry of watering_round and performing it before reserve_item.The original capture_shoulder itself is ready/current, real left shoulder position, same framemmo authorization and new key check maintained, and the subsequent validation of display and loading remains unchanged.Resource recovery cannot reset rejected bullet-take buttons.The original empty-load inventory, the award of eligibility, the opening time, the original Reloating ownership, animation, the bolt path and the entry time limit check are maintained.The candidate has not changed the strategy of rebuilding after the expiry of his or her resources, avoiding mixing into a second recovery.Two sets of candidates were successfully compiled, each running its original full-time double test and 6 A new resource phase passes; engines and physical bolts are used as doubles at borders, and actual games are not validated.

Supplementary recovery scenario: real inventory is inserted in one shot, old process has been interrupted and closed, displaying resources are not ready and not currently displayed. The original inventory can be restored and closed with new valid mechanical authorization, only once, and reserve only once. Two sets of double tests added to each other ' s original operating cycle 7 A new scene passes. Independent example longshot-1110-233856 Ready;180 The sec-controlled waiting period is not allowed to start without the start.log, Stard.json or workingr. This candidate has no actual game validation yet and cannot be followed 53500 As its game through evidence.


### 2026-11-10 Controlled windows continue to wait for front-line access with hand signals owners

Current round-page candidate is not started; this round is waiting 180 Seconds, wait.240 Second test slots in the period are used for different examples of survival, with less room available for the latter segment of the evidence disk 4 GiB. No external conversation is intervened, no threshold is lowered. The resource life cycle review found that: current in wrapper includes both full life-cycle identity, source prescription and resource availability; existing constructor advance supports ending when time is short, not simply keeping resources across the life-cycle based on changes in the power.

To follow up on the completion of Grab Pose, a pre-source code for the owner to be isolated is also prepared: On ClassPath, which was read and closed by same_path/content, weap_AssaultRifle ancestral by using a cache name to record known/is_lancer and incorporate classification into Same_key and same_context. The minimum changes are reversible to remove the precise freezing source; no access or game call has been increased.18 Actual values/same_key/same_context extraction pass (RAM and type-crossing double), including old/new read call sequence, inheritance/sniper gun/approach/number/fail/visual exclusive/known empty weapon/32 Layer path and identity closed changes. Only the source code and host compilation test are isolated, not complete Native compilation, not V 3/V 4 The current mechanical test candidate and Native 612 Maintain. Follow-up will also require access to the existing generic release and the same C 0 Consumption path, with fixed bone length and established owner protection; no secondary bone writer.


### 2026-11-11：c 985 Freezing of the Native Generic Gear Access Candidate

In exact native-c 985-scope-retention-r 3（e 9cda 3 ae) Transplant saved général V 3/V 4 And the only C.0 Consumption path, generating isolation of Native 05197471 change 7- A file.5 Interfaces/adaptation files; maintenancec 985 Scene owners use the last round of closed changes;0 Generate as a certified model in this frame 15 A reference rotation, which consumes current generic releases.2 Old interface retained, V 3/V 4 No action or inventory permissions, no secondary bone writer.

The entire Native compilation was successful.7 Targeted host mission passed: Practical generic V 4 Fit 49 Check, owner policy 44 Check, old V 2/ New V 3 Longshot left hand C for release and revocation, mixing/release, real reference model 0 Local consumption 815 Check and Native/full PG side V 2/V 3/V 4 Layout comparison. Frame 1416 Bytes, V 21496、V 31512、V 42984；16 The results are consistent. Broker/Core enters the boundary as a double, C 0 Local tables are based on the current PG original; these are not real-time Native consumption, GPU drawing or natural adhesion. Native is not installed or used for games and the current machine candidate is still in use 612ac 98e。

Mechanical instancelongshot-1110-233856 The reading check was free, but the space at atomic start-up was obtained in other cases. Start refused, Stared=false did not generate a workingrker. Keeping this rejection as a quota competition record is not a game failure and does not rename it.

2026-11-11 Controlled resource clean-up: delete only 9 Normal end, restoration of the original Native/entry and exit of XR 0, in the case of old precision game/XR process identities that no longer exist 27 Duplicate DLL/EXE copy, all 112756736 Bytes. Confirms that original build/archive copies exist and are consistent with SHA, removed by the absolute path and mtime/size review, and three recent examples, source code, log, media remain unaltered. Old example names are no longer used. Keep cleanup plan, file-by-file results and scripts. New machine instancelongshot-1111-001703 Ready (Native)612/PGf 11/XR 137/0.4 New Native 051 Not used in this case; the current reading still shows two automatic spaces occupied and no workker created.

Complementing the actual owner adaption test: New Native Actual Automed_grass_owner_live.inl (not just pure strategy) in C 0 Backends Down 20 Example: OFF 1 Example: Cover unknown/invalid context/world, pawn, mesh, wateron and power change denied. Other weapons are turned off from the original Lancer C/E additional owner check, backend closed. Identity conversion and original owner getter is a double, not real-time Scene proof.

### 2026-11-11 Actual 58796 Initial Record Missing

Can you get me a new one?1111-001703，Native 612/PGf 11/XR 137/0.4 Right shoulder actually gets a sniper through; normal.180 The operator stopped the original inspection and did not enter the bolt, take a bullet, load or lock. Neither the repairs nor the late frameseek branch was covered at this stage.10 A boarded longshot_r 2_source records are all triggers 0, containing both presmed/released counts 1 The release and the failure of copy; this cannot be used to prove that all GT readings or precise failures during pulses were made, nor can they claim that the physical original bullet was never launched. Follow-up check for the loss of actual dispatch/source from the first hit; if a continuous trigger is used under restricted restrictions, the form should be classified as different coverage, and this short run failed, which cannot be claimed to have been repaired or relaxed.100 Ms and re-emergence.

63 Frame private GPU recording without error, maximum frame spacing 0.826 sec, encode exit 0; personally view the entire process 1 Pages and 4 Original map, access is still blocked by the arm and not checked. Normal cleanup: fire_disable 0/hand_stop 5/drainedtrue, Native/entry/virtual handle recovery, XR exit 0 The game and the recording subprocess are gone, no termination is imposed.051 This game is not used. The last three after the original media retention strategy is implemented are 58796/53500/56280，55560 Old media removed, values and text evidence retained.


### 2026-11-11: First read-only publisher watch candidate

Targeted 58796 First 180 ms trigger pulse has no authoritative track record, with an optional - first-trigger-observation only read in the first original trigger held/leaved waiting 4 Input and Stereo 4 Map, maximum 64 General Assembly 20 ms at one time, all maps are read-only and mutex is waiting to open the back-to-back check for precise game/publishing device identification and protocol/clock frequency. Record whether the latest releaseer trigger value/sideline and matching stereo instrument appear separately, attached to the command issued before and after the same QPC domain. Matching instruments are diagnostics, not Native action authorizations; no new input orders, no extension of source duration/reset past edges, still request original 180ms/700 ms, abnormal path ensures that the attempt is unwinded. Samples change and are awaiting realization, with actual scheduling effects to be recorded and not considered as a strict matching comparison.1 Keep the source 1, only physical source is accepted without calling 0 and the calibration decoding device, and not re-mark bytes.30 The actual Wire structure and generator operator function check passed, including identity/agreement refusal, expired visible retention, unsynchronized distinction between the publisher and the instrument, original input load and request waiting, failure to release and shut down the reader. No proof of cause or effect of the game.612/f 11/137 New Native 051 Do not load this example.


### 2026-11-11 Actual 56100: Enter the embolism, original history deficit 5060

Private Candidates PGf 11 / Native 612 / XR 137 Yes.0.4 Under Controlled Simulator 5 Basic inspection: shoulder-to-shoulder gun, protobyte echo, after shooting 6 The second is empty and no automatic recharge, the actual firing timed back, and the bullet resources are ready.1287 Get a gun-trap authorization.1290 Get into opening;1292 SetPosition has not been executed because of a lack of continuity in the original history.5059 Current 5065, the only thing back in history 5061..5065; original C++ reads logic offline replaying is equally rejected. All saved points are sstreamed 43/tracking 1/1 Consistently, it cannot be characterized as a proven source switch.5060 The reasons for the missing issue end are still unproven; no patches, no relaxation 100 ms or continuity.

♪ And I'm gonna be a little bit more like you ♪1, so the branch of the game that can be plugged without resources being ready is not covered. Read only the initial monitor actual sample because of an import path error 0 Number of times; amended 31 The field offline check passed, no proof of the game as amended, and no explanation for previous 58796 Short-click failed.

112 Zhang private GPU eye map checked 2 Zhang Panorama and 4 original map.00062 All black, maximum recording interval.9.4534 seconds, arms cover the exposure area; code exit 0 This does not mean that the images or the GrabPose pass. Normal cleanup: Fire 0/hand 5/drained, Native and Access Restoration, XR 0, the game disappears and no forced termination. The whole remains incomplete; complete hand-to-hand reloading, mirror image, grab-and-capture match is not passed. See the record.analysis/longshot-development-20261108/run56100-review.json。


### 2026-11-11 Original history stop point diagnosis (not yet validated by game)

To locate 56100 Missing 5060, add an optional historyBundary diagnostic Pod to remove stop positions, quoted/prior/next, original clocks/stream/tracking and all the original conditions from the same original history package that the reader has copied.100 ms, window capacity, production end and movement process; maximum output following sniper previously decided 8 Line, no new engine calls, reads shared areas or takes time. The default empty pointer is for the rest of the callers and does not expand the original share package.

13 A real Windows mapping offline scenario covers missing records, error sequence numbers, sstream/traking/focus, predicting time, sync interval, reception time, attitude lapses, countback/multitance, key change and normal history; a single-article return history with/without diagnosis is consistent, as the old test passes.2d 1c 3c 99, complete version 16101218 It's compiled, and each one changes.4 A file and reversible. Originalr 1 Log Naming Space Error Retainr,r 2 Revised and compiled 0。013302 Ready; machine resources are full, waiting and not started. Full version is not selected as an example of this bolting; no diagnosis is considered 5060 Missing repair.


### 2026-11-11 Actual 15748: Initial monitors are available and pre-preparation is not operational

PG 2d 1c / Native 612 / XR 137，0.4 Private backstage controlled validation:5 Basic check passed; initial-read-only observation 41 Subsequence, no errors, real source 1 Release record display 5705..5713 Push the trigger.5743 Returned to Pressed/released 1/1. It only proves that this wheel is usable and that the original pulse of this wheel does not explain the previous period.58796 Shortness of the button does not prove a Native consumption licence or performance equivalent.

562 Get a gun-trap authorization.563/564/566 Waiting for return to approach or exceed 100ms，567 Original curve/lock matches completed and delayed HUD preparation. Multiscope copy is not available after that.580 Original current_point 7/current_guard 4 Shows that source_current_on_gt is not passed;581 Re-establishment of baseline;582 Prepare to exit point 4 And clean up. No opening, no bolt setPosittion, no call for historical stoppoint diagnosis; therefore 56100 Missing 5060 The reasons for this are still unknown. The precise non-validation subconditions prior to the current round of preparation for withdrawal remain to be verified and no timer, animation or field is identified as root causes.

96 Zhang GPU projector,1 Zhang Panorama and 4 The original contact map is viewed. Encoding 0,Maximity interval 0.53921 Seconds, arms are still blocking contact. GrabPose is not accepted. Normal fire 0/hand 5/drained, Native/entry/hand handle restored, XR 0♪ Game disappeared, no forced termination. ♪013302(a) Just start a competition for resources, no game and no longer;014829 The media only keep 15748/56100/58796 Recent 3 Copies, text/original history and source code retention. Full reloading/simulation image/hand patch is not complete. Next read 580 Physical failure of source check. No relaxation.100 ms, release and re-energizing.


### 2026-11-11 Remove continuous animation checks during preparation (new candidate, no game results available)

15748 I'm sorry.563 The outer layer of service is ready, and then the second service within Bolt: prepare is in service_gate 4 Returning pending, final source age 100.0721 ms. Disable the original preparatory function to prepare_serviced and independent accessprepare:acquiring only calls from this outer level service back to ready, without an engine call between them. Independent entry is still active. There is no cross-span saving or reuse of service results, input, attitude or permission. Share path check current attribution, inventory identity, stop/play/neutral status and source validity, then execute the original curve, lockdown 2mm/2 degrees, models and paths and ultimately 100-Mms check.

Actual old/new prepare offline authentication with real BoltPath: old double 26 Check the twigs.50 ms source pushed to 102 ms rejected; after new check 76 ms can continue, but geometric reading takes time.101 ms still refuses. Geometric refusal to use examples and 12 New scene passes; action/engine/clock boundary is injected to test, not to mention proof of game performance. The switch and full version of the actual runtime offline change/4 sec boundary/restore/stop 7 Resource scene passed. Root directory test folders lack previous source scope, bone review and new identity fields to directly compile failed record-keeping; common root directory testing/execution after sync 0。

Privately-Kicked PG 6bc 3f 37 f. Complete PG 8f 4af 3a 1 Compile 0, only two product documents changes; Native 612/XR 137 No change. Corresponding changes have been consolidated into two root directory Longshot files, testing the folders to synchronize.021135 Ready, no game has yet been played to prove the preparation phase or complete reloading.


### 2026-11-11 Actual 59140: Launch DoWN has reached the consumer level but no fire

New PG 6bc / Native 612 / XR 137，0.4 The backstage is subject to controlled verification, the shoulder is taken through; no actual ballistic response was produced for the first shot and normal exits were stopped following the established failure. Therefore, the current round did not enter the preparatory process and it cannot be evaluated whether heavy service improved the game preparation time.

Read-only observers 40 Subsampling, no error, original source 1 It's...6808..6816 Record tigger=1. The most important is the actual PG longshot_r 2_source：436 Frame Selection 6809，current=1、pressed/released 1/0、trigger 1、muzzle_ready 1、preparation_budget 1、continuity_matched 1，result 8；437 Frame Selection 6816, same current =1 And still down, rest 2. Actual count 8=blocked、2=accupted. This round can no longer be characterized as a distribution-end without short pressing or consumption-end never seeing DoWN.8 Distinguishing between late pross_ready not satisfied, await_release or original execute returns not_ready.

Code check found that the only existing Input 250 The call-up eligibility is currently given only to the base_is_lancer, and not to Longshot.100 These are valid Frame, publisher/stream/tracking/identity, same count, trigger still held; release is abandoned and the time limit does not restart along the original producer sync. Only the gap to be assessed has been recorded here, and Longshot has not been extended or declared as the root cause of this round.

71 Zhang GPU projector,1 Zhang Panorama and 4 The original map is viewed, coded 0,Maximity interval 0.54904 sec. arm shield still in place, unaccepted grip.0/hand 5/drained, Native/entry/hand handle restored, XR 0♪ Game disappears, no forced termination. Media update 3 Yes 59140/15748/56100; original text evidence is retained. Complete reloading, mirror image and GrabPose are still incomplete.


### 2026-11-11 Existing unsubstantiated waiting eligibility for sniper launch (new candidate)

Sniper guns are now added to the original caller's defer_new_press_for_muzle, and still require current, known binding, identical-to-weapon and hard_gendering, which is immediately banned from use. The root directory retains other existing weapons qualifications, adding Longshot only; freeze candidates only add Longshot. Input, OriginalR 2 Fire and Fire are the same.250 ms original producer time frame, every new 100 The ms are valid Frame, same number/puller/stream/tracking, hold still, release cancelled, and single-issue. No new queue or fireman.

Use it.59140 Record 6809/6816 Serial numbers, count and QPC time constructions with their own source/geometric folder, injected into late-stage press_ready=false and original execute=not_ready: Old qualification was pre-blocked before acceted leak, new eligibility was pre-deferred then only once. It is not actually fully Frame replayed or unproven 59140 Which branch is it? Old/new with the original Input test, all together.2137/2141 Claim passed; Root Directory Actual Qualifications and Input Test 2141 Pass. The initial test using the name of the member with the wrong identity was retained and changed to previous registry_index, which passed.

Most added 16 Read games or time queries without adding a new button.7b 6da 8 dc. Complete PG 97bafad 9 Compile 0，Native 612/XR 137 No change. The first set of full versions was withdrawn before the original baseline did not exist and retained;r 2 The first step is to make it clear that the diagnosis has been compiled and adopted.022826 New examples are ready, and no new game repair or complete reloading conclusions have been made.


### 2026-11-11 Actual 4044: Waiting for the branch to actually be implemented, release correctly cancelled, initial launch still failed

PG 7b 6 / Native 612 / XR 137，0.4 The new diagnosis is clear.420♪ The world is so full of shit ♪1、trigger 1、held 1、animation 0、busy 0、defer_eligible 1、await-release 0, but preparation_budget 0、muzzle_ready 0、press_ready 0，result 10=Deferred, original press call 0。421 Frame Original Trigger 0 And released 1♪ Waiting for cancelled, still pres ♪0; no release to re-issue. This round actually covers the branches and cannot be called initial repair.

420 Original sync 120355758662000，R 2 Record now=120355848328400, enter age 89.6664 ms. There is input_elapsed_ns=4591700 So I'm in R.2, and then enter already 85.0747 ms, left behind 20 ms prepared/sent reserved.100 ms and 20 ms are not relaxed. Next steps will be to locate and reduce the work before firing; source code prepositioned equipment observation, hidden bullet preparation and Lancer starter inspection that is not applicable to Longshot, but there are no causal conclusions for taking over the current budget, which would lead to a presumption of time-consuming or rerun without modification.

Read-only observers 39 No mistakes.67 Zhang GPU projector,1 Zhang Panorama and 4 The original map is viewed, coded 0 Maximum interval 0.54501 sec; exposure is still hidden, not hand-to-hand check. Normal fire 0/hand 5/drained, Native/entry/hand handle restored, XR 0♪ Game disappears, no forced termination. Media latest 3 Yes 4044/59140/15748. Complete reloading, mirror image, close-eye scaling and GrabPose are still not accepted.


### 2026-11-11 Sorting of non-action work before firing (new candidate, no game validation)

Only the exact possession of Longshot, reload runtime free, unachieved isem, right trigger activity and (.)35,1 The current round inventory and the authorization to collect ammunition remain unchanged. The re-loading, having obtained bullets, non-Longshots and ineffective DOWN remains in order. The actual stock disposal status.tick is still before firing, the automatic reloading of the weapon is still ahead of fire; no resource preparation results are considered a firing/takeout permit. Longshot has been confirmed not to enter the original Weap_AssaultRifle Starter test, while other weapons remain on the original Starter path.

Has_item reads only the gert and its prefix statements for the current translation module, avoiding direct reliance on a later defined field. The first version of the field definition after access leads to a failure of compilation,r 1 Reservations;r 2 Quetter compiled 0. The main product is only 4 Change of file, original fire/Input/20ms/100ms、Native 612 And XR 137 No change. Pre-existing diagnostics add up to ready-made frame.now, R.2 Starts/replicates finished/gunshot starts with no additional clock or game query.

Actual offline testing of the nuutral and movement code: bolting 42 Complete 43 Root Directory 45 The scene passed, Ham.10 Adds a new sorting/boundary scene. Former Lancer/Handman with Setem/Active Reload/Invalid Simulation Channel Sort, Manual before Fire, and Resource perform only one check. The root directory extract section contains the existing Gnasher resource boundary, which was initially tested and failed to be compiled; fill in only the mock count and confirm that it is still called once. This is not a practical conclusion for the engine boundary control test.

Open the PG.13546673 PG complete 4792db 37 all;new 024940 No changes to the old example Hashi or protection are made, using current shared start-up source and Hash snapshot (the same first access can have resources like a lockout, but not retry after registration). No game has yet been shown to start/preparate for restoration.


### 2026-11-11 Actual 34720: original set, but no initials were sent and weapons failed

PG 1354 / Native 612 / XR 137，0.4 Controlled rear; shoulder-cuted gun passed, initial ballistics were missing and no preparations for a change of ammunition were made.414 Frame Original 5204 To DoWN, current 1、muzzle_ready 1、preparation_budget 1、press_ready 1；result 3=pressed、press_calls 1、execution_plus_one 2=executed。415 Frame 5210 For UP, result 4=released. This round succeeded once in the original ButtonPress, not to say actually successfully fired.

414 Original input in R 2 Access 49.7051 ms, observation point 52.7718ms；frame.now-source.sync=7.2472 ms. This is the test of this round, not with 4044 The existing log then records the initial_queued certificate of cause and effect of the isolation of A/B.1/parameters 8/synchronous_press 0;starting BeginSate event by step, waiting for log stage 4, no original_fire_divered or shot_trajory. Return to the status quo ante athandoff 61, in combination conditions of original ShouldRefire Args size, Longshot typeed certification or returnValue authentication; specific subconditions are not proven and do not relax it or UP/100ms/20ms。

Important Failure: launcher_passed=false, fire_disable 2、hand_stop 5, Drained=false 2,hand stop after which fire is not passed again. Native/entry/ handle restored, XR 0 The loss of the game process, normal exit by the workingr and no forced termination does not mean that the weapon was passed. The next step is to prioritize the conditions for the cancellation and the start-up sequence, which prohibits rerun without modification.

66 Zhang GPU projector,1 Zhang Panorama and 4 The original map is viewed, coded 0 Maximum interval 0.57684 Seconds, contact is still blocked.3 Yes 34720/4044/59140, the original evidence of text is retained. Complete reloading, mirror image, and close-eye scaling are still not accepted.


2026-11-11 Status function search candidate: Actual field/ Args search for parent categories, not WeaponFring status subfield. Longshot ShouldRefire after de-shooting the first gun is searched from proven original Beginstate; its original function identity, signature,96 Byte functions, return values, actual easing, RefireCheckTime and state check all remaining after return. Class/status search 11 The same number as the root. 11 passage;old, new functions certifying 28 Item, external verification 13 The result corresponds to the number of times you read. Open the lens for PG 31261b 5 d. Full candidature PGEC 90b 4e 0 Compiles, but not yet validated. Private candidate increases the number of failed clauses.34720 The weapons were still being retained for failure and no current round hits/changes/hand type inspections were available.


2026-11-11 Controlled operation connection: Local operators are waiting for first-time access to the existing starter, only available is temporarily busy; the rest of the anomalies are still spreading.5 The line offline check passed.034408 Actual implementation 60 The second is refused for two automatic case slots, no work process or game, and the name is consumed and re-used. Add a new clearly marked 0.6 Second-second first gun to verify variant, default 0.18(a) The seconds are kept word for word;24 The sequence is offline and denied. The variant changes only the duration of a single first shot, with other actions remaining unchanged from product code, not short-fixing. Next real result will still have to be recorded separately.

---

## 中文

# Longshot 狙击步枪需求与技术方案

2026-11-08起草，2026-11-10持续更新。目标工作区为 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`。用户已要求进入开发；本文保留需求、技术方案和后续实现／验证记录。

本文先列可审阅的玩法、范围、手姿和瞄具要求，再列技术与验证方案。各部件尺寸是本项目物理空间的实现初值；最终接触感和头显舒适性仍需实际检验。当前实现进度见第五节，原运行与候选记录完整保留在文末附录。

## 一 玩法需求

### 1 体感持枪与第二握持点

- 主手控制枪的位置与完整旋转，真实枪口方向参与原版射击。头部提供观察和贴镜检测。
- 第二握持点按用户图 1 中左手托住枪身前段的区域设置，具体中心、朝向和范围结合原模型标定。图片支持接触区域判断，三维坐标需从模型与实际持枪变换取得。
- 副手靠近握持点自动吸附，手姿贴合枪体并随枪运动。此时枪由主手控制。
- 副手在有效范围内按 Grip，取得双手控枪权限。副手移动参与现有双手求解，枪口、瞄具和射线使用同一最终武器姿态。
- 松 Grip 且手仍在范围内，回到吸附跟随。物理手离开范围后解除吸附。范围判断持续使用原始跟踪手，避免显示手被吸附后无法离开。
- 食指取弹交互与护木 Grip 控枪分别使用现有语义。重叠范围遵循当前单一交互仲裁，持弹、抓栓和护木控制各自取得明确所有权。

第二握点状态为 `自由 → 接近吸附 → Grip 控枪 → 松 Grip 后吸附 → 离开自由`。按住 Grip 从范围外进入的情况沿用现有输入边沿及反重放约定，并纳入测试。

### 2 所有可交互部件的范围与手姿

Snap Volume 定义接近吸附及对齐区域。Grab Volume 定义真实抓取可取得控制权的区域。GripPoint 定义最终接触变换，GrabPose 定义对应手型。所有部件统一使用已标定的武器局部坐标，并随当前武器变换。

| 部位 | Snap 与 Grab 行为 | 完成条件与反馈 |
| --- | --- | --- |
| 主握把 | 接近对齐，按既有取枪规则取得主手所有权 | 主手稳定持枪，手指与握把贴合 |
| 图 1 第二握持点 | 靠近吸附，Grip 后控枪 | Snap 阶段无副手控枪效果，Grab 阶段双手求解有效 |
| 左肩供弹区 | 复用当前肩取范围和输入 | 一次有效抓取提供一枚实弹，按住期间保持唯一物品 |
| 单枚实弹 | 实弹有抓取范围和捏持姿态，持握后弹体跟随左手 | 弹头方向、物品身份与入膛过程一致 |
| 膛室入口 | 放大的吸附入口接纳持握实弹，内部对齐段引导入膛 | 弹药已抓持、枪膛开放、弹头朝向及插入行程有效后提交 |
| 枪栓把手 | 接近吸附，真实抓取后约束到机构运动路径 | 完整开膛与闭锁端点分别确认，显示当前所需方向 |

#### 2.1 首轮范围配置

下表给出可直接用于首轮实现的参数，新增尺寸均为设计初值。左肩一行来自现有公共供弹实现。长度以玩家交互空间的 cm 表示，盒体填写完整长宽高，球体填写半径。模型局部坐标通过当前有效的世界变换和单位比例转换，当前测试渲染 scale 另行处理。镜片和枪栓中心在模型标定后写入同一配置，范围与可见引导共用该配置。

| 部位 | 接近吸附 Snap Volume | 允许开始抓取的 Grab Volume | 退出与有效条件 |
| --- | --- | --- | --- |
| 主握把 | 以掌心接触点为中心的球，半径 7 cm | 同中心球，半径 7 cm | 未抓取时超过 9 cm 退出 Snap，持枪后服从既有主手释放规则 |
| 图 1 第二握点 | 沿护木轴的盒体，长 16 × 宽 12 × 高 12 cm | 同一盒体内按 Grip 可抓取 | Snap 保持盒为 20 × 16 × 16 cm，超出即退出；Grab 期间超过保持盒撤销副手控制，恢复需新有效抓取 |
| 枪栓把手 | 随把手运动的球，半径 5 cm | 同中心球，半径 5 cm | 未抓取时超过 7 cm 退出；抓取后手与约束路径的垂直偏差超过 7 cm 撤销控制 |
| 单枚实弹 | 沿弹轴的胶囊体，中心线长等于实弹实际长度，半径 3 cm | 同一胶囊体 | 未持握时半径超过 5 cm 退出；持握后沿用物品释放规则 |
| 膛口与持弹接触 | 膛口向外的圆柱，半径 6 cm、轴向长度 8 cm，弹头进入可对齐 | 已持有实弹的抓取延续进入同一范围 | 对齐保持区半径 8 cm、外向长度 10 cm；枪膛开放且弹轴夹角 ≤30° 才吸附 |
| 膛口最终入座 | 同轴引导段，对齐后横向误差 ≤1.5 cm、弹轴夹角 ≤10° | 使用当前实弹租约 | 原始弹头先在膛前至少 2.5 cm，再真实送至膛口轴向 ±6 mm；连续性 ≤100 ms、速度 ≤3 m/s，最多提交一次 |
| 左肩供弹区 | 复用公共椭球，默认左右半轴 20.45 cm、上下及前后半轴各 24 cm | 复用相同公共命中区与肩取输入 | 横向半轴随身体布局为 16–22 cm；保留身体中线排除及原有左右手选择 |

左肩默认椭球完整尺寸为 40.9 × 48 × 48 cm。半轴计算来自 `body_slots.h` 的 `min(22, max(16, armspan_cm × 0.135 − 2.5))`，默认臂展布局为 170 cm。其中心使用已有 `shoulder_ammo_center` 和当前肩取参考系，跟随现有身体校准。肩区提供物品生成位置，取出实弹后立即应用实弹 GrabPose。[公共身体槽位代码](../tools/vr-common/body_slots.h)

主握把、护木和枪栓用未吸附的物理手接触锚点判定范围，实弹抓取使用拇指与食指捏持锚点，入膛使用弹头位置与弹轴。控制器 Grip Pose 到这些锚点的变换由当前手模型提供。肩取继续使用公共路径的原始左手 Grip Pose。

进入边界使用小于等于，退出使用严格超出保持范围。获取时验证手部朝向及可接近侧，主握把、护木、枪栓建议接近方向容差为 60°。吸附显示复用公共C0的120ms过渡，有效Grab按下当帧即可进入权限检查；早期草案80ms建议由此统一。过渡仅处理显示，接触判断持续使用原始跟踪；连续来源中断超过100ms后，旧吸附保持资格失效。

#### 2.2 重叠与动作连续性

有效持有物品或部件的租约保持优先。持弹时膛口是当前插入目标，开膛和闭锁阶段枪栓是当前空手目标，普通持枪阶段护木是副手目标。剩余同优先级候选按到其接触锚点的归一化距离选择，完全相同时用稳定部件顺序确定。已选中的 Snap 目标在退出保持区或步骤改变后才切换。

抓住枪栓后，手沿原机构运动路径推动状态。轨迹外偏差超过表内容差、松开或失追时撤销抓取，实际机械状态留待重新认证。旋转段和直线段各自具有标定端点，当前机械回执要求实际骨骼相对目标的平移误差 ≤2 mm、转角误差 ≤2°，完整经过必要轨迹后确认端点。物理手单帧跳跃及旧姿态不能跳过行程验证。

持续抓着子弹进入膛口即可完成对齐和插入。完成后释放这枚物品的手部所有权，按键需遵守现有重新抓取规则。空膛表面保留交互目标及状态反馈，装弹操作由手中实弹驱动。闭膛、反向子弹或隔着枪体接近时显示当前条件。

#### 2.3 每个实体部件的 GrabPose

所有参与交互的实体部件均需独立的接触锚点、掌部朝向、五指姿态和前臂接续。手骨沿用固定骨长及已有唯一手部消费者。Snap 显示接触预览，Grab 显示受力握姿，两者使用相同的真实表面基准。抓取范围放大后，最终接触点仍落在模型表面。

| 部位或阶段 | 必须制作和检查的贴合 |
| --- | --- |
| 主握把 | 掌心贴背部握面，中指、无名指、小指绕握把，拇指落侧面，食指随真实扳机状态落在扳机附近 |
| 第二握点 | 按图 1 托住前段，掌根承托底面，四指包住侧面、拇指自然落位；Snap 与 Grip 受力姿态都需检查 |
| 枪栓开膛 | 拇指与食指包住把手，其他手指避开机匣，腕部跟随原枪栓旋转和平移 |
| 枪栓闭锁 | 使用闭锁方向适配的把手抓姿，检查完整回推及旋转终点的手腕、指节和袖口 |
| 左肩取出实弹 | 抓取成功时以拇指和食指或中指捏住弹壳，弹头指向可自然转入膛口的方向；肩区过渡采用同一实弹手型 |
| 实弹携带 | 捏持点固定在弹壳实际表面，随左手完整旋转，手指持续保持与同一枚子弹的接触 |
| 膛口对齐与插入 | 从携弹姿态连续转为送弹姿态，指尖与弹壳贴合；入座后解除物品约束，手返回自由或枪栓候选状态 |

首轮视觉验收目标为主要接触点表面间隙 ≤3 mm、局部轻微穿插深度 ≤2 mm。完整手掌、指节或袖口明显穿入枪体判失败。上述阈值是拟合目标，需结合实际三角面和可见皮肤变形复核。其余手指须自然弯曲，枪托、镜筒、机匣和袖口保持合理空间关系。

每套 GrabPose 从掌侧、手背侧、拇指侧、前后端检查近景，并覆盖吸附、抓取、最大拉栓、最终闭锁和释放。手姿采用连续过渡，骨长、手腕和前臂保持一致。所有部件均通过实际可见贴合检查后，才能记录本枪完整 GrabPose 验收。

手型配置必须随同一部件在 Snap、Grip 控制、松 Grip 回 Snap 和渐退之间保留。双手求解可以更新当前握点的世界变换，但不能重新创建一个只含握点的目标，丢掉原部件的五指姿态。第二握点的左手专用手型只作用于左手；右手继续使用主握把的手型和真实扳机变化。定制手型缺失或绑定失效时撤销该显示目标，保留原始跟踪与既有机构状态。

#### 2.4 膛口吸附、送弹与手姿的同一结果

膛口外向获取圆柱半径 6 cm、深 8 cm，已吸附保持圆柱半径 8 cm、深 10 cm；弹头最多越过膛口平面 1.5 cm，原始弹轴夹角 ≤30°。以上为物理尺寸，枪模型缩放不扩大或缩小实际交互范围。只有开膛、本轮唯一实弹和原持弹租约都有效时才启用。

对齐用 120 ms 过渡帮助横向位置和轴向朝向，保留玩家当前原始弹头的轴向深度。显示子弹、精细捏持 Grip 目标和入膛判断共用同一个当前真实帧的计算结果。手姿通过现有唯一 C0 消费者发布，原控制器／原始手姿继续用于范围与真实运动判定；不能从已贴合显示手反向读取“玩家已送弹”。

入膛须同时满足获取／保持范围、真实接近再送入、最终对齐和原库存事务检查。吸附过渡或退场渐变不能自行完成送弹；弹头静止在入座点也不能生成装弹。实际送入的一枚子弹只提交一笔事务；显示撤销与库存退还由各自原所有者处理，隐藏模型不代表库存已恢复。

松开、来源失效、参考空间／输入代次／武器／物品／租约改变时撤销当前辅助；旧接近进度不能跨代次沿用。相邻原始源超过 100 ms 时不沿用较大保持区，必须重新进入获取区。离开保持区可渐退显示，但渐退期间无入膛许可。

#### 2.5 玩家逐动作与反馈明细

以下表格规定玩家实际操作。范围获取、输入边沿、当前物品身份和阶段条件都有效后才能进入下一状态；吸附预览本身不提交机构或库存动作。

| 玩家动作 | 允许发生的变化 | 当前可见反馈与下一步 |
| --- | --- | --- |
| 空左手进入第二握点获取盒 | 左手按对应 GrabPose 靠向护木，右手继续独立控枪 | 显示当前可握部位；轻微移动左控制器不改变枪口 |
| 在第二握点按下有效 Grip | 保留同一左手手型，接入双手控枪 | 托枪姿态持续贴合，左手移动开始参与枪向求解 |
| 松 Grip，原始左手仍在保持盒 | 撤销副手控枪，保留接近吸附 | 枪重新由右手控制，左手仍贴护木；移出保持盒后渐退至自由手 |
| 射后把空左手移到栓柄 | 只吸附到当前栓柄位置，显示开膛手型 | 开膛方向仍显示；未抓住时枪栓不因显示手靠近而移动 |
| 在栓柄有效范围按 Grip 并拉动 | 在现有部件租约下沿原模型轨迹控制枪栓 | 显示尚未完成的旋转／平移方向；实际到开膛端点后才提示取弹 |
| 左手伸到公共左肩供弹区，执行与 Lancer／兽族步枪相同的取弹动作 | 取得一枚狙击枪实弹，立即使用该实弹的捏持姿态 | 显示膛口和送弹方向；持续按住或重复读取同次按压不会多生子弹 |
| 捏持实弹移入膛口获取圆柱 | 显示实弹和左手一起对齐，保留原始轴向深度 | 弹轴不合适时显示纠正方向；入座前库存不提交 |
| 实际向前送弹，完成规定接近与入座行程 | 同一枚实弹至多入膛一次，备弹扣一，解除物品手部约束 | 膛口步骤结束，切换到闭锁引导；枪仍不可射击 |
| 抓住栓柄回推并完成闭锁 | 认证完整机械端点，按本发实际射击至闭锁的时间判定结果 | `<4.000 s` 完美，`≥4.000 s` 普通；确认原版结果后可重新射击 |
| 抓取中松手、失追、切枪或暂停 | 停止本次部件控制，保存可认证的机械／库存结果，释放当前显示目标 | 恢复后显示实际尚缺步骤，重新接近并按有效抓取；不能由旧吸附或旧按压跳过步骤 |

范围边缘测试需分别覆盖刚好进入、刚好位于保持边界、越界后重新进入，以及按住旧 Grip 从外侧移入。所有显示范围与实际判定共用物理尺寸配置。首轮范围可调，调整时同时更新文档、配置和对应边界用例。

### 3 手动装填流程

1. 装弹并闭锁时可正常射击。原版确认实际射出一发后，记录本发唯一身份和计时起点，进入待手动换弹状态。
2. 显示枪栓开膛引导。左手离开副握点、抓住枪栓，按实际机构轨迹开膛。
3. 开膛到位执行对应退壳视觉步骤，显示左肩取弹引导。
4. 左肩按当前肩取规则取出一枚完整实弹，显示膛口轮廓和插入方向。
5. 持握实弹进入放大的膛口区域，吸附对齐并完成入膛。完成一次真实库存提交，备弹减一、膛内装入一发。
6. 显示闭锁引导。左手抓栓完成前推及模型所需锁定动作，确认机械完成时刻。
7. 原版完成回执确认可射状态及换弹结果，退出引导，可重新握住第二握点射击。

枪栓的旋转、平移及闭锁顺序以原模型和动画轨迹核定。现有记录显示 `b_LS_Bolt` 有旋转轨迹，开发时需完成复合动作标定。

射后等待、空枪扳机、收枪再取枪等入口均须保持手动补弹规则。库存已装入但枪栓尚未闭锁时保持射击门控。初次拿到空枪时允许普通装填，本发射击计时需来自实际开枪事件。

### 4 完美换弹

令 `t0` 为本武器原版确认实际开火的游戏时刻，`t1` 为本轮完整装填后枪栓闭锁完成的游戏时刻。`Δt = t1 − t0`。

| 情况 | 结果 |
| --- | --- |
| 全部机械步骤及库存提交有效，`0 ≤ Δt < 4.000 s` | 请求原版完美换弹奖励，确认原版结果 |
| 全部机械步骤及库存提交有效，`Δt ≥ 4.000 s` | 普通换弹 |
| 缺少本发开火起点 | 普通装填 |
| 插弹成功但枪栓未完成闭锁 | 继续闭锁引导，保持不可射 |
| 重复按键、重复闭锁通知或重复完成回执 | 本轮结果及奖励保持至多一次提交 |

计时终点取真实机械完成时刻，异步回执延迟另行记录。达到 4 秒后仍可继续装填。再次抓栓、再次取弹和重新进入交互范围沿用本发起点。

暂停计时拟采用原版游戏时钟语义，真正暂停期间冻结。失追或切枪中断当前交互租约，恢复时重新认证机械及库存状态，并保留可认证的原开火起点。原记录无法可靠关联时按普通装填恢复。该中断策略是方案建议，需在实际适配中核验。

### 5 GearVR 通用交互引导

进入换弹流程即显示当前一步的部件高亮、方向箭头和必要短提示。规则适用于 GearVR 武器交互，本枪接入现有引导通路。

| 当前状态 | 引导内容 |
| --- | --- |
| 射后空膛 | 枪栓高亮与开膛方向 |
| 已开膛 | 左肩供弹提示 |
| 已持弹 | 膛口轮廓、实弹插入轴 |
| 实弹角度不合适 | 对齐方向提示 |
| 已入膛 | 枪栓闭锁路径及终点 |
| 备弹耗尽 | 缺弹反馈，沿用真实抓取一次反馈规则 |
| 换弹完成 | 短暂普通或完美结果，关闭本轮步骤引导 |

引导随目标部件运动，双眼可读，避免手和枪遮住全部信息。通用语义从现有 Lancer 引导复用，具体轮廓、位置和路径由 Longshot 几何提供。所有权撤销、暂停和切枪时同步处理引导生命周期。

## 二 狙击镜方案评估

### 1 镜内放大的目标体验

- 放大场景限定在实际瞄具镜片孔径内，镜外维持正常 VR 世界视野。
- 镜内保留原版放大倍率语义、分划和对应瞄准状态。倍率数值从认证的原版配置或实际状态核定。
- 眼睛位于目镜后方、距离与横向位置合适时看清镜内。偏离时出现合理遮黑和可视区域变化。
- 左右眼分别计算可见性，支持自然选择观察眼。镜框、枪体和世界遮挡使用一致深度关系。
- 准星与原版权威射线核对，覆盖近处遮挡、远目标和双手移动。保留原版散布、伤害及后坐力规则。

本方案将上述体验作为项目验收要求。OpenXR 提供视图、位姿和图层提交约定，应用负责镜片图像及武器交互实现。双眼渲染与提交须使用匹配的视图位姿和空间。[Khronos 帧提交指南](https://github.com/KhronosGroup/OpenXR-Guide/blob/main/chapters/frame_submission.md)

### 2 A 路径 镜片内呈现原版放大场景

优先目标采用独立的镜内场景图像，再经镜片孔径、深度和当前眼位合成到主双眼画面。先检查现有原版开镜场景是否能在同一帧获得独立可复用纹理，同时保留普通双眼场景。若当前渲染只有一套相机结果，则需要在已有 Native 渲染所有者内增加受控镜内场景 pass，并复用原版倍率及分划逻辑。

现有 `weapon_longshot_scope.h` 已提供 eye relief、瞳孔重叠、双眼观察、独立相机视锥、遮挡证明及同帧键校验的计算基础。其输出是渲染计划，仍需接入原游戏场景绘制、镜片表面、GPU 资源及回执。

关键工作包括实际目镜变换、镜片孔径标定，镜内视图及深度资源，倍率与原版瞄准状态映射，分划合成，左右眼遮挡，以及视图历史隔离。工程风险集中在额外场景渲染与原有双眼历史、资源生命周期和帧预算的结合。

验收需看到镜外正常场景与镜内实时放大同时存在，双眼镜片边缘正确，头动和枪动分别响应，准星与实际开火结果一致。新增镜内绘制的 GPU 帧时及资源开销与同场景基线比较。

### 3 B 路径 贴眼切换原版瞄准画面

这是用户允许的交付回退方案，当前代码复用程度更高。眼睛进入目镜后方有效观察区时，触发原版开镜路径，保留原版放大画面与镜框。眼睛离开、放下武器或进入装填时退镜，回到普通第一人称 VR 画面。

进入条件联合检查实际持枪身份、当前眼部跟踪、目镜前后方向、横向偏差、眼距和朝向。首轮进入参数为眼距 4–10 cm、横向偏差 ≤2.5 cm、视线夹角 ≤15°，连续满足 80 ms。退出参数为眼距小于 3 cm 或大于 13 cm、横向偏差大于 4 cm、夹角大于 22°中的任一项。失追、暂停或身份失效立即退出。距离按实际观察眼相对目镜平面计算，左右眼分别检测，当前观察眼优先保持至退出。这些数值属于可调原型参数，需根据模型、控制器接近头显的空间及头显反馈调整。

切换仅处理观察模式。枪械控制与开火仍使用同一武器姿态和当前权威枪口。原版全屏开镜路径需要额外核实头动缩放、双眼镜框、准星方向和瞬间视图跳变。出现不舒适的全屏缩放时，评估在原有固定空间显示通路中承载原版瞄准图像的形式，并明确记录实际交付形式。

现有 2026-10-22 修复记录包含原版镜框双眼绘制及 `bZoomed` 相机选择，适合复用。该记录的头显放大效果、开关镜连续性仍待本枪验证。[镜框修复记录](sniper-scope-vr-exit-20261022.md) [第一人称修复记录](sniper-first-person-20261022.md)

### 4 贴眼时 R3 切换原版倍率

用户指定右手柄摇杆向内按下的 R3 映射为原版开镜后的缩放操作。每个真实按下边沿执行一次原版倍率切换，具体倍率档位、循环顺序及收镜后的倍率保留行为沿用认证的原版逻辑。

两条瞄具路径共用贴眼门控，采用上一节的进入与退出参数。A 路径还需镜内视图当前有效，B 路径还需原版瞄准上下文和画面已确认。倍率切换前后均读取实际缩放状态。枪械身份有效、眼部与控制器跟踪有效、当前无暂停菜单、无装填交互时才允许执行倍率操作。

| 输入情形 | 路由规则 |
| --- | --- |
| 已进入有效贴眼瞄准，按下 R3 | 本次边沿由倍率消费者独占，向原版缩放入口分发一次 |
| 已贴眼，原版开镜或镜内图像仍在准备 | 该次 R3 在瞄具上下文消耗，图像有效后重新按下可操作 |
| 按住 R3 | 保持本次按压归属，后续帧无重复倍率切换 |
| 在远离眼睛时按住 R3，再靠近目镜 | 本次按压保持原归属，松开后在有效贴眼状态重新按下才能缩放 |
| 缩放按压期间离镜、切枪、失追或暂停 | 撤销执行许可，本次按压保持已消费至释放；恢复后需要新边沿 |
| 镜外普通 R3 操作 | 使用既有输入映射；Longshot 装填由物理动作入口驱动 |
| 暂停菜单中的 R3 | 由菜单输入上下文处理 |

倍率边沿在现有统一输入路由中先确定唯一消费者，再执行动作。贴眼倍率接管时，原有 R3 换弹、视角切换和组合键检测均识别同一次消费记录。离镜后也以真实松开作为该次按压的结束，防止原 R3 功能在按住途中补触发。

既有双摇杆菜单组合键保留菜单优先级：命中组合键或已经进入菜单时不切倍率。仅靠近眼睛但尚未完成有效眼盒停留，也不能把镜外已按住的 R3 变成一次倍率操作。

短提示显示在瞄具附近或原版镜内 HUD，贴眼有效时显示“按右摇杆切换倍率”，倍率切换成功后短暂显示原版实际倍率。无认证倍率读数时仅显示操作提示。切换结果通过原版状态回读确认，镜内渲染读取同一确认后的倍率。

### 5 推荐决策

镜片内实时放大作为首选目标。实施开始时用现有渲染代码和已保存证据核定独立镜内图像入口、资源归属及帧预算，再做一个最小镜内输出验证。如果获得独立放大图像需要大范围重构渲染链，则本轮采用 B 路径完成贴眼开镜和完整手动装填闭环，记录 A 路径剩余工程量。

按当前源码可判断 A 的主要缺口在实际渲染接入，B 已有原版显示链基础。两条路径的具体工作量仍取决于当前原版开镜相机与资源流核验。舒适性要求关注自然头动响应、信息遮挡及合理空间位置，具体眼盒参数由本项目验证确定。[Meta 舒适性指南](https://developers.meta.com/vr/design/comfort/)

## 三 现有基础与实现改动

### 1 已核对的基础

| 位置 | 当前内容 | 本次适配 |
| --- | --- | --- |
| `tools/vr-common/weapon_longshot_reload.h` | 单发库存、步骤回执、左右手身份、完成提交及引导描述 | 从实际开火计时，采用严格小于 4 秒，接真实机械步骤 |
| `tools/vr-common/weapon_longshot_reload_test.cpp` | 现有纯状态机测试 | 新增本次计时、恢复、库存边界 |
| `tools/vr-common/weapon_longshot_scope.h` | 独立镜内场景光学规划 | 实际 Longshot 参数、产品渲染接入或贴眼检测复用 |
| `tools/vr-common/weapon_longshot_scope_test.cpp` | 使用合成参数的光学测试 | 实际标定配置、模式切换和渲染端验证 |
| `tools/playground/weapon_lancer_manual_reload.inl` | 原版自动补弹阻断及调用许可框架 | 核查 Longshot 子类入口并接入专用适配 |
| `tools/playground/weapon_unified_fire.inl` | 统一真实枪口开火通路 | 验证 Longshot 类与资产接线，接已确认射击事件 |
| `tools/playground/weapon_native_lancer_follow.inl` | 当前持枪和第二手跟随接入点 | 通过专用 Longshot 配置提供握点 |
| `tools/vr-common/weapon_lancer_interaction_guide.h` | 分步引导几何及状态模式 | 复用语义，提供本枪轮廓与动作箭头 |
| `assets/weapon-6dof/offline-v2/longshot-round.glb` | 已有完整实弹资产 | 检查单位、抓持点、入膛朝向并接运行时物品 |

本轮已将原 `begin_r3` 换弹入口迁移为 `begin_manual`，使用物理开始边沿，并以认证实发事件的游戏时刻为起点，严格 `< 4 s`。状态机要求完整五步语义顺序及最终闭锁，原有测试调用已迁移。实际射击事件提供者、空膛自动补弹阻断与原版换弹状态取得已在游戏触达；枪栓、单发取弹、插弹及闭锁适配已有源码和离线测试，完整实际机械流程尚未通过。

R3 倍率适配通过现有输入通路取得真实右摇杆点击边沿，保存该次按压的消费者归属直到释放。原版倍率入口、档位读数及调用回执在实施时核定。A 路径更新镜内相机的倍率配置，B 路径使用原版瞄准画面的实际倍率；两者共用贴眼状态及输入消费记录。

原模型工单记录 Root 9962、Bolt 894 三角，Scope/LensCap 在该 LOD0 缺少独立完整面。开发应从实际可用几何绑定镜片区域及枪栓，使用资产已有内容确定接触配置。[模型工单](weapon-specs-20261028/model-work-orders.md) [既有 Longshot 规格](weapon-specs-20261028/weapon-cards.md)

### 2 运行时数据流

`当前跟踪手与眼位 → 武器局部几何候选 → 现有交互仲裁 → Snap 显示或真实 Grab → 枪栓及实弹状态机 → 原版库存与完成事务 → 引导及手姿显示`

真实射击回执另外向本武器的换弹记录写入本发起点。镜内视图读取同一最终武器变换与当前眼位，镜内显示与射击消费者共同校验身份及帧来源。

实现优先增加 Longshot 专用握点、枪栓几何、装填适配和瞄具适配文件。公共消费者只增加必要的已认证分派，手骨与武器姿态通过现有唯一执行点消费。

### 3 自动换弹阻断与库存事务

- 对当前玩家实际拥有的 Longshot 实例核定 `ShouldAutoReload`、`CanReload`、`Reloading.ForceReload`、原版补弹和退出路径，覆盖装备、收枪和重新装备。
- 在既有 Native 调用所有者内按武器身份及调用语义施加手动换弹策略。真实手动流程取得有限许可后执行原版必要动作。
- 取弹阶段预约一发备弹。插入完成后提交一次 `loaded 0 → 1`、`reserve R → R−1`。物品身份、库存版本及提交令牌对应同一次操作。
- 原版自动定时器到达时仍须满足手动事务要求，防止再次补弹。原版完美奖励也需要唯一完成令牌及确认回执。
- 提交前释放实弹可解除预约。已提交库存后发生失追、切枪或回执超时时，读取原版实际库存与机械状态进行对账，保持已发生的结果。
- 长期按住输入、重复调用及延迟回执均需测试。零备弹沿用既有缺弹反馈及去重行为。

### 4 开发顺序与检查点

1. 核定原模型握点、膛口、枪栓轨迹、实弹资产和现有原版狙击镜资源入口，冻结本枪几何配置。
2. 完成主手与第二握点 Snap/Grab、同源枪口开火及实发事件计时。
3. 完成自动补弹阻断、左肩单发供弹、持弹入膛、枪栓闭锁、4 秒结果和全步骤引导。
4. 根据镜内输出评估落实 A 或 B，完成准星与实际射线校准。
5. 执行新增和受影响的离线测试，编译并冻结私有候选身份。
6. 在现有受控运行窗口执行针对性模拟器验证，检查实际双路图像，保存结果及清理记录。
7. 交付已标识候选、最多三组头显操作和日志位置，由用户验证握持舒适度、贴眼体验与自然换弹。

## 四 测试与受控模拟器验证计划

### 1 自动测试

| 测试组 | 必须覆盖 |
| --- | --- |
| 第二握点 | 靠近吸附、主手移动时跟随、无 Grip 时副手无控枪效果、Grip 控枪、松手回 Snap、原始手离开、按住进入、失追与换枪 |
| 部件几何 | 世界与武器坐标转换、完整旋转、实际模型 scale、各范围边界内外及精确边界、保持区滞回、膛口宽入口、重叠仲裁、错误侧接近 |
| GrabPose | 主握把、第二握点、枪栓开膛与闭锁、实弹取出与携带、送弹入膛逐套检查；验证接触变换、固定骨长、手腕连续及实际多角度贴合 |
| 单发装填 | 一次肩取一个物品、按住去重、实弹正反向、膛闭拒绝插入、持握连续入膛、重复插入、提前闭栓、真实完整行程 |
| 库存 | R=0、R=1、多备弹、取弹后取消、插入前失败、插入后回执失败、重复完成、收枪再取、总量守恒 |
| 完美计时 | 3.999 s 完美、4.000 s 普通、4.001 s 普通、无实发事件、空击、重复射击通知、计时回退、重复抓栓、跨暂停及切枪恢复 |
| 自动换弹 | 射后静置超过原换弹时间、连续扣空枪、原自动入口、收枪期间定时器、手动完成后旧回调 |
| 瞄具 | 目镜前后、距离和横向边界、左右眼、阈值抖动、失追、原开镜失败、退镜清理、枪轴与镜轴映射 |
| R3 倍率与冲突 | 贴眼点击一次、长按、连续点击、镜外点击、镜外按住后贴眼、按住离镜、开镜准备中点击、菜单与组合键、失追恢复、倍率回执失败和重复回执 |
| 渲染 | 镜片遮罩与遮挡、左右眼变换、近远目标、资源状态恢复、视图历史、超时或无效帧退出 |
| 引导 | 每个装填阶段立即给出正确目标、步骤切换、缺弹、暂停、切枪、完成后退出 |

优先扩展已有 Longshot 两个测试文件，并为实际适配新增必要的模拟回执测试。测试入口需明确登记与运行，保留断言和失败输出。集成用例经过产品正常调用入口验证输入、库存及原版事件。

### 2 实际模拟器检查

使用已购游戏及既有私有包装器，沿用当前受控 profile、后台窗口、observe-only、测试静音和物理输入隔离。启动前核实运行窗口归属，既有工具负责身份、新鲜度、配置恢复及正常关闭。所有动作使用同一产品实现。

| 场景 | 操作与实际证据 |
| --- | --- |
| 握持与射击 | 取枪，副手靠近但不按 Grip，移动双手分别观察，再 Grip 双手瞄准并实际打一发，核对枪口、trace、弹药和可见手姿 |
| 自动补弹阻断 | 射后等待超过原版换弹时间，扣扳机并检查空枪，核对 loaded 持续为 0 |
| 普通换弹 | 等待超过 4 秒，开膛、左肩取一发、持握入膛、闭锁，确认普通结果后实际再射 |
| 完美换弹 | 新一发后在 4 秒内完成全套动作，核对实发与闭锁时间、原版奖励及下一发实际结果 |
| 库存及中断 | 有限备弹最后一发、零备弹、持弹取消、未闭锁尝试射击、换弹中暂停及恢复，检查对账与引导 |
| 瞄具 | 近镜、离镜、横向偏移、左右眼观察、头动与枪动、近远目标开火，检查选定 A 或 B 的实际图像 |
| 原版倍率 | 贴眼后逐次按 R3 遍历原版倍率，核对画面、状态和单次回执；长按、镜外按住再贴眼、缩放按住再离镜分别核对输入归属及冲突 |
| 全部 GrabPose | 保存主握把、第二握点、取弹、携弹、送弹、开栓与闭锁各阶段多角度近景，核对接触、穿插、袖口和手指变化 |
| 收尾 | 暂停恢复、正常结束，核对 Native、入口、XR、游戏、编码状态和配置原始字节恢复 |

4 秒严格边界由离线确定性测试覆盖。实际模拟器分别执行明显位于边界两侧的动作并记录真实游戏时间，避免将脚本等待时长直接充当完美判定证据。出现具体失败时只修复并复查受影响动作。

每个候选记录源码与二进制身份、实际动作、首个失败、库存及时间回执、完整保存跨度双路概览和关键接触近景。视觉验收需检查膛口入弹、枪栓闭锁、手姿与镜内放大，沿用现有媒体保留策略。

### 3 完成条件

本枪交付需同时具备产品接线、已运行测试、编译候选、实际模拟器玩法证据和图像审阅记录。头显贴眼舒适度、双手握持自然程度及物理控制器接近头部的可操作性由用户实测记录。

## 五 本次方案状态

本节以下表为当前状态；后面的“历史实现与验证过程”和附录保留各自时点的记录，其中“最新”“未接入”“待交窗”等措辞不覆盖此表。

| 需求 | 当前源码／产品状态 | 当前实际验证 |
| --- | --- | --- |
| 主手体感持枪与枪口射击 | 已接统一消费者及原版首发入口 | 37448实际肩取和首发通过 |
| 射后手动换弹，禁止自动补弹 | 已接原版自动补弹阻断 | 37448射后连续6秒空膛通过 |
| 枪栓Snap／Grab与真实行程 | 已编译；5cm获取、7cm保持，原路径与骨骼端点确认 | 37448首次开栓被历史轨迹构建拒绝，尚未完成开栓 |
| 左肩取一枚实弹 | 已复用公共肩区和现有取弹输入、唯一物品租约 | 资源ready，尚未实际取出单发 |
| 放大膛口、120ms吸附与真实送弹 | 已接根源码及完整冻结产品；获取半径6cm／深8cm，保持半径8cm／深10cm；显示实弹与手Grip使用同一当前帧 | 完整产品离线生产函数／状态机检查通过；尚未完成游戏入膛 |
| 库存提交、闭锁与完美换弹 | 一发原库存事务；从实际开火至真实闭锁，小于4秒为完美、达到4秒普通；原严格边界已测 | 完整普通换弹、完美奖励与回枪返射尚未实际通过 |
| 图1第二握点与所有GrabPose | Snap不控枪、Grip后控枪及零件手姿已接；自然接触仍有待验／未采用的拟合 | 全套Snap／Grip／松开／离开及无遮挡接触未验收 |
| 狙击镜与右摇杆按下变倍 | 已编译近眼原版全屏备选及R3归属；离眼不占用该输入。镜片内PiP尚未实现 | 原版低／高倍率及进出镜画面尚未实跑 |
| 换弹各步骤引导 | 已编译枪栓目标／方向、肩区单发线框与取出箭头、缺弹实际拒绝标记、膛口轮廓及射后重启／完成隐藏生命周期 | 完整逐阶段可见性和可理解性尚未验收 |

膛口接线已重新核对：`product-guide-lifecycle-r1`的1123份冻结源及PG`0f04387ef560eb159002548b7c155ae9d0e9cb805aacf2dfb5d2dcb76c11b53a`均匹配既有记录。入膛、显示缓存、持弹、几何和换弹状态机继续沿用这条已离线接线；引导另有下面的新派生。无需重复实现或重跑未改测试；这不能代替真实入膛和库存验证。旧附录关于“尚未接入”的记录仅指其当时的候选。

37448已正常收尾并通过最终原始交接`134c7369…`释放给唯一GN-v4。下一次定位候选已实际冻结为`longshot-prepared-20261110-050140-manual-open`：1597源、6制品、原18动作驱动，prepared SHA`f0c263ed7e5dc7f66f02e99aea074f293f70a158b8a1b49712cda24ea8f5f52b`，PG`e2fc7447…`。相对37448只增加失败后限量原始输入记录的三个Longshot文件，另外5个制品和18驱动保持不变。意向已通过既有GN路线追加在Bow之后；尚未受理、没有实际前任绑定／执行入口／claim／ticket／窗口，不允许执行。下一步先取得确切历史拒绝证据，保留全部原时效和身份限制，再完成普通整轮换弹与回枪射击。

GN随后确认本枪意向已记录但未受理；此前已先到序的Troika045716/core2位于Bow之后，因此本枪排在Troika之后。该回复仅更新意向顺序，不是前任实际身份绑定、正式受理或运行许可。

完整产品的引导现新增`product-guide-single-round-r2`，PG`c3e572b7f0ba4dd0bbc074e5a6e79d4c2b4d32afe737fb65149545daecf71292`，完整编译链接0。左肩取弹提示增加同一枚15cm实弹的两面线框和8cm取出箭头，使用同一个已认证身体／参考变换；线框不是弹药资源。膛口轮廓改为直接引用实际吸附半径／深度常量。相对guide-lifecycle-r1仅改引导包含文件、几何头及对应测试，另外1120文件保持一致，现有一帧寿命／8ms绘制预算保持。冻结几何测试通过；18组比例、单位和旋转下，膛口31条原线段各端点与旧函数精确相同。初版独立编译发现缺少显式stereo_frame.h依赖，r2补齐并重建，初版记录保留。`single-round-guide-integration-result.json`记录源码、构建和测试；`guide-single-round-r2/guide-design-preview.png`由冻结函数实际线段生成并亲审，仅是设计预览，不是游戏像素验收。此完整产品未准备运行入口／排队／部署／实跑，已冻结的050140拉栓定位候选保持不变。

完整产品另补缺弹拒绝反馈，`product-guide-no-ammo-r1` PG`482d45ee1f0d6e5f61e4c135978334a6cd5b7fa3a23ae29ec2ed902eb5ef0514`已完整编译链接0。只有原有库存读取成功，且同一次有效抓栓尝试确认为容量1、空膛、有限备弹0，才记录这次拒绝并在左肩显示红色划掉子弹；不再显示取出箭头。提示显示750ms，同一意向持续按住不续期，身份／参考／输入上下文或握持代次变化时不显示，后续有效正库存读取清掉提示。无限备弹0、已装弹或读取失败不会触发。750ms是已发生事件的显示时长；每一当前绘制和动作继续执行原100ms检查，反馈不更新旧输入时间、不取得租约、不触发原版装弹或库存写入。根及冻结源的实际换弹状态机／几何测试通过，保留原普通／完美整轮、恰好4秒、库存单写和恢复测试。相对上一完整产品仅改五个本枪文件，1118文件未变；050140定位候选原样保留。记录`no-ammo-feedback-integration-result.json`，设计预览`guide-no-ammo-r1/feedback-design-preview.png`已亲审，仍未部署／排队／实跑或验证实际提示像素。

镜头控制补充一个已复现的缓存样本退出缺陷：旧Control在序号未推进时提前返回上次near状态，跳过当前暂停／换弹／眼盒退出条件；实际旧函数四种针对性夹具均返回near=1，探针退出1。初修后又复现“进入前80ms停留期间，缓存样本已离眼，但返回时仍沿用旧停留”的第二处缺陷。最终`product-scope-cached-exit-r2` PG`0e4ff8178892be42dfab3776401ea7718bd108399eeee353b4d40403fbfe0514`完整编译链接0：同源样本不推进停留或变倍，但当前暂停／换弹／眼盒失效会撤销旧near及候选停留；序号倒退或同源计数冲突拒绝时保留输入高水位和已消费按压，重新贴镜须重新停留并遵守真实松开／新按压。原100ms时效、80ms进入停留及眼盒尺寸不变。

根与冻结控制函数均通过80项检查；另将冻结Control的实际输出接到原scope owner生产包含文件，四种退出都确认显示OFF、R3解除占用、可见性恢复，并验证旧源回读不能重新开镜、真实新边沿可以恢复低／高倍率。该联测的原版调用、状态读取和Native桥仍为注入边界，不是实际游戏画面或真实按键冲突通过。相对缺弹完整产品只改两个本枪头／测试文件，1121文件未变；Native／协议／输入写者未改。记录`scope-cached-exit-integration-result.json`，两个失败探针与中间版本保留。该产品未准备运行入口／排队／部署／实跑；050140开栓定位候选继续保持原样，不能借本次离线修正宣称开栓或整枪通过。

完整验证脚本另补原始开火关联，产品`product-original-shot-binding-r1` PG`e18d9b0d6a2e66e72dedf1b52ac404456a21e6652bde9f8c388b69c9608d7557`完整编译链接0。生产逻辑只在两个既有日志后追加已确认的枪械／世界／持有代次和原开火事件编号，不增加引擎读取、时钟或写者，也不改变实际换弹判定。新增验证器要求闭锁记录的枪械、串号、原事件、开火时间戳与最初真实开火回执精确对应，再计算闭锁减开火的持续时间。原脚本只用闭锁记录内部的两个时间戳，夹具已复现它能接受被改晚的起点；新检查拒绝该反例，并检查普通换弹没有增加完美奖励。19项纯回执用例包含3999999／4000000／4000001微秒边界，均通过；这些是验证器夹具，不是游戏通过。

普通换弹分支在任何机构输入前，使用原有只读库存快照中已读取的TimeSeconds明确等到开火满4秒，不以墙钟等待或脚本运行慢推断普通结果。实际脚本注入测试显示旧版在源时钟1.1秒开始移动，新版等到5秒（原开火1秒）才移动；缺失／异枪开火回执、30秒墙钟预算内游戏时钟不推进、身份丢失时，均停止后续正常输入。完美分支保留新的真实射击和真实开栓／取弹／送弹／闭锁动作，必须实测小于4秒，不能缩短行程或改时间。根及冻结C++原开火与换弹状态机测试通过，并实际检查新增日志；36项真实65mm接近路径、身份丢失后零输入和6项首次拒绝停止回归通过。未来完整准备脚本已明确加入新只读helper清单，旧完整候选不冒充已重准备；050140定位候选完全保持。记录`original-shot-binding-integration-result.json`。该产品和新完整脚本未排队、部署或实跑，普通／完美整轮与回枪射击仍未通过实际验证。

用户恢复目标后，另为050140准备固定源离线复算器`history-replay-050140-r2/FrozenReplay.exe`，SHA`490738695f9c632fd53e48a3a70a56b96ccd3ab4c74b1741148ca66d5663002c`。编译器依赖报告确认7个项目头全部来自该候选的冻结目录，逐项匹配prepared源清单，不引用后续可变根源码。该程序必须提供预期游戏PID及创建时间，身份不匹配、参数缺失／无效、原记录截断、magic或布局不匹配均拒绝；8项夹具检查通过。已知夹具是人工输入，不能冒充37448原历史或新游戏证据。实际next-run原始窗口仍缺失，gate7的具体失败子判据仍未知。候选PG、18驱动、入口权限及排队顺序均未改变；离线复算工具不是运行许可。记录`history-replay-050140-r2/result.json`。

## 历史实现与验证过程（保留原时点陈述）

最新实际游戏为37448（creation134387277995045930，v5-repair已消费且不重试）：本轮真实肩取、权威首发、射后约6秒保持空膛、原fire clock回执、单发资源ready五项检查通过。第一段手动开栓在frame746中断，actual／wanted进度仍0，原样本间隔112.2413ms、当前源龄91.4049ms；历史select已到gate7，即relative.build拒绝，尚不知具体内层条件。cached sample gate6只是样本函数完成检查点，不是拒绝编号；count0是输出segment.count，不代表原输入ring为空。未到实际seek，旧25820 seek13不算本轮触达；不放宽100ms／identity／lease／epoch。操作器1、wrapper0分别记账。

本轮正常Native／input／entry／drain、XR0／launcher0／gamegone及十份原字节恢复独核通过，2页完整双路及6张原图已亲审，803game／1128sim、双路52.8秒、末端≤17ms；捕获最大缺口2199.9711／316.6612ms保留，不作引擎卡顿判断。枪、瞄具和原HUD可见，但腕／前臂明显遮挡接触与枪栓，全部自然手型不接受。编码及唯一48h／最新3留存实际0，留37448／13712／49912、删除1722媒体267299124字节、errors0；7份数值文件另存。完整开栓、肩取实弹、送弹闭锁、普通／完美奖励、返射和近眼倍率仍未通过。记录run37448-closeout-review.json（6910024a）。GN SG35-v4仅身份metadata独核、实际准备0并受理唯一后继；最终fresh idle／十raw复核及交窗另记。以下早先v4／v5准备文字仅历史，不覆盖此实际结果。

最新实际尝试为已消费的011500-v4，已stage但未启动游戏：新E check作为主脚本exec原O startup时未设置O搜索路径，line9 retain_test_media导入失败，随后入口拒绝旧HB done时间戳。实际进程／idle、worker／done／launcher.log与stage前字节、launcher-session旧时刻和失败前仅stdlib imports均核过；十项原件、五份stage归档旧数值及原stop标记已恢复，claim／ticket保留为历史，不复用，也未伪造Game／Native／XR cleanup0或媒体通过。最新真实Longshot游戏证据仍为47996取枪失败。

现仅准备同一意向v5-repair，未新consume／stage／运行：新check只加原O路径，实际E角色原startup import AST旧失败exit1／新通过0，两个依赖来自原冻结路径，原build字节和18驱动不变。新授权必须直接绑定本次已消费v4 claim、24590337失败收尾、f62d36be票据和18dd28d4恢复基线；原539／HB13712仅历史，不当作当前未消费许可。26项实际冻结修补函数检查通过，包括旧正常交窗独立拒绝、唯一一次／scope／活动父进程／原件／raw text及新claim复核。新入口与实际只读准备0：1596源／6制品／18驱动／1078 current及归档／13435host／7来源；未改C++、原输入或正常清理流程。仍待新的明确修补授权，当前窗口由本LS保管，GN及后序未获并行执行许可。包acquire-current-admission-v5-repair/tail-packet.json，prepared eb7d8863／plan05a7ab2c／ready3ac593af／entry76833894。以下v2／v3／v4准备记录均保留当时状态，以上实际失败和新准备不代表完整枪械目标完成。

当前未运行候选更新为同一意向的011500-manual-open-admission-v4，等待HBv6按完整正常收尾合同独核和最终交窗。实际组合检查发现v3原执行体会覆盖load／sha为只接受Path的lambda，而新增claim绑定和terminal哈希传入字符串；执行原body两helper的AST已取得read_text／read_bytes AttributeError，之前隔离fixture使用外部Path强转helper，漏了这层覆盖关系。v4仅对这两个原body helper加Path(p)，14个验证函数AST、所有枪械源码／六bin／原动作和旧1075Python pins保持，新增入口为1076。以实际body helper命名空间运行38项gate／绑定／inode／保存原件／恢复／terminal digest检查通过，交接接收脚本6项和固定Jvenv准备0；1596源／6制品／18驱动／13435host／7来源均匹配。v3未消费资格退役，无实际新claim／ticket／stage／game；原47996玩法失败及全部完整枪械验收未改变。包为acquire-current-admission-v4/tail-packet.json，prepared0e3888c8／plan3a8eba2d／ready13e61093／entry4ecde59f。

以下v2／v3记录按当时状态保留，其执行资格已由上述v4替代。

**当前完整交付尚未通过。**最新工程开发／可交付自评按统一A20%／B30%／C20%／D15%／E15%加权为41.25%，分块为A基础取用瞄准0.5、B完整专属流程0.5、C各部件吸附／GrabPose／反馈0.25、D中断／库存恢复0.5、E当前可交付回归0.25。该分数不是工时、旧成熟度分或头显接受率；旧25820通过的首发与空膛切片不自动继承到当前未运行构成。详细七步玩家结果、证据、风险和剩余项已写入本枪独立 `analysis/coordinator-plan-review-20261109-2243/longshot.json`。

最新当前47996更早的VR肩部获取失败，须先恢复这一基本环节；此前25820实际开栓到全开也仍未通过，后面整条取一发、入膛、闭锁、返射和严格小于4秒奖励仍待实证。实施顺序纠偏为先用现成窄段定位第一个原始拒绝，修复后立即推进同一正常构成的完整普通装填循环，再集中验证奖励、近眼倍率、相关中断和全部手型；不把“取到一枚子弹”当阶段终点，也不继续枚举未接入产品的手型候选。需求中的所有部件自然贴合和逐步骤引导保持完整。

当前后续候选为011500-manual-open，PG be655f557b03e2ba4a8d40346048032f89cfecbb19e99f920d7655cc50c9efc8。它在实际47996使用的08537基础上仅改两处私人装备消费者源码，为23个原有current检查位置及9个原有谓词加线程局部POD拒绝编号，包括外层原有第二次时钟检查；逆擦除可还原原字节，无新引擎读取／时钟／输入写入／Native ABI变更，100ms及所有权检查保持。160后端／640外层原函数对照通过，原／新生产Runtime各120用例1548检查通过；观察开销未游戏测量，47996没有新编号，不能据离线测试宣布根因或修复。

同一后续候选还接入三个E自有check／operator／core小助手，沿用原轨迹及按键，在首次Grip后读到本轮明确获取拒绝就停止后续普通动作；8项旧／新实际core隔离对照通过，不修改原18参考驱动，也不制造已握持结果。另已独核HB实际启动归档：同安装slash别名导致重复绑定，修复本枪自有config生成器只归一所选安装，冲突设置别名仍拒绝；11项纯fixture使用未改graphics_policy.locate通过／拒绝符合预期，无真实配置／Steam／注册表／游戏写入。这不是47996握持失败原因。

此前同一011500意向位替换为未运行的011500-manual-open-admission-v2，并由HB独核受理为未来HB010515/v5实际完整收尾后的唯一后继；原始受理收据SHA为3c55d24a，明确execution_grant=false。原入口存在两个实际反例：换成新自身身份后仍可接受旧HB005428→Frag原始交接，且环境清理漏掉无下划线的GEARS／OPENXR变量。v2仅修入口／合同：固定HB当前五项身份和真实claim、terminal及最终交接路径，独核实际清理／双路亲审／编码／retention原始证据；与实际恢复使用的同一十份原件及raw交接SHA一起fsync到首个O_EXCL消费记录，stage前和恢复前再次核对，移除原第二次capture。大小写无关清除GEARS*／PYTHON*／XR*／OPENXR*后固定覆盖，prepare／idle／原正常stop／各子进程均应用，OS变量保留。原正常stop与清理条件、枪械动作和所有C++／六bin／原1073Python pins不改，仅追加一个实际入口为1074。实际入口AST的37项针对性fixture通过，包括重现旧缺陷、新门拒绝、活动进程／错误证据／原件及同inode或替换inode篡改／四前缀隔离；没有触碰真实未来claim或游戏。1596编译源码／6制品／18原驱动／1074 current及独立归档Python／13435host／7来源的固定Jvenv准备模式0。真实claim／ticket不存在，没有stage或运行。详细packet为acquire-current-admission-v2/tail-packet.json；旧v1保留为撤回运行资格的未消费历史，旧Cover58df、002143／47996、3bfc交窗均不得复用。完整普通装填／回击优先，之后仍需奖励、近眼倍率及全部自然GrabPose验收，不能以准备检查通过收工。

当前执行绑定再按同一意向派生为011500-manual-open-admission-v3：HB上游实际HOD49912发生已记录的启动取消，HB仅为此接收分支改用执行v6，故本枪只换精确HB前驱plan93c9130f／readyf7ee0986／entryb85850bf、固定claim和未来final路径，以及自身别名／metadata；HB原prepareda186不变。本枪14个函数AST与v2全部相同，列出常量替换后反向恢复v2源码字节精确，普通Native／XR0及完整媒体合同不改，不接入HOD取消例外。1596编译源／6制品／18原驱动／旧1074Python pins保持，新增v3入口为1075，固定Jvenv实际准备0，13435host／7来源不刷新，原37项不重跑。v3 prepared e47c9833／plan7c7edaa7／ready80cf212a／entryd433e1af，包acquire-current-admission-v3/tail-packet.json，等待HB独核；v2已撤回未运行资格，v3真实claim／ticket不存在，未stage／运行。本枪完整取枪、普通装填与返射、严格4秒奖励、近眼倍率及全部GrabPose验收状态未因此改变。

v3最终交接接收脚本现已补齐，尚未接收实际交窗：只接受固定HBv6 final原文的精确SHA，复用已冻结入口的14个实际语义函数；先验证真实前驱、执行现有只读准备模式并确认idle，再保存同一十份原字节，落盘本方独核记录，重走原完整verifier及实际入口gate后才签发唯一票据。该脚本不consume运行claim、不stage、不启动游戏或发送输入。6项新增针对性检查通过，包括实际缺失／错误／格式错误final在任何原件复制、票据或queue写入前拒绝；首次fixture误要求argparse显示内部ValueError详情已修正并保留记录，接收脚本未因该夹具问题改动。原37项及枪械套件不重复运行。当前仍待实际交窗，完整枪械目标不变。

最新实际运行47996（creation134387152660895255）已正常结束，但玩法失败：原版X拾取后当前武器记录为Longshot，随后VR肩部获取断言失败，core_checks为空，未到首发或开栓。wrapper／launcher为0，operator为1，不能用外层退出码宣称成功。实际frame389记录native_equipment status=unavailable、input_status=event；该行不足以证明超时、空间区域或身份根因，旧25820的开栓seek13及其core5不能借给当前版本。两页完整双路总览与六张原图已亲看，画面未建立可见VR持枪；45.18秒双路，game630／sim965帧，最大捕获缺口分别3066.6112／349.9965ms，均覆盖结尾500ms内，缺口保留且不等同引擎停顿。Native／input／entry／drain、XR0／launcher0／gamegone及十项基线原字节恢复通过，encode0／retention0，最新保留3会话47996／9764／34568。已fresh核唯一Boom225349-V2实际准备0、身份／基线／idle并正式交窗3bfc6a24，LS不再持窗或写共享runtime。详见run47996-closeout-review.json及run47996-final-handoff-boom225349-v2.json。

本次已运行源候选为002143-manual-open-admission-v4，同一唯一后继到序，不新增队位。Cover交窗前发现旧235126保守清单中的hammerburst_reload_chain.py由c7c210b0改为d4c106a6，因此正确拒绝旧包。已独核仅新增三行HB专用首次拒绝即停止分支；LS仍选择自身core/open驱动。最小派生只更新该快照和V4入口身份，其余旧1067快照、1596编译源／6制品／18LS驱动／13435host清单全部不变，加新入口为1069 current/archive。实际固定Jvenv准备模式0，源码／制品／驱动／独立归档／host／7来源均通过；独占消费、非断言flags、环境清理与原运行体经反向常量替换证明未改，不重复不变race测试。packet见owner-admission-v4/tail-packet.md。旧V3未运行撤回，真实V3／V4 claim和ticket均不存在；已送Cover按同一到序独核，仍需新最终交窗，前序仍为已接受的Coverb144 pregame全终态。

此前源候选为 `235126-manual-open-admission-v3`，原231059-v2因Python优化参数能移除断言校验而撤回运行资格，尚未运行。同一到序V3在独占消费记录后立即以非断言条件拒绝优化／隔离参数，并在子进程工作前清理继承的PYTHON环境变量。反向源码比对证明只改这两项行为和自身身份常量；PG08537ef8／Nativec98568f0／XR16840b27、1596冻源／6制品／18动作驱动／13435host及旧1067项目快照全部不变，只新增V3入口快照为1068。实际准备模式0、7类整入口失败重试、8进程独占竞争，以及-O／-OO／-I／继承优化拒绝和准备不消费检查均已通过。Cover已按同一唯一后继到序独立核对并正式受理V3，记录 `cover235126-formal-acceptance.json`。其旧f10因同一断言缺口撤回，替代前序现精确绑定20261110 Cover admission-v3／prepared b14404ca。该前序已消费的一次实际调用因正反斜杠参数字符串不一致，在entry39、读取plan及stage前退出1；本枪已独立核对五身份、实际退出日志、保留claim及PID不存在、Bol66b239交接、十项当前／保存基线与独立idle，接受此pregame全终态，new_game_identity为null。没有虚构游戏／录制／编码／保留作业或旧41460清理通过。记录 `cover-b144-pregame-closure-acceptance.json`。仍需Cover fresh核本枪后发正式唯一最终交接；接受前序终态本身不授窗。旧f10、41460或本枪旧25820都不能授窗。真实LS claim、最终ticket、stage和游戏都未发生。仅在实际变更或交窗前做必要核验，不在等待中反复扩封套。

本枪自己的唯一后继已按同一先序独立受理Boom225349 admission-v2窄替代，V1运行资格撤回：实际准备模式0，五身份、3955源码／6制品／86实际依赖／7737host及7个模块来源通过；完整入口反向差异只有非断言参数拒绝、PYTHON环境清理及自身路径，正常停止恢复体逐字节未变。亲自执行整个复制入口的优化拒绝／不可重试检查通过。较晚HB和Frag只沿真正新尾转交，未登记第二队位。受理不等于执行许可；本枪开跑不以其后继准备为前置条件。记录 `successor-boom225349-v2-admission-check.json`，原Cover231059受理记录保留为历史，不给V3授窗。

未来完整换弹驱动已补一处具体停止条件缺口：等待子弹阶段3被取消到idle0，或任何当前同枪换弹进入recovering8时，立即停止后续正常取弹输入；正常完成阶段7退出到0及其他枪的记录不误判。直接执行旧／新实际驱动的六项主机隔离对照中，旧代码在两种等待子弹中断后仍尝试一次肩部动作，新代码均为零；只保留最后发布姿态的一次零按键释放。证据 `full-driver-waiting-round-stop-r2/result.json`。这不是实际游戏全流程通过，也未重打包进235126；先定位并修复原开栓拒绝，再推进普通整轮换弹和回击。

2026-11-10另修复生产引导生命周期：原后端每帧调用guide，idle时也会画栓头黄色提示，导致刚拿到已装填的枪或完整换弹结束后仍提示拉栓。现依据现有已确认开枪事件和本轮已完成回执显示／关闭提示；中断后的空膛与入膛后未闭锁债务保留引导，同枪换握持不重开已完成提示，新的真实开枪才重新启用。仅新增显示事实，不读新游戏字段、不更新射击时钟、不提供交互或库存权限。直接编译执行生产runtime及新增九类生命周期检查通过，原严格4秒、库存、奖励、中断和旧租约清理检查仍通过。证据 `guide-lifecycle-r1/result.json`。随后已在原完整私人构成上仅合入runtime／guide／runtime_test三个文件，其余1120文件保持原字节，完整编译链接通过，PG SHA256 `0f04387ef560eb159002548b7c155ae9d0e9cb805aacf2dfb5d2dcb76c11b53a`，Native仍为原ced416be，不重建。对应1123冻结源码的生产runtime测试再次编译执行为0，近眼原版回退／R3、原单发库存与部件手姿接线仍在真实后端中。记录 `guide-lifecycle-build-result.json`。没有生成新的prepared包、排队、stage或部署；235126诊断版未变，实际引导画面仍未验证。

此前230242准备模式因HB共用驱动刚变更而正确拒绝；在HB230535源定版这个具体变更后只做了一次必要不可变派生，231059只更新该文件与自身入口常量，其余编译源／制品／host清单均不变。保留230242和222840的未运行撤回记录，以及一次Windows路径字符串匹配的准备阶段断言失败；未生成任何真实一次票据或借用其他任务的claim。

工作区与分支已核对为 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`、`codex/6dof-single`。起草时的 HEAD `6269cc9` 作为历史定位保留；后续源码保存另有独立记录。工作区同时存在其他会话的源码变更，后续运行与公共接入需要遵守当前窗口和共享文件归属。

### 第二握点手型传递修正与表面拟合检查（2026-11-09，最新增量）

实际冻结的 C0 消费者存在一个已复现的缺陷：有效 Grip 进入双手控枪后，两个分支以新的聚合目标替换左手目标，只保留握点和权重，丢掉原有 preset／15 节 authored pose。私人修正复用原目标，只更新既有求解得到的握点和权重；原双手求解、原始控制器、右手手型、身份／租约、时钟与唯一 C0 写入点均保持原路径。

同一组实际 C0 测试用原 Marcus115 reference／local 夹具复现旧代码 11,105 检查中 128 项失败，私人修正后 11,105 检查全部通过。覆盖有效双手求解、非持枪挂载回退及仅接近吸附三种路径，四种手型选择、定制15节姿态保留，以及原始观测、右手与其他骨骼、固定平移和比例。定制表故意使用不同的栓柄姿态作为测试标记，不能把它当作托枪手型。首个测试包装因原文件 CRLF 与 LF 匹配差异在编译前断言失败，失败脚本保留，后续修正保持原始换行字节。

完整私人 Native `native-support-style-v4` 编译链接成功，SHA256 `600fc2f982a6a80f488670f83dc20dc13fac32af9c29313d964266f19e965f02`；1582 份冻结源码中仅 `body_c0_hands.h` 改变。未改共享 Native，未准备、部署、排队或运行这版，也未替换用于定位实际开栓首拒绝的 201902。记录 `support-authored-c0-regression.json` 及 `native-support-style-v4/build-result.json`。

第二握点表面贴合仍未通过：实际通用托枪手型重建后有180对手／枪三角面交线；后续两次完整拟合分别有345和369对，均保留失败并拒绝接产品。原下侧导轨在不同高度有空槽，之前选择的3.6cm高度落在内槽侧壁；修正为实际外导轨及完整掌部空间约束后为112对。再把五指的接触点／法线可达性与不连续的碰撞代价分开求解，最新fit5为5对手／枪交线，但仍有116对手部自身交线，其中66对跨手指分支；食指接触法线点积仅0.881，拇指关节也需自然度复核。已亲看fit4／fit5三视图，全部未接产品。这里的计数是超过既有交线阈值的面相交见证，不是穿透深度。不能用范围命中、单个握点靠近或五个指腹到位替代完整 GrabPose 验收；袖口、整个手臂及实际游戏皮肤仍须单独核验。

201902的保守Python运行边界出现变更后，已撤为历史未运行候选，新派生情况见下节；不放宽原固定哈希检查。完整换弹、原版倍率画面和所有部件自然贴合均保持未验收。

### 购买版路径派生与未执行的空窗交接（2026-11-09）

**最新用户选择已覆盖本节当时的Steam路径限制：**用户随后直接指定 `J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64` 用于本枪验证。已实际核对其中EXE SHA256为 `bdc6705d…`，并从原不可变私人配置核定对应 `76561197960271872EngineUserSettings.ini` 设置绑定；不再要求本次验证先重启Steam。下面215840／4380保留为历史失败和未执行记录。

新的 `222840-manual-open-owner-copy` 已静态冻结并实际只读预检通过：1596编译归档／6制品／18原驱动／1只读依赖／1059当前Python和独立归档均无bad项，EXE、指定路径、私人设置及解释器身份均通过。保持原PG08537ef8／Nativec98568f0／XR16840b27，仅按本用户明确选择派生路径包装和当前Python边界；未改Steam、注册表、游戏资产、公共入口或后台隔离保护。8项实际入口前缀、3项错误路径／哈希／设置拒绝及精确恢复测试均再次通过。这是新的后到窗口申请，已经Frag沿原唯一后继转真实尾；不要求返还已交出的空窗，不占第二队位。部署前仍须真实尾独核、新鲜边界、唯一最终交接和独立idle，当前未stage／run。

原201902及旧计划仍选择历史游戏目录。新的不可变 `215840-manual-open-retail` 明确选择现存 Steam 购买版 `J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe`，实际EXE哈希仍为受支持的 `bdc6705d…`。1596份编译归档、6制品、18基础驱动与201902逐项一致，PG08537ef8／Nativec98568f0／XR16840b27不变。当前1054份项目Python逐SHA／AST独立归档，明列13项新增或漂移，其中3份本枪路径／基线包装，其余是其他未用分支及离线工具；这不是声称1054份都会执行或静态分析证明了所有动态依赖。

新包装仅在实际最终交接及独立idle后取得即时十文件基线，绑定购买版路径与私人设置，保持0.3、后台、静音、物理输入隔离及原动作驱动。实际原normal-stop、Native／入口／pad／武器drain／XR／launcher清理回执和fresh idle通过后才能恢复基线。新增360秒边界只请求既有正常停止，不强杀游戏。8项实际入口前缀检查、3项错误路径／哈希／设置拒绝、原配置其他字段保留、BOM／CRLF及缺失文件恢复、公开状态变化时写入前拒绝均已执行通过。

215840制作时归档一致，随后的实际预检正确拒绝了又被修改的 `solve_foreend_staged_grip.py`；1596归档、6制品、18驱动和归档内1054份仍一致。失败预检保存在 `retail-215840-refused-preflight.json`，该候选不可部署，未将当前文件静默覆盖进旧候选。

前驱Boom214704的实际game4380已证明同一购买版后台入口在注入前被原生“需要执行Steam才能游玩”对话框挡住；Native、操作驱动和游戏录制均未启动。这是前驱的启动失败，不能算本枪游戏验证。已核其正常WM_CLOSE退出、十文件恢复、retention终态0及当前独立idle。只读观察还发现Steam登记PID51320不存在而实际Steam进程为50140，根因尚未证明；已请用户方便时自行重启并登录Steam，未改注册表、账号、许可、Steam或游戏文件。既有Steam客户端启动分支明确要求手动前台，未用它绕过当前后台与隔离要求，也未回退历史目录重跑。

本枪收到的是唯一空窗管理权，明确没有215840执行许可；期间零stage、零游戏、零输入和零录制，本枪审片／编码／retention为不适用。随后已独核Frag200930完整当前预检0、精确prepared／plan／ready及十文件当前基线，按原唯一后继最终交出窗口。其所有者单独获得的历史验证副本授权只属于该任务，不扩展到本枪。后续维持其已受理的真实尾顺序；本枪待购买版启动条件及新exact候选就绪后重新向真实尾申请，不占第二队位或自动要求返还。凭据 `retail-no-run-final-handoff-frag200930.json`。

### 原骨长与掌面承托的后续拟合（2026-11-09）

从原reference局部轴控制指根扭转、张合和弯曲，并把掌部改为横向承托后，fit7／8的跨手指交线降为0，但枪体相交与拇指接触不通过；均拒绝接产品。耦合手腕和五指得到fit9，原整枪交线为0，但实际掌面悬空12.22mm，不能算托住枪。

进一步检查发现之前的掌面点10462实际上只有48.2%掌骨权重，其余跟随拇指骨25／24。改用100%掌骨权重的原皮肤点1036，对齐原枪面4744的1.2mm外侧接触目标后，fit10掌面目标误差0.95mm、法线点积0.994；拇指两节保持自然展开，而不是为碰到侧壁过度扭转。这一版仍有4对小指／原枪后导轨凸缘交线、4对无名指／小指根部交线，整手自身交线66对，尚未接入产品。

两次有界后续均被完整审查拒绝：只调无名指／小指并加入实际相交面的近似约束，fit11变为11对枪体／6对跨指；按原后凸缘AABB推导整体前移1.914cm的fit12虽避开原4面，却与原枪前段宽面产生34对新交线。没有用近似损失或局部避让替代原整枪／整手三角面审查。fit9／fit10三视已亲看；当前生产护木手型未被这些失败候选替换。记录 `foregrip-reference-progress-review.json`。

后续固定真实掌点定向翻腕的fit13虽然跨指为0，但整枪交线46对；原有限三角面内滑动的fit14为19枪／80自身／11跨指；前移并消除四指指根轴扭转的fit15为43枪／90自身／45跨指。全部拒绝，未改产品护木配置。现已停止继续枚举离线变体；待普通装填可玩后集中完成剩余接触，不用拟合分数替代玩家可观察的完整动作。

### 膛口吸附与手姿共用当前帧结果（2026-11-09）

新的隔离完整组合 `product-full-chamber-presentation-r2` 已编译链接成功，PG SHA256 为 `9d2e0afad1d3b5d870677707d8106e57a5973eabde8af50d1f800cf2de9d3664`。1115 份冻结源已逐项核验；六个受影响模块从实际冻结源编译执行全部为 0：显示／手姿缓存、原肩取持弹、生产入膛、生产换弹状态机、部件握姿及物理尺寸引导。覆盖静止吸附不装弹、真实轴向送入、同帧显示／Grip 一致、失效撤显、参考／输入代次变化后重新接近，以及原库存和严格小于 4 秒的边界。每次准备只读一次原 rig，显示与 Grip getter 不重复读 rig；这是离线检查，不是游戏耗时证明。

这一版保持近眼原版画面回退、R3 输入归属、V3 部件手姿目标、原首发入口、UI 分帧及现有引导接线，新增膛口物理范围、120 ms 过渡与真实送弹约束。没有修改共享 root 的 Main、backend、Native、refire；Main／backend 的读取作用域仅组合到私人冻结源。结果见 `analysis/longshot-development-20261108/chamber-presentation-integration-result.json`。

完整组合现已静态准备为 `182222-full-reload`，3199 归档源／6 制品／18 基础驱动／7 附加入口与只读依赖预检通过；未排队、部署或运行，也未替换已受理的 `172406-manual-open` 窄版。旧前驱 HODcore2 已撤回，替代 core3 后又撤回；最新 core4-query 由 Boom 重新受理，本会话独立核1247源／6制品／81依赖通过，仍须等其最终完整交窗。当前无运行窗口，唯一后继保持 Frag172809。Public v71 和现有手动入口保持原样。

未来完整换弹驱动的接近点也改为固定物理 6.5 cm：实际执行旧驱动的注入路径在0.5倍模型下复现仅3.25 cm的错误，修正后36组单位／模型比例／枪体旋转／实弹挂载由独立四元数重建全部通过，最大误差2.55e-16 m。七种机构拒绝／意外回退／身份或武器改变的首拒绝停止检查仍通过。这些是无游戏、无实际输入的离线证据；第一版测试夹具标签错误而未触达几何的失败也另存保留。

单发 GrabPose 的后续原表面审查已找到明确缺陷：用相同冻结 C++ 手指算法输出15节四元数，重建原1395手面与实际1248子弹面，原V3精细捏持有148对超过1mm交线的非共面相交。原图三视亲看，弹体穿过食指／中指／拇指附近，拒绝称自然贴合。一次有界本枪独立候选降至64对，仍有穿插和不自然指形，同样拒绝接产品；原骨长／输入／队位未动。该指标不是穿透深度，也不是实际游戏skin证明。下一点按原蒙皮法线选对掌侧指腹与可达目标，保留非接触手指自然卷曲，再做有界拟合。记录 `round-skin-v3-review.json`；完整候选仍含旧V3，只能作为未运行机械验证准备，不能当最终手姿交付。

后续单发姿态fit5现已进入私人未来产品源码／编译：由原掌面法线选指腹，侧向携弹、后段捏持且原骨长不变；亲看三视，原手-弹148对交线降为0。食指／拇指自接触从172对降至10对，未新增对；仍为原食指12／13关节褶皱交线，不能当作完整自然皮肤或袖口通过。fit3因33对穿插及接触面错误拒绝，fit4因14对中指接触拒绝，保留原结果。

复用现有V4的完整32字节模型／15节reference绑定、原Broker和唯一C0，仅在私人ff1冻源窄组合；保留原瞄具R3／view及V2／V3，未改共享Native，也未复制Lancer握姿常量。新Native `ced416be`、完整PG `d254c5ac` 均编译0；实际冻结左C0消费者用原reference／local夹具815项0，V4 owner44／真实provider注入67项0，V3/V4过渡与三个冻结PG模块检查0，另有24组原皮肤挂载／单位／旋转接触检查。取弹时锁定同一实弹mount与手表，V4失效不回退成不匹配旧V3指形；原始rig仍决定真实弹头，机构／库存权限不从显示手读取。

该组合已静态准备为 `190957-full-reload`，3208归档源／6制品，未排队、部署或运行；不替换172406。三次测试包装编译失败与两次准备路径／cache类型失败均保留，后续成功不能抹掉它们。实际V4手腕／皮肤跟随、其他部件、完整装填与倍率画面仍未验收。记录 `authored-round-integration-result.json`。

最新172406实际25820已完整收尾并最终唯一交Frag172809，本会话不再有窗口：取枪／权威首发／连续6秒空膛／原实发clock／单发资源ready5项通过；46条当前contact、原UI起点ack、33点把手路径和实际seek均触达。frame764首次拒绝落在私人原谓词POD的seek_gate13，即 `prepare_controls` 在SetPosition调用前返回false；reconcile_gate8已越过，pre-call原源年龄87.2834ms、grip和lease有效。它不能证明函数返回时仍未过期，也不能定位内部谓词或归因于纯时效。实际开栓仍未完成，未取出单发，不宣称17072的接触失败由idle Scope因果修复。

本局wrapper50208终态0／operator1／launcher0，Native／entry／pad／drained TRUE、XR0／game退出；早期finally仍live提示保留，无强制终止。独立核对并恢复8项manifest／config／settings／head／public-state／3私有制品原字节；首次比对发现新增默认 `lancer_interaction_ranges:false` 与settings BOM差异，在确认除此无未知配置改动后恢复原保存字节，late writer未证明。原worker／done／日志及新数值回执未被旧档覆盖。

2页完整双路及6张原图亲审，843game／1163sim、跨度54.539／54.522秒且双尾覆盖；原场景沙袋、前臂／袖口和枪角度遮挡机构接触，完整自然握姿、开栓可见完成和倍率均未接受。最大game采集缺口2.073秒，不是FPS／卡顿根因证明。encode37067／retention36113实际0，实删1090媒体／176836700字节／errors0，保留25820／11536／49864最新3局48小时，源／制品／save／日志不删。最后再次全核后继1080源／6bin／19root＋27frozen／187PG＋381Native及289Python SHA／AST无差异，fresh idle后明确唯一交窗。闭合 `idle-validation-first-run-25820-review.json` SHA `e2889d9935c557e8b1d7ac993668ebdbb1497012b3dc0009e882acc9205efe28` 此后保持不改；下一点为原 `prepare_controls` 内部拒绝定位，不重跑未改172406，不放宽100ms／身份／租约。

上一实际窗口 17072 的历史结论：原首发适配触发、权威首发和连续 6 秒保持空膛通过，之后因缺少新鲜 `longshot_reload_contact_current` 停止，未进入本轮抓栓。该局已正常全终收尾并唯一交还。实际开栓完成、左肩取出单发、入膛／闭锁／普通及完美奖励、原版低高倍率画面、所有手掌／手指／袖口表面贴合和实体头显验收仍未通过。镜片内 PiP 尚未实现；不能把显示目标或 CPU 测试记作皮肤贴合通过。

### 25820 拉栓预检的定向定位候选（2026-11-09）

`195551-manual-open` 已准备，私人 PG `08537ef8` 完整编译通过。它直接继承实际25820的1096份PG冻结源；1596份运行归档源／6制品全核，18个输入驱动及只读reader与25820逐字节相同，Native `c98568f0`、XR `16840b27` 复用。仅两个私有文件改变：原动画预检路径增加POD判据编号，原有最多12条的本枪seek行补充编号。`prepare_controls`、`observe_expected`、`observe_kind` 与 `selected_node` 保持原条件、短路顺序和读取次数；去掉观测后整份原动画文件精确还原。没有增加引擎读取、时钟或有效期，也没有将 `enclosed_observation` 改为跳过原读取。

实际新旧 `prepare_controls` 函数提取后编译运行104组注入对照，包含25个拒绝点、两种外围观测模式及观测开关，返回值、调用序列及输出完全一致，失败输出清零；所有25个拒绝点在普通模式下都被实际触发。该夹具没有dispatch API，不能证明游戏中原getter、皮肤或拉栓成功。结果见 `analysis/longshot-development-20261108/controls-trace-test-result.json`；计划见同目录 `controls-trace-runtime-plan.json`。

本候选用于找出25820中 `seek_gate13` 内部的首个拒绝条件，尚不是功能修复。已向现有排队末端请求追加，未受理、未部署、未运行，没有运行窗口；25820的最终交接与不可变回顾保持关闭。下一次仅在明确唯一交窗及独立预检后做原短片，首明确中断停止正常动作并正常清理。严格100ms、新鲜身份／租约、反重放和实发到闭锁严格小于4秒均保留。

### 2026-11-09 运行边界与枪栓握姿更新

当前唯一已受理运行身份为 `201902-manual-open-wrapper`，PG仍为 `08537ef8`、Native仍为 `c98568f0`、XR仍为 `16840b27`。原195551未运行；为补充运行依赖，先派生201149，再因wrapper错误地要求后继已准备而派生201902。两个旧归档保持不改并撤回运行申请。新候选保守归档1045份项目Python并逐份SHA／AST核验，包含未执行测试和其他分支；实际入口、exec／runpy、launcher子进程及sys.path映射另记，不能称1045份全部运行或完整动态导入闭包已证明。18个原输入驱动／reader、1596份编译源和6制品保留；前后核验同时检查当前root和独立快照。

wrapper只移除“successor／successor_thread已准备”这一运行前置，保留前驱最终交窗及其review SHA、当前state／candidate／ready／prepared、idle与前后预检。实际新旧准入前缀8组对照通过：旧版无后继真实拒绝，新版在前驱交窗正确时可通过；错误的前驱hash、状态、final、candidate及两个制品记录hash均仍拒绝，测试在任何preflight／subprocess／stage之前截断。Boom191450已独立全核并正式受理201902为其唯一后继，但尚未最终交窗；本会话未stage/run，无窗口。后到Lancer200139因其自身视觉缺陷明确撤回，Frag200930已独立全核并登记为LS唯一后继；后继准备不会阻挡前序。

枪栓表面另有新证据：复用已验证格式解码器，核对原Longshot导出 `741d9653`，9786顶点／10856三角面及逐顶点CPU／GPU／BoneMap一致性。原暂定vertex356锚点位于栓杆上部，距原栓头中心约3.926cm。新锚点来自50个独立外帽顶点的拟合，mesh局部坐标约为 `(-4.81249,13.94165,5.35609)`；通过当前mesh与已闭合bolt变换转入机构坐标，Snap／Grab／引导和手姿共用当前栓头点。此发现不解释25820的 `prepare_controls` 拒绝，也没有改动已受理201902的原几何。

旧通用圆柱手型在12个滚转采样中最少仍有292对手—枪表面交线，三视亲审拒绝为最终贴合。直接复用捏持种子后虽能避开枪体，完整自接触检查仍发现指间穿插：首拟合65对、放松非接触指后28对、沿腕Y小幅展开后最少19对，均保留并拒绝。最后按已屈曲手指的方向做小幅展开，拟合4的枪体穿插与不同手指之间穿插均为0。六张三视图已亲审；仍有55处同指关节或拇指掌根严格表面交线，不能视为无害或自然整手通过，完整行程、前臂／袖口和实际皮肤消费者仍待检查。

原栓头锚点及独立15节V4枪栓捏握已接入私人完整组合 `product-authored-bolt-v4`，PG `7c05ef9d` 完整编译0，1123份冻结PG源逐项核验，复用Native `ced416be`。24组原指腹—原栓头三角面接触跨单位和世界旋转检查0，冻结生产发布模块检查0；生产换弹runtime实际冻结函数的完整周期、严格4秒边界、打断奖励／租约与清理检查0。runtime测试首次仅因命令路由保留强制包含路径引号而未编译，纠正无空格路径后通过，失败不作为产品或游戏故障。这些均为离线证据。新姿态与腕部挂载成对使用，V4缺失不发布不匹配旧握拳；原始跟踪仍决定动作，显示手不给机构权限。拟合只覆盖原物理尺寸，其他物理尺寸撤销该显示拟合。

该完整组合只完成源码与编译，尚未静态准备新运行包、排队、部署或实跑，不替换201902；Publicv71和固定手动入口未改。记录 `authored-bolt-integration-result.json`、`bolt-grasp-surface-review.json` 与 `authored-bolt-frozen-checks.json`。下一步离线检查真实原动画全行程中的手—枪接触；取得Boom新最终交窗后，201902仍只做原短段以定位机械预检内部首拒绝，完整换弹、倍率画面和头显继续保持未通过。

### 原动画行程采样后的枪栓腕姿修正（2026-11-09）

上面的拟合4并非最终通过：用原 `LS_Reload` 的896字节／50键、实际冻结的decode／native-relative转换／BoltPath／sample／grip函数，以及原逐顶点四槽蒙皮，检查33个开栓进度点时，在0.5625进度发现拇指掌根与根骨加权镜筒区域外壳有5对交线。原先“接收机”简写已按实际三角面坐标纠正：该处raw范围约X−6.06..−5.39、Y20.11..21.59、Z16.46..18.02，邻近原Scope参考点。失败图与5个原三角面见 `bolt-grasp-stroke-audit-r2`，不能用其他32点或端点通过覆盖此失败。

离线检查最初误将原参考骨骼与重载首帧当作31.6微米内完全相同，实际原数据相差约69.49微米／0.00276度；R1断言失败保留。修正检查方法后，首帧与实际25820／frame757停止骨骼回执的位置误差仅0.000562mm，旋转矩阵误差约1.31e−7。原世界根基另外用该局原knob／port两个非共线点核对，误差约1.94e−8m／3.15e−8m；未拿未参与机构的常量动画root轨替代当前世界根，也未改产品坐标转换或有效期。R2脚本一次多余括号导致的解析失败单独保留，不作为游戏或产品故障。

拟合5因15对静止手—枪交线及拇指法线不合格拒绝。拟合6将腕局部接触点由 `(10,−5,−5.5)cm` 调为 `(8.5,−4.5,−6)cm`，重新拟合食指／拇指，放松其余三指并作小幅展开。原骨长、实际机构栓头、原曲线及18.398cm开栓路径未变；33个相同规范进度点全部无手—枪交线，包含原失败位置。已亲审静止及0／0.5625／1进度三视图；这不是采样点之间的连续碰撞证明。不同手指之间交线为0，仍有49处同指关节或拇指掌根严格表面交线，继续标为未接受；完整前臂／袖口、吸附过渡和实际皮肤消费者尚未验收。

拟合6已接入私人完整组合 `product-authored-bolt-balanced-v4`，V4 profile_id更新为 `0x4c53424f4c543032`。新PG `dbe9a415` 完整构建0、1123份冻结源码核验0；三个实际冻结检查——24组原指腹／原栓头三角面接触、生产发布、生产换弹runtime——全部0。复用Native `ced416be`，不重编或增加写入者；其实际左12..26骨消费者对原115骨reference／local夹具815项0，实际V4适配器用注入的既有Broker／CoreSnapshot验证本枪真实15节表、主动magazine及被动socket发布87项0。后者仍不是实机Broker身份／Scene时序或皮肤证据。

新完整组合未准备运行包、排队、部署或实跑；已受理的201902机械定位候选和原几何保持不改，仍等Boom最终唯一交窗。严格实发至闭锁 `<4s`／达到4秒普通、100ms、新鲜身份／租约、真实边沿与单C0均保留。记录 `authored-bolt-balanced-integration-result.json`、`bolt-stroke-surface-review.json` 和 `native-bolt-v4-checks.json`。下一项离线工作为第二握持点及前臂／袖口接触，实际机械开栓、单发取弹、完整入膛／闭锁奖励与倍率画面仍未通过。

### 首轮实施检查点

- Longshot 换弹核心已实现实际射击时刻起计、严格 4 秒边界、闭锁终点、同一实发记录重试时钟保持及完成后反重放。MSVC `/W4 /WX /UNDEBUG` 编译并运行 231 项检查通过。
- 新增 `weapon_longshot_scope_control.h`，实现逐眼贴眼范围、80 ms 进入稳定期、退出滞回、R3 单次按压归属、菜单及失追恢复门控。40 项离线检查通过，测试目标已登记 CMake。
- 现有镜内光学规划器的 136 项检查通过。以上共 407 项为离线代码检查。
- 首轮换弹测试因旧断言仍期待 R3 开始时刻而失败，修正断言为实际射击时刻后通过。记录保存在 `analysis/longshot-development-20261108`。
- 原版已保存的 `Weap_SniperRifle.HandleButtonPress` 导出出现 `R3`、`bIsTargeting`、`SetZoomed` 及音效和视角修正调用。该文本 SHA256 为 `6af91f648e34f5698bf72e77aa25a435be7a24b9e6949f4a87b30f0c4663ec16`，含未解析指令，仅作为后续核对函数和参数的定位证据。需要验证缩放切换两侧的瞄准上下文，确保原版低倍率状态仍可再次按 R3。
- 产品运行时消费者、实际倍率调用、自动换弹阻断、模型几何、GrabPose、镜内图像或回退路径、引导及模拟器实测仍在待办中。目标保持 active。

已确认的用户规则为左肩单发取弹、第二握点靠近吸附与 Grip 控枪、全部交互实体部件的 GrabPose 贴合、逐步换弹引导、手动补弹、实际开枪至闭锁严格小于 4 秒的完美判定，以及仅贴眼状态启用 R3 原版倍率切换。新增交互尺寸、姿态容差、贴眼阈值、中断恢复策略和镜内路径选择是本方案提出的实施参数与决策点，待模型和受控实测核验。

### 产品资产与副握点接入检查点

本轮为 progress，工作位置核实为 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`。

- 公共资产读取器已增加 Longshot 类、网格名及根包身份验证，支持当前 basic hold 与同源枪口消费者的资产查询。51 个实际读取器内存夹具用例通过，包含错误包拒绝以及 Lancer、Frag 等既有回归。
- `weapon_longshot_foregrip.h` 给出独立的暂定护木表面点，取自原模型零基顶点 6423，原坐标为 `(0.7677327, 43.251, 3.37458)`。已查看原模型侧视及俯视，掌部变换重建测试通过。此坐标仍需实景多角度校准。
- `weapon_native_lancer_follow.inl` 已接入本枪 support geometry 和有效副手租约下的双手求解分派。`weapon_lancer_grasp.inl` 新增独立 Longshot 被动显示分支，使用当前跟踪、仲裁结果及本枪几何，真实 Grab 仍由既有所有者授予。
- 冻结 1051 份源码的独立 Playground Release 完整编译链接成功，DLL SHA256 为 `b8ffad36d145ee728b758833b1d17c4e21000d4a932e62400f08807900ea288f`。冻结源哈希逐项复核通过，构建证据为 `analysis/longshot-development-20261108/product-first-hold/build-result.json`。
- 读取器夹具首次以默认栈执行发生栈溢出，使用 32 MiB 测试栈后通过，测试链接参数及失败保留在专用 JSON。首份成功产品构建的包装器还发生日志解码错误，实际编译与链接返回 0；后续包装器已显式处理日志编码。

本轮尚未部署或启动狙击枪验证。方案规定的精确盒体 Snap/Grab 范围、完整 GrabPose、枪栓机构、单发供弹与自动换弹阻断、实际倍率调用、瞄具输出和逐步引导继续实施。运行窗口由 HB 会话持有，独立 Longshot 共享分派编辑结束后已交还编辑窗口。

### 实弹资源与实发时钟提供者检查点

本轮继续为 progress。

- Longshot 已接入现有手动 `CanReload` 准入策略和公共库存读取器。原版保存脚本中的 `WarWeapon.ForceReload` 先查询 `CanReload`，本枪 `ShouldAutoReload` 在特定开镜条件外返回公共实现。游戏运行中的射后等待、空击和收枪再取仍需实际验证。
- `weapon_longshot_round.inl` 已在正常装备 tick 接入完整实弹的隐藏资源准备，复用现有分帧创建、根引用、光照绑定和资源清理。初始显示材质复用当前武器的原版材质，弹壳、弹头和底火的外观仍需独立校准。抓取和入膛运动继续实施。
- 两个实际实弹 LOD 输入通过几何测试，分别为 3744 顶点／1248 三角和 1248 顶点／416 三角，验证 +Z 到引擎局部 +Y 的轴转换、索引绕序及三段材质索引。测试首次错误使用单位比例 1，受到已有 10–500 范围拒绝，已改为有效比例 100 的基准比较。
- `weapon_longshot_fire_receipt.inl` 在既有认证 `CalcWeaponFire` 调用观察点记录候选游戏时间，后续正常 GT 确认单发弹药已消耗才发布实发回执。直接包含生产实现的离线夹具覆盖未消耗、重复事件、错误容量、身份改变、失效来源及坏时钟。该通路的实际游戏覆盖仍待验证，原版浮点 `TimeSeconds` 的精度仍限制运行时边界观察。
- 新候选独立目录 `analysis/longshot-development-20261108/product-round-fire-receipt` 完整 Release 编译链接成功，1054 份冻结源逐项哈希复核通过。DLL SHA256 为 `0ad0a781c7139d177491dd95ce2e0b46cc81aea4855b6b56e71cd797c28c6a19`，已另存按哈希命名的二进制。构建及测试记录为该目录 `build-result.json`。
- 第一持枪构建目录曾继续用于迭代，早先 `b8ffad36` 中间 DLL 已被覆盖，旧结果已标明其原路径不再提供该版本。其后 `0d72be32` 与本次 `0ad0a781` DLL 已另存至 `analysis/longshot-development-20261108/binaries`。

当前运行窗口已交给手雷会话，本枪准备工作继续使用独立构建。完整机械装填、原版奖励、瞄具输出／倍率路由、引导、GrabPose 和受控模拟器验收仍未完成，目标保持 active。

### 原部件读取与首次基础验证准备

本轮为 progress。`weapon_longshot_parts.inl` 通过现有具名骨骼读取器验证原模型 Root0、Bolt3、Scope2，原始骨名和索引来自已保存 `core-bone-audit.json` 的 LongShot 条目。实弹准备现需通过本枪原部件读取，失败会在现有资源回执中给出 bounded gate。枪栓停止／定位及物理行程尚待接入。

新独立 Playground Release 编译链接通过，DLL SHA256 为 `7ebfff3e46f08d2ab25020c524c2908a5df3b862deaa84550ae7e09ec92990ad`。冻结构建复用前一已编译源码底座，只覆盖本枪相关接线，工作区并行修改另行保留。基础批次 `analysis/solo-m1-20261028/longshot-prepared-20261108-212838` 保存 1552 份源码／驱动文件及全部原版配套二进制，哈希复核通过。

首次基础驱动复用现有受控启动、真实走路／原版 X 拾取和肩取通路，按实际 `CharacterSlot` 选择肩位，实射一发后连续六秒检查空膛、实发时钟回执与隐藏实弹资源 ready。驱动 AST 及唯一注入入口检查通过，尚未 stage 或执行。该批次只覆盖基础接线，完整手动装填仍须另行实现及验证。

运行窗口现归 Snub 会话，已申请后续交接。手雷会话报告两次正版 Steam 启动未通过认证、Native/XR 未触达，并已请求用户确认 Steam 启动状态。本枪未重试该启动问题，继续离线开发。依据现有源码仍可推进，目标保持 active。

### 枪栓复合动画读取检查点

本轮为 progress。新增 `weapon_longshot_animation_tracks.h` 和对应测试，解码四条骨骼的常量关键帧 Float96NoW 数据，同时保留平移与旋转。解码校验所有跨度、完整数据覆盖、关键帧数量、旋转范围、四元数以及非有限值，失败保持输出原值。合成轨迹、时间采样、损坏数据及发布检查已用 MSVC `/W4 /WX /UNDEBUG` 编译并执行通过。

`weapon_longshot_animation.inl` 已编译接入本枪的原版曲线读取接口。它要求当前 `LS_Reload`、原资源包、AnimSet 类型和 Root/LensCap/Scope/Bolt 轨迹顺序一致，并重复核对读取前后的属性、表、内容与动画节点。首轮完整编译因把动态字符串指针传给现有字面量模板辅助函数而失败，修正后完整编译链接通过。实际游戏内的原始曲线读取分支尚未触达，停止／定位与手部行程驱动仍待接线。

冻结 1058 份源码的 DLL SHA256 为 `840608d5f8522f7537b36ee865ae6c98c8c01ce0952584d229f237a02b16c283`，按哈希另存二进制，证据为 `analysis/longshot-development-20261108/product-bolt-track-reader/build-result.json`。已准备的 `7ebfff3e` 基础批次继续独立保存。

已收到主人经统筹转达的新环境路径，并只读核对该路径下 EXE 的 SHA256，匹配已有支持配置 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。后续验证沿用项目游戏直控／模拟器脚本及双路录像，遵守主人明确的 Computer Use 禁用要求。后续实际执行状态见以下记录。

### 原版拉栓动画与手部轨迹映射的编译进展

新增 `weapon_longshot_clip.inl`，沿用已有唯一动画 ownership journal，预检原版 `PlayWeaponAnim` 与 `StopAnim` 接口后只播放枪身 `LS_Reload` 并在同一次 GT 操作立即停止。位置控制复用原有禁用动画通知的 seek 接口，清理核对当前枪、节点及动画身份，遇到已更换的节点不停止其他动画。这是接线接口，实体手部状态机尚未调用，不能计作拉栓游戏通过。

`weapon_longshot_bolt_path.h` 以原版 Bolt 的平移和旋转计算拉柄轨迹，每两个原始关键帧之间细分八段，按弧长把连续手位移映射为原版动画时间。它拒绝跳段、离开轨迹、非闭合端点和异常根骨移动。补充稀疏旋转轨迹与四元数反号的回归，两项目标测试通过。冻结 1061 个源文件的 Playground Release 构建成功，DLL SHA256 为 `9039a77b7c745750c543ef8c8310372fe1fa10d5ff10a48904648f99918deaed`。证据为 `analysis/longshot-development-20261108/product-bolt-clip/build-result.json`。本候选未 stage，本次基础实跑仍使用已冻结的 `7ebfff3e`。

### 基础实跑 36596 的首个失败

Snub 正常收尾后明确把独占窗口交给本会话。本次准确 stage `longshot-prepared-20261108-212838`，PG `7ebfff3e`、Native `c98568f0`、模拟 XR `16840b27`，保持公开 v71 不变。已进入实际游戏和 XR 投影，但从出生点直行到地面 Longshot 时受场景障碍阻挡。实际位置从 `(-3900,0)` 到 `(-3918,-154)` 后不再接近 `(-3920,-335)` 的拾取物，触发原有有界行走停滞拒绝。地面拾枪、肩取、首发、自动装填禁用与资源准备均未到达，全部仍待验证。

原版正常停止已执行，Native 清理验证、入口恢复及 XR 退出成功，launcher 返回 0，游戏已退出。双路录像已编码，亲自查看完整保存跨度的一页总览和末尾游戏／模拟器原图，画面中可见仍为空手并停在场景边缘。保存约 19.47 秒，双路末尾覆盖在 500ms 内；游戏捕获最大间隔约 2.38 秒，不能作为逐渲染帧证据。失败文件保留在 `analysis/solo-m1-20261028/menu-firstperson-36596-220104`，双路录像在同 PID 的 `startup-20261108-220011/dual-output`。下一步仅修本枪检查器的普通摇杆绕行路线，复用现有场景通路，不改变产品准入或角色位置。

### 实体拉栓驱动编译及窗口交还

本次媒体清理已 terminal 0，删除 2874 个文件、493109926 字节，错误数 0，保留最近 36596／49012／16292 三次媒体及 48 小时约束。配置、模拟器设置和头部命令的内容恢复核对通过，原本不存在的头部命令仍不存在。核实 idle 后窗口已明确唯一交给 HB，后续只能按队列再次取得窗口，不因短暂空闲自行启动。

本枪独立检查器已改为经过现有后方通路的普通摇杆绕行，接近 Longshot 时从障碍外侧进入。修改只在 `run_longshot_operator.py` 的内存选择内，不改公共检查器、角色位置、拾取原版调用或产品准入。AST 和唯一替换点通过，修正后路线尚未实跑。36596 失败保留，不能把脚本修正当拾枪通过。

补充 `weapon_longshot_bolt_stroke.h` 和目标测试。真实新 Grab、当前租约、有效双手追踪、同一绑定和 100ms 连续输入是行程前提。进入拉柄 5cm 获取球后，以该次真实手位置为物理行程起点，沿原版复合轨迹连续求解。提议到达端点后还必须在后续帧看到原版 Bolt 与请求曲线的位置误差不超过 2mm、旋转误差不超过 2 度，才返回开栓／闭栓完成。动画自行到端、持旧租约、离开轨迹、跳段及失追均不能完成。此处毫米与角度是实现中的骨骼一致性门槛，不是外观 GrabPose 验收结果。

`weapon_longshot_bolt.inl` 已编译进入 Playground，提供实际枪身 Root/Bolt 矩阵读取、当前模型与闭合骨骼间的拉柄接触点映射、当前 rig 控制器转换、原曲线内容核对及既有无通知 seek 调用。拉柄接触点仍是原模型 LOD0 顶点 356 的初始选择，实际贴合与全角度外观尚未验证。总换弹运行时尚未调用这个驱动，供弹、入膛、奖励及逐步引导仍待串接，不能把接口编译通过计作实际拉栓通过。

本轮另修正共享 `observe_kind` 对 Longshot 已允许类型却仍只接受 `AR_Reload/HB_Reload` 的问题，增加本枪专用 `LS_Reload` 条件，其余枪保持原规则。修正已与 HB 协调独立编辑范围。独立冻结 1064 份源文件，以完全相同的 GEARS 构建选项完整编译链接通过，DLL SHA256 为 `900d0049f075b0f8780c559041e7f29b87fbdf886ba034b69e8f8b4f5313f75e`，按哈希另存。证据为 `analysis/longshot-development-20261108/product-physical-bolt/build-result.json`，未 stage／未实跑。测试最初因多个大型栈夹具溢出而失败，改为堆分配后在默认栈通过；产品首次编译的局部名遮蔽警告也已修正，失败日志保留。目标继续 active。

### 单发实弹的持握与显示接口

本轮为 progress，继续同一 checkout／分支，没有启动新的游戏或改变公开候选。新增 `weapon_longshot_round_hold.inl`，沿用既有资源 owner 与实际 `ammo` 租约，不建立第二份库存账。只有当前帧真实新供弹 Grab、左肩原有供弹区命中、物件序号与输入 token 一致、当前绑定和资源 ready 都成立，才接受一枚实弹的显示绑定。后续逐帧读取匹配当前来源的 rig 左手握点，按固定手内安装变换驱动物件；不拿吸附后的视觉手去证明真实入膛行程。

已直接读取实际 authored LOD0 顶点数据，子弹长度为 15cm，半径约 9.37mm，原始底火端在 +Z=0、弹头端在 +Z=15cm，资源转换后长轴为枪模局部 +Y。因此需求中的子弹 Grab 胶囊可具体表述为沿实际 15cm 弹体轴线、半径 3cm 的获取胶囊，保持半径 5cm。这是区域设计尺寸；本轮实现的是供弹租约后的持握显示接口，掉落物重新获取胶囊尚未接入。

初始手内安装使用原有 Marcus 左掌解剖标定，弹体轴线转向手腕纵轴，握在弹壳靠底部。它仍是需要实际画面检查的初始 pose，不是完整自然 GrabPose 通过。铜壳／弹头／底火目前复用原枪材质的限制也仍保留。

持握函数的生产源码目标测试通过，注入边界仅为资源、rig 与当前来源。测试覆盖原肩区、非新获取、错租约类型／生命周期／token、失追、当前 rig 不匹配、跟手位移、重复帧、撤下失败保留物件，以及来源失败撤下显示后正常恢复。显示来源失效会撤下旧可见物，同时保留库存 owner 仍需处理的物件身份；显式正常 hide 成功后才清除该身份。

冻结 1066 份源码的 Playground Release 完整编译链接通过，DLL SHA256 为 `c1491b3739f74373a331dc5599c194184b950cd2bc412b83ed7e25bcad27e838`，按哈希另存，证据为 `analysis/longshot-development-20261108/product-round-hold/build-result.json`。尚未 stage／未实跑。总换弹状态机、单发库存事务调用、入膛几何、完美奖励、逐步引导及完整 GrabPose 仍待接线，不能用本接口测试关闭这些要求。

### 总换弹状态机已接运行分派，尚未实跑

本轮为 progress。新增 `weapon_longshot_runtime.inl`，已接入既有手部 propose／committed 回调和正常 GT 的 reload tick、fire block、switch 通知分派。Longshot 的专用候选不再落入 Lancer 部件提案。与 HB 协调过共享插入点，只增加本枪条件和调用，构建仍使用前一冻结来源加本枪 overlay，不混入其他会话的并发修改。

当前源码阶段链为真实抓栓→取得原版 Reloading lease 并立即停止枪身 clip→连续手动开栓→左肩原区新 Grab 取单发→手持实弹连续入膛→原有 revision 库存事务提交一发→重新真实抓栓并连续闭锁→非补弹退出→原版奖励调用。有限备弹扣一发，无限备弹不伪造有限库存。插入后未闭锁会记录同一枪的 chamber debt 并阻止射击；中断／收枪走原有恢复和库存对账，不能靠自动超时或补弹解除。

完美时间接收实际开枪回执的游戏时钟，微秒坐标严格 `<4000000`。共享账本的原接口是包含截止端点，因此本枪传 `4000000-1`，不修改其他枪的边界。没有实际开枪回执的初始空枪装填传零窗口，只能普通完成。新目标测试经真实共享账本的一发 reservation／insertion／receipt／closure 验证 3.999999 秒为完美、4 秒／4.000001 秒／8 秒为普通及一发库存守恒，执行通过。首次夹具错误地把 deliberate-zero 的 expected.revision 加一，未超过 before.revision，断言失败；已改为提供真实观察语义的 before.revision+1，生产账本未放宽。

入膛检测从生产源码独立抽为 `weapon_longshot_insertion.inl`，供运行时和目标夹具原样包含。使用未吸附的实际弹头位置与弹轴，必须连续进入 8cm 深、6cm 半径、30 度范围，再推到座端 ±6mm、横向 1.5cm、10 度范围。输入间隔不超过 100ms，相对速度不超过 3m/s。目标测试覆盖先 approach 后 seat、直接生成在座端拒绝、角度／横向界限、越界、速度跳段和间隔，执行通过。夹具只注入原骨骼与实弹姿态读取边界，没有游戏进程或库存 writer。

`weapon_longshot_guide.inl` 已通过原版 Actor 的单帧绘制路径，为抓栓、左肩、入膛和闭锁阶段发出下一部件球形提示；尚未验证实际可见性、逐步文字／音效或最终引导表现。膛室点目前是原模型 `(0,26,10.5)` 的初始表面位置，后向前的入膛方向也需实际模型和操作画面确认，不能当最终几何校准。

冻结 1071 份源码的最新 Release DLL 为 `fc4ecc33906204c1d8f84e89121da91a575b8580d504d8af201882f6d28751b5`，完整编译链接通过并按哈希保存。证据在 `analysis/longshot-development-20261108/product-manual-runtime/build-result.json`。初次构建的局部名遮蔽和 guide 缺失 namespace alias 已修正，失败日志保留。当前候选尚未 stage／实跑，完整状态机本身也尚未做注入边界执行；不能把局部入膛／时间测试当整条游戏换弹通过。

仍明确待完成：弹壳实际弹出视觉、部件中立 Snap 与完整 GrabPose、入膛吸附显示、阶段文字／声音、近眼镜内放大和 R3 产品路由、完整正常／中断／清理验证。插入后中断再取枪的独立重新拉栓路径目前按普通完成，需要继续完善原始完美时间与奖励的跨中断衔接。运行窗口继续按其他会话的唯一交接安排，本轮未启动新游戏。

### 基础链路 21724 实际窄通过

HB 56220 正常收尾、双路审片和 retention terminal 后明确唯一交窗，本次只运行先前排队的基础段，不叠新总换弹。stage 的仍是 `longshot-prepared-20261108-212838`／PG `7ebfff3e`、Native `c98568f0`、XR `16840b27`。准备阶段实际核对 1552 份冻结源码、六种构建制品、12 个当前驱动的哈希／AST，以及主人指定游戏副本的 EXE 哈希。公开 v71 保持不变，控制仅用既有游戏直控／模拟器脚本，无 Computer Use。

实际游戏 21724／creation `134386234086007894` 通过修正后的后方普通摇杆路线，完成原版地面 Longshot 拾取、CharacterSlot2 实际左肩取枪和同源枪口单发。唯一实际 shot1 的 `trace_valid/write_source_consistent/trace_source_consistent` 均为 1，trace start 与当前 muzzle origin 相同。实际开枪回执游戏时钟 `39033260us`，容量 1／AmmoUsedCount1。随后 6 秒、24 个实际读取样本均保持 AmmoUsedCount1，没有自动补弹。单发实弹隐藏资源在 frame612 达到 ready，库存写入 0。暂停／B 返回通过。此处只证明本次首发后的有限等待，不覆盖干扣扳机、收枪再取或完整手动换弹。

游戏与 XR 双路录制已编码并亲自查看两页完整保存跨度总览，以及肩取、首发、空膛等待的关键原图。游戏 957 帧、约 55.234 秒，模拟器 1180 帧、约 55.217 秒，末尾双路覆盖在 500ms 内。游戏最大捕获间隔约 1.833 秒，不能作为逐渲染帧／FPS 证明。画面可见实际握枪和暂停返回，但自然主握／副握／各部件 GrabPose 与完整贴合继续未验收，不能因本次自动检查通过而关闭。

Native 清理、入口恢复、虚拟输入恢复、XR exit0、launcher exit0 与游戏退出均确认。配置和模拟器设置已按原哈希核对恢复，头部命令恢复原存在状态，运行后 12 个驱动哈希均未变。证据在 `analysis/longshot-development-20261108/basic-21724-review.json`；录像在 `analysis/solo-m1-20261028/menu-firstperson-21724-startup-20261108-225640/dual-output/paired-output.mp4`。本次媒体清理和唯一交窗的最后状态另记在该 JSON，不以进程暂空自行交窗或启动下一段。

本次 retention20600 已 terminal0，实际删除 3704 个文件／660032393 字节，errors0，保留最近 21724／56220／54740 三次媒体及 48 小时限制。核对 idle 后已明确唯一交窗 Lancer212510，并转达后续 Frag 效果→Snub 原匣→HB C0 视觉顺序。`basic-prepared.json` 已同步真实 stage／recorded 状态及两次结果，原 36596 失败保留。本枪继续离线完善总换弹，不连续重占运行窗口，目标 active。

### 完整生产状态机的注入边界执行与三项恢复修正

本轮为 progress，没有启动新游戏。新增 `weapon_longshot_runtime_test.cpp`，原样包含生产 `weapon_longshot_runtime.inl` 和入膛函数，实际运行共享 revision 库存事务与真实 Frame／Decision／Lease 类型。注入的是 Native 骨骼与拉栓端点回执、原版引擎调用、资源、库存读取／setter、GT 与来源有效性边界。因此它证明状态转换、记账与恢复控制，不证明实际动画、手部行程、奖励引擎效果或画面。

完整执行已覆盖普通／完美整轮、恰好 4 秒普通、无实际 shot 的初始普通装填、供弹后放弃再取、插入后的 chamber debt 恢复、有限零备弹拒绝、无限备弹零 reserve 装一发，以及当前生产入膛函数。实弹插入始终只产生一笔库存写入，已入膛再拉栓不多扣一发，只有已确认的完美完成才调用一次奖励边界。

另外先实际复现三条失败断言，再作针对性修正，原 RED 文本均保留。

- 已闭锁、已记账后的原版退出暂时失败，原代码进入 recovering，但尚未发放的完美 receipt 又阻止恢复继续。现在保留同一 completing，重试原有幂等撤下／退出／clip 恢复，成功退出前不发奖励、不重新插弹。
- 已完成物理闭锁后收到 stop 请求，原顺序先走恢复，也会卡住待完成 receipt。现在先完成已提交闭锁的既有收尾／奖励，再处理 stop／switch 对新动作的阻止。没有新增输入或启动动作权限。
- 插入 setter 被明确拒绝、实际库存也保持 before 时，原代码仍留下预先保留的 chamber debt。现在先核对原 counts 并让账本 acknowledge 拒绝，随后清掉本次未入膛的预留 debt。未决、半写或库存不一致仍保留恢复状态，未放宽对账。

这三项修正及全部当前目标场景已执行通过。独立冻结 1072 份源码，Playground Release 完整编译链接通过，DLL SHA256 为 `e104456b681643b3f091f8a5e2f2d98b27eb32411051d2188f11d9a6b38bcc34`，按哈希另存。证据为 `analysis/longshot-development-20261108/product-runtime-recovery/build-result.json`；RED 文件为同级的 `runtime-exit-retry-red.log`、`runtime-stop-completion-red.log`、`runtime-rejected-insert-debt-red.log`。

本候选未 stage／未实跑，完整换弹游戏验收仍未完成。跨中断保留原始完美时钟／奖励、实际原版曲线与动作验证、膛室校准、全部 GrabPose／Snap、弹壳视觉、完整引导和近眼瞄准镜仍待完成。没有把 21724 的旧基础段或本次注入端点当新换弹通过。

### 原版奖励资格与跨中断原始时钟接线

本轮为 progress，继续指定 checkout／分支，无新游戏。核对真实 `lancerreload_state::perfect_reward` 接口发现调用前必须有 `capture_reward_eligibility` 的资格回执；先前夹具仅模拟奖励成功，没有体现这个前置条件。补入真实接口约束后实际测试 RED，现已在正常装填开始前读取并保留原枪奖励资格，当前回归通过。不能把先前过宽的奖励边界模拟当成实际引擎奖励曾经通过。

入膛后的中断现在保留同一笔 transaction、observer revision、插入前 opening 库存、原始 shot 时间和奖励资格。恢复沿用已有 `ObservedInventory::rebind_hold` 与 `ReloadTransaction::rebind_suspended/resume`：当前同一世界／玩家／枪／游戏与 Native 身份、输入／空间等身份仍须匹配，实际库存必须与已提交的一发一致，且需要新的真实抓栓激活。它不会重新 ForceReload 或提交第二发库存，也不把旧手部租约或姿态带回来。原资格回执通过既有同实例跨 hold 验证交给原版奖励接口。

增加了入膛前收枪再取的真实测试反例，原代码因 hold 身份改变而丢失已确认 shot 时钟、只能普通完成。现只允许完全匹配的同一实例保留这条不可变的开枪时间事实，所有实际动作仍需当前来源／租约／几何。输入来源不匹配时不续用旧时钟或旧账本。

生产状态机原样执行的测试已覆盖入膛前换 hold、入膛后同 hold／换 hold、重复中断、原始时钟下 `<4s` 完美和 `4s` 普通、一次库存提交／一次奖励，以及输入或库存变化拒绝恢复。原 RED 保留在 `runtime-reward-eligibility-red.log`、`runtime-interrupted-clock-red.log` 和 `runtime-preinsert-hold-clock-red.log`。这些是注入 Native／引擎边界的控制流证据，尚未证明实际收枪／动画／奖励画面。

独立冻结 1072 份源码的 Release 完整编译链接通过，最新 DLL SHA256 为 `52d05fbe6a1dc380c4e5de718771531c00042204bbb29b02923285e57a03925e`，按哈希另存；证据为 `analysis/longshot-development-20261108/product-interrupted-cycle/build-result.json`。本候选未 stage／未实跑。本节替代前述“中断后独立拉栓仅普通”的源码限制，游戏效果仍待验证；当前重取枪后使用新的完整实体开栓／闭栓行程，没有宣称部分行程和外观姿态已原位恢复。其余镜内放大／R3、完整 GrabPose／Snap、弹壳与引导、膛室校准和整枪游戏验收仍未关闭。

### 实际目镜拟合与当前帧近眼观察

原始中立 LOD0 模型后部玻璃在原模型 Y=20.38986 平面有 18 个环形顶点，圆拟合中心为 `(-2.581341104,20.38986,18.036765533)`，半径 `1.853067493` 原单位，即默认 100 单位／米时约 1.85cm。最大圆拟合残差约 `3.5e-6` 原单位。这个点替代早先只取镜片表面某个顶点的临时选择。模型 OBJ SHA256 为 `bc019162…95c7`，完整坐标／面／顶点记录在 `analysis/longshot-development-20261108/ocular-disk-fit.json`。它证明几何平面，不是出瞳、眼距或实际光学标定。

新增 `weapon_longshot_eye_contact.h` 和实际 `weapon_longshot_eyebox.inl`。沿当前枪模矩阵把该中心映射到世界米制坐标，用同一 ticket 的 rig base/reference 转换两个实际眼位。UE 的前／右／上轴与 XR 的右／上／后轴分开计算，不能直接把 UE 的 +X 前向送入 XR -Z 前向助手。近眼阈值与既定 80ms 停留／保持带沿用需求，菜单、换弹、失追或来源不匹配不保留 near-eye。观察回执绑定当前 FrameKey、inventory identity、reference/profile generation 和原始 100ms 时效。

生产观察源码的注入边界测试通过左右眼选择、当前参考／profile／年龄、换弹／菜单／rig 丢失；几何测试通过前后眼距、横向／角度和世界平移旋转不变性。正常持枪远离眼睛时先用原有固定 mount 做只拒绝不放行的宽相位，完整通过仍要求当帧真实 mesh/rig。tick 放在既有优先 fire/follow/reload 工作之后。独立冻结 1076 份源码的 Release 构建成功，DLL `717567008cbe39e2c5293531e4fd72d38d039b3ed47ce1c297baf1077886d623` 按哈希保存，证据 `product-eyebox/build-result.json`。该候选未实跑，尚未改变 targeting/zoom，也未消费原版 R3，不能计作可见瞄准镜。

### 原版倍率回退的渲染约束与 R3 消费准备

保存的原版 `SetZoomed` 会改 bZoomed、隐藏枪身并通知服务器；`GetTargetingFOV` 在高倍率返回 ZoomFOV，R3 handler 仅在 PC.bIsTargeting 时切换并处理声音／ExtraRot。原始导出保留未解析 token，仍需真实接口／效果验证。现有 Native `stereo_matrices` 会用头显双眼 FOV 覆盖投影的 0/5/8/9 系数，因此只调用原版 SetZoomed 不能证明 XR 图像真正放大。

首选镜内独立场景渲染仍需要新增受控 capture／纹理／镜片遮罩的完整 owner。现有纯光学 plan 没有 GPU 通道；原版 zoom 也没有自动给实体镜片生成画面。主人允许的近眼原版画面回退同样需要接最终视图选择，并保持当前同源瞄准方向和正常退出，不能用状态位冒充图像效果。这是源码评估，尚未宣布任一方案已运行通过。

为避免近眼 R3 被模组和原版重复处理，`vr_gamepad_input.h` 新增显式 `consume_buttons(mask)`：清掉已处理短按 pulse，并把仍按住的原版虚拟按键抑制到真实释放。记录组合键转换前的物理层级，保留双摇杆 Menu 组合，并避免退出组合键时迟到重放 R3。原有路由加新增脉冲／持按／组合／松开再按目标测试通过。默认无人调用时原行为保持，当前仍未连接 Native 导出或枪械 R3 owner，不改原始控制器、姿态、时间戳或其他枪行为。

已收到主人经统筹转达的等待规则：共享窗口排队时保持会话、有界等待并继续独立工作，不因正常占用结束等待或标 blocked。继续只用直控脚本和双路记录，无 Computer Use，不抢占持有者。

### Native R3 隔离桥已编译，失效自动释放已测

新增 `GearsCaptureLongshotScopeInputV1`，请求携带完整原始 Frame，只在既有 Broker 的当前 GT、原始 committed frame、相同身份／内容及 100ms 时效准入后接受。原版 XInput 仍经现有唯一 Route；本桥只消费 R3，不生成按键、不发布姿态、不提供手部动作权限。停机清掉隔离状态。新增源码已接入工作树 `capture.cpp` 和 `vr_gamepad_live.inl` 中事先协调的独立插入点，未替换公开或当前运行包。

检查退出条件时发现只等显式 disarm 会在来源丢失后留下隔离。修正版用原始 `controllers.sync_ns` 限定最长 100ms，按键采样不能续期；过期或时钟倒退自动解除，已消费的持按仍等真实松开后才可产生下一次 R3。实际生产 `.inl` 与实际 Route 的注入边界测试覆盖请求形状、Broker 拒绝、停机／禁用、排队短按、持按解除、真实松开再按、100ms 端点、超时和时钟倒退，执行通过。它仍不是实际游戏 hook 或倍率切换验证。

以已验证 Native `c98568f0` 的冻结源及全部原构建开关为基线，仅覆盖本桥和 Route 改动，Release 完整编译链接成功，DLL SHA256 `ae3bf39e1cefb85413fb54816c3224e53b8395035faee96e73f5af35f1971d83`，按哈希保存。导出表确认新入口存在。CMake 补齐的独立测试源只用于生成工程；实际 `GearsCapture.vcxproj` 的 C++ 编译输入只有冻结 `capture.cpp`，原生产汇编和 MinHook 来源另有哈希记录。初次缺失归档构建输入的 configure 失败及首个未加超时的 DLL 均保留，没有作为最终通过。证据在 `analysis/longshot-development-20261108/native-scope-input/build-result.json` 与 `exports-r2.txt`。

当前 Playground 近眼观察尚未调用该桥，也没有接通原版 targeting／zoom owner 或最终可见放大。此节更新前述“尚未接 Native 导出”的源码状态，不改变瞄准镜和整枪仍未完成的结论。该 DLL 未 stage、未启动游戏。本轮全树 `git diff --check` 被并发共享 `weapon_lancer_grasp.inl` 的行尾报告阻断，未擅自重写该文件；本桥自己的改动单独核查。

### 副握长方体与释放保持接线（2026-11-09）

公共 `SupportBox` 只计算以当前握点为中心的定向盒接触，独立于引擎对象、输入发布、资源与租约。`GripGameState` 增加默认关闭的盒字段；启用者才使用盒区域，其他枪继续球形分支。这个结构是 Playground 内部观察／准备状态，不在 Native 的 Frame／Proposal／Decision 或导出参数内；Native 源中不依赖此结构，没有为这次尺寸改动增加 hook 或相机／手部 writer。

本枪独立 foregrip 配置把原模型 +Y 作为枪身长轴，获取盒长宽高为 16×12×12cm，保持盒为 20×16×16cm，中心沿用已测护木表面到掌心／控制器握点的安装变换。盒轴随当前枪身相对主手的旋转变化，尺寸以交互空间米为单位，不随模型原始单位或测试渲染分辨率改变。UE／XR 向量基变换含手性变化，因此盒轴接受正交反射；它不会被误当作姿态四元数。

按照表 2.1 的既定需求，仅本枪启用盒时，真实副握超出保持盒会撤销控制；按住 Grip 返回获取盒不会重新取得租约，需要真实松开再按。松手留在保持盒内时，只保存已经选过的 socket 身份，重新验证当前原始手与当前盒接触后发布被动手部吸附，不复用旧姿态。实际 Grip 的权限继续由原有共享仲裁产生。

Longshot 分支现调用武器无关的 `gears_weapon_support_socket::mode`；HB 模块由其拥有会话提供原接口的薄转发，本枪不再依赖 HB 的专有模式实现。公共层不引用 Longshot 几何，Longshot 尺寸与手掌安装留在本枪配置。共享 grasp 文件只覆盖本枪分支及公共 include，没有把并发的其他 Lancer 编辑带入隔离构建。

几何测试覆盖三个轴的获取／保持边界、旋转、无效尺度与正交性。共享手部测试 69 例通过，包含原有 65 例和新增盒角点、当前旋转、保持区越界、真实松开再抓及已选 socket 变更；本枪完整生产换弹状态机回归也通过。最初几何测试错误地要求 UE→XR 盒基行列式必须 +1、第一次产品构建的新默认参数与旧二参声明冲突，均已修正；原日志保留。纯计算与本地主机测试不证明真实手部表面／五指贴合。

冻结 1078 份来源的 Release `37d936b5…609f` 编译链接通过；在同一来源上仅加本枪限量只读状态转换／机构几何回执，再次构建 DLL `601511a52b624204770699806661d692a03ad929289dfcdd519bded81c679259`，证据在 `product-support-handoff` 和 `product-open-observation`。这些回执不添加游戏操作或端点权限。已准备 `longshot-prepared-20261109-005036-manual-open`（1577 份组合来源），复用已验证 Native `c98568f0`／XR `16840b27`，尚未 stage／实跑。

下一实际窄段只验证真实开栓、左肩单发取出及释放，驱动只发既有模拟器手部姿态与按键。原始库存类型／身份双读沿用公共读取，几何日志只作输入规划，完成必须另有生产状态机和原骨骼端点回执。完整入膛／闭锁／奖励、可见瞄准镜、R3 产品接管和所有部件自然 GrabPose 继续未验收。已向现有统筹申请顺序运行窗，继续等待明确唯一交接。

### 近眼全屏回退的最终投影适配已编译（仍默认不启用）

原版状态改变不会自动绕过当前双眼 FOV 覆盖，因此增加独立纯计算 `weapon_longshot_scope_projection.h`，接受原版 getter 的目标水平 FOV 和已认证的枪轴方向，计算本次双眼窄投影。保留两个原始 view、视点间距、深度及其他投影系数，只调整 0/5/8/9，将枪轴远方方向投到画面中心。这是主人允许的近眼全屏回退通道，不是镜片内 PiP，也不改变弹道、身体基座、rig 或 HMD 来源。

新增 `GearsCaptureLongshotScopeViewV1` 与独立 Native 适配模块。准入沿用当前 Broker 的 committed 原始 Frame；原始对象和属性通过公共 `weapon_hand_scene_binding` 的身份、类、属性及布局证明读取。每次实际选择重新检查世界／参考／会话／空间、原对象链、当前枪模 LocalToWorld、当前双眼位置与目镜眼距。模型 +Y 作为原型枪轴，需在实际画面与权威弹道中复核，不能将模型方向单测当射击瞄准验证。失败保留当帧正常投影，不刷新旧 Frame 的时钟。菜单显示有独立的当前帧 veto；掩体和身体选择先完成，现有遮蔽及其来源继续生效。

生产纯计算及实际 Native `.inl` 的注入边界测试通过：原 FOV 两档、枪轴居中、双眼 view／深度保持、单眼失败整组拒绝、原对象链变化、当前矩阵离眼、来源拒绝、元数据失效、世界／参考变化和显式关闭。Broker 与引擎对象元数据／读取边界在夹具中注入，不等于真实对象元数据或渲染通过。夹具最初的对象类地址未对齐及全局名遮蔽已修正，未修改产品身份检查来迁就夹具。

独立冻结 1574 份来源，以前一已验证 Native 来源加本枪局部覆盖构建。首个 DLL `56c5ed7c…12f3` 及其精确 capture 源保留；加入当前菜单 veto 后 Release 再次链接成功，DLL `cf72bc5fdd26987c910580101c410e27463b2db5b32d6b70cd53d4e221175fee` 按哈希保存，完整证据在 `native-scope-view/build-result-r2.json`。源插入点已与掩体拥有会话明确确认，未复制当前其他会话的 Native 变更。

Playground 原版 targeting／zoom owner 和本导出的调用仍待接线，所以当前功能默认不启用，尚无实际镜像放大或 R3 倍率切换证据。本 Native 未 stage／实跑，已排队的 manual-open 仍使用旧 Native `c98568f0`。统筹确认当前运行顺序为 Cover r7→HB 原匣→Snub→Boltok→Longshot；正常等待不构成完成或 blocked。

### 按原序号完成 reload 收尾，避免影响后来租约

后续队列已消歧为 Snub→Boltok→Frag011230（原004429同一失败点）→Longshot005036，由 Frag 全部收尾后唯一交窗。本枪没有提前 stage／启动；前述队列文本是当时记录，不覆盖这次明确顺位。

公共原版状态模块提供 `recover_serial(serial)` 后，本枪两处清理已改为按自己的原序号恢复。该接口对零、外来或已经退出的序号返回“此序号不再占共享 lease”，不会退出后来取得的新 lease；它不证明旧零件恢复完成。本枪仍要继续自己的 clip、实弹资源、库存与奖励对账。

给生产状态机夹具增加可暂时失败的 clip 恢复边界，实际执行旧 `601511a5` 冻结 runtime：原版 lease100 退出后 clip 仍 pending，再出现 lease777，旧恢复清掉了777，触发保护断言，RED 保存在 `runtime-scoped-cleanup-red.log`。当前源码通过相同中断恢复场景，也通过已经物理闭锁完成但 clip 恢复 pending 的场景；两者均保留新777，旧 lease 的实际退出计数始终为1。第二个完成场景在当前源码通过，没有单独声明其旧源码 RED 已复现。

完整本枪生产状态机回归仍通过。独立冻结1078份来源，Playground Release DLL `7319548e1e7075ffe38b4d31ea394f1c075ed91a2ed8584e6e5e34146354de4e` 编译链接通过。公共状态源码只覆盖精确的 `retains_serial`／`recover_serial` 两个接口，没有带入其他正在开发的HUD或武器修改。证据为 `product-scoped-cleanup/build-result.json`。该候选未 stage／实跑；当前排队的005036仍保持原冻结版本，不以新源码测试替代其游戏结果。

本枪另公开 `pending_work()`，仅由模块自己判断非 idle 阶段，供组合层汇总生命周期，不能用作其他枪的新动作 veto。已测试原版 lease 退出但 clip 仍 pending 时必须报告 true、旧收尾完成后报告 false，同时保留777。隐藏预备资源和纯逻辑 chamber debt 分别沿用自己的资源 drain／射击阻止责任。该接口及回归加入后，独立 Release DLL `b4dd64e71393729d790872472995a5e55a735fae916747b512108543a6eb8a26` 编译链接通过，证据 `product-lifecycle-pending/build-result.json`，未 stage／实跑。共享 outstanding 的组合汇总仍由其拥有模块独立完善，不能把本接口完成说成全局生命周期已经整理完。

实际开栓驱动预检发现外层 `run_longshot_check.py` 过去将 `checks` 清空，没有把新 `--longshot-open` 传给内层 operator，若直接运行会只重复旧基础段。已修复本枪包装器；实际执行其转换、拦截启动 exec，检查生成 AST 的 Popen 参数，确认默认仍跑基础段、显式参数才进入开栓段，没有启动游戏。最初检查器只查编译码顶层常量，漏掉嵌套函数里的参数，因此其断言失败；已改为检查实际生成源和 AST，`manual-open-wrapper-preflight.json` 两种情况通过。

修正驱动后同一候选重新冻结为 `longshot-prepared-20261109-012911-manual-open`（1577文件），仍为 PG601511a5／Nativec985／XR16840，产品二进制未变，验证范围未增加。012911替代005036作为Frag之后的唯一接收项，Frag已明确确认。本段尚未 stage／实跑，没有把参数修正或预检当成实际开栓通过。

### 4632 实际开栓窄段失败与正常收尾

Frag2044 全部正常收尾后明确唯一交窗。实际重新核对1577份来源、6项制品、18个当前驱动及主人指定游戏 EXE SHA 后，精确暂存012911，使用PG601511a5／Nativec985／XR16840、0.3、既有后台直控路径运行游戏4632／creation134386334627412787。原版地面拾枪、实际肩取、权威单发、6秒24样本保持空膛及单发隐藏资源ready均取得本次记录。实际shot1游戏时钟为37851505us。

新手动段已实际执行到“移向枪栓并真实按Grip”，随后等待 `longshot_reload_geometry` 超时，operator exit1。没有实际开栓或左肩取弹通过。launcher exit0 与 runtime aggregate passed 只证明其覆盖的生命周期／提交过程，不能覆盖这条动作失败。当前日志没有区分入口拒绝、来源不可用、共享占用，以及“同帧取得后又恢复到idle”；不能把没有最终状态变化日志直接说成没有发生原版调用。

完整66.85秒双路三页总览及48.52／50.98／55.97秒三张原图已亲看，左手确实靠近枪身，未接受机械行程或自然手姿。游戏999帧、模拟器1429帧，双路尾部覆盖在500ms内；游戏最大捕获间隔约1.867秒，仍不是实际渲染FPS证据。没有宣布新瞄准镜或所有GrabPose通过。

Native清理、入口恢复、虚拟输入恢复、hand_stop5／drained、XR0、launcher0、游戏退出均确认。设置原SHA、配置原内容和头部命令原不存在状态核对恢复；18个驱动运行后无漂移。最初配置核对用read_text自动归一化换行，与保留CRLF的备份字符串比较导致假失败，改为原始字节解码保留换行后核对通过，没有重写配置来迁就检查器。编码31013 terminal0，retention77892 terminal0，删除1808个文件／326585823字节，errors空，保留最近三次／48小时约束。核idle后已明确唯一交窗Cover reader，后续按HB→Lancer→Snub原队列。

证据在 `manual-open-4632-review.json`、`manual-open-4632-contact-selection.json`，录像在 `analysis/solo-m1-20261028/menu-firstperson-4632-startup-20261109-014413/dual-output/paired-output.mp4`。本次失败完整保留，目标active。后续只补本枪真实输入／提案／决定、入口门槛、共享占用及Frame缺失的限量只读回执，定位上述未区分分支；不改输入、来源时间、范围、准入条件或100ms限制，也不重跑没有新信息的同一候选。

### 持枪模型近眼坐标回归与 scope view V2（离线，未部署）

现有权威枪口适配会把原版普通组件姿态映射到同帧持枪 `plan.target`。近眼判定此前直接使用普通组件矩阵的位置，可能与持枪显示计划不一致。实际生产 eyebox 回归夹具中，保持同一持枪计划与双眼不变，仅将普通组件恢复到远处：旧实现错误退出近眼，RED 退出 -1073740791 已保存。修正后从同帧、同武器／组件／资产／hold_generation 的已有效持枪计划计算目镜位置；原版组件仍用于认证真实对象、刚体和实际缩放。过期计划、无效计划、握持身份不匹配仍拒绝。

Native 显示包升级为 `GearsCaptureLongshotScopeViewV2`，携带该 Frame 的既有持枪模型姿态和实测缩放。Native 保留当前 Broker 准入、实际对象链、原版矩阵双读、缩放与双眼距离检查，再从此持枪模型计算目镜与轴线。它不再把已恢复的普通组件位置当成当前持枪位置，也不新增或续期 tracking 时间。最终投影选择位置、头部遮挡有效检查和 flat-menu veto 保持；不写 C0、rig、身体或 view 矩阵。模型轴线仍需实际瞄准画面核验。

生产 eyebox 与 Native adapter 的边界注入测试通过，涵盖普通组件恢复不移动目镜、真实 held-model 移出后拒绝、来源／引用／握持身份变化、过期、菜单／换弹、双眼及 disarm。完整隔离构建通过：Playground `4089bd5ca930a67efbb57c560df9bb649a889e16388901e93e42c79d04c5a1c7`，Native `664cf7e7d4b85a328208aef77233881e896124b2dfbae9053b40b615538688cb`；V2 导出已核验，源码归档及不可变 DLL 均验 SHA。记录为 `held-scope-result.json`。

这不是 4632 拉栓入口失败的因果结论，也不是渲染／R3／实体镜片画面通过。原版状态 owner 和 PG 发布调用尚未接通，`scope_ready=false` 保留。排队中的 `021930 / PG3cabf19e + Nativec98568f0` 继续只定位原失败，未混入本次坐标／开镜修改。

### 原版开镜调用适配层（离线，尚无生命周期调用方）

新增独立 `weapon_longshot_scope_original.inl`：仅通过现有 GT dispatcher 调用原版 `WarPC.SetTargetingMode`、`Weap_SniperRifle.SetZoomed`、`SetOwnerNoSee` 和 `GetTargetingFOV`。逐项认证原版声明类／包、函数身份、script hash、执行入口、参数名／类型／flags／范围／不重叠及闭合查找；不猜 bool 参数布局，不写倍率常量，不借用其他枪的实现。执行前复核原始当前 Frame 与 Actor 关系；清理接口只允许既有 action lifetime 下的 OFF，不允许旧来源打开开镜或换倍率。调用方必须在任何 setter 前登记恢复债务，该层本身不拥有输入或相机。

完整隔离 Playground 编译通过，SHA `362b98a00a01e3e0a8e8cef0331057fb4c2c4ff361449a493749c584ba93a470`。生产适配层的注入边界测试通过：三类 bool 调用、参数／掩码／越界／循环／外来声明／script 变化拒绝、当前来源失效、旧 lifetime 仅 OFF、真实 getter 返回值检查。测试不证明 retail metadata 已接受，也未执行游戏原版调用。

原版 `SetZoomed` 会调用同时影响枪身、身体、头部部件的 `SetOwnerNoSee`；该方法还改变 `bIgnoreControllersWhenNotRendered`。完整 owner 接入前必须测量这些实际原状态、明确可见性归属与精确恢复，避免低倍率枪身遮住全屏回退画面或退出后改变已有身体／头部显示。原版瞄准模式、真实倍率切换、R3 隔离、实际 FOV 发布、全部退出路径仍未连通，不能宣布狙击镜完成。

### 原版全屏近眼回退的源码流程已连通（未部署／未实机）

本节更新上一节“尚无生命周期调用方”的源码状态。新增 `scope_state` 只读适配与独立 `scope_owner`：测量控制器／pawn targeting、target key、bZoomed、原版 DefaultFOV、枪身／身体／两个可选头部组件身份及 bOwnerNoSee、身体 bIgnoreControllersWhenNotRendered；完整双读不一致或任一必需字段未知即拒绝。第一版只在实际原状态全中性且没有其他 L2 owner 时进入；已有不同可见性归属会在任何 setter 前拒绝，暂不支持任意非中性状态的还原。

近眼稳定后，先登记关闭债务并通过当前 Frame 隔离原版 R3，再调用原版 targeting 和 visibility；低倍率也隐藏枪身，避免实体镜片挡住全屏画面。原版 getter 的返回 FOV 送给 Native V2 显示包；R3 只使用近眼控制器确认的一次新按下，调用原版 SetZoomed 切换后重建低／高倍率下的可见性并重新取真实 FOV。同一 Frame 重入不能再次切换，不确定调用失败只走恢复，不重放按键。

离眼、菜单／换弹、来源丢失、身份变化、切枪和停止时退出：原版旧 action lifetime 只允许 OFF；关闭 debt 在实际 targeting／key／zoom／ignore／可见性和同一组件身份恢复后才清除。Native 在当前来源失效时拒绝旧显示包；R3 隔离还保留原始 100ms 超时和持按到真实松开规则。stop 服务、Frame unavailable、原 enable／drain 条件和正常停止请求都已局部接入。手动开栓只有真实 grant 后先退出本枪 scope，未恢复时不开始原版 reload，不改换弹计时或库存。

与 Lancer 协调后的 L2 插入只查询当前匹配 Longshot hold 的实际 scope targeting 债务；不把其他枪的私有 stage 当作阻塞。R2 同源权威射击路径保持，未采用 Frag 的整段 fire 退出方式。源文件仍各司其职：eyebox 负责近眼和按键边沿，scope_original 负责原版 typed 调用，scope_state 负责只读测量，scope_owner 负责本枪债务，Native 只消费原版 R3／选择投影。没有新增输入、C0、相机、骨架或 rig 写者。

完整隔离 Playground `b071234705d1c65b86f854944bf9224e9c85f5c4acdd9e6ebd16fa2bc908e66e` 编译通过；它与 scope Native `664cf7e7…688cb` 都未 stage／游戏运行。两次较早的编译失败分别为 Windows near 宏和 /WX 全局 module 遮蔽，原归档保留，已更名修正。原版调用、状态读取、scope journal、生产 eyebox 和生产 reload 的注入边界测试通过；覆盖已有可见性／L2 归属拒绝、Native fence 失败、一次 R3／同帧不重放、部分 setter 失败、延迟关闭债务、来源／lifetime 失效、换 hold、停止／yield，以及 scope 未退出前不开始手动 reload。真实环境反射／hook／画面并未由这些测试覆盖。

记录：`scope-composition-result.json`，测试 EXE 已按 SHA 另存不可变副本。实际原版接口能否在本版本匹配、近眼低／高倍率画面与准星、体感射线、身体／头部恢复、按键冲突及退出舒适度仍待受控游戏验证。实体镜片 PiP 仍未实现。原 4632 拉栓入口失败原因未证明；021930 原诊断候选不变，继续按唯一运行窗口交接执行。

### 49736：实际拉栓入口在 12.01cm 接触处拒绝，未获租约

Cover49584／65700 完整收尾后明确唯一交窗。先审查其共享只读 `world_state.py` 的五行修改：Query.prop 缓存键加入 structure，避免继承查询结果混入本类声明查询。原021930源码归档没有收这项外部 reader，保留这一依赖缺口事实；由原冻结派生 `021930-manual-open-reader-r1`，新增该文件 SHA／来源记录和结束时核对。枪械 Nativec985／PG3cab／XR1684、18个输入驱动及原动作顺序均未改；1578源码／6制品／18驱动＋1只读依赖预检通过。新 scope 均未部署。

实际49736／creation134386398105656748：原拾取、肩部取得、权威首发、6秒24样本空膛不自动填弹、round ready通过。真实原版 shot 计时为51917518微秒。随后真实左手 squeeze 到0.896484375时，Frame745 的 tracked／known／down 均1，busy0、animation0、main1；当前 root 局部手位置为(0.210708539,0.0949293835,0.179750655)m，到栓柄距离0.120099638m，大于原5cm半径。提案 request_magazine0，仲裁 no_candidate4、无lease／new_grant；入口gate1在原版begin前拒绝。本轮仍因等不到 opening geometry 退出1，没有开栓或取弹通过。

这是本次真实按下的入口证据，不能单凭它认定12cm来自控制器偏移、数学转换或某个骨骼。驱动使用的 planning marker 为Frame584，早于实际射击和Frame745；旧标记只输出最初两条，没有来源时间或当前 root／原始左右Grip的对照，因此仍不能把更深的偏差原因说成已证实。下一窄改只补当前空膛接触的有界只读数据、来源时间及实际Grip／规划端点对照，输入仍需真实靠近并按下，不扩大5cm或放宽任何准入。

83.95秒游戏／84.00秒模拟器双路三页全跨度总览与64.47／66.53／71.48秒三张原图亲看；接近栓柄与自然手姿仍未验收。游戏1098帧，最大捕获间隔2.317秒；模拟器1838帧，最大0.567秒；双路尾部覆盖在500ms内，捕获统计不是渲染FPS或头显验证。启动等待卡、空隙和失败图像保留。

Native／入口／虚拟输入／hand drain正常，XR0、launcher0、游戏退出；设置原SHA、配置原内容和原缺head精确恢复；18驱动＋新reader结束时无漂移。编码65148 terminal0，retention12945 terminal0，删3018文件／471140855字节，errors空，最近三局／48小时。核无game／XR／recorder／encode／retention后已明确唯一交HB023513，后续由其交Lancer023554，本枪不延续该窗口。记录：`manual-entry-49736-review.json`、`manual-entry-49736-observations.json`及保存的完整 `playground-49736.log`。目标仍active，完整手动／完美换弹、scope画面和全部GrabPose均未通过。

### 后续离线源码：scope 归属和可选零件握姿

scope 的 same-hold 进一步绑定完整 controller/weapon 注册身份、Native cookie、lifecycle 和 world generation；新的帧可以继续，变更来源不能续用旧瞄准归属。原型反例曾使断言失败，失败 EXE 和日志保留。关闭时若原对象关系已消失，仅在全部原接收者的相关标志已认证恢复，或精确旧注册对象已由现有只读退休证明确认不存在时清债；未知保持债务，不拿新 pawn 或过期输入补做动作。隔离 Playground `6e02e03d30d3564290a8c019cd1f2179a81558bbbec42dae5fdf0795ad14ee0e` 编译及生产 owner 注入边界测试通过，未部署／游戏验证。记录 `scope-cohort-result.json`。

为子弹和栓柄增加可选通用 `GearsVrPublishPartGraspV3`：外层选择 precision／cylinder，内层沿用原 V2 Request 和当前 Broker 认证；老 V2 导出、布局、默认手形均保留。Native 继续只经唯一 C0 处理皮肤目标与已有120ms过渡，不产生控制器、枪械、租约或动作权。原始输入／100ms／当前对象／线程／松手复核保留。隔离 Native `ff1c8126be0c4fc506c5de9709388c2b2dc6d624d73d71892348b6068bbd4b03` 编译通过，V2/V3准入测试通过；固定骨长／原始输入／owned-buffer几何夹具8090检查0失败。这些不是实际皮肤拟合验收。

Longshot 零件层正在接入上述通用接口：子弹以 precision 夹持弹壳，显示与实际装膛尖端共用唯一挂点；栓柄以当前已读取的骨骼支点到旋钮径向确定 cylinder 包握方向。纯几何测试已检查不同世界旋转／单位比例下指腹与弹壳同点、手掌接触和栓柄轴线一致、无效输入拒绝；实际生产持弹层的肩部、租约、跟随和撤回测试也通过。新零件发布器尚待组合编译和游戏验证；不宣称自然贴合、避免穿插或全部角度合格。

上述新 scope／握姿都不进入已冻结 `034956-manual-open`。该次受控候选仍是 Nativec985、PGf14695a1、XR1684，只补原空膛接触的有界只读 POD 和同次规划来源时间；不增加动作、不扩大5cm、不调整仲裁或装弹机制。其1578源码／6产物／18驱动／1只读依赖预检通过，已在Cover23420完全收尾后的明确唯一交窗下开始运行，结果另记。

### 65996：真实接触和新租约通过，换弹原状态入口失败

`034956` 实际65996／creation134386447353782239：正常拾取、肩部取得、权威射击、6秒24样本空膛和单发资源ready通过，实际shot game_tick_us为35802471。取用了Frame740、原sync979359634294200的当前空膛几何，规划时间979359723589200，标记年龄约89.3ms；真实请求端点与实际输入同时保留。实际Frame801左右原始Grip到达目标，左手tracked／known／down均1、raw squeeze1，局部旋钮距离6.39003719e-7m；request_magazine1、decision granted2、magazine lease3／id1／new_grant1。原5cm接触准入已通过，不再是49736的12cm偏差。

随后runtime gate4、serial0，未取得原版Reloading状态，最终没有opening geometry／取弹收据，operator1。该gate包含transaction.bind和原版begin两者，日志没有记录更细子条件，不能单凭gate认定原版函数已经调用。紧邻记录的game_difference6，以及下面生产提案回归，支持“同帧提案变更”的可复现源码缺陷；实际修复仍待受控复核。更晚Frame1036／1037的无当前接触／跟踪可用数据发生在首个grant之后，不是首个入口拒绝的原因。

双路67.88秒／67.87秒、3页全跨度总览和game00757／00816／00818原图亲看。游戏912帧、最大捕获间隔9.050秒；模拟器1256帧、最大9.150秒，双路尾部在500ms内。大间隔和启动等待卡保留，不能把捕获FPS视作渲染FPS，也未证明间隔根因；实际手／零件贴合未接受。

Native／entry／pad／hand drain／XR／launcher正常，game退出；config原内容、settings原SHA、head原缺精确恢复，18驱动和reader结束一致。编码68660 terminal0，retention4187 terminal0，删2951媒体／509266573字节、errors空、最近三局／48小时；核idle后唯一交HB044514。最初审查脚本多传一层dual-output路径，未启动编码，原FileNotFound日志单独保留；更正父目录后正常完成。记录 `current-contact-65996-review.json`、`manual-entry-65996-observations.json` 和完整 `playground-65996.log`。

### 同帧交互提案修复与050328（未运行）

新增回归用生产Longshot提案、真实`same_pose_consumer_observation`比较规则及原版begin边界注入：旧代码在同一Frame把request_magazine改成held incumbent，确实产生game_difference6并拒绝，RED EXE／日志保留。修复只在Longshot producer保存已成功提交的exact Frame的四类route字段；当前观察仍先认证真实Actor、输入、身份、时效、主手、busy、动画、mount。下一Frame由新状态计算incumbent；外来incumbent或基线route变化不会被覆盖。共享HandClient/Broker完全未改，不延长100ms、不保留过期动作权限。

修复后同一回归difference0，后续Frame发布incumbent；busy／animation／主手丢失／输入或reference变化／外来incumbent仍拒绝。完整手动循环、严格4秒、终止／换hold／原版恢复债务测试保留通过。新隔离PG `3133acaf541d984a8782e5e38c802e2a7f4f86e105a17ae40b00c4a4df6538cd` 全编0，从原f146观测冻结仅提取该修复，不混scope或新握姿。归档fixture首次带了未组合scope的专用yield断言，因此单独保留失败，并在测试副本只剔除该不适用用例后通过；实际归档生产runtime未修改。现`050328-manual-open`的1578源／6产物／18驱动／1reader核对通过，待Cover后续唯一交窗，结束后唯一交HB050438。这只是准备状态，不是游戏修复通过。

### 单发／栓柄GrabPose已组合编译（未部署）

后续组合PG `981e6c7f70e032edb0a3d54bcdeb3f1d895d8bd9a6db6a19a8c4cea47608ad98`，1091源、全编0；与Native `ff1c8126…b4b03` 的可选V3头文件完全一致。Longshot零件层只选择几何和手形：栓柄首次吸附球5cm、已吸附／抓住后的保持7cm，真实Grip租约才是动作owner；松手在保持区回Snap、离开后重新进入需要5cm，按住Grip返回不凭皮肤吸附取得新租约。精确当前Frame／source sync的骨骼POD才可产生目标，右手使用同一计划内未转向的真实Grip。未支持V3的旧Native不接受这个可选皮肤请求，不改变动作流程。

从组合归档实际编译并执行：几何变换、生产零件发布器、生产持弹层及生产runtime均通过；scope owner相关测试在此组合版本保留。原Native C0唯一求解器、固定骨长、输入保护及默认V2路径均保留。当前子弹指腹和栓柄手掌接触是作者定义的几何目标，仍需原图检查穿插、朝向和完整表面贴合；本组合尚未排入运行，不与050328机械修复复核混用。

### 后续吸附时效与方向引导（源码／离线，未运行）

已吸附的皮肤保持元数据增加原sync连续性检查：与上次有效皮肤请求相隔超过100ms或时间倒退，就不再享有7cm保持区，必须重新进入5cm获取区；这不是续租或新动作。该边界的归档生产发布器测试通过，组合PG `bf3318be00ba3dddf49dd856728b8e1e98f9e38706c8e99d388c7183cc7b3d32` 全编0，记录 `part-freshness-result.json`。

随后在该组合上添加单帧方向图形：持弹时绘出沿实际Root+X入膛轴的8cm深／6cm半径接近轮廓、1.5cm横向就位圈和向前箭头；开／闭栓的箭头从已经读取的原版曲线和当前目标进度采样下一小段，不凭引导完成机构动作。保留球形提示作为现有路径。所有线条通过原Actor.DrawDebugLine的声明类、参数类型／范围／非重叠、Vector类型、Native入口认证后绘制；无持久线。仅可选绘制批次最多8ms，每条线仍检查来源／函数身份和原100ms时效；外层动作／恢复不等待轮廓完成。

最终后续组合PG `45359b4b1a9aa05889d7b3292dab44c677c792b64eed84df97ec175b2425b01c`，1093源、完整编译链接0；从其实际归档编译的几何测试检查了尺寸、插入方向、世界旋转和单位比例不变性，通过。Native仍为ff1c8126，同帧修复、scope和V3握姿均继承。未stage／未游戏运行，绘制是否可见、易读、遮挡或影响操作仍需实际审查；文字／音效未实现。记录 `guide-shapes-result.json`。已排队050328仍是3133acaf/c985，不包含这些后续改动。

### 71272：同帧修复已越过原入口，后续准备gate7失败

Cover61492全收尾后明确唯一交窗，050328再次1578源／6产物／18驱动／1reader核对一致。实际71272／creation134386471850461237：基本取枪、肩部取得、权威射击、6秒空膛及资源ready继续通过。原shot game_tick_us=36000591；Frame824左手到栓柄6.02123759e-7m、request1、decision granted2、magazine lease3／id1／new_grant1，随后实际0→acquiring、serial1，证明原版begin已acquire，旧gate4确已越过。

Frame825出现gate7（clip.service非ready或bolt.prepare拒绝），没有opening geometry／取弹收据。后续实画面中枪械原版动画将枪带离双手，最后真实释放后acquiring→idle。当前日志仍不能区分停止节点、序列认证、控制准备、曲线或骨骼端点的具体子条件，不把“看见动画”单独当节点原因证明。完整手动循环仍失败。

双路3页全跨度及game00840／00851／00861原图亲看；游戏999帧／65.234秒／最大捕获间隔1.567秒，模拟器1396帧／65.267秒／最大0.300秒，双路尾部在500ms内。所有恢复正常：Native／entry／pad／hand drain、XR0、launcher0、game退出，config原内容／settings原SHA／head原缺、驱动和reader结束一致。编码86601和retention74123均terminal0，删2021媒体／351190123字节、errors空；核idle后唯一交HB050438。记录 `begin-publication-71272-review.json`、`manual-entry-71272-observations.json` 与完整 `playground-71272.log`。

### 原版clip／身体姿态接管修复060433（未运行）

使用reverse-engineer-anything工作流，先查询主共享KB `J:/gearsVr/analysis/re-kb/knowledge.sqlite`，核验保存脚本SHA后读取原文；没有使用本checkout的空占位KB、启动游戏或打开native分析数据库。原版PlayWeaponReloading会按WeaponReloadAnim启动枪械动画；PlayWeaponAnim另有CustomAnimNode和普通sequence路径；FreezeWeaponAnim在调用时读取当前节点。脚本含未解析native token，CustomAnimNode的实际节点选择仍不能由静态声明独立证明。71272的更细运行原因保留未知。

源码另确认，原身体换弹停止助手原只包含其他已适配类型，Longshot直接返回。本次经Lancer／Boltok双方确认编辑非重叠，仅增加Longshot类型和白名单条件两行；原StopWeapon条件、所有typed／identity／owned_state、序号恢复及其它dirty内容保留。普通Longshot流程接管原版已经启动的LS_Reload，经现有动画服务acquire_prepared认证并停止准确当前节点，不再二次Play。动作前的读取不足则仅等下一真实新Frame，单Frame不重复读取尝试；原版状态、真实magazine租约、当前对象和原100ms时效仍必需。停止后的原版自动时间推进按需要通过同一已拥有的禁用notifies seek归零；真正开／闭栓仍由实际骨骼与物理手行程判定。

clip恢复类／拒绝结果现在立即转清理；准备中的未知状态仍保持pending。新增至多20条状态变化POD记录，区分adopt／capture／controls等阶段，不额外读游戏对象。仅后续提示从动作路径移到当前follow和瞄准处理之后，避免先花真实输入时效绘图，左肩提示也可使用本帧计划；其角色仍是提示，没有完成权限。中断后已有膛内实弹的独立gun-only Play分支未由普通流程修复认证，仍待独立闭合。

隔离PG `6814108b9626c6680c629a79ffa9745a0b27f42a1ae0ae64e6b68ff20c60fbc0` 完整编译0；实际归档adopt适配层、runtime回归通过，覆盖无二次Play入口、准确所读节点、一次原Stop接管、新Frame读取重试、未知／外来owner拒绝、恢复债务及准备失败清理。引擎／metadata边界为注入，不是原版调用运行证据。首次构建失败是旧冻结动画接口不接受新增第四个诊断参数，失败归档保留；已改用兼容的三参数调用，不复制另一枪的当前动画服务。

`060433-manual-open` 1580源／6产物／18驱动／1reader冻结预检通过，Nativec985／XR1684不变，未stage／未运行，不包含scope、V3手姿或方向图形。登记顺序为Frag053206→HB055335→Frag053956→Longshot060433→Cover r12，最终交接是必要条件；Cover准确plan为b5e097b018a98bd1d576d088e60cb80a778ebd8a4a4853ca3035dc370c42373a，旧ddc91b7a不用。记录 `original-clip-result.json`、`original-clip-queue.json`。


### 063907：接管停止时间校验与本次窄段验证

发现原capture在核对停止／seek前先更新缓存，可能遮蔽意外的时间或速率变化。现在动作读取先使用同一冻结公共reconcile_seek，成功后才可更新缓存；该冻结版本对待定seek仅接受请求时间，旧时间／未知结果继续拒绝。只有恢复路径允许Stop-only读取，不由该读取提交seek、机械端点、弹药或奖励。未修改公共动画服务。

PG `d0cd6edee9214534fc4fa9aa7066409cb2280009382481f607b0a5319798ae77` 完整编译0；实际冻结clock辅助函数与原公共reconcile提取件测试，以及完整runtime回归均执行0。测试覆盖停止时间／速率漂移、未知seek、待定旧时间拒绝和清理不推进证明，仍为注入引擎边界的离线证据。旧060433未运行而保留；新063907为1581源码／6制品／18驱动加1reader，Nativec985／XR1684。Frag063214实际68512完整收尾后明确唯一交窗，再核所有冻结和依赖通过并stage。仅运行既有开栓／肩取单发／释放段；实际结果另记。后继唯一为Cover reader-r12+eye-r13，prepared cover-r13，plan6838d7b93804c01aac3e101d8d20be602bcead458714d02459067faae1b7e274。


### 8092：原版clip停止通过，栓柄准备仍失败

精确063907／PGd0cd，game8092／creation134386514191954964。正常取枪、原肩部取得、原权威射击、6秒保持空膛及单发资源ready过，shot game_tick_us=36306198。Frame811到把手3.05e-7m并取得新magazine租约，原版clip报告ready2／adopt0／capture0／service0、playing0／stopped1／time0、序列长3.33333325。Frame812仍gate7，未发布栓柄路径／opening geometry。clip报告只记录键值变化，其后没有不同clip状态；范围缩小到栓柄prepare，但曲线读取、实际闭合骨骼、路径或时效的更细原因仍未记录，不作因果修复声明。

3页完整63.85秒双路总览及game00824／00830／00837原图亲看：本段枪械不再随原版换弹摆离双手，手枪拟合仍明显未完成，也未实际开栓或左肩取单发。游戏982帧、最大捕获间隔1.800秒；模拟器1366帧、最大0.333秒，双路尾部500ms覆盖过。启动等待卡／缺口保留，非逐渲染帧／头显证明。

runner87008及launcher0，operator1真实失败；Native／entry／pad／hand drain恢复通过、XR0／game退出，config原内容／settings原SHA／head原缺精确恢复，18driver及reader结束一致。编码34413／retention72543均terminal0，删2718媒体／473729225字节、errors空；刚核idle后明确唯一交Cover reader-r12+eye-r13，对方受理，仅使用其冻结cover-r13；本会话不在该窗追加case。记录original-clip-clock-8092-review.json、manual-entry-8092-observations.json与完整playground-8092.log。


### 065312：栓柄准备拒绝细分候选（未运行）

仅在原有曲线读取和prepare步骤保留已读POD，失败区分clip、curve、单位／骨骼、闭合位、mesh、路径与最终来源检查；变化记录最多16条，不增加读对象、重试、输入、权限或容差。解码gate也记录原有frames／format／offsets／bytes值。失败不发布准备状态。PG `84593cc90b477ee877081f171d36c321c734a3032a35b5a737c90c04384e3b9a` 全编0，065312共1581源／6制品／18driver及reader预检一致。准确冻结prepare函数、真实BoltPath与注入引擎边界测试执行0：验证位置／角度／根骨异常／未知来源拒绝且未发布，成功才提交一次；首个测试编译因tmpfile弃用警告被WX拒绝，原日志保留后改用已检查tmpfile_s。

本次同一请求排在所有更早受理的Coverr13→Snub060545→Boltok063023→Frag064829后，等待实际尾端确认；唯一后继已受理HB065400-refire-supply。没有stage／运行，也不借空闲抢窗。后续完整组合已把停止时间校验和此观察合入，PG `90c8067ccba05f35ab1be83b692bc06c7d0778cb77bce1cbdbd7dd0843482e92`／Nativeff1c8126、1096源完整编译0；公共动画服务和clock辅助函数与已执行测试版本完全相同。该组合未部署，不用其编译结果声明scope／GrabPose／引导或全换弹通过。记录bolt-prepare-observation-result.json。


### 原包确定性回归与070405同位替换（未运行）

重新查询主KB的package_objects和WarfareGame artifact后，复用已保存的原包、完整export inventory及名字表，不再以LS_Reload显示名称作对象路径查找。包SHA91c0f56e、正常动画export SHA5db59fb2f47b8229f7dafc57be9a7b30c60171cc0d85329723915ce13fd4ab4f均核验。实际正常LS_Reload的RotationCompressionFormat为IntervalFixed32NoW（3），NumFrames100、压缩栓柄平移／旋转各50键，896字节；失败动画才是Float96／160键。原生产只接受Float96及1或NumFrames键，确定拒绝正常动画，不能把先前所有假曲线测试当正常原包覆盖。

旧冻结decoder读取同一实际896字节流RED退出1；修正格式和独立压缩键数后，新冻结decoder50键与独立Python interval oracle全数一致，原始BonePath可绑定；原Float96、损坏count／span／format／range及失败不发布检查通过。位域公式另核对[固定版本UEViewer定义](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMeshTypes.h)。不改变2mm／2度、真实手行程、100ms或任何原owner／库存／奖励条件。原先“正常动画Float96／键数等于帧数”的假设由实际数据纠正。

隔离PG `90b5e763cb849ab20dc8f483f90d7b4d3cd28dff726c3ad81a58bc912c7408f6` 全编0，实际冻结源两项回归执行0，070405仍1581源／6制品／18driver＋reader全核。未运行065312仅在同一已受理尾位替换，旧freeze保留；最终前驱Frag064829，后继同位更新HB070119／PG30239f21。没有stage／游戏运行，也未证明实际运行时解码、存储坐标到骨骼约定或原插值时间；其余实际骨骼／路径检查继续保留。记录original-interval-candidate-result.json和真实原包回归日志。


### 后续完整循环的输入规划（未接入070405）

后续组合增加已成功肩取实弹的同一item mount POD（上限16条、无新增对象读取），用于实际插弹输入规划；绘制弹体与物理弹头仍共用一个mount。原始hold／lease／来源／资源撤销测试从实际归档编译执行0。首个完整构建因局部reported遮蔽资源层同名变量被WX拒绝，失败保留，改为mount_report_rows后隔离完整PG `1f427be04960125a6ae23d0a5fd28b27750f29ebb4ab02cf9dc0339663d5af17`／1096源编译0，Native仍ff1c8126。未stage／排队。

独立离线bolt输入规划器从实际33个路径点生成连续轨迹，保留初次原始手接触偏移，关闭时反向路径，固定原控制器朝向，包含速度限制；不向产品写姿态或授予机械完成。开闭两方向50个规划样本检查通过。此前弹头规划器已用独立四元数重建覆盖96种单位／枪体比例／姿态／mount组合及两个极点。两者均尚未接线到真实driver，070405的18驱动没有改动；实际物理路径、插弹和奖励须另行验证。


### 后续闭锁计时回执（未部署）

已有物理闭锁与transaction确认成功后，记录本轮原始shot_tick_us、closed_tick_us、elapsed_us及请求结果；不额外读取游戏，不改变严格4秒条件、库存或奖励。后续PG `dd559233d8e43cb6d35fb4f56c57ec6abe8a0e8885d6931ac8337ce7a89f4366`／1096源完整编译0，实际归档runtime测试执行0，覆盖原完整循环、4秒边界、旧起点／换hold／中断奖励及恢复债务；Nativeff1c8126不变。尚未部署，也未接入排队070405；实际闭锁和完美奖励仍待证。记录closure-clock-receipt-result.json。


### 50728：元数据契约拒绝，正常收尾

Frag071258／15196最终明确交窗时，保留对方Native清理超时／drain0／entries0／launcher1失败；其游戏、XR、录像、编码及retention全部终态、配置精确恢复，本会话未把那段失败改成正常通过。收到唯一交窗后070405的1581源／6制品／18driver＋reader再次全核一致并stage。

实际50728／creation134386549558313796，shot_tick_us37689041；基础肩取、权威射击、6秒空膛及单发资源ready过。Frame818真实新租约与原clip ready／stopped1／time0过。Frame819 bolt gate3／curve_gate3，在字段认证之前返回，没有数据解码、闭合骨骼、栓柄路径或取单发成功。静态主KB完整Engine.AnimSequence对象清单包含NumFrames／两个CompressionFormat／两个Compressed数组／SequenceLength，却没有KeyEncodingFormat；原七字段合取要求不存在字段，因此足以拒绝，但本轮未区分更早六项各自结果，不能夸为实际sole cause已证。

3页65.450／65.500秒全跨度双路与game00843／00849／00857原图亲看；游戏1000采样、最大捕获缺口2.133秒，模拟器1400采样、最大0.350秒，尾部覆盖过。枪体在接触时稳定，手枪拟合未达标，无实际开栓／取弹。Native／entry／pad／hand drain正常、XR0／launcher0／game退出，config原内容／settings原SHA／head原缺精确恢复，18驱动及reader结束无漂移。编码42093、retention82021终0，删4663媒体／915976716字节、errors空，核idle后唯一交HB070119。收尾reader首次宽glob误纳hook-entry／restored sidecar，在写记录前拒绝；已改显式本局report路径，原错误保留，未改产品或恢复。记录interval-50728-review.json。


### 075334：旧式AnimSequence字段契约（未运行）

只在Longshot动画读取层加入精确Engine.AnimSequence类及完整稳定UField链认证，证实该版没有KeyEncodingFormat，才按旧式uniform压缩流读取实际存在的六项字段。不存在属性的普通lookup失败不产生兼容资格；必须完整遍历并重新核对所有链接／身份／父类结构。新增selector、外来outer、类型未知、链循环／过大、断读或中途变化均拒绝。原始typed、extent、字节全覆盖、双读及100ms／真实机械端点要求不变。日志第三format槽的0表示已认证的旧式uniform契约，不是读到一个不存在的字段。

实际归档helper通过稳定类正例及15个拒绝分支，元数据边界为注入，非实际游戏字段读取证明。隔离PG `ed9a5bbad953b6b1e4266167d2af72bca80c4e4d73f1ccbfe80a8985ad1365e3` 全编0，075334共1583源／6制品／18driver＋reader预检一致，Nativec985／XR1684复用；原896字节／50键解码及Float96回归因相关源完全未变而复用。尚未stage或游戏运行。仅申请所有更早已受理的HB070119→Coverr16→Snub071103→Boltok072100之后同一窄段，等实际尾端确认唯一前后继。记录legacy-schema-candidate-result.json、legacy-schema-queue.json。


### 实际read_curve的字段契约RED／GREEN回归

分别提取旧070405和新075334冻结源中的完整生产read_curve函数，给定真实896字节／50键动画及原始字段接口：旧函数在请求未声明KeyEncodingFormat时RED退出1，尚未读stream；新函数GREEN执行0，仅请求六项实际字段并认证旧式schema边界后发布50键。另8种字段缺失／unsupported格式／帧数不一致／源或数据、节点变化均保持输出原值。类／元数据／引擎调用边界仍为注入，完整类链helper另有实际生产15拒绝分支测试；不声称游戏字段已通过。记录curve-reader-fixture-source.json和curve-reader-red／green日志。

后续完整组合PG `8abb1f9f8a0f67351fa942f0e471e5de6f17d96ba43f9c606b217cc2f4f1ca22`／1098源全编0，使用与075334完全相同的本次两个修改源，包含前期scope、V3握姿、方向引导、item mount与闭锁时刻记录；未stage／排队。运行尾链已直接去重确认为HB→Coverr16→Snub071103→Boltok072100→Frag074333→Longshot075334。Frag已核075334 prepared5eb6dc7c／plan db546879／1583源6制品18helper＋reader一致；这只是尾位受理，不是当前授窗。本会话尚无受理后继。


### 后续完整循环与近眼检查入口（仅源码，未运行）

独立full入口复用原wrapper／operator／核心取枪与射击流程，只在自身副本选择单次普通完整循环或额外一次练习完整循环；原18个open helper保持原SHA。链路以实际新lease／物理路径／肩取item mount／原始弹头插入／闭锁时刻／真实计数及奖励字段判结果，严格4秒不因输入慢而放宽。输入所有者失效注入回归证明一次已发输入后停止后续输入，不再复位pose；纯bootstrap锚点／AST和原文件不变检查通过。当前仅源码，未准备组合运行包、未stage／排队或运行，不构成完整循环通过。

近眼输入规划用真实镜片相对Grip变换与眼位反解控制器目标，64组双眼偏移／头部旋转／极点及离眼目标由独立四元数重建通过。首份规划在极点因asin舍入出现约1.5e-8rad伪倾斜；改用完整矩阵行atan2求pitch，未调宽oracle容差，scope64及feed96+2通过。该规划仍无游戏调用或输入；实际镜片mount／scope状态与画面另证。

另查询主KB逐一复核四个scope原函数及参数名、八个现用状态字段，均有保存声明；这是静态清单核对，不是实际内存布局、调用成功或倍率图片通过。记录future-test-preparation-result.json及scope-saved-contract-audit.json。


### 65324闭合位旋转约定拒绝与085615候选

实际65324／creation134386585341708092已越过六字段和旧schema验证，curve_gate0；NumFrames100、格式3/0/0、16offsets、896bytes，完整原轨迹50键。闭合Bone实际P(17.9751892,-0.0772024846,10.5666047)，Q(0.00848905012,0.00651679957,-0.707132657,0.70699991)；原轨迹闭合Q的XYZ相反、W相同，gate6角误差179.965367°，位置5.899e-7m。操作者最终缺少fresh geometry回执退出1，保留实际失败。

固定对原轨迹四元数做共轭，放在生产read_curve的Native相对变换适配边界；原始解码、位置、键数、时序、库存／输入／所有权和2mm／2°校验均未改。先全样本验证再转换，非法输入不部分修改。真实原包RED1复现179.965367°，GREEN0得到8.5377e-6°／5.8984e-7m并可绑定50键路径；提取新冻结生产reader的真实896bytes正例与8拒绝执行0。仅闭合Bolt有实际测量，其他骨骼和动态插值仍待游戏证明。

新窄版PG `92eb8f719caf2518edd14bb8e006c8dd7e1d0bd2a9007eefc599423bc48ca778`／1084源、完整组合PG `a09cd8325d1f0bf3857a6035ad02d7a7e8475f87cdae222e056e96aecfcda16c`／1099源分别全编0。085615窄版prepared f92db796／1584源6制品18helper+reader全部核对通过，已由Bol085057明确受理为同一尾位之后的唯一后继，尚未获最终交窗／stage／运行。085657完整组合配Nativeff1／3183源、6制品、18基础+7后续helper+reader核对通过，未申请运行；旧083748保持未运行历史。当前排队链Coverr19→Snub082247→Bol085057→Longshot085615；085057只替换原084022同一槽。

本次3页双路完整跨度及3张原图已亲看，1010游戏／1424模拟器帧，跨度66.58／66.60秒，采集最大缺口2.033／0.267秒，保留启动第三人称／等待卡；不当逐渲染帧或自然手姿证明。自身正常清理／所有原字节恢复通过，编码0、retention0／errors0，实删3912媒体／760141638字节；独立idle后唯一交Coverr19。详见legacy-65324-review.json、native-convention-regression.json。


### 原轨迹采样时序的独立参考核对

固定commit的[UEViewer采样实现](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/Mesh/SkeletalMesh.cpp)使用均匀键时间，与当前 time/SequenceLength×NumKeys 的采样位置一致。真实50键的相邻最大旋转约62.99°；每段99个分数点上，当前归一化线性插值与球面插值的最大差约0.310°。这只是外部实现／静态数学参考，不能证明本游戏Native动态时序，故未修改采样时间或2°实际一致性门槛。详见stock-interval-interpolation-check.json。


### 后续近眼R3验证准备（未运行）

目镜规划记录仅复用当前已绑定握把→模型矩阵和已有双眼Pose，最多8条；倍率状态记录仅复用已读取并传给显示请求的原版zoom／FOV／body和head可见性，最多32条，明确image_verified=0。没有新增Engine查询、Hook、Native或骨架写者。完整PG4702a3ee／1099源编译0，原眼盒／scope所有权测试执行0；原GetTargetingFOV保存恢复文本SHA de997be5与知识库一致，zoom分支返回原ZoomFOV，普通分支调用父方法，仍须实际元数据和画面验证。

未来scope只读快照工具使用现有持有reader，类型／位掩码／双读值／组件链接／描述符／对象身份均关闭复核，11项拒绝及中立／低／高／缺省第二头部组件离线通过。独立输入案例已准备镜外R3、镜外按住后贴眼、松开后新R3、持续按住、持按离镜／再入及最终原状态精确恢复；只发正常控制器输入，不执行原版状态setter。6文件AST、两个原runner精确替换／4拒绝和丢失所有权后零后续输入离线通过；第一次离线检查误用CRLF原字节而非生产read_text换行归一化，锚点拒绝保留，修正检查后执行0，没有游戏影响。

092652-scope已冻结3184源／6制品／18基础+8专用helper+reader并核对通过，PG4702a3ee配Nativeff1；未排队／stage／游戏运行，仍先完成085615实际开栓窄段。085615二进制和18驱动未变。记录scope-check-prepared.json、scope-snapshot-test-result.json、scope-driver-preparation-result.json。


### 16872实际闭合位通过、HUD起点与动画receipt离线冲突修正

085615／PG92eb8f实际16872／creation134386612967401307越过closed gate0／curve0并发布33条真实handle路径。实际误差1.27348e-6m／1.02453e-5°，本次闭合旋转转换通过；还未构成动态开栓通过。Frame813 HUD配置成功，Frame814 capture1／service2失败，操作者最终缺fresh geometry退出1。两个controller sync相隔110.4793ms，但该capture首门是owned／GT／clock reconciliation，日志不足以归因为输入间隔；没有放宽100ms。

共享HUD setter明确把ReloadStartTime写成本发reward_start并更新其精确journal，原动画read_owned仍要求旧reload_start，先时钟校验后刷新cache导致冲突。以实际冻结shared reconcile函数、注入typed读取边界，旧clock RED1复现该变化对停止time0动画的拒绝；新clock GREEN0接受仅同一已成功HUD setter的显式回执，11种producer admission及10种时钟／节点／事务／对象变化拒绝且缓存原子回滚。未知新起点、seek_pending和自行观察到的变化均不能自动接纳。实际16872未单独输出起点数值，因此记录为源码和离线证实的冲突，不能称已证实本局唯一原因。

窄版PGed239336／1084源、后续完整组合PG5ae303f8／1099源全编0；冻结窄版和完整runtime（含ack失败清理／严格4s／恢复／新租约保护）执行0。094103已冻结1584源／6制品／18原helper+reader并核对，prepared fca1f094／plan b0d150f1；已由实际尾Snub093107直接确认为其全终后的唯一后继，尚未最终交窗／stage／运行。完整组合尚未重新prepare。无shared动画源码、Native、hook、输入、库存或门槛改动。

本局3页全跨度双路+3原图亲看，1009／1426帧、66.667／66.650秒，最大采集缺口1.917／0.267秒；HUD已出现，前臂遮挡栓柄，完整手姿未接受。正常Native／entry／pad／drain／XR0／launcher0／game退出、配置／settings／head原字节恢复及18driver+reader无漂移；编码0／retention0 errors0，删884媒体／136577426字节，独立idle后明确唯一交Frag090321，再Coverr24和Snub093107。详见convention-16872-review.json、owned-ui-start-result.json。


### 后续完整流程补齐未闭锁阻断和真实返射（源码未运行）

独立longshot_full_chain未来入口新增普通流程中“已插弹但未闭锁”的实际R2按下／释放，核对原实发回执无新增、used仍0及备弹不变；流程完成后实际返射，要求新原实发回执、used从0变1且备弹不变。每次运动和按键均通过已有reader确认仍为同一Longshot；不确定后零后续输入的注入回归执行0。仅完整流程源码改变，需新freeze后才能进入其未来窗口；094103原18helper及scope092652专用8helper未改，没有在当前窄段增加动作。


### 095812同一队位身份更新与有限本地日志

为区分成功producer ack与之后原节点clock拒绝，仅在clip_clock加入最多8条已有本地journal／cache数值；日志明确node_values_are_cached=1，没有额外节点／Engine读取、调用或权限。PG913725d8全编0、冻结实际clock正例及21拒绝执行0，095812／prepared9f772390／plan50ed9829核1584源6制品18helper+reader通过，未stage。Snub已明确接受它替换旧094103同一槽，先序仍Frag090321→Coverr24→Snub093107→Longshot095812，无第二申请或前移；本枪全终唯一后继登记Bol095943，其最终plan-v2 95b0dc72／21driver+74支持依赖由该会话维护，当前不授窗。


### 后续引导绘制顺序优先显示方向与入座

原膛口31条一帧线的箭头排在末尾，既有8ms绘制预算可能截断它。只调整纯几何队列：前三条送弹箭头，其后8条最终入座小环，再画外轮廓；全部尺寸、总线数、原DrawDebugLine调用与预算不变，没有任何新增Engine函数／调用或机械权限。几何维度与旋转／单位不变性测试执行0，最新完整组合PG37095c9b／1099源编译0，未prepare／排队／运行；095812窄候选不变。详见guide-priority-result.json。


## 附录 既有运行与候选记录

以下记录按原时点完整保留，其中“当前”“未接入”“未受理”等表述属于当时状态。最新组合／排位和验收结论以第五节本次更新为准；历史失败不撤销、不冒充已修复。

172406后续候选已编译、离线检查和静态准备，未部署／运行：只在私人冻结Main加入optional头／两个前声明，backend对当前选中且入口idle的Longshot使用现有逐次鲜读Scope，所有已活动机构和别枪在入口不启用。原observer／proof／HandClient源码完全保持，证据变化仍完整重读，读取失败仍拒绝；无100ms／Frame／身份／租约／输入／几何的放宽，没有额外Engine查询或时钟。生产Scope原26项人工内存检查实际0（2418个证据字节变动全检），人工1000guards的116.44ms→6.70ms不是游戏成本证明。精确源1096、归档1596／6制品／18驱动＋reader预检0，PG `1b27c30383f0f8f32e51832fe8966fe2e931688dc5df3ee3152c62bd4c82d65c`，Nativec985／XR1684不变；继承17072实际走过的原事件适配，不能把继承证据当新二进制实跑通过。记录`idle-validation-candidate-result.json`、`idle-validation-runtime-plan.json`。已沿Frag真实尾申请在更早已到候选之后，尚未得到受理或最终交窗。r1缺前向声明编译失败、r2构成锚点断言失败及首个测试包装引号／解码错误均保留，最终r3编译0／测试0；未运行失败原型。

最新160159实际17072已完整收尾并最终唯一交Frag161511，本会话不再有窗口。新增首发分支实际触发：retained1／参数8／synchronous_press0，原kind1事件invoked1在真实Frame591执行，返回时原source年龄88.1618ms；source_ready2／packet_ready10／stale_sample0，权威trace_valid1和本发game-clock回执均产生。取枪、首发、连续6秒不自动补弹及单发资源ready通过；未派发事件的取消／原状态交还分支尚未游戏执行，不能扩大本次结论。之后手动开栓规划因“No fresh longshot_reload_contact_current receipt”停止，无bolt新Grip／UI／行程／取弹。本局仅一条当前contact在Frame599／source2254；后续记录有stale_before及stale_or_changed_after，但没有足够证据确认首个bind_contact子门、hold-generation变化或纯时效唯一根因，不据此放宽身份或100ms。

本局Native／entry／pad／drained／XR0／launcher0／game退出和原config/settings/head精确字节恢复均核对通过，18驱动＋reader无漂移。791game／1146sim捕获样本，跨度60.400／60.434秒；完整3页双路与4张原图亲审，原HUD、TPS／等待卡、前臂与袖口遮挡保留，完整自然贴合未接受；最大捕获间隔game2050.0143ms／sim1083.331ms，不是逐渲染帧或FPS证明。编码及唯一retention均terminal0（删830媒体／132477029字节／errors0，最新3局48小时），最终独立idle并再次全核Frag依赖后唯一交窗。闭合审查见`first-event-17072-review.json`，只读后续分析另存`current-contact-17072-source-audit.json`；不重跑未改160159。后续scope／V3／引导＋同一首发适配隔离组合1111源编译0，PG `28d33e35dd008e0d4a81a78ff663af53fb3b4fb6300b95d4dd4e1e4bde4f42a3`，尚未准备／排队／部署，膛口磁吸几何仍未接产品。

最新144539实际35152已完整收尾并唯一交Bol144535，当前窗口不属于本会话。肩部取枪通过，但首枪只有弹药11→10和射击样像素，权威trajectory为空，未产生本发开枪计时回执，驱动因此在核心检查停止，未进入6秒空膛／UI／拉栓。复查原PG日志行137–138确认两个实际InstantFire入口都在stale_sample拒绝：源valid1、held_current1，控制器年龄104.107ms、stereo104.1143ms，frame_active0；跟踪位置／旋转差为0。该证据证明准入时过期，不能证明具体耗时来源，也不能归因于准备UI分帧改动。原POD未带拒绝源Frame／sequence，禁止事后拼造时间对应。Native／entry／pad／drained／XR0／launcher0／game退出，原config/settings字节与head原缺恢复、18驱动＋reader无漂移均已核对；完整2页双路和4张原图已看，编码58517／retention88464实际0。正式收尾review保持原SHA，新增只读分析见`first-shot-35152-source-admission-audit.json`。

160159候选仅完成源码、编译、离线检查和静态准备，尚未部署／运行：从精确144539冻结源叠加狙击枪原版首发入口适配，复用现有唯一original-event队列。原包确认狙击枪有41字节BeginState包装（super.BeginState后设置PS5反馈定时器）和96字节ShouldRefire覆盖；后者仅用于真实R2-up后的原版无射击状态交还。入口／父Engine首发体／原timer／身份均须通过，等待期间重新校验包装及父函数，保持真实按压cohort、250ms事件期限、当前Frame100ms、20ms派发余量和先claim再调用的防重放规则。无第二队列／输入写者／Hook或自动连发路径，公共root的refire文件未编辑。原包字节与入口适配1321项、原有first-body／timer／once-only队列145项检查均0，完整PG编译0；原版release元数据适配已编译，尚未独立游戏执行，不宣称取消／交还已通过。PG `bfeac3a9e89e99f8fcd9f85f9a58e9e60c3ce0424cf73e0f1c4b0e37d0b38db4`，1596归档源／6制品／18驱动＋reader预检通过，Native仍c985、XR1684。未运行原型a091因源码审查发现旧交还路径不接纳狙击枪ShouldRefire而被r2替代；不把该原型当运行失败。证据`initial-event-candidate-result.json`及`initial-event-runtime-plan.json`。真实首发修复、开栓完成、单发装填／闭锁／奖励、倍率画面及自然手姿继续开放。

160159补充验证：冻结生产`longshot_should_proof`另有28项人工元数据检查编译执行0，包含正常原包body、24项签名／枪种／状态／按键字段／字节码／读取及closing witness拒绝和3项实际actor参数错配。它验证生产适配函数的离线分支，不代表原版状态交还已游戏通过；候选、prepared／ready／plan SHA及18驱动保持原样。记录`initial-event-release-adapter-checks.json`。本候选现已由真实队尾HB153818直接核身份受理，等待其最终完整交窗，尚未stage／运行。

以下各候选和运行条目保留其历史时点，不代表当前队列或当前验收状态。

最新133536实际61036已全终收尾，未触达本轮seek定位点：正常首发与6秒空膛通过，frame756曲线／闭合骨骼gate0、33点路径ready，随后同帧UI timing setter返回失败，在ack之前acquiring1→idle0；无UI ack、bolt begin／motion、history或seek子门回执。因此28872的seek内层原因仍未定位，61036的setter具体子因也未证明。驱动首拒绝即停。2页完整双路与4张实际原图亲审；59.6秒、800game／1253sim采样、末端覆盖通过，前臂遮栓和自然贴合仍不接受。Native／entry／pad／drained／XR0／launcher0／game退出及原配置、settings字节／head原缺恢复、18驱动＋reader无漂移全部通过；编码61338／唯一retention69785实际0，实删789媒体／119742590字节／errors0。最后独立idle并唯一交Coverr39（修正后plan2b96e8d9），再HB135655，未追加旧窗。记录`bolt-seek-61036-review.json`。

142004已编译冻结用于这项准备期失败的分帧改动，尚未部署／运行：首次真实曲线／骨骼准备完成后，UI设置只能在后续真实Frame发生；同Frame重复调用不推进，下一Frame重走原来源／clip检查并增加当前真实held租约相同检查。松手或失去租约即按原清理流程退出；不增加Engine查询／时钟、不制造Frame／续时／放宽任何时限，完美换弹开枪起点和4秒边界保持。生产运行器注入“准备耗尽来源”取得旧流程预期RED，新根future及精确窄版冻结测试GREEN；四项回归覆盖准备后源失效、同Frame重复、后续无来源／松手、失去held租约，并保留原完整循环、严格4秒、库存及中断清理用例。完整PG编译0、1592归档源／6制品／18未改驱动及reader SHA预检通过，PG`35d01bf5be1363be58f31125eb3e8da64b817b5b0be0b4eb9ebe3d736257b26c`。私人冻源仅选取本枪调度改动，未混入根future的scope yield／闭锁回执，也未采用离线批次或附着点新API。此调度改变未在游戏证明因果；已沿新真实尾申请单次同scope manual-open，当前无运行窗口。

同一未运行槽现替换为144539，旧142004保留并明确不部署：收到前置9896因正在重写的自有状态JSON发生PermissionError、没有执行武器动作的失败后，只在本枪`run_longshot_operator.py`载入的内存模板替换共同脚本一行读取；遇PermissionError或JSONDecodeError等待20ms并继续原45秒循环，原live／error／ready检查全部保留，其他I/O仍立即失败。共同模板与其他17基础驱动不变；8项实际本枪变换循环CPU检查通过，包括瞬态恢复、永久占用／半写超时、身份丢失、已记录错误、就绪字段不足、其他I/O及正常ready。PG／Native／XR和准备分帧代码完全不变；1592源／6制品／18驱动＋reader重新预检通过。新prepared SHA`ce9281403fc54546f122fb4e8fb08d419256ef59478ac188531c7c82685f6916`，正在向原唯一前驱确认同槽替换，尚未stage；不会据前置XR／launcher退出1声称运行通过，资源销毁／恢复和独立idle另行核验。

最新125546实际28872已全终收尾：原8帧生产历史通道实际可用，首条回执gate9／6点／原序3026至3031，覆盖111.1077ms；此条点位静止，随后真实Frame808执行seek成功。实际机构观测进度达到0.498613169；Frame861在希望进度0.592625852时phase4／action_gate5拒绝于`longshot_clip::seek`，pending缓存0、当前采样年龄85.4656ms，内部首个短路条件未证明。曲线／内容检查已越过，不能把消费者间隔或随后Native时效撤回称作本次seek唯一根因；没有开栓完成或取弹。完整3页双路和4张实际原图亲审，前臂遮栓／HUD／自然接触仍未接受；62.2秒、940game／1328sim采样，末端覆盖通过但不是逐渲染帧／头显证明。Native／entry／pad／drained／XR0／launcher0／game退出、原config/settings字节及head原缺恢复、18驱动＋只读依赖无漂移均核对通过；编码46594和唯一retention13920实际0，实删2582媒体／419166808字节／errors0。最终独立idle并准确唯一交Bol125154，当前本会话无运行窗口。记录`analysis/longshot-development-20261108/bolt-history-28872-review.json`。下一步离线定位seek子门，旧125546不原样重跑。

133536已为这一个seek首拒绝准备独立观测候选，未部署／运行：只在私人冻结的原`seek_impl`与`reconcile_seek`谓词前记本地阶段号，本枪seek结束后输出至多12条去重缓存回执；不增加Engine读取、时钟或SetPosition调用，两函数擦除POD记账后逐字等于旧冻结原文。共享工作区公共动画文件未写入，不引入当前其他枪械的完整实现。history成功回执另按waiting／moved／interrupted分别去重，12条头／27点的上限保持。原64项及拉栓回归的数学／时效源码SHA相同复用，完整PG编译0，1592源／6制品／18未改驱动与只读依赖SHA预检通过，PG`c64af0ea463065ca6714d849a10734b9f9ecf4e36c09d46b24d0c3abfca59e66`。此候选用于定位，不称为修复；已沿真实尾申请一次相同manual-open窄段，尚未取得运行窗口。首次构成脚本因误以为旧冻结含较新的HB专用seek分支而在编译前拒绝，已按真实旧源码修正，失败归档保留，未混入新HB分支。

另外仅离线构成读取批次实验PG`bf39b1e76d573399614b0420c30b90ac05f714833412b90366f2b6d7554e7b70`，未prepare／排队／部署／运行，不替换133536。它从精确冻结消费通用重读头文件及匹配测试两个文件；调用scope、HandClient、离线fixture和所有武器逻辑保持原冻结，所有证据字节每次重读并掩码比较。同4KiB页、每批≤1KiB、空隙≤32B、每轮额外填充≤256KiB，任何读取故障立即失败不拆分重试，原逐范围回退保持。27项生产观察器检查、679证据字节变动检测、真实PAGE_GUARD一次失败及Frame／身份回退检查和完整PG编译均通过。人工30000范围／84轮计时原版54.1075ms、批次59.0244ms，本次更慢；不宣称加速或seek因果修复。记录`read-batch-preparation-result.json`，待133536具体拒绝条件再决定是否采用。

当前状态：真实游戏中的正常取枪、权威射击、空膛不自动补弹、栓柄接触／新抓取租约、原版换弹状态取得、旧式动画解码及闭合骨骼校验已有证据。最新28872继续确认HUD起点相容，并越过历史连续性与实际seek，但在行程中遭遇后续seek拒绝。实际开栓完成、肩部单发取弹、插入／闭锁／完美奖励、近眼倍率画面和全部自然手姿尚未通过。后续scope＋V3零件握姿组合已通过编译和针对性离线测试，未部署。当前窄版引导仍是球形提示，后续源码已加入膛口轮廓和栓柄方向箭头；文字／音效和最终可见性验收待完成。以下尺寸是需求与当前实现的可调初值，不能代替实际接触和头显体验验收。

最新095812实际20908已完整收尾：原clip闭合骨骼gate0／curve0，位置误差约0.000417mm，旋转误差约0.0000146°，33点把手路径发布；实际UI ack phase0及后续capture phase2均accepted1，精确起点52.6677742→40.1346207，原节点缓存time0，frame719进入opening。驱动未读到opening→opened，最终超时；本局缺少行程内逐项拒绝回执，不能判定为骨骼、连续输入或seek中的某一个原因。完整3页双路与4张动作原图已亲看，前臂遮挡栓柄，不能用可见手臂移动代替完成证明。Native／entry／输入／XR／launcher正常退出，config、settings与原缺head精确恢复；编码及retention实际0，独立idle后唯一交Bol095943。本轮未触达取弹、插入、闭锁、奖励或倍率画面。

104905实际35040已完成首拒绝定位和完整收尾：实际UI ack／capture再次通过，frame810取得物理开栓；frame812第一个微小movement在曲线复核／seek分支phase4明确拒绝。该次sample gate6、capture gate0、grip／lease有效，source连续差90.1495ms、采样年龄80.305ms，位置误差约0.000465mm，旋转误差约0.00000724°；未开栓完成。随后原Native撤回记录年龄100.4974ms，但当前缺少该分支内部精确拒绝项，不能把它说成唯一根因。驱动在首拒绝后立即停止余下动作，仅同枪最后发布位置松手清理。两页完整保存跨度双路总览及3张game／1张sim原图亲看，前臂遮栓、HUD与启动等待保留，未接受自然贴合。两路约53.184秒，game856／sim1136采样，非每渲染帧或头显证明。Native／entry／pad／drain／XR0／launcher0／game退出及config、settings原字节／head原缺检查通过，18驱动＋只读依赖无漂移；编码9333和retention43977实际0，保留最新3局48小时。独立idle后唯一交Bol105222；当前窗口不再属于本会话。

最新085615实际16872：旧式AnimSequence／Interval解码及闭合骨骼校验全部通过，gate0／curve0，发布33点真实把手轨迹；闭合位置误差0.00127mm，旋转误差0.0000102°。HUD setup成功后下一帧capture1／service2拒绝，仍未开栓／取出单发。源码与离线回归发现HUD写入本发开枪起点后，旧动画receipt仍比较原换弹起点的冲突；095812新候选（仅替换未运行094103同一队位）只接纳同一HUD事务已成功写入并回读的精确起点，继续调用未修改的实际节点时钟／身份校验，已编译／离线通过，尚未再次游戏验证。真实16872没有单独输出该起点谓词，故不能把它宣称为已证实的本局唯一原因。3页双路及3原图亲看，自身正常清理和原字节恢复通过、编码／retention实际0，独立idle后已唯一交Frag090321。旧65324／50728失败保留。后续完整组合和scope候选未实跑；完整换弹、倍率画面及自然握姿仍未验收。

用户已确认，从实际开枪到枪栓完成闭锁，小于 4 秒为完美换弹，达到 4 秒为普通换弹。取弹沿用 Lancer 和兽族步枪的左肩交互与现有按键规则，每次取出一枚完整狙击枪实弹。

112230实际12980已完整收尾，未开栓完成：取枪／首发／6秒空膛／UI／闭合骨骼与33点路径／begin均通过；frame793→794第一次observe在phase3连续性检查拒绝，两次被机构接纳的source同步时间相差111.6819ms，当前采样年龄65.6107ms。sample gate6、grip／lease有效，但没有进入curve／seek，也没有实际fast-proof使用／成本POD，因此不能称35040的失败已修复。驱动首拒绝即停同枪原位清理。两页完整保存跨度双路与3张game／1张sim原图亲看；52.800318秒、821game／1127sim采样，遮栓、HUD、启动等待及完整自然贴合未接受。Native／entry／pad／drain／XR0／launcher0／game退出及config、settings原字节／head原缺检查通过，18驱动＋只读依赖无漂移；编码74268／retention7279实际0，独立idle后已唯一交Coverr32。

112230的通用读取校验保持逐次鲜读、原100ms／身份／反重放和全部动作门；26项冻结生产源人工内存检查与5项真实HandClient时限／身份／读取失败检查及完整PG编译通过。该证据仍只属于离线／编译范围。12980现存XR CSV不含原始手部轨迹与跟踪代次，不能事后证明中间左手样本或控制器失追。

125546已冻结新的拉栓连续性候选，尚未部署／运行，替代同一排队位置的未运行124434。实现复用已有已认证的8帧控制器生产历史，仅在GT消费者漏看中间帧时证明原始手部运动：必须保留全部原序号和同步时间、双手跟踪与输入代次、同一真实按压、稳定Native参考及刚性主手安装关系；当前Frame、租约和当前采样100ms仍独立有效。每跳保持100ms，逐跳与整段位移均保持原1.5m/s＋2mm限额，路径偏离上限15mm。历史点只能补足运动连续性，不能生成世界Frame、续租／续时或当作骨骼端点回执；seek只作用于当前真实Frame，完成仍需要后续真实Frame中的Native骨骼2mm／2°确认。任一拒绝清空缓存，重复源不能替换已接纳锚点。公共输入后端、桥接协议、Native和读取校验源保持原冻结版本。

本候选实际完成22项原始相对轨迹、19项拉栓连续性、23项生产适配器检查及原有拉栓回归，全部编译执行0；包含原110ms消费者间隔拒绝复现、缺帧／左右手代次／松键／当前过期拒绝、逐段容差不能累计放宽净位移，以及手静止而动画到端点仍不完成。原资源50键Root静止检查通过，但运行时仍须从实际曲线重新证明。完整PG构建0，1092编译源与1592归档源／6制品／18未改驱动及只读依赖SHA核对通过。PG SHA为`1a2ccf6ef86a73e11e3023872edcacfe9e26a5f8d4cd7eb60decd82516cb349d`；证据见`analysis/longshot-development-20261108/producer-history-test-result.json`。这些是离线／编译证据，不能证明游戏中历史可用、真实开栓、装弹或手姿贴合。已登记在Cover r34之后，尚未收到最终唯一运行窗口，Public v71保持原样。

用户补充要求将原版开镜后的倍率切换映射到右摇杆按下 R3，仅在狙击镜贴近眼睛的有效观察状态启用。所有可交互实体部件必须有对应的 GrabPose 贴合，包含吸附预览、实际抓取和运动过程。

膛口对齐辅助新增了独立几何实现`weapon_longshot_chamber_snap.h`，20项针对性检查编译执行0：进入／保持边界、反向弹头与非法变换拒绝、枪体整体平移旋转／单位与模型比例、原始轴向深度保持，以及由同一实弹安装反解精确手Grip目标。它只计算对齐目标，不产生装弹或库存回执；尚未接入产品，也未改排队中的125546。后续接线必须让显示实弹、C0精细捏持手和装弹判断共享同一当前帧对齐结果，接近／离开、真实送入深度及速度仍取未吸附的原始弹头。需要当前开膛、实弹及租约有效，步骤／租约／来源改变立即撤销，并保留120ms显示过渡。几何通过不代表已有实际磁吸交互或自然接触验收。

后续几何修正（未接产品）：原辅助将阈值解释成模型归一化距离，Scale2会把实际8cm横向当4cm接纳，Scale.5又会拒绝实际3cm深／4cm横向。新物理范围回归在旧头文件真实编译0、执行3复现；修正只将深度／横向指标换回真实物理米，膛口与原始弹头局部点继续保留模型坐标，吸附目标仍保持当前原始轴向位置。72项旋转枪根／非零膛口／Scale.5、1、2及50、100、200units检查和原20项几何检查编译执行0。实际28872／61036／35152记录的scale都接近1，不把此离线问题当旧游戏失败原因；160159和后续未准备的28d33e组合均未引入这个新头文件。记录`chamber-physical-range-result.json`；实际磁吸显示／手Pose／入膛接线仍待实现。

膛口吸附接线的具体交互约定（待产品实现）：仅在实际开膛、持有本轮唯一LIVE实弹且原左肩供弹租约仍有效时允许。获取体为膛轴半径6cm、轴向前方8cm；已吸附保持体为半径8cm、前方10cm；反向越过膛口最多1.5cm，弹轴与膛轴夹角不超过30°。离开保持体、松开、丢失跟踪／当前100ms来源、物品／租约／枪体／参考代次改变时撤销吸附；撤销显示辅助不能擅自消掉库存事务尚欠的清理。时序倒退或相邻原始源相隔超过100ms不继承较大的保持体，重新进入获取体才可再次吸附。

吸附只辅助横向位置及轴向朝向；真实送弹深度取未吸附弹头在同一当前枪根坐标中的投影，不能向膛内自动推进。120ms过渡的每个中间姿态也保留这一个原始轴向深度：先在相同深度平面内插值弹头横向位置，沿最短转动对齐弹轴，再由真实15cm弹长反解底火位置，避免直接插值底火和旋转造成假轴向运动。由同一实弹挂载反解手Grip目标，通过既有通用零件姿态请求交给唯一C0；禁止另加手、控制器或枪体写者。显示实弹与请求的精细捏持姿态必须来自同一个当前Frame结果。Native实际消费与皮肤显示仍须另核同帧接触，不能把请求目标当作已渲染的手。

120ms过渡所需的纯几何辅助已追加，54项检查编译执行0：在5个过渡权重、3种单位、3种模型比例及共同枪根旋转下，弹头始终保持当前真实轴向深度；实弹与反解Grip位置一致。辅助只保存有界横向／旋转修饰，离开接触体后也可渐退显示，但此时不能作为接触或入膛许可。原72＋20几何版本擦除新增函数后SHA精确一致。记录`chamber-transition-geometry-result.json`；尚未进入任何产品、当前Frame显示缓存或库存判定。

逻辑入膛保留先接近再送入：原始弹头先在膛前至少2.5cm，随后真实移动到轴向±6mm的座位区；保留原100ms连续性及3m/s速度限制。横向1.5cm／夹角10°的最终就位检查应用于本次有效辅助结果，辅助的原始输入仍必须处于上述获取／保持体内；不能显示已经对齐却暗中仍要求玩家无辅助地满足原始1.5cm。过渡尚未就位、原始弹头未真实前进、静止手仅靠动画到终点、库存物品不匹配或过期Frame均不得提交。入膛只产生一笔原库存事务，扣一发、消耗本轮唯一物品；保持或退出吸附本身不扣弹、不生成新物品，也不提前开始闭锁。该接线尚未进入160159冻结产品。

为这条装弹接线，原相对轨迹helper仅追加附着点API，原project／build主体和所有既有签名保持原文。新API先验证原生产轨迹全部连续性条件，再把实际左手旋转及同一实弹挂载计入弹头位置。16项检查在16文件的冻结源编译执行0，包含当前Marcus115精细捏持挂载与原Native rig变换在50／100／200单位下吻合、源缺失／代次／松键拒绝及输出原子性；证据`attached-motion-geometry-result.json`。原20项对齐版本的精确头文件SHA及原二进制另存保留。附着点与膛口几何仍未接产品，133536运行候选保持原冻结helper；不把几何吻合当作皮肤、真实入膛或库存提交通过。

后续完整组合113340已编译、冻结和静态依赖核对，未排队／未部署／未运行，仍要求先通过实际开栓窄段。它保持近眼原版全屏备选／R3门控及V3零件握姿，增加原动画准备期间枪栓目标不断显；真实方向箭头和膛口轮廓仍需像素验收。完整换弹驱动新增首个行程拒绝或装弹／闭锁异常回退后即停，七项真实驱动离线用例确认停止与同枪原位清理，身份失效后零输入。通用读取部分与已通过31项检查的源码SHA相同；完整PG构建0、3187归档源／6制品／18基础驱动／7附加入口与只读依赖预检通过。膛口磁吸对齐、完整自然接触、倍率画面和普通／完美换弹实际验证继续开放。

后续完整组合现更新为130712（取代未运行113340，仍未排队／部署／运行）：仅叠加125546同一六文件的原生产历史连续性实现，保留完整组合已有的scope／V3／引导和七项首拒绝停止驱动。完整PG编译0，PG`e848129146de70f0edaa949b9070f9eaf07c05f31c54ee5de6ad6e2acafeac06`，3191归档源／6制品／18基础驱动／7附加入口与只读依赖SHA预检通过；Native仍为ff1c8126。此组合尚未包含新的膛口对齐几何接线，不能作为完整磁吸、换弹或瞄具已经通过的证据。125546开栓窄段依然是下一次唯一申请的实际运行。

| 交付项 | 源码与编译状态 | 当前实际证据 |
| --- | --- | --- |
| 体感持枪与权威枪口射击 | 已接现有统一消费者 | 实际取枪与首发通过 |
| 射后保持空膛 | 已接原版自动补弹阻断 | 实际连续6秒未自动补弹通过 |
| 枪栓机构 | 原clip、复合路径、原生产历史连续性和物理行程适配已编译 | 实际闭合骨骼与33点路径、原history和seek执行已有证据，机构观测推进至0.4986；后续seek拒绝，开栓仍未通过 |
| 左肩单发实弹 | 现有肩区／输入、唯一物品租约与实弹资源已接线 | 资源ready；实际取出单发尚未触达 |
| 插弹、闭锁与完美换弹 | 连续送弹、库存事务、真实闭锁时刻及严格4秒边界已编译／离线测试；膛口磁吸对齐辅助尚未接入 | 尚未实际完成；普通／完美奖励与返射待验证 |
| 第二握点与全部GrabPose | 本枪握点配置、Snap／Grip状态和V3精细／圆柱握姿已编译 | 本枪完整四态与所有可见表面贴合尚未验收 |
| 近眼原版画面与R3倍率 | 全屏备选路径及输入归属已编译，独立验证入口已准备 | 原版低／高倍率画面尚未实跑；镜片内PiP未实现 |
| 每步交互引导 | 当前窄版球形提示，后续轮廓／方向箭头已编译 | 完整逐阶段可见性、文字／音效仍待完成 |

下文尺寸和接触阈值是本项目的实现初值与验收目标；源码测试、编译、模拟器画面和实体头显体验分别记账。

2026-11-10 本轮实际运行37448补充：按用户指定的P2P目录启动，实际左肩取枪、权威首发、射后连续6秒保持空膛、原生开火计时回执和单发资源ready五项通过。第一次手动开栓在frame746中断：源端点2870→2875相隔112.2413ms，当前样本年龄91.4049ms；sample_gate_cached=6是采样成功检查点，history gate7表示原relative::build拒绝。日志count=0是失败后的输出segment数量，不能推断原输入窗口为空；确切子判据仍未证实。未完成开栓、左肩取出单发、入膛、闭锁、奖励、回枪返射或镜内倍率验证。旧0.4986行程证据属于更早运行，不计作37448已经到达的动作。

37448正常退出、十项原始配置恢复、两页完整配对时间线与六张原图亲审、双路编码和一次保留清理均完成。枪身及栓接触被腕部／前臂遮挡，完整自然贴合与实体头显体验仍未验收。窗口已经通过唯一最终原始交接移交GN-v4，原始文件run37448-final-handoff-GN-v4.json的SHA256为134c73699fb7667dfe609e077f9d1e53a3de40923c09de5ca4a57cbacab1cb19；本枪不再写共享运行状态，也不复用已消耗的运行许可。

针对这一次拒绝，新增私有product-history-evidence-r1，仅在首次history gate7失败的动作判定之后，限量记录已读到的原始两端点、窗口全部条目、握持控制和固定根变换。原判据、100ms逐跳／当前时效、身份、租约、单一输入源与骨骼回执保持原文；不补造丢帧或延长输入历史。离线提取器拒绝缺块／不完整记录，原生产relative::build直接复算记录中的参数。五种端点间隔相同但中间事实不同的测试，在根源码及冻结源码均通过；包含完整链通过、原窗口截断、左手tracking变化、预测时间重复与握持边沿变化拒绝。测试夹具不是37448的原始输入，不据此宣称已找到实际根因。

该私有候选完整PG编译链接0，PG SHA256为e2fc74470ed9ec3e4d5780077a9cf0d61a61271b42b3561c667ace282932bf2f。相对实际37448对应冻结产品仅修改两个Longshot包含文件、增加一个Longshot记录头，其余1094文件不变；没有采用根源码中另外待验的栓接触几何。初次记录时尚未准备运行入口、排队、部署或实跑；最新独立入口状态见下文。

### 2026-11-10 独立运行与实施顺序更新

按主人最新要求，开发顺序为：①先打通取枪、射击、实际开栓、左肩取一枚实弹、物理入膛、闭锁及再次射击，验证普通与严格小于4秒的完美换弹；②再完善吸附、获取／保持／退出范围、容错和交互引导；③最后逐部件完善并验收自然GrabPose。现有体验与手型成果保留，基础抓点继续供流程使用，不以精细手指贴合未验收阻止流程开发。真实机构端点、身份、输入时效和库存守恒保持。

狙击枪独立使用`run_longshot_independent.py`和`independent-history-plan.json`。050140源与六制品保持不变，18项原动作驱动不变；新增入口复用现有私有启动互斥，启动前检查实际游戏／XR／录制占用、保存十项原始目标及私有Config／SaveData，正常结束后精确恢复并释放。独立编码、审片和媒体保留不再作为占用游戏的条件。旧前驱链、逐级签收与一次性最终交窗要求只作历史，不再约束新的独立入口；旧已消耗入口仍不复用。

新入口已完成只读源、制品及执行依赖预检；正常停止上限为360秒，仅在核对本次真实launcher身份后请求原有正常停止。当前定位目标仍是实际37448首次history gate7拒绝，确切失败子判据尚未知。新入口准备完成不代表已完成新的游戏验证、完整换弹、倍率画面或实体头显验收。

实际39920已用主人指定P2P目录完成短测及正常收尾。五项基础检查再次通过；首个开栓动作在frame976、进度0时拒绝。7576字节完整原始记录由该实际进程及创建时间绑定，冻结050140解码器直接调用原`relative::build`复现拒绝：前次接纳sequence2974，本次sequence2979，原始返回窗口仅2976～2979；缺少2975，首个连续配对即失败。输入端点间隔111.7769ms、当前年龄91.8691ms。该记录证明缺少必要的原始中间点；没有记录完整底层生产包，尚不能区分短环覆盖与该点未成功发布，不补造2975，也不把扩大缓存当作已通过的实际修复。

39920的Native／entries／pad／drain／XR／launcher均正常结束，18项原始及私有配置存档精确恢复后立即释放共享环境。三页完整双路总览及六张原图亲看，编码正常；腕袖遮挡栓接触，全流程及自然贴合未验收。捕获中约9.33秒的最大间隔保留，只是采集证据，不能直接当作引擎卡顿诊断。

针对已证实的历史缺口，私有history-retention-r2把底层生产环保留量由8增为16，输出仍最多8个原始样本；当前样本及右手按压记录仍限制在原8项范围，当前100ms、逐跳100ms、严格连续序号、追踪代际、按压／松手边沿与身份校验不变。协议使用独立v4映射和版本，混用旧组件会拒绝，不影响公共v71。旧／新实际InputBridge回归显示短环覆盖夹具旧拒绝、新完整链通过；真正漏发布、左右追踪变化、松手、失焦和流切换仍拒绝，原有身份、互斥、生命周期测试通过。夹具不是39920缺失样本的重建。

匹配私有Native／PG／模拟XR已编译。Native采用之前已编译的`native-part-grasp`完整冻结源加同一历史头，包含既有可选V3导出，不能声称与实际39920的c985基线仅一文件差异；PG相对050140仅历史桥头改动。模拟XR额外补齐两个只用于配置注册、未编进目标的遗漏测试／录制源。第一次尝试从旧交付源直接重建Native因归档遗漏测试／构建文件而失败，保留history-retention-r1，不作为通过构建。新候选只供受控模拟器，旧物理XR没有升到v4，不可将此组合交付头显。

上述组合实际55436在权威首发前失败，只通过左肩取枪，未到历史保留修复的覆盖点；不能把此局作为开栓修复通过。Native／entries／pad／XR／launcher已退出，但hand drain回执为false，保留完整收尾失败。外层最初因尾部占用未恢复，待本方尾部结束后，持统一RuntimeGuard及原内部锁补做18项精确恢复并核对释放，public未写。两页完整双路及两张原图亲看、编码结束；近末段有等待VR帧卡片，完整贴合和机构无通过结论。

新的152926候选撤回该次整套Native基底变动，改用原c985归档的1332项清单；其中C++及头文件保持原文，历史桥是唯一清单内修改。缺失的汇编／构建输入从保留的私有源补齐，逐项记录来源与SHA；`cover_body_thunks.asm`另与此前Git90f3a131归一化内容吻合。它不宣称与原c985二进制逐字节可重现。Native612ac98e、PG6aab5f2b、模拟XR137c147e均编译／预检通过，下一次实际验证仍待进行。

该新入口持统一`RuntimeGuard('solo-m1-1028d')`覆盖备份→stage→启动→恢复全过程，保留原内部Owner锁，最多等本方尾部30秒再判断安全恢复。对运行互斥只排除独立离线编码，不排除真实游戏、XR、录制或动作进程；编码与审片仍使用各自目录。旧55436及39920占用记录不复用。正常等待不重建前后继链，完整流程优先级不变。

### 2026-11-10 原始历史链恢复后的实际进展

实际5212采用原c985归档重建的Native，五项基础动作全部通过；原生产历史连续性检查通过，枪栓实际推进0.0926963，随后在动画控制准备阶段因当前输入时效检查拒绝。正常收尾、18项精确恢复及释放完成。两页完整配对总览已亲看；六张原图在查看前被滚动保留策略清理，明确不计作已亲审。完整开栓及补弹没有通过。

实际49240增加既有的有界批量读取：原见证字节仍逐项、双次新鲜验证，无权限或时效缓存，无失败拆分重试。679项实际见证字节变更、PAGE_GUARD及原观察器回退检查通过。五项基础动作再次通过，枪栓推进0.0764822；frame800最终执行前检查拒绝，同帧后续原撤回回执显示样本年龄100.521ms。较早缓存的frame794撤回不是frame800直接原因证据。正常收尾、18项精确恢复和释放完成；总览已生成，但未亲看，不计作画面验收。

新私有164405候选复用原`read_curve`成功结束时已有的动画节点观察，仅当完整输入、帧、角色、武器、节点及序列与当前操作完全匹配时，使用原有enclosed-observation准备路径跳过一次重复getter。最终当前输入100ms、所有权、函数元数据、参数、无通知位置设置及执行后核对仍保留；回执不提供执行许可，不跨帧缓存。57项回执匹配和33项实际函数体用例通过；窄段及完整补弹PG均编译通过。共享动画函数仅增加默认关闭的可选参数转发，其他武器调用保持原路径。

该候选尚待实际开栓短测；完整取弹、入膛、闭锁、普通／完美奖励、返射、近眼原版倍率及全部GrabPose均未完成实机验收。模拟器v4组合不能交付仍为v3的物理XR。最新入口直接使用现有统一互斥的30秒等待，不消耗占用期间的单次入口，也不修改其他会话的运行。

实际54064已完成该候选短测：五项基础检查通过，原动画准备、初始握栓及生产历史链通过；frame526在枪栓tick的第一个current检查处拒绝，尚未获取该帧样本，也未执行seek。因此新的同帧曲线回执路径仍未被这一局覆盖，不称为修复通过。完整观察器产生并封存了原始见证，但该条记录没有精确的失败子判据或输入年龄，不能把缓存sample_gate6或空sample中的grip_down0当作本帧采样成功或实际松手证据。Native／entries／pad／drain／XR／launcher正常结束，18项原始文件精确恢复后释放。

54064两页完整配对总览及六张原图已亲审，双路编码结束。49240保留下来的两页及六张原图也已补做亲审与编码；此前“未亲看”状态由本条更新，不倒填先前记录。腕袖遮挡枪栓接触，全表面贴合不予通过。54064启动仍包含第三人称和等待卡片；捕获最大间隔游戏约2秒、模拟器约267ms，只是采集证据。核对发现54064及继承的私有入口使用0.3渲染比例；新172501候选恢复约定的0.4，不将先前结果作为0.4或画质验收。

172501进一步把Longshot已有的同步原字节核验范围提前至原copy_frame成功之后、接触绑定之前；枪栓仅借用完全匹配且未禁用的同帧核验范围，独立调用或不匹配仍走原有范围／原观察器。原身份、当前100ms、每次原字节双重读取、变化回退、读取失败拒绝及所有动作调用保持。实际冻结Scope／observe和新借用函数体离线检查通过，涵盖变化、读取故障、禁用／帧／武器／模块不匹配、软件状态及范围销毁恢复。窄段PG编译通过；该调整的实际成本改善与完整开栓仍待新短测证明。

实际40964按0.4比例执行172501：五项基础检查通过；开栓驱动在原8秒等待中没有接纳100ms内的定位回执，未发出靠近枪栓或拉栓动作。这一局不覆盖新Scope的实际枪栓借用路径。日志只产生27条contact_current，排除48条上限耗尽；最后一条为frame665，此后出现一次输入源不可用、资源重新准备及持枪恢复。没有记录新旧身份各字段，不能把生命周期变化当作已经证明的根因。正常收尾、18项精确恢复和释放完成；两页完整双路及六张原图亲审、双路编码0。左右手停在休息位，枪栓和入膛均无验收结论。

新174832候选仅调整定位输出对历史开火事实的匹配：使用已有same_instance_across_hold，允许生命周期和握持代次向前；世界、角色、武器注册身份、游戏／Native、注册cookie、世界代次、会话、空间及输入代次仍精确相同。只输出当前bind_contact已验证的位置，不复活旧Frame、租约、按压或执行权限，也不重置原开火时钟。最多8条记录说明实际发生的身份归一化，供下一局核对；该改动尚不能作为实际原因或修复通过。实际冻结Identity函数体离线检查通过，包含12种身份／会话／输入变化及代次倒退拒绝。窄段PG编译通过；完整补弹组合继续保留同一修改。

### 2026-11-10 完整瞄具组合的底层准备

为避免沿用55436未经首发验证的整套Native-part-grasp基底，独立构建native-c985-scope-retention-r3：以实际已通过基础动作的612ac98e归档为底，仅合入保留的近眼画面选择和R3消费接口。capture.cpp新增原接口包含、两个导出及现有投影的显示选择，反向去除后与原文完全一致；其余变化是原虚拟手柄的R3消费、相关头和测试。控制器历史v4、原身体／手部C++及汇编不替换；原构建选择的MinHook依赖补齐归档，并核对重叠项相同。Native编译0，SHA256 e9cda3ae714422c92d62da9f4c682271f3035cd209f1b92bf60677c4aa59e04b。该版本未部署或运行，174832开栓候选继续使用原612ac98e。

针对新冻结Native实际接口与函数体，四项离线检查分别通过：R3准入／消费及防止按住重放、当前原对象和眼位／模型验证、原版FOV投影与双眼视图及深度保持、原手柄映射及松开重新按下。边界使用测试替身，不构成游戏倍率画面或头显验收。构建helper最初因PG不含Native专用投影头、随后因已有依赖目录而停止，保留r1/r2；测试r1因旧夹具未声明已有scope时间戳而编译失败，改用当前完整夹具后r2通过，未改生产接口来适配测试。

新Native与完整PG的Frame、控制器输入／历史、rig及scope输入／视图头逐字节一致；V3/V4精细手型接口仍未并入该Native。下一阶段实际补弹组合需要分别核对基础V2握姿协议和可选接口行为，不能把缺少精细接口当作全部GrabPose完成。原镜片内PiP仍未实现，当前待验的是主人允许的近眼原版画面备选路径。

基础V2握姿协议已追加独立翻译单元检查：分别编译新Native和完整PG实际头，比较Request／Frame尺寸、对齐、字段偏移、枚举、版本与导出名，一致；Request1496字节、Frame1416字节。该结论只涵盖线协议，不能证明可选V3/V4或可见手型贴合。r1夹具因C链接不能返回std::array而编译失败；r2使用C++链接及分离头命名空间后通过，生产头未改。

### 2026-11-10 狙击枪迁移独立实例及55280实际结果

按最新独立运行约定，停止部署／恢复共享solo实例；174832共享入口已明确退役。新prepare_longshot_isolated.py按已编译候选生成本枪唯一实例及私有Native／PG／XR副本，配置维持0.4；prepare_isolated_longshot_operator.py只改已有动作脚本的实例、日志、输入与输出路径。游戏由本实例launcher-session绑定，XR由同一session_report中的PID及创建时间绑定，输入只写自己的命令文件，不使用无实例定位的MCP或全机唯一进程断言。旧40964共用运行的18项恢复责任已闭合。原动作、100ms、库存和机构端点规则不改。

首次longshot-1110-180729在游戏创建前的渲染尺寸查询失败：原候选保存目录只有XR EXE，私有目录遗漏同一构建的openxr_loader.dll。没有游戏创建；原失败及stop返回1保留。新实例longshot-1110-181402从同一已校验EXE的完整构建输出复制加载库，启动和真实Source1双眼投影通过。没有复用失败实例名、改原EXE或写共享PG／输入／配置。

实际55280／XR34612通过五项基础检查，取得新鲜contact_current并进入真实握栓及生产历史连续行程。frame710首次明确记录curve_receipt_supplied=1且原seek成功，Scope读取核验39次；frame716实际进度0.0175842后，在prepare_controls gate20的当前输入检查处拒绝。回执里的withdrawal_frame715是旧缓存；同帧后续原撤回frame716记录reason5、age100.5397ms，保留100ms拒绝。相对运动155.3118ms通过原历史链，不补造输入。没有完整开栓、左肩取出单发、入膛、闭锁、奖励或返射通过。本局未产生contact_plan_rebind，因此40964身份失配的具体原因及归一化修复效果仍未证实。

本局正常停止命令0，launcher0，Native／entries／pad恢复及XR0，自己的游戏／XR已再次核对不存在；但hand drain=false、fire_disable_code2／hand_stop_code5，launcher passed=false，完整收尾不予通过。未强杀或操作别人实例。两页全跨度配对图与六张原图亲审、双路编码0；腕袖遮挡枪栓接触，完整GrabPose不予通过。捕获从动作入口就绪开始，不算完整标题／启动画面覆盖。

主人随后报告再次出现前台窗口，最新要求是停止新的game/XR启动、现有本方运行正常收尾，只继续离线工作。本方55280在该通知前已结束并核对自己的PID／创建时间不再匹配。此后后台修复短测的恢复结论又因主人再次报告前台窗口而撤回：必须等待主人明确恢复指令，不能仅因启动代码修复或另一轮短测通过自行恢复。runtime-start-paused.json及全局automatic-runtime-pause.json均由本枪runner在任何启动准备之前检查；不得绕过标记或使用旧入口。目标继续离线开发，实际后台与静音表现尚未验收，不向统筹发送启动或进度通知。

### 2026-11-10 动画控制查询及正常停止回执的离线修复

针对55280的prepare_controls gate20，现有StopAnim和SetPosition两次独立元数据遍历改为同一次局部查询；只在Longshot同帧曲线回执完全匹配的既有路径启用。重复名称但类型不匹配仍回到原Function类型查询，接收对象注册及类保持、函数身份、参数大小／类型／偏移／掩码、无通知设置和最终100ms时效检查均保留。没有跨帧缓存、输入权限或新Engine调用。共享动画函数新增参数默认关闭，其他调用保持原查询路径。

窄段product-control-function-pair-r2编译0，PG f2d673a5；完整补弹product-full-control-function-pair-r1编译0，PG70944e8d。r1因旧冻结helper在名称功能关闭时仍引用component_names而编译失败，保留原失败；冻结组合仅补同一已有功能开关边界，未改遍历。实际冻结Args／field／fields／prepare_controls函数体在普通及受限名称路径各66项等价与拒绝检查通过；800成员夹具名称读取1605→804，仅是离线读取计数，不能称游戏耗时改善。共享seek及Longshot真实函数体33项检查通过；原57项曲线回执检查对应头未变，复用原结果，不重复计算为新测试。

55280停止时fire_disable2表示仍在停止，hand_stop5是允许的已停止／未活动结果，不能把5单独称作错误。原Session.close在手部停止后没有再读取待定的开火停止回执；本轮仅当原返回2、手部已停止且原期限仍留有两秒调用余量时，再读一次同一停止回执。没有新的期限、启动、库存或输入操作；第二次仍为2、其他错误、身份丢失及超时均继续失败。实际close函数体14项离线检查通过，包含原55280返回模式的失败复现；r1夹具浮点时间比较断言失败后只改测试容差，生产逻辑未改。游戏内收尾修复尚未证明，55280原drain=false和launcher passed=false不改。

isolated-next-candidate.json记录上述两份编译制品和源码清单。未来prepare会校验制品、完整源码哈希及原Native612ac98e／XR137c147e，创建新的独立实例，不改旧恢复归档。当前未创建新实例、未启动game／XR／模拟器；完整开栓、取单发、入膛、闭锁、普通／完美奖励、返射、近眼倍率画面及完整GrabPose仍待实际验证。先完整逻辑，再吸附／Volume体验，最后精细GrabPose的顺序不变。

下一步准备／执行版本核对已补闭合：prepare在任何新实例创建前核对实际sixdof_unified_runtime.py路径和已测试哈希；runner在启动前核对已准备的选择记录哈希、Native／PG／XR、PG源码依据及同一退出代码。共享退出文件在准备后被修改会明确拒绝，不能把另一版本的退出行为算作本轮修复验证。实际preflight函数体用私有临时文件执行9项检查通过；暂停闸仍在读取准备记录和版本核对之前返回。本轮未创建新实例或启动游戏。首次测试仅在检查AST语句位置时失败（分号行产生多条语句），修正夹具定位方式后通过，生产函数未随夹具修改。

### 2026-11-10 近眼控制与原版状态管理的组合检查

scope-control-owner-tests-r2将完整PG70944e8d冻结源码中的scope Control与scope owner按equipment backend的实际顺序连接：先读取owner ready，再观察本帧近眼／R3，最后owner tick。原版函数、状态读取和Native桥使用已有测试边界；没有把桥接成功当作图像结果。42项断言通过，覆盖低／高／低倍率、按住不重放、远处按住再贴近、按住离眼再进入、菜单／换弹／输入失效及恢复、停止时禁止新启用，以及setter产生副作用后返回失败的恢复与重新离眼准入。较早r1的37项通过结果保留，r2扩展到42项，不相加冒充79项。

两份实际生产文件SHA分别为Control41300582与owner04474a9c。该组合没有修改生产代码，也没有启动游戏／XR；原版边界的实际行为、R3对其他功能的实际隔离、眼镜内图像或近眼全屏备选图像仍待受控实测。镜片内PiP未实现；当前实现仍为主人允许的近眼原版瞄准画面备选。

### 2026-11-10 完整补弹驱动迁移到独立实例（仅源码准备）

prepare_longshot_isolated增加显式mode：opening默认仍用窄段PGf2d673a5；full-ordinary和full-practiced选择记录内的完整PG70944e8d。未指定模式不会偷偷开启完整流程。对应operator准备把四个完整补弹helper复制到本次私有drivers目录并记录原文及生成哈希，仅替换已有核心末尾的开栓分发边界，取枪／实际首发／六秒手动空仓验证原文不变。普通模式只进行一轮普通换弹和返射；practiced模式由既有完整驱动在普通轮后以新实际首发开始第二轮，要求严格小于4秒。输入仍由同一实例Context发往自己的命令文件，原手势、库存及物理机构回执要求不改。

模式工具本身是纯转换，无启动、输入、文件或游戏访问。三种模式的生成代码语法、实际生成分发调用、四种非法模式拒绝及动作边界变化拒绝均离线通过；候选版本核对扩展为13项，完整模式误配窄段角色明确拒绝，operator与prepared模式也必须相同。原9项结果保留在此前源码检查点，13项不与其相加。本轮没有调用prepare创建实例、没有部署／启动，也没有执行完整驱动输入。下一次实际仍先解决已记录的开栓首败，再按完整流程推进；自动启动暂停规则不变。

### 2026-11-10 主人明确恢复后的32612首败

主人在本会话明确回复“恢复狙击枪受控验证”。本次只恢复Longshot受控范围，全局停止标记和其他武器的停止状态未删改。新增按实例、manifest／config哈希及当前全局停止标记哈希绑定的owner-runtime-resume回执；仅longshot前缀及精确作用域可接纳，新的停止标记使旧回执失效。现有入口start与worker均检查，11项实际函数体离线检查通过。本枪runner另绑定本次preparation及本枪／全局两个确切暂停标记；旧实例不复用。

新实例longshot-1110-191858使用当前offscreen1110启动组件及模拟器背景制品、原Native612ac98e、PGf2d673a5、XR137c147e，比例0.4。实际game32612／XR32948进入Source1并有epoch1投影提交回执；取枪通过，但首发shots为空，实际射击检查失败，未进入拉栓。frame626回执sample age101.2497ms；frame627为89.7644ms、preparation_budget0，触发计数已是pressed1／released1。相关“prepare stage8／follow6”是先前成功准备的状态，不是本帧重做准备失败的子判据。保持原100ms及20ms执行余量；确切首发失效原因未证实，不能归因于新paired lookup（本局没有进入该路径）。

操作脚本1、正常停止0、worker launcher0，自己的game／XR消失且未强杀；Native／entries／pad恢复、XR0。但新增退出补读实际执行后仍fire_disable2／hand_stop5、drain=false、launcher passed=false，明确没有修复通过。两页全跨度配对图和六张原图亲审，完整双路编码0；游戏侧有持枪图像、末尾为WAITING FOR VR FRAME，模拟器镜像持续白色，不能用提交成功声称实际双路图像通过或背景焦点验收。记录run32612-review.json。没有不变重跑；下一点是首发与当前输入准入的精确对应，并保留收尾待定及白色镜像的独立问题。

### 2026-11-10 55924实际初始行程与历史几何首败

先在原f2d673a5冻结PG上只增加最多16行Longshot的现有tick POD记录，反向去除include后原R2函数逐字相同；无新增游戏读取／时钟／输入或重试。product-r2-source-trace-r1完整编译0、PG7af56eba。新独立实例longshot-1110-192925／game55924／XR56344仍采用0.4及原Native612ac98e、XR137c147e；属于旧offscreen策略的最后一次记录，后续不得复用。

实际五项基础检查通过。扳机按住的frame586回执current1、pressed1／released0、preparation_budget1、muzzle_ready1，原shot1／game_tick37527317成功。32612首发失败没有重现，不据此宣称其原因已修复。真实握栓、历史链和同帧curve receipt seek通过，实际枪栓推进至0.200844716；frame787在stroke观察阶段phase3中断，而非seek／prepare_controls失败。当前样本65025400ns、Grip及租约有效，前后样本差110585700ns，历史Context双方valid、same_context通过，但select gate3拒绝了scale／root位置／角度的一项比较，尚未调用历史传输读取。记录仅含后一Context，不能凭后一scale值推断具体子项，更不能直接放宽阈值。

后续product-history-pair-trace-r1仅在gate3已决定拒绝后，输出既有a／b Context的比例、位置及角度差与三个原阈值比较结果；没有改变select、stroke或动作逻辑，没有新增对象查询或时钟。反向去除证明通过，完整PG编译0、4d3cd00d；已记录为下一候选，未部署运行。新R2记录及历史对比记录均为定位原失败，不算机制修复或验收通过。

55924两页全跨度配对图与六张原图亲审、双路编码0；本次模拟器镜像中真实双眼场景与双手可见，32612白色镜像没有重现但根因未证明。腕袖仍遮挡枪栓接触，完整GrabPose不通过。正常停止0／worker0，Native／entries／pad恢复、XR0、游戏消失，无强杀；开火停止补读仍2、hand5、drain=false／launcher passed=false，不改历史失败。证据run55924-review.json。

### 当前后台入口更新（覆盖旧暂停与offscreen复用规则）

最新主人要求后台问题解决后继续运行；统一入口已归档中央停止标记并切换build-instance-cloak1110及background-runtime.json绑定的模拟器。按最新约定，本枪只归档自己的“等后台修复”标记为runtime-start-paused-archived-cloak1110.json，未解除任何身份／输入／机械／候选保护，也不改其他枪状态。后续只用当前候选manifest／config重新prepare全新名字，不复用上述offscreen实例、不以改旧proof路径冒充重新准备。保持CLOAK与静音，若HWND录制不适用则使用现有私有投影录制，不解隐藏、不捕获用户桌面。公共后台工具由其维护者更新，本枪不再在共享工具并改例外；原实例授权记录保留为历史。

### 2026-11-10 首次CLOAK准备的工作进程身份拒绝

longshot-1110-193813已按最新cloak制品重新prepare、选择PG4d3cd00d，但尚未创建launcher或游戏时，worker在“Own resource reservation identity differs”处拒绝。原运行脚本保留120秒就绪超时及尝试stop返回1（无launcher-session），没有补写成功收尾或运行通过。实际worker记录PID15872已不存在、worker日志明确在launcher创建前拒绝、无launcher日志／session／birth，离线新分类函数核对该真实目录后确认仅为启动前失败；不冒充枪栓用例。

不启动游戏的独立对照复现了Windows虚拟环境Python转发：venv Popen PID43852、实际代码PID56688；直接基础解释器Popen／代码均51268，退出均0。不是删除身份保护，而是后续统一用C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe直接启动，numpy／PIL已核可用。本枪runner对execute加venv拒绝，并仅对“精确worker身份已消失＋明确的上述启动前拒绝＋没有launcher／birth证据”提前结束等待；9项真实函数体离线检查通过，单凭缺日志、缺session、超时或身份观测失败均不判终止，出现任何launcher证据继续原正常收尾。原193813结果不追溯改写。证据python-child-identity-check.json、prelaunch193813-review.json及prelaunch-worker-failure-tests.json。

新的longshot-1110-194333同样使用cloak及PG4d3cd00d，动作脚本已准备。直接解释器入口的资源检查两次发现本机已有两个实际自动实例，因而均在启动前返回；没有本枪新game／XR／输入动作，没有复用193813或抢占其他实例。当前候选保持准备状态，待资源实际释放后继续同一gate3对比短测，不据资源等待标blocked、不引入前后继／签收或固定排程。

独立实例录像保留也已接入本枪目录：retained_isolated_media.py只列入有本枪review、精确game身份、已结束execution、编码0且亲审完成的dual-output。保留最新3份且48小时内的媒体，仅将超限／超时会话中的jpg／png／mp4列为候选；json／jsonl／csv／日志／二进制和源码不删除，未结束或未审阅目录不触碰。删除前复核解析绝对路径在本枪目录、文件不是符号链接且大小／mtime不变，不递归删除目录。临时目录检查通过，实际dry-run保留55280／32612／55924三份，删除0；此工具在新录像编码审阅完成后调用，不占游戏运行资源。资源检查继续发现两个真实实例占用，本枪194333仍未启动，不用离线保留规则检查冒充枪栓验证进展。

### 2026-11-10 CLOAK实例46196实际采样首败与双骨骼读取

资源释放后，longshot-1110-194333以直接基础解释器成功启动；game46196／XR38852、PG4d3cd00d、Native612ac98e／XR137c147e、0.4及cloak1110。五项基础检查通过，历史原始输入链多次gate9通过、同帧curve receipt seek多次成功。frame805在sample阶段拒绝、sample_valid0／sample_gate_cached2，上一wanted0.0623586792，尚无有效BoltSample；具体read／matrix／current子项没有记录，不能把空sample中的grip0／lease0解释为松手，也不能给本帧捏造年龄。没有触发history gate3或新的pair输出，故55924几何子项仍不明。真实完整开栓／入膛／闭锁未通过。

正常停止0／worker0、Native／entries／pad恢复、XR0、game消失；fire_disable补读仍2、hand5，drain=false／launcher passed=false继续保留。两页完整配对图与六张原图亲审、双路编码0，真实双眼场景可见；腕袖遮挡接触，GrabPose不通过。当前四份已审媒体按原规则保留46196／55924／32612，清理最旧55280 dual-output中的1543个媒体文件；55280源码、日志、原数值及亲审记录保留，不再声称其原媒体仍在。记录run46196-review.json与isolated-media-retention.json。

离线源码核对发现bolt::bones只使用Root／Bolt两个world矩阵，但longshot_parts::read还调用Scope的MatchRefBone／GetBoneMatrix。增加read_bolt_pair并只由bolt::bones使用；通用read_part_group默认3，只有Longshot允许显式2，其他count／类型在读取前拒绝。根骨／枪栓原名称、预期索引0／3、函数及参数、矩阵有限性、初末资产身份、元数据及最终current检查全部保留；其他需要Scope的路径仍用read三根。每次该bones观测的原版getter调用6→4，不缓存帧、矩阵、租约或时效结果，不改行程阈值。

窄段product-bolt-bone-pair-r2完整编译0，PG5cf214dd；完整补弹product-full-bolt-bone-pair-r1编译0，PGd7d85ac5，两份实际read_part_group函数体一致。原／新真实函数体48项离线检查通过：默认3等价、前两骨结果相同、Scope不参与双骨观察、相关名称／getter／索引／非有限矩阵／资产替换／元数据／current拒绝及非法模式边界。Engine／元数据边界注入，不能声称实际成本改善或开栓修复。窄段r1组合因多处相同短锚停止；改用parts_failure附近唯一锚，原失败保留。测试r1仅assert宏中的模板逗号导致夹具编译失败，加括号后r2通过，生产代码未随测试修改。新两份候选未部署或运行。

### 2026-11-10 双骨骼候选57172实际结果及正常退出顺序修复

longshot-1110-201126／game57172／XR7376实际使用窄段PG5cf214dd、Native612ac98e／XR137c147e、0.4与cloak入口。五项基础检查通过，双骨骼采样及原生产历史链通过；frame760采样时有效、Grip／lease有效、age92261300ns，历史gate9／count7，随后seek gate5的item ownership/current检查拒绝，尚未SetPosition，实际枪栓进度仍0。seek内的withdrawal_frame759是旧缓存，不当作frame760直接证明；同帧后续原Native撤回frame760明确reason5／age101032300ns／game_difference0／frame_difference0。保持原100ms，不称具体总成本已改善或双骨骼修复通过。原46196的无效采样没有重现，55924的geometry gate3也未重现，旧原因均保留。

正常停止0／worker0、Native／entries／pad恢复、XR0、game消失；fire_disable2／hand5／drain=false／launcher passed=false仍失败。两页完整配对图与六张原图亲审，双路编码0，实际双眼场景可见，腕袖遮挡枪栓；未完成开栓、取单发或后续闭锁。记录run57172-review.json。原始review-selection是在编码尚未生成页时列页为空，记录脚本最初按2页断言停止；仅重新枚举已生成且已亲审的两页，未改实际失败结果。媒体按原规则保留57172／46196／55924，32612最旧媒体600项清理，源码／日志／数值及审阅结论保留。

收尾源码核对发现既有launcher.stop先发RepositoryLauncher.Stop取消，再调用runner.request_stop立即发Capture.Stop；后者可能先使Native注册退出，而run_session finally本应先unified_session.close，再停止Native。这是源码中实际存在的顺序竞态，尚非所有历史drain失败的已证实唯一原因。本枪runner新增request_owned_orderly_stop：精确校验自己index、session_id、owner／game身份，发既有launcher取消通道并在原95秒内等原report终态，不直接发Native停止，保留原owner finally正常释放。终态失败仍返回失败；超时、事件缺失、身份变化或报告变化保留错误，随后回原isolated.stop处理，不改变公共launcher、Native或PG。实际函数体11项离线检查通过（没有真实发送事件），包含身份／报告拒绝、超时不发Native、失败终态及已终止中立路径。尚未实机证明收尾修复。

为验证此顺序修复准备新的cloak实例longshot-1110-203254、同PG5cf214dd，动作逻辑不变。先因可用内存不足，随后因两个实际实例占用而在启动前返回；没有本枪新游戏或输入。不重启／修改别人实例；当前目标仍推进，待实际资源空出后做本次有具体修改的收尾／开栓验证。



### 2026-11-10 同帧骨骼结果交接候选（尚未游戏验证）

针对57172实际seek前源时效超限，活动拉栓帧把原有HUD读取前置到接触骨骼读取之前；随后仅在同步调用内，用栈上一次性结果交接根骨/枪栓骨给采样。原始输入、空间、挂点、动画状态或序列不匹配仍拒绝复用并走原读取；最终100ms和原版执行检查保留。没有跨帧缓存动作、延长时效或添加写入者。

窄候选d9adbde4、完整候选99374297编译成功。实际helper/native_pose主机边界73项通过；窄版状态机40次交接、完整版50次交接均通过，涵盖提前HUD失败不采样/不运动。主机测试不能证明游戏开栓完成或性能改善。新独立实例longshot-1110-205714使用私有GPU投影录制，不捕获桌面或解除窗口隐藏。之前57172实际失败、cleanup fire2/drainfalse及未验收的完整换弹/镜内画面/GrabPose仍保留。


### 2026-11-10 实际55252：准备阶段失败，正常退出修复首次游戏验证

独立实例longshot-1110-205714，PG d9adbde4，Native612ac98e，XR137c147e，0.4、cloak1110、私有GPU投影。实际取枪、射击、6秒保持空膛、开枪计时记录、单发资源准备五项通过。frame766左Grip获得枪栓租约、原版动画已停止，进入acquiring；frame767控制源超时撤回，age100139000ns。随后未取得longshot_reload_geometry，未开始实际枪栓行程，同帧骨骼交接分支未被实际覆盖，不能宣称修复57172。准备阶段缺少后续记录的具体子条件仍未知。

正常退出按原启动器顺序先清武器再停Native，本次fire_disable=0、hand_stop=5、drained=true，启动器0、XR0、Native与入口恢复，游戏消失，无强制结束；这是本次实际验证，不是所有场景均已修复。

私有GPU投影410帧、编码成功，亲自审阅1页完整时间跨度概览及4张原始左右眼图。末段存在13.42s与6.98s图像间隔，不能当作连续画面，不能据此证明控制源超时原因。枪身/手接触遮挡、GrabPose完整贴合、完整开栓/取弹/插弹/闭栓/奖励和镜内缩放仍未验收。媒体按已完成且已审阅记录保留最新3份、48小时；源码、数字与失败日志保留。下一步定位acquiring阶段的源时效与原有查询边界，不重复相同整段测试。


### 2026-11-10 实际34640：更早的肩部获取失败，非枪栓修复结果

PG180b编译和原状态机/40次交接检查通过，只增加最多32行事后acquiring位置/接触检查/耗时记录，反向擦除可恢复原逻辑，无新游戏查询、动作或时效。完整候选9a2e也编译并通过50次交接的原状态机检查，尚未暂存运行。

新独立实例longshot-1110-211039实际原版地面拾枪成功，但未完成肩部实际握持，未射击，基础5项本次均未通过；不能借用55252的通过项。日志26次input baseline，无实际event获取记录，未覆盖新增枪栓记录分支。原版100ms保持。正常清理再次fire0/hand5/drainedtrue、Native/入口恢复、XR0、游戏消失、无强制结束。

私有GPU280帧编码成功，亲自审阅1张完整时间跨度概览及4张原始左右眼图，存在长截图间隔，不宣称连续采集或手贴合验收。源码核对录制器每100ms在xrEndFrame路径请求一次同步GPU截图；下一受控尝试只将本实例私有录制副本的请求间隔改为500ms，保留同一候选、同一动作和所有身份/新鲜度规则，针对录制压力与输入连续性检验，不将录制相关性当作已证实根因。不改共享模拟器或其他实例。


### 2026-11-10 实际4440：进入真实行程，曲线读取前时效拒绝

独立实例longshot-1110-212034，保持PG180b/Native612/XR137和原动作，私有GPU截图间隔500ms。五项基础动作本次通过。新增事后记录证明acquiring四次调用分别进入clip取得、骨骼/曲线准备、HUD准备及opening；frame1489开始行程，耗时16.78ms、输入初始年龄68.30ms/末尾85.09ms。frame1491原SetPosition确认成功；后续实际骨骼行程已测到0.0465875327。

首个实际动作失败为frame1499 phase4/action_gate2/curve_gate1，sample_valid=1、Grip与租约仍为1、历史gate9；selected_node在第一个current检查拒绝，尚未运行该getter。失败点输入年龄98.81ms，同帧Native撤回reason5记录99.9975ms，game_difference0。frame1498旧撤回与frame1499同帧记录分开保存。Frame.now_ns的独立年龄未记录，不能把98.81ms当作所有时钟均未到边界的证明；具体拒绝子条件仍不能超出已有证据宣称。源证明12次、0回退、无读取失败。不能证明骨骼交接当帧走复用还是原回退；未全开、未取弹插弹闭栓。

正常清理第三次连续fire0/hand5/drainedtrue，Native/入口恢复、XR0、启动器0、游戏消失、无强制结束。GPU投影96帧编码成功，亲自审阅2张完整时间跨度概览及4张原始左右眼图；长画面间隔仍存在、手臂遮挡接触，GrabPose未验收。与34640资源条件不同，不能把本次进展认定为录制频率的因果修复。

下一步削减每次骨骼读取内重复的同查询函数元数据查找，保留各原函数/参数/接收者和时效检查、不跨原版调用缓存；后续证据需分别记录Frame和输入年龄。不重复相同整段运行。


### 2026-11-10 同查询骨骼getter元数据合并候选（未运行）

只在既有Longshot count2骨骼读取中，将每个骨骼的MatchRefBone/GetBoneMatrix元数据查找合并为一次本地字段遍历；不跨原版调用缓存。原component_getter、参数布局、MatchRefBone索引、矩阵结构/数值、前后资产及当前源检查保留。其他武器和默认count3仍走原路径。编译窄候选28fdd5b5、完整候选5bcf3524成功。实际Args/field/fields每配置30项元数据等价/拒绝/继承/同名/大小写检查，两种构建配置通过；71项原骨骼读取对照通过，完整状态机40/50交接通过。800成员主机夹具名称读取1605降804，不代表游戏耗时改善。

下一实际受控实例longshot-1110-215400已准备，保持0.4、cloak、私有GPU500ms录制。失败日志增加已有Frame.now_ns及Frame年龄，与输入年龄分别记录，不增加时钟或游戏调用。具体时效/行程改善与完整开栓仍未验证；完整交付评分沿用41.25%。


### 2026-11-10 实际9224：getter路径产生接触，录像失败后无新鲜规划记录

longshot-1110-215400实际PG28f，Native612/XR137、0.4、cloak、私有GPU500ms请求。五项基础动作数值检查通过；新合并getter的count2路径产生18条接触定位记录，最后frame835。进入手动流程后8秒内没有被驱动接纳的100ms新鲜接触记录，未移动左手去枪栓、未进入acquiring、未有新Frame年龄证据，不能宣称解决4440的真实行程拒绝。

GPU记录仅59帧、0.05至29.82秒，随后5秒等待新鲜图像超时；事件时间持续至69.32秒，后段动作没有画面覆盖。正常stop写下错误记录但不编码，不能当作完整视频。亲自审阅1张已记录的部分跨度概览和2张原始图，不覆盖射击/枪栓，不计整段画面验收。接触规划失败与GPU超时分别保留，不推断因果。正常清理fire0/hand5/drainedtrue、Native/入口恢复、XR0、启动器0、游戏消失，无强制结束。

受控驱动现已增加录像错误保护：在下一次原live检查处停止正常动作，随后走原有正常清理；保持原5秒截图超时、100ms输入/权限时效及原动作。实际生成保护函数4个离线检查通过（身份有效/失效、录制未开始/正常、录制失败前置拒绝），不代表GPU超时本身已修复。下一独立准备实例longshot-1110-221517，尚未启动。概览工具会把距最近画面超过1秒的事件列为uncovered，不再列作接触图片。

时钟口径更正：bolt motion日志的age/新增Frame age使用传入的、stroke.observe和read_curve之前已取得的时钟，不是独立的失败后时钟。4440的98.81ms应读为动作观察前的输入年龄，Native同帧撤回99.9975ms有自己的采样时刻；不要把前者当作失败瞬间仍在所有边界内的证据。终点实际骨骼2mm/2度/进度1.5%检查保持，4440动态角误差7.85度并非该次首个拒绝点，未到终点；采样时序对应关系后续需要真实结果核对，未归因或放宽。


### 2026-11-10 实际55560：真实行程26%，末端执行前current拒绝

longshot-1110-221517保持PG28f/Native612/XR137、0.4、cloak、私有GPU500ms请求，加入录制失败动作保护。五项基础动作本次通过；acquiring4921—4924完成原clip、路径、HUD及begin。frame4926原SetPosition成功，后续实际骨骼行程测至0.262122011。首个拒绝frame4958 phase4/action_gate5、curve_gate0；seek_gate13、prepare_controls gate20最终current失败，尚未SetPosition。观察前输入年龄90.553ms、Frame年龄71.6923ms；同帧后续Native撤回reason5、input100.5893ms、game_difference0/frame_difference0。seek记录内withdrawal4807是旧记录，不当作4958直接证据。旧4440曲线读取gate1未重现，但不宣称整体成本或根因修复已证实。

正常清理fire0/hand5/drainedtrue，Native/入口恢复、XR0、启动器0、游戏消失，无强制结束。录制92帧、无错误、编码0，全部动作事件距图像不超过1秒；亲自审阅2页完整记录跨度概览及4张原始左右眼图。起始黑帧、长帧间隔、手臂遮挡保留，不称连续画面/前台保护/GrabPose完整验收。录制失败保护本局没有触发，不据此宣称9224的GPU超时修复。完整开栓/取弹/入膛/闭栓、完美判定与狙击镜仍未游戏验收，交付评分41.25%不变。

下一步有界方案评估：在消费本次stroke.observe之前，若本帧剩余执行时间不足，则不做原版seek、不消费行程、不改变历史锚点，等下一真实新鲜Frame按原连续性/身份/租约检查继续。原100ms、实际骨骼终点及4秒判定保持，不从已经派发或结果不明的setter重试。方案仅记录，未实现。

完整候选的Native握姿接口另有静态缺口：冻结scope Native e9c源码未含hand_authored_grasp头，capture.cpp仅见V2发布口；完整PG5bc的枪栓/子弹使用V3/V4可选发布。此前V2 wire通过不证明V3/V4可交付。未做二进制导出审计或新构建；按先逻辑后精细握姿的顺序，后续需在精确私有基线上接入已有通用消费者和唯一C0，再作接口与实际显示验证，不得把完整PG编译称作完整贴合已接通。


### 2026-11-10 真实新鲜帧等待候选（实现、编译与离线检查通过，未游戏验证）

原BoltStroke/Path代码不变，在栈副本上运行原observe/observe_continuous一次。原检查若失败仍提交原失败；只有原结果为moved、无seek_pending、已有Frame/输入时钟均在原100ms内，且任一年龄达到80ms（剩余20ms或更少），才返回waiting而不提交该行程副本、不更新history::previous、不调用曲线读取或原版setter。下个真实新鲜Frame重新走原source/身份/租约/连续性检查；历史缺失/松手/身份变化仍拒绝，不制造Frame或补齐点。上个已观察锚点保留，不能把延后的物理位移丢失后重新锚定。

窄候选a2728d81、完整候选81035ba5编译成功。实际旧/新tick与未改的原BoltStroke/Path一起执行39项离线检查：正常行为对照、80/100ms边界、延后不派发不消费锚点、后续真实观测保留位移、原连续段检查、错误身份/松手/缺失历史/过期输入、结果不明setter不延后、实际终点2mm/2度门槛等通过。Engine及历史传输边界使用主机替身，不是游戏修复证明。

磁盘保护按共享维护来源审阅：prepare/start资源准入/worker启动前保留4GiB，不影响已有运行或删除文件。后续独立准备记录并保存所用isolated_sim.py/runtime_receipts.py源码快照与哈希，启动前核对变化；不改旧候选哈希。自有106份成功构建中清理784个可重建obj/pch共763163534字节，DLL哈希、源码、证据与调试符号保留；不递归删目录，不碰他人目录或进程。新实例longshot-1110-224826准备完成，尚未启动，交付评分仍41.25%。


### 2026-11-10 实际56280：第一枪栓grant在启动流程前超时，等待分支未覆盖

longshot-1110-224826，PGa272/Native612/XR137，0.4、cloak、私有GPU500ms记录，带来源快照和4GiB磁盘准入。五项基础动作本次通过。左手精确到达枪栓，frame1083 phase1决策granted/magazine lease1/new_grant1；同帧Native reason5撤回、age100756000ns、game/frame difference均0。runtime未进入acquiring，没有longshot_seek_deferred或实际枪栓motion，随后1084 idle entry_gate1拒绝。idle bind具体失败子条件未记录，不把晚期普通时效撤回扩大为所有子检查的根因。新等待处理未被此局覆盖，不能称39项离线检查为实际修复通过。

正常收尾fire0/hand5/drainedtrue，Native/入口恢复、XR0、启动器0、游戏消失，无强制结束。GPU117帧、无错误、编码0，亲自审阅2页完整记录跨度概览及4张原始左右眼图；手臂遮挡接触、长间隔保留，不称完整贴合或连续画面验收。媒体保留最新3份已完成/审阅/编码成功的本枪录制，源码数字和失败证据不删除。

下一步复核Longshot机制处理在backend中的先后：现有射击准备先于fresh follow及reload处理，第一grant后的反射/读取开销可能消耗本帧预算。只在精确Longshot部件权限且已确认没有待履行的停火/释放义务时考虑机制优先，保持fresh follow在机制之前；其他枪和需要优先释放射击的情形保持原顺序。不恢复过期grant，不重放Grip，不拓宽100ms；先检查现有射击所有权/释放接口，再实施、编译、验证，不能仅凭本局日志宣称R2准备是唯一根因。完整交付仍41.25%。


### 2026-11-10：枪栓交互处理顺序候选（实际效果待验证）

实际56280在首次枪栓授权后发生100.756ms输入过期，尚未进入acquiring；不能将原因直接归给开火准备。新增严格受限的处理顺序候选：仅当前Longshot、精确同帧有效枪栓授权、右扳机中立、原版R2与L2已知松开、运行模式且无任何现存开火/瞄准/首次开火/释放/手雷债务时，先fresh follow/apply与原有reload处理，再在同一帧执行一次完整开火准备和tick。完整功能版同时要求狙击镜无待处理所有权。任何未知、待释放、停止或不同武器仍保留原顺序。所有动作自身重新验证，100ms、lease、端点及松手保护保持。新增中立状态与实际调度代码的针对性测试；游戏收益待本候选实际运行证明。

本轮验证：窄范围候选32项、完整候选33项，另从当前工作树提取实际中立检查与调度代码35项通过。后者对已有其他原版所有者的调度边界使用替身，并不证明其他武器的实际行为。两个独立候选均编译成功，Native/XR未替换。首次准备因空间低于4GiB未启动；空间恢复后因两路自动实例名额已占用，继续等待。源码私人检查点e5cf3f70已保存；保存期间共享HEAD/index稳定性断言未通过，事后逐个确认私人检查点内容与所属文件一致，未恢复或覆盖共享状态。当前仍未证明处理顺序候选的游戏收益。

实际53500（longshot-1110-232701）：五项基础动作通过。875帧grant/new_grant=1且round_ready=0，同帧先记录idle entry gate1，再记录L2处理，支持本候选调度已执行；无独立分支标记，不据此证明耗时收益。未进入acquiring，未发生实际枪栓移动，晚帧seek分支未覆盖。下一帧876记录输入过期163.784ms，不能倒推875的全部子条件。子弹资源在628→634就绪，849后多次重建，867再次就绪，868清空，873新建第5代；875仍未就绪。应审查打开枪栓是否不应被子弹展示资源挡住，并保留后续真实取弹/入膛的完整资源、库存、身份与动作校验。

本轮私有GPU录制102帧无错误，最大间隔0.756秒，编码退出0；亲自查看全程1页及4张原始图，手臂遮挡枪上接触点，不能确认贴合。正常退出：fire_disable=0、hand_stop=5、drained=true，Native/入口/虚拟手柄恢复、XR退出0、游戏消失，无强制终止。完整换弹、开镜画面、Grab Pose仍未验收，41.25%总分保持。


### 2026-11-10：子弹展示资源按交互阶段检查

根据53500的875帧round_ready=0入口失败，移除idle开栓对round::ready/current的要求，将相同检查移到waiting_round的真实左肩取弹入口，且在reserve_item之前执行。原有capture_shoulder本身的ready/current、真实左肩位置、同帧ammo授权和新按键检查保持，展示和入膛后续验证不变。资源恢复不能重放已拒绝的取弹按键。原有空膛库存、奖励资格、开火时刻、原版Reloading所有权、动画、枪栓路径和输入时效检查保持。本候选尚未改变资源过期后重建策略，避免混入第二种修复。两套候选编译成功，各自原有完整运行时替身测试及6个新增资源阶段场景通过；引擎和物理枪栓边界使用替身，实际游戏未验证。

补充恢复场景：真实库存已经插入一发、旧流程中断并保留闭栓义务后，展示资源未就绪且不属于当前展示上下文，仍可凭新的有效机械授权恢复并闭栓，原库存只提交一次，reserve仅减少一发。两套原有运行时替身测试加各7个新场景通过。独立实例longshot-1110-233856已准备；180秒受控名额等待期间未获准启动，无start.log、started.json或worker记录。本候选尚无实际游戏验证，不得沿用53500作为其游戏通过证据。


### 2026-11-10：受控窗口继续等待与手势所有者前置接入

现有round-stage候选仍未启动；本轮先等待180秒，再等待240秒，期间第二个测试名额由不同仍存活实例使用，后段证据盘可用空间低于4GiB。未干预任何外部会话、未降低门槛。资源生命周期审查发现：wrapper的current同时包括完整生命周期身份、源时效和资源存在性；现有构造器advance已经支持时效不足时pending，不能只凭generation变化就保留跨生命周期资源。故未改资源销毁策略。

为后续完成Grab Pose，另准备隔离的所有者分类前置源码：在原有已读取、并由same_path/context完成闭合验证的ClassPath上，复用缓存名称判定Weap_AssaultRifle祖先，记录known/is_lancer，并把分类纳入same_key和same_context。四处最小变更可逆擦除回精确冻结源码；未增加内存读取或游戏调用。18个实际values/same_key/same_context提取场景通过（内存与类遍历使用替身），包括旧/新读取调用序列相同、继承/狙击枪/近似名/带编号/失败/视觉专用/已知空武器/32层路径和身份闭合变化。仅隔离源码与host编译测试，不是完整Native编译，不是V3/V4已接入，也不是手势贴合验收。当前机械测试候选及Native612保持。后续还需接入现有通用发布与同一个C0消费路径，保留固定骨长及既有所有者保护；无第二骨骼写入器。


### 2026-11-11：c985 冻结 Native 通用手势接入候选

在精确native-c985-scope-retention-r3（e9cda3ae）上移植已经保存的generic V3/V4及唯一C0消费路径，生成隔离Native05197471。改动7个既有文件，增加5个接口/适配文件；保持c985连续性、狙击镜桥接、原始编译开关和依赖，未复制当前主源码中的Lancer前臂、腕部或肩部求解修改。Scene所有者分类使用上一轮四处闭合变更；C0以本帧已认证模型生成15个参考旋转，再消费当前通用发布。V2旧接口保留，V3/V4不提供动作或库存权限，无第二骨骼写入器。

Native整体编译成功。7项针对性host任务通过：实际通用V4适配49检查、所有者策略44检查、旧V2/新V3发布与撤销、混合/释放、真实参考模型的Longshot左手C0局部消费815检查，以及Native/完整PG两侧V2/V3/V4布局实编译比较。Frame1416字节、V21496、V31512、V42984；16个大小/对齐/偏移结果一致。Broker/Core输入边界为替身，C0局部表使用当前PG原始表；这些不是实时Native消费、GPU绘制或自然贴合验收。Native未安装或用于游戏，当前机械候选仍使用612ac98e。

机械实例longshot-1110-233856的读检查曾见空闲，但原子启动时名额已被其他实例取得，start拒绝、started=false，未生成worker。保留该拒绝为名额竞争记录，不算游戏失败，也不重用该名字。

2026-11-11受控资源清理：只删除9个已正常结束、原Native/入口恢复且XR退出0、旧精确游戏/XR进程身份均不再存活实例中的27个重复DLL/EXE副本，共112756736字节。逐个确认原始构建/归档副本存在且SHA一致，按解析绝对路径和mtime/size复核后删除，最近三个实例、源码、日志、媒体未动。旧实例名不再复用。保留清理计划、逐文件结果与脚本。新机械实例longshot-1111-001703已准备（Native612/PGf11/XR137/0.4），新Native051未用于此例；当前读检查仍显示两个自动名额已占用，尚未创建worker。

补充实际所有者适配测试：新Native实际authored_grasp_owner_live.inl（不仅是纯策略）在C0后端ON下20例、OFF下1例通过。覆盖未知类/无效上下文/世界、pawn、mesh、weapon及generation变化拒绝，其他武器与原版Lancer C/E额外所有者检查的分流、后端关闭拒绝。Identity转换及原版所有者getter为替身，不是实时Scene证明。

### 2026-11-11：实际58796首发记录缺失

round-stage候选实际启动：longshot-1111-001703，Native612/PGf11/XR137/0.4。左肩真实取得狙击枪通过；正常180ms扳机脉冲后没有native_lancer_shot_trajectory记录，操作者据原检查停止，未进入开栓、取弹、入膛或闭栓。当前阶段依赖修复及晚帧seek分支均未覆盖。已有10条bounded longshot_r2_source记录全为trigger=0，包含pressed/released计数均为1的松开后记录和copy失败记录；不能据此证明脉冲期间的全部GT读取或精确失败原因，也不能声称物理原版枪弹绝对未发射。后续先检查首个按下的实际dispatch/source丢失；若使用受限持续扣扳机变体到达换弹，应单列为不同覆盖，保留此次短按失败，不能声称短按已修好或放宽100ms与防重放。

63帧私有GPU录制无错误，最大帧间隔0.826秒，编码退出0；亲自查看全程1页及4张原始图，接触仍受手臂遮挡，不验收贴合。正常清理：fire_disable0/hand_stop5/drainedtrue，Native/入口/虚拟手柄恢复、XR退出0、游戏及记录子进程均消失，无强制终止。Native051没有用于本次游戏。原媒体保留策略执行后最新三个为58796/53500/56280，55560旧媒体移除，数值及文本证据保留。


### 2026-11-11：首发只读发布器观察候选

针对58796首个180ms扳机脉冲无权威轨迹记录，新增可选--first-trigger-observation，仅在第一次原有扳机按住/松开的等待内读取已有GVC4输入及Stereo4映射，最多64次、约20ms一次，所有映射只读、mutex零等待，打开前后核对精确游戏/发布器身份及协议/时钟频率。记录最新发布器触发值/边沿与匹配立体票据分别是否出现，附同一QPC域中的命令发布前后时刻。匹配票据只是诊断，不是Native动作授权；不新增输入命令、不延长源时效/重放旧边沿，仍请求原180ms/700ms，异常路径确保尝试原松开。采样会改变等待实现，实际调度影响待记录，不视为严格匹配性能对照。Source1保持Source1，未调用只接受物理Source0的校准解码器，也不重标字节。30个实际Wire结构和生成操作者函数检查通过，包括身份/协议拒绝、过期显式保留、发布器与票据不同步的区分、原输入载荷和请求等待不变、失败松开及关闭读取器。无游戏因果证明。Native/PG/XR仍612/f11/137，新的Native051不装入此例。


### 2026-11-11 实际56100：进入开栓，原始历史缺5060

私有候选 PGf11 / Native612 / XR137 在0.4受控模拟器下完成5项基础检查：肩部取枪、原生弹道回执、开枪后6秒维持空膛且禁止自动补弹、实际开火计时回执、子弹资源就绪。1287取得枪栓授权，1290进入opening；1292因原始历史不连续中断，尚未执行枪栓SetPosition。前次5059，当前5065，返回历史只有5061..5065；原始C++读取逻辑离线重放同样拒绝。所有已保存点的stream43/tracking1/1一致，不能把它定性为已证明的来源切换。5060缺失的发布端原因仍未证明；不补造点、不放宽100ms或连续性。

本次取栓时round_ready=1，故未在游戏中覆盖资源未就绪仍可开栓的分支。只读首发观察器因导入路径错误实际采样0次；修正后31项离线检查通过，尚无修正后的游戏证据，亦不解释此前58796短按失败。

112张私有GPU双眼图已检查2张全跨度总览及4张原图。frame-00062全黑，最长录制间隔9.4534秒，手臂遮住接触区域；编码退出0不代表连续画面或GrabPose通过。正常清理：fire0/hand5/drained、Native和入口恢复、XR0、游戏消失、无强制终止。保留完整失败；完整手动换弹、镜内图像、抓握贴合与头显验收未通过。记录见analysis/longshot-development-20261108/run56100-review.json。


### 2026-11-11 原始历史停止点诊断（尚未游戏验证）

为定位56100缺5060，新增可选HistoryBoundary诊断POD，从读取器本次已经复制的同一个原始历史包中取出停止位置、expected/prior/next、原始时钟/stream/tracking及各项原始条件。读取条件、break位置、100ms、窗口容量、生产端及动作流程不变；仅在狙击枪原有interrupted决定后最多输出8行，无新引擎调用、读共享区域或取时间。默认空指针供其余调用者使用，不扩大原始共享包。

13种真实Windows映射离线场景覆盖缺记录、错误序号、stream/tracking/focus、预测时间、同步间隔、接收时间、姿态失效、计数回退/多跳、key变化及正常历史；带/不带诊断的返回历史逐条一致，旧测试通过。PG诊断开栓版2d1c3c99、完整版本16101218已编译，各自只改4个文件且可逆。最初r1日志命名空间编译错误保留，r2修正后编译0。013302已准备；机器资源满时只等待，没有启动。完整版本未选为该开栓实例；不将此诊断视为5060缺失修复。


### 2026-11-11 实际15748：首发观察器可用，准备阶段先失效

PG2d1c / Native612 / XR137，0.4私有后台受控验证：5项基础检查通过；只读首发观察器41次采样、无错误，真实Source1发布记录显示5705..5713扳机按下，5743回到pressed/released1/1。它只证明本轮观察器可用及本轮原始脉冲，不能解释此前58796漏短按，也不证明Native消费许可或性能等价。

562取得枪栓授权，563/564/566等准备返回接近或超过100ms，567原始曲线/闭栓匹配完成并推迟HUD准备。之后多帧copy不可用。580原有current_point7/current_guard4显示source_current_on_gt未通过；581重新建立baseline；582准备退出point4并清理。没有进入opening、没有枪栓SetPosition，没有调用历史停止点诊断；因此56100缺5060的原因仍未知。本轮准备退出前的精确失效子条件仍需查证，不认定某个定时器、动画或字段为根因。

96张GPU投影、1张全跨度总览及4张接触原图已查看，编码0，最大间隔0.53921秒，手臂仍遮挡接触，GrabPose未验收。正常fire0/hand5/drained，Native/入口/手柄恢复、XR0、游戏消失、无强制终止。此前013302只是启动资源竞争拒绝，未创建游戏且已作废；014829已消费并正常终止。媒体只保留15748/56100/58796最近3份，文本/原始历史和源码保留。完整换弹/镜内图像/手部贴合未完成。下一步读取580来源检查的实际失败子条件，禁止放宽100ms、释放和防重放。


### 2026-11-11 准备阶段去除连续的重复动画检查（新候选，尚无游戏结果）

15748第563帧外层service已就绪，随后bolt::prepare内部第二次service在service_gate4返回pending，最终来源年龄100.0721ms。现在把原准备函数拆为prepare_serviced与独立入口prepare：acquiring仅在本次外层service返回ready后，立即调用prepare_serviced，两者之间没有引擎调用。独立入口仍执行原service。没有跨帧保存或复用service结果、输入、姿态或许可。共享路径先检查当前归属、库存身份、停止/播放/neutral状态和原来源有效性，再执行原曲线、闭栓2mm/2度、模型与路径及最终100ms检查。

实际旧/新prepare函数与真实BoltPath离线验证：旧双次26ms检查把50ms来源推到102ms而拒绝；新一次检查后76ms可继续，但几何读取再耗时到101ms仍拒绝。原几何拒绝用例和12个新增场景通过；动作/引擎/时钟边界为注入测试，不称游戏性能证据。开栓版和完整版的实际runtime离线完整换弹/4秒边界/恢复/库存/停止及7个资源场景通过。根目录测试夹具缺少此前source scope、骨骼回执和新身份字段，直接编译失败记录保留；同步夹具后普通根目录测试编译/执行0。

私有开栓PG6bc3f37f、完整PG8f4af3a1编译0，仅两项产品文件变化；Native612/XR137未改。对应更改已合入两个自有根目录Longshot文件，测试夹具同步。新021135已准备，尚未游戏证明准备阶段或完整换弹修复。


### 2026-11-11 实际59140：首发DOWN已到消费端，但未开火

新PG6bc / Native612 / XR137，0.4后台受控验证，肩部取枪通过；未产生第一枪实际弹道回执，按既有首失败停止流程正常退出，因此本轮没有进入准备流程，不能评价去重service是否改善游戏内准备耗时。

只读观察器40次采样、无错误，原始Source1的6808..6816记录trigger=1。更关键的是实际PG longshot_r2_source：436帧选中6809，current=1、pressed/released1/0、trigger1、muzzle_ready1、preparation_budget1、continuity_matched1，result8；437帧选中6816，同样current=1且仍DOWN，result2。实际枚举8=blocked、2=accepted。不能再把本轮定性为发布端未收到短按或消费端从未看到DOWN。尚不能仅凭result8区分晚期press_ready不满足、await_release或原版execute返回not_ready。

代码核对发现，现有唯一Input中的250ms未派发且仍按住的边沿等待机制，其调用端资格目前只给bound_is_lancer，没有给Longshot。该机制每次仍要求新的100ms有效Frame、同发布者/stream/tracking/身份、相同计数、扳机仍按住；释放即丢弃，时限沿原始producer sync不重启。这里只记录待评估缺口，尚未扩展Longshot或宣称它就是本轮根因。

71张GPU投影、1张全跨度总览及4张原图已查看，编码0，最大间隔0.54904秒。手臂遮挡仍在，未验收抓握。fire0/hand5/drained、Native/入口/手柄恢复、XR0、游戏消失、无强制终止。媒体最新3为59140/15748/56100；原始文本证据保留。完整换弹、镜内图像和GrabPose仍未完成。


### 2026-11-11 狙击枪首发的现有未派发按压等待资格（新候选）

狙击枪现在加入原有调用端defer_new_press_for_muzzle资格，仍要求current、已知绑定、完全相同weapon和hold_generation，原有release_required立即禁用。根目录保留其他现有武器资格，仅追加Longshot；冻结候选仅追加Longshot。Input、OriginalR2Fire两个实现不改，既有250ms原始producer时间期限、每次新100ms有效Frame、相同计数/发布者/stream/tracking、仍按住、释放取消、单次派发不变。没有新增队列或开火者。

用59140记录的6809/6816序号、计数与QPC时间构造自有来源/几何夹具，分别注入晚期press_ready=false和原版execute=not_ready：旧资格先blocked再accepted漏发，新资格先deferred再仅pressed一次。它不是实际完整Frame重放，亦不证明59140到底是哪种分支。旧/新各含原有Input测试，共2137/2141断言通过；根目录实际资格与Input测试2141通过。初次测试使用错误身份成员名而编译失败已保留，改为原registry_index后通过。

新增的最多16行Longshot原有诊断记录派发前held/animation/busy/press_ready/defer/await-release，以及原版press是否被调用、返回failed/executed/not_ready，不新增读游戏或时间查询。开栓PG7b6da8dc、完整PG97bafad9编译0，Native612/XR137未改。完整版本第一次组合因旧基线无该诊断文件而在编译前退出，已保留；r2明确加入既有有界诊断后编译通过。022826新实例已准备，尚无新的游戏修复或完整换弹验收结论。


### 2026-11-11 实际4044：等待分支实际执行，释放正确取消，首发仍失败

PG7b6 / Native612 / XR137，0.4受控后台；肩部取枪通过，无首发回执，未进入换弹准备。新诊断明确420帧current=1、trigger1、held1、animation0、busy0、defer_eligible1、await-release0，但preparation_budget0、muzzle_ready0、press_ready0，result10=deferred，原版press调用数0。421帧原始trigger0且released1，等待被取消，press仍为0；没有松开后补发。本轮实际覆盖等待分支，不能叫首发修复。

420原始sync为120355758662000，R2记录now=120355848328400，输入年龄89.6664ms。既有input_elapsed_ns=4591700，故进入R2时原输入已85.0747ms，剩余不足原20ms准备/派发预留。既有100ms与20ms都不放宽。下一步定位并减少进入开火前的工作；源代码已定位前置equipment观察、隐藏子弹准备和对Longshot不适用的Lancer starter检查，但尚无各项占用本轮预算的因果结论，不据此猜测具体耗时或重跑未修改方案。

只读观察器39次无错误。67张GPU投影、1张全跨度总览及4张原图已查看，编码0、最大间隔0.54501秒；接触仍遮挡，不是手部贴合验收。正常fire0/hand5/drained、Native/入口/手柄恢复、XR0、游戏消失，无强制终止。媒体最新3为4044/59140/15748。完整换弹、镜内图像、近眼缩放与GrabPose仍未验收。


### 2026-11-11 开火前非动作工作排序（新候选，未游戏验证）

仅在精确已持有Longshot、reload runtime空闲、尚无取得的item、右trigger active且有限且(.35,1]时，将本帧隐藏子弹准备从开火前移到同一原开火路径之后。现有round current/eligible/advance/restore和取弹库存授权均不改。换弹中、已取得子弹、非Longshot和无有效DOWN保持原顺序。真正库存处理state.tick仍在开火前，manual自动补弹veto仍在fire前；不将资源准备结果当开火/取弹许可。已确认Longshot不进入原Weap_AssaultRifle starter特技检查，其他武器原starter路径保留。

has_item只读getter及其前置声明用于当前翻译单元，避免直接依赖较后定义的字段。首版直接访问后定义字段导致编译失败，r1保留；r2经getter编译0。主产品只有4项文件变化，原开火/Input/20ms/100ms、Native612和XR137不变。原有有界诊断增加现成frame.now、R2开始/复制完成/枪口开始时间，不增加时钟或游戏查询。

实际neutral与调度代码的离线测试：开栓42、完整43、根目录45场景通过，含10项新增排序/边界场景。原Lancer/已上膛手持item/活动换弹/无效模拟通道排序、manual先于fire、resource只执行一次均检查。根目录提取片段含既有Gnasher资源边界，最初测试缺该边界而编译失败已保留；补齐只计数mock并确认其仍调用一次后通过。此为注入引擎边界的调度测试，不是实际性能结论。

开栓PG13546673、完整PG4792db37均编译；新024940准备使用当前共享启动源与哈希快照（同一次首次准入可在锁外有界等资源，登记后不重试），未修改旧实例哈希或保护。尚未游戏证明首发/准备修复。


### 2026-11-11 实际34720：原版按下执行，但首发事件未派发，武器drain失败

PG1354 / Native612 / XR137，0.4受控后台；肩部取枪通过，首发弹道仍缺失，未进入换弹准备。414帧原始5204为DOWN，current1、muzzle_ready1、preparation_budget1、press_ready1；result3=pressed、press_calls1、execution_plus_one2=executed。415帧5210为UP，result4=released。本轮按下原版ButtonPress成功一次，不能称实际开枪成功。

414原始输入在R2入口49.7051ms、观察点52.7718ms；frame.now-source.sync=7.2472ms。此为本轮实测，不是与4044隔离A/B的性能因果证明。现有日志随后记录initial_queued retained1/parameters8/synchronous_press0；首发BeginState事件异步到达，等待日志stage4，未见original_fire_delivered或shot_trajectory。取消后的原状态归还停在handoff61，位于原ShouldRefire Args大小、Longshot typed证明或ReturnValue验证的组合条件；具体子条件未证明，不放宽它或UP/100ms/20ms。

重要失败：launcher_passed=false，fire_disable2、hand_stop5、drained=false（初次fire2，hand stop后再次fire仍未通过）。Native/入口/手柄恢复、XR0、游戏进程消失、worker正常退出且无强制终止，并不代表武器drain通过。保留为失败，下一步优先查该取消状态归还条件及异步首发时序，禁止重跑未修改方案。

66张GPU投影、1张全跨度总览及4张原图已查看，编码0、最大间隔0.57684秒，接触仍遮挡，未验收GrabPose。媒体最新3为34720/4044/59140，文本原始证据保留。完整换弹、镜内图像、近眼缩放仍未验收。


2026-11-11 状态函数查找候选：实际 field/Args 查找只遍历类的字段与父类，不进入 WeaponFiring 状态子字段。取消首枪后的 Longshot ShouldRefire 改为从已证明的原始 BeginState 所属状态查找；其原始函数身份、签名、96 字节函数体、返回值、实际松键、RefireCheckTimer 与归还后的状态检查全部保留。类/状态查找 11 项及根源码同样 11 项通过；旧、新函数证明各 28 项、外层验证各 13 项结果与读取次数一致。开镜候选 PG31261b5d、完整候选 PGec90b4e0 编译完成，尚未游戏验证。私有候选增加限量失败条款记录。实际34720武器 drain 失败仍保留，尚无本轮命中/换弹/手型验收。


2026-11-11 受控运行衔接：本地运行器改用现有启动器的限时首次准入等待，仅 ResourceUnavailable 属于暂时忙；其余异常仍传播。5项离线检查通过。034408 实际执行60秒首次准入等待后因两个自动实例名额占用被拒绝，无工作进程或游戏，名字已消费、禁止复用。新增明确标注的0.6秒首枪保持验证变体，默认0.18秒短按逐字保留；24项离线组合与拒绝测试通过。变体只改变单次首枪保持时长，其他动作与产品代码不变，不代表短按问题修复。下一次真实结果仍需单独记录。
