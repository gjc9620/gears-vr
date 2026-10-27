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
        self.hand_needs_stop=False
        self.hand_draining=False
        self.hand_drain_deadline=0.
        self.hand_busy_attempts=0
        self.hand_retry_exhausted=False
        self.next_attempt=0.
        self.state={'phase':'waiting_for_real_armspan','hand_connected':False,'fire_enabled':False}
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

    def poll(self):
        if self.fire_enabled or self.hand_retry_exhausted or time.monotonic()<self.next_attempt:
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
        if not getattr(self.args,'unified_fire_ready',False):
            self.state.update(phase='equipment_enabled_fire_pending',fire_enabled=False)
            return self.state
        code=self.bridge.export('PlaygroundVrFireEnable')
        self.state['fire_enable_code']=code
        if code!=0:
            raise RuntimeError('Unified fire activation rejected: '+str(code))
        self.fire_enabled=True
        self.state.update(phase='equipment_and_fire_enabled',fire_enabled=True)
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
