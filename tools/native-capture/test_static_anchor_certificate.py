"""Static certificates retain guards and make no diagnostic pose claim."""
import struct
import unittest
from unittest import mock

from test_anchor_config import Fixture
import anchor_config as anchor
from start_capture import acquire_observer_preparation
from world_rebind import build_candidate


class StaticAnchorTests(unittest.TestCase):
    def test_configuration_and_ini_equal_diagnostic_mode(self):
        ordinary,metadata=anchor.build_anchor_config(Fixture())
        static,evidence=anchor.build_anchor_config(Fixture(),sample_pose=False)
        self.assertEqual(ordinary,static);self.assertEqual(anchor.anchor_ini(ordinary),anchor.anchor_ini(static))
        self.assertTrue(metadata['sample']['accepted']);self.assertTrue(metadata['pose_sampled'])
        self.assertIsNone(evidence['sample']);self.assertFalse(evidence['pose_sampled'])
        self.assertIsNone(evidence['sample_acquisition']['accepted'])
        self.assertFalse(evidence['sample_acquisition']['pose_observation_authority'])
        self.assertGreater(evidence['strict_guard_attempts'],0)
        self.assertFalse(evidence['renderer_alignment_verified'])

    def test_mounted_configuration_and_wire_format_are_identical(self):
        from test_mounted_integration import MountedUIFixture
        ordinary,_=anchor.build_anchor_config(MountedUIFixture())
        static,evidence=anchor.build_anchor_config(MountedUIFixture(),sample_pose=False)
        self.assertEqual(ordinary['schema'],3);self.assertEqual(static,ordinary)
        self.assertEqual(anchor.anchor_ini(ordinary),anchor.anchor_ini(static))
        self.assertIsNone(evidence['sample'])

    def test_motion_between_guards_does_not_require_frozen_diagnostic_pose(self):
        guard=anchor.LiveRegistryAnchorSampler.guard
        def advancing(sampler):
            result=guard(sampler)
            stamp=struct.unpack('<f',sampler.r.read(sampler.stamp,4))[0]
            sampler.r.put(sampler.stamp,struct.pack('<f',stamp+1.))
            return result
        with mock.patch.object(anchor.LiveRegistryAnchorSampler,'guard',advancing):
            with self.assertRaises(anchor.AnchorSamplingRejected):anchor.build_anchor_config(Fixture())
            config,metadata=anchor.build_anchor_config(Fixture(),sample_pose=False)
        self.assertIsNotNone(config);self.assertIsNone(metadata['sample'])

    def test_static_mode_never_calls_diagnostic_sampler(self):
        with mock.patch.object(anchor.LiveRegistryAnchorSampler,'sample',side_effect=AssertionError('Diagnostic not requested')):
            anchor.build_anchor_config(Fixture(),sample_pose=False)

    def test_strict_identity_and_link_failure_remain_terminal(self):
        for mode in (True,False):
            r=Fixture();original=anchor.LiveRegistryAnchorSampler._strict_guard
            calls=[0]
            def changing(sampler):
                calls[0]+=1
                if calls[0]==2:
                    offset=sampler.layout['controller_pawn']['offset']
                    sampler.r.put(sampler.controller+offset,struct.pack('<Q',0))
                return original(sampler)
            with self.subTest(sample_pose=mode),mock.patch.object(anchor.LiveRegistryAnchorSampler,'_strict_guard',changing):
                with self.assertRaises(anchor.AnchorSamplingRejected):anchor.build_anchor_config(r,sample_pose=mode)

    def test_static_mode_does_not_relax_inside_guard_timestamp_bracket(self):
        original=anchor.LiveRegistryAnchorSampler._strict_guard
        def changing(sampler):
            read=sampler.r.read;stamp=[1.]
            def unstable(address,size):
                if (address,size)==(sampler.stamp,4):
                    stamp[0]+=1.;return struct.pack('<f',stamp[0])
                return read(address,size)
            with mock.patch.object(sampler.r,'read',side_effect=unstable):return original(sampler)
        with mock.patch.object(anchor.LiveRegistryAnchorSampler,'_strict_guard',changing):
            with self.assertRaises(anchor.AnchorSamplingRejected):anchor.build_anchor_config(Fixture(),sample_pose=False)

    def test_static_recovery_requires_attestation_before_acquisition(self):
        with self.assertRaisesRegex(ValueError,'live entry attestation'):
            build_candidate(1,{},None,1,None,None,signatures={},sample_pose=False)
        with self.assertRaisesRegex(ValueError,'attested anchor observer'):
            acquire_observer_preparation(1,{},None,{},None,anchor_observer=True,anchor_sample_pose=False)

    def test_attested_static_acquisition_forwards_mode_and_closes_reader(self):
        import start_capture
        camera=dict(local_player_address='0x1',module_base='0x1000')
        r=mock.Mock();r.base=0x1000;r.resolve.return_value=camera.copy()
        entry=mock.Mock(return_value=b'ok')
        with mock.patch.object(start_capture,'Resolver',return_value=r),\
             mock.patch.object(anchor,'build_anchor_config',return_value=({},{}))as build,\
             mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'):
            result=acquire_observer_preparation(1,{},camera,{16:b'ok'},None,
                anchor_observer=True,anchor_sample_pose=False,entry_reader=entry)
        self.assertIs(build.call_args.kwargs['sample_pose'],False)
        self.assertEqual(result[2],'anchor\n');r.p.close.assert_called_once()
        entry.assert_called_once_with(0x1010,2,r.read)

    def test_mode_requires_explicit_boolean(self):
        for mode in (0,1,None,'false'):
            with self.subTest(mode=mode),self.assertRaises(ValueError):anchor.build_anchor_config(Fixture(),sample_pose=mode)


if __name__=='__main__':unittest.main()
