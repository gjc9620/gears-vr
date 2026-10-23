"""Real mounted builders and serializer with synthetic memory and attestation."""
import dataclasses
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest import mock

import start_capture
import cinematic_anchor
import world_rebind as rebind
from test_mounted_integration import MountedUIFixture


def mounted_candidate():
    f=MountedUIFixture().stream_levels();f.p=SimpleNamespace(close=mock.Mock())
    camera=dict(f.resolve(),executable=str(Path('synthetic-game.exe').resolve()))
    camera['offsets']=dict(camera['offsets'],Rotation='0xc')
    game=dict(pid=f.pid,creation_time=12345,image=camera['executable'].casefold())
    owner=dict(pid=f.pid+1,creation_time=54321,image=str(Path('synthetic-owner.exe').resolve()).casefold())
    local=rebind.identity(f.identity(int(camera['local_player_address'],0)))
    dll=Path('synthetic-native.dll').resolve()
    proof=SimpleNamespace(pid=f.pid,game_creation=game['creation_time'],owner=owner,dll=dll,module_base=f.base,
        reverify=mock.Mock(),read_entry=lambda address,size,reader:reader(address,size))
    f.put(f.base+0x1234,b'synthetic entry')
    with mock.patch.object(rebind,'process_identity',side_effect=lambda pid:game if pid==f.pid else owner),\
         mock.patch.object(start_capture,'Resolver',return_value=f),\
         mock.patch.object(f,'resolve',return_value=camera),\
         mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
        candidate=rebind.build_candidate(f.pid,f.profile,local,1,owner,dll,signatures={0x1234:b'synthetic entry'},
            attestation=proof,sample_pose=False,crosshair_assignment_observer=True)
    f.p.close.assert_called_once();assert proof.reverify.call_count==2
    return candidate


class MountedSerializationTests(unittest.TestCase):
    def test_full_mounted_candidate_serializes_with_streamed_levels_and_suspension(self):
        candidate=mounted_candidate();before_anchor=rebind.thaw(candidate.anchor_metadata)
        before_ui=rebind.thaw(candidate.ui_metadata);data=rebind.serialize_candidate(candidate)
        text=data.decode('utf-16')
        self.assertIn('[WorldRebind]\n',text);self.assertIn('mounted_enabled=1\n',text)
        self.assertIn('[CrosshairAssignment]\nschema=2\nnative_contract=2\n',text)
        self.assertIn('crosshair_assignment_suspended',candidate.ui_metadata)
        self.assertIn('guard_acquisitions',candidate.anchor_metadata)
        self.assertEqual(rebind.thaw(candidate.anchor_metadata),before_anchor)
        self.assertEqual(rebind.thaw(candidate.ui_metadata),before_ui)

    def test_mounted_suspension_and_final_source_changes_cannot_serialize(self):
        candidate=mounted_candidate()
        for fault in ('missing_suspension','suspension_role','final_controller','resolver_camera','ui_controlled'):
            anchor=rebind.thaw(candidate.anchor_metadata);ui=rebind.thaw(candidate.ui_metadata)
            if fault=='missing_suspension':ui.pop('crosshair_assignment_suspended')
            if fault=='suspension_role':ui['crosshair_assignment_suspended']['controller']^=0x1000
            if fault=='final_controller':anchor['certificate_final_guard']['controller']['number']+=1
            if fault=='resolver_camera':anchor['sampler']['resolver']['camera_address']='0xdead'
            if fault=='ui_controlled':ui['config']['roles']['controlled']=ui['config']['roles']['pawn']
            changed=dataclasses.replace(candidate,anchor_metadata=rebind.freeze(anchor),ui_metadata=rebind.freeze(ui))
            with self.subTest(fault=fault),self.assertRaises((ValueError,KeyError)):
                rebind.serialize_candidate(changed)


if __name__=='__main__':unittest.main()
