"""Equipment/fire lifecycle inside the owner's normal manual VR session.

No game launch, input simulation, test entry point or skeletal writer. Native
and Playground are already pinned by the ordinary startup. Real calibration
may complete while first-person/free hands remain available.
"""
from pathlib import Path
import sys
import time

ROOT=Path(__file__).resolve().parents[2]


class Session:
    def __init__(self,args,identity,check_live):
        self.args=args
        self.check_live=check_live
        self.bridge=None
        self.connected=False
        self.fire_enabled=False
        self.calibration_ready=False
        self.slot_layout_ready=False
        self.turn_configured=False
        self.reload_configured=False
        self.part_inputs_configured=False
        self.ranges_configured=False
        self.original_parts_ready=False
        self.hand_needs_stop=False
        self.hand_draining=False
        self.hand_drain_deadline=0.
        self.hand_busy_attempts=0
        self.hand_retry_exhausted=False
        self.next_attempt=0.
        self.state={'phase':'connecting_hands','hand_connected':False,'fire_enabled':False,'calibration_ready':False}
        sys.path.insert(0,str(ROOT/'tools/vr-launcher'))
        from playground_loader import artifact
        sys.path.insert(0,str(ROOT/'tools/playground'))
        from runtime import Bridge
        self.check_live()
        manifest=args.unified_playground
        self.bridge=Bridge(identity,artifact(manifest),expected_sha256=manifest['playground_sha256'],attach_only=True)
        from start_capture import modules
        loaded=modules(self.bridge.handle)
        item=loaded.get(args.dll_name.lower())
        expected=(args.build_dir/args.dll_name).resolve()
        if not item or Path(item[1]).resolve()!=expected:
            self.bridge.close();self.bridge=None
            raise RuntimeError('Unified Native module does not match the active manual candidate')
        self.native=item[0]
        for export in ('PlaygroundWeaponHandStart','PlaygroundWeaponHandStop','PlaygroundVrFireEnable','PlaygroundVrFireDisable'):
            if export not in self.bridge.exports:
                self.bridge.close();self.bridge=None
                raise RuntimeError('Unified equipment/fire export missing: '+export)
        if type(getattr(args,'weapon_part_assets',None)) is int and args.weapon_part_assets==1:
            from manual_part_preparation import verified_inputs
            self.check_live()
            try:
                prepared=verified_inputs(identity['image'],ROOT/'analysis/weapon-part-cache',
                    getattr(args,'prepared_original_part_inputs',None))
                self.state['weapon_part_inputs']=prepared['magazine']
                self.state['lancer_handle_inputs']=prepared['lancer_handle']
                self.original_parts_ready=prepared['magazine']['prepared']
            except (OSError,ValueError,MemoryError) as error:
                # Preserve first-person and existing equipment. A failed
                # original-part source cannot authorise use of a stale cache.
                self.state['weapon_part_inputs']={'prepared':False,'error':str(error)}
                self.state['lancer_handle_inputs']={'prepared':False,'error':str(error)}
            self.check_live()

    def poll(self):
        if (self.fire_enabled and self.calibration_ready) or self.hand_retry_exhausted or time.monotonic()<self.next_attempt:
            return self.state
        self.next_attempt=time.monotonic()+.5
        self.check_live()
        if self.hand_draining:
            # Start may have installed the parent before a child returned busy.
            # Stop owns that exact cookie; only quiescence permits another start.
            code=self.bridge.export('PlaygroundWeaponHandStop')
            self.state['hand_stop_code']=code
            if code==4:
                if time.monotonic()>=self.hand_drain_deadline:
                    raise RuntimeError('Unified equipment connection cleanup timed out')
                return self.state
            if code not in (0,5):
                raise RuntimeError('Unified equipment connection cleanup rejected: '+str(code))
            self.hand_draining=False
            self.hand_needs_stop=False
            if self.hand_busy_attempts>=16:
                self.hand_retry_exhausted=True
                self.state.update(phase='equipment_busy_retry_exhausted',hand_connected=False,fire_enabled=False)
            else:
                self.state.update(phase='waiting_to_reconnect',hand_connected=False)
            return self.state
        if not self.connected:
            # Even a failed/partial start must be retired during session cleanup.
            self.hand_needs_stop=True
            code=self.bridge.export('PlaygroundWeaponHandStart',self.native)
            self.state['hand_start_code']=code
            if code==1:
                self.state.update(phase='waiting_for_real_armspan')
                return self.state  # no fabricated calibration, no repeated input
            if code in (3,4):
                # Off-state PG tick contention and partial child attachment share
                # busy. An off-state stop is quiescent; a partial one drains.
                self.hand_busy_attempts+=1
                self.hand_draining=True
                self.hand_drain_deadline=time.monotonic()+5.
                self.state.update(phase='draining_before_reconnect',hand_busy_attempts=self.hand_busy_attempts)
                return self.state
            if code!=0:
                raise RuntimeError('Unified equipment connection rejected: '+str(code))
            self.connected=True
            self.state.update(phase='enabling_fire',hand_connected=True)
        if not self.reload_configured and 'PlaygroundLancerReloadPolicy' in self.bridge.exports:
            from reload_policy import read_policy
            config_path=ROOT/'6dof-reload.ini'
            if not config_path.exists():config_path=ROOT/'tools/vr-launcher/6dof-reload.ini'
            policy=read_policy(config_path)
            code=self.bridge.export('PlaygroundLancerReloadPolicy',policy['packed'])
            if code!=0:raise RuntimeError('Lancer reload configuration rejected: '+str(code))
            if 'PlaygroundWeaponReloadPolicy' in self.bridge.exports:
                for kind,weapon in ((1,'Snub'),(2,'Hammerburst')):
                    selected=read_policy(config_path,weapon)
                    code=self.bridge.export('PlaygroundWeaponReloadPolicy',selected['packed']|(kind<<40))
                    if code!=0:raise RuntimeError(weapon+' reload configuration rejected: '+str(code))
                    self.state[weapon.lower()+'_reload_policy']={key:value for key,value in selected.items() if key!='packed'}
            self.reload_configured=True
            self.state['lancer_reload_policy']={key:value for key,value in policy.items() if key!='packed'}
        if not self.part_inputs_configured and type(getattr(self.args,'weapon_part_assets',None)) is int and self.args.weapon_part_assets==1:
            export='PlaygroundOriginalPartInputsEnable'
            code=self.bridge.export(export,int(self.original_parts_ready)) if export in self.bridge.exports else None
            self.state.setdefault('weapon_part_inputs',{})['enable_code']=code
            self.state['weapon_part_inputs']['enabled']=code==0 and self.original_parts_ready
            self.part_inputs_configured=True
        if not self.ranges_configured:
            requested=getattr(self.args,'lancer_interaction_ranges',False)
            if type(requested) is not bool:raise ValueError('Lancer interaction ranges must be boolean')
            export='PlaygroundLancerInteractionRanges'
            code=self.bridge.export(export,int(requested)) if export in self.bridge.exports else None
            if requested and code!=0:raise RuntimeError('Lancer interaction range display unavailable: '+str(code))
            self.ranges_configured=True
            self.state['lancer_interaction_ranges']=dict(requested=requested,enabled=requested and code==0,code=code)
        if not self.turn_configured and 'PlaygroundControllerTurnDeadzone' in self.bridge.exports:
            from right_stick_policy import percent
            configured=getattr(self.args,'right_stick_deadzone_percent',None)
            deadzone=percent(18. if configured is None else configured)
            code=self.bridge.export('PlaygroundControllerTurnDeadzone',round(deadzone*100))
            if code!=0:raise RuntimeError('Unified controller turn configuration rejected: '+str(code))
            self.turn_configured=True
            self.state['controller_turn_deadzone_percent']=deadzone
            self.state['controller_turn_mode']='original_yaw_delta_90_degrees_per_second'
        if not self.calibration_ready:
            # New candidates register hands independently of body-slot sizing.
            # A default slot layout permits equipment without pretending that
            # the owner's body was measured. Real sizing can replace it later.
            if 'PlaygroundEquipmentCalibrationRefresh' in self.bridge.exports:
                code=self.bridge.export('PlaygroundEquipmentCalibrationRefresh')
                self.state['calibration_code']=code
                if code not in (0,1,3,6):
                    raise RuntimeError('Unified calibration refresh rejected: '+str(code))
                self.calibration_ready=code==0
                if code in (0,6):
                    self.slot_layout_ready=True
                    self.state['slot_layout_source']='measured' if code==0 else 'authored_default'
                    if code==6:self.next_attempt=time.monotonic()+5.
            else:
                # Older PG required a valid calibration before accepting start.
                self.calibration_ready=True
                self.slot_layout_ready=True
            self.state['calibration_ready']=self.calibration_ready
            self.state['slot_layout_ready']=self.slot_layout_ready
        if not getattr(self.args,'unified_fire_ready',False):
            self.state.update(phase='equipment_enabled_fire_pending',fire_enabled=False)
            return self.state
        if not self.fire_enabled:
            code=self.bridge.export('PlaygroundVrFireEnable')
            self.state['fire_enable_code']=code
            if code!=0:
                raise RuntimeError('Unified fire activation rejected: '+str(code))
            self.fire_enabled=True
        self.state.update(phase=('equipment_and_fire_enabled' if self.calibration_ready else
            'equipment_and_fire_enabled_default_slots' if self.slot_layout_ready else
            'hands_enabled_waiting_for_real_armspan'),fire_enabled=True)
        return self.state

    def close(self,timeout):
        result={}
        if self.bridge is None:
            return result
        try:
            if self.bridge.control.game_absent(self.bridge.identity['pid']):
                return {'game_exited':True}
            if self.fire_enabled:
                result['fire_disable_code']=self.bridge.export('PlaygroundVrFireDisable')
            if self.hand_needs_stop:
                deadline=time.monotonic()+timeout
                while True:
                    code=self.bridge.export('PlaygroundWeaponHandStop')
                    result['hand_stop_code']=code
                    if code==5 or code==0 or time.monotonic()>=deadline:
                        break
                    if code!=4:
                        raise RuntimeError('Unified hand stop rejected: '+str(code))
                    time.sleep(.05)
            result['drained']=result.get('fire_disable_code',0)==0 and result.get('hand_stop_code',5) in (0,5)
            return result
        finally:
            self.bridge.close();self.bridge=None
