import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import instance_policy as policy


class InstancePolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.private = self.root / 'analysis/instances/one'
        for folder in ('LocalAppData/Coalition/WarGame/Config', 'LocalAppData/Coalition/WarGame/SaveData',
                       'RoamingAppData', 'Documents', 'GameLogs', 'native'):
            (self.private / folder).mkdir(parents=True, exist_ok=True)
        (self.private / 'instance.json').write_text(json.dumps(dict(schema=1, name='one', root=str(self.private),
                                                                  original_localappdata='original')), encoding='utf-8')
        self.env = dict(GEARSVR_INSTANCE_ID='one', GEARSVR_INSTANCE_ROOT=str(self.private),
                        GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA='original')

    def test_default_and_partial_selections(self):
        self.assertIsNone(policy.context({}, self.root))
        for env in ({'GEARSVR_INSTANCE_ID': 'one'}, {'GEARSVR_INSTANCE_ROOT': str(self.private)},
                    dict(self.env, GEARSVR_INSTANCE_ID='../one')):
            with self.assertRaises(RuntimeError):
                policy.context(env, self.root)

    def test_validated_namespace_and_profile_identity(self):
        self.assertEqual(policy.context(self.env, self.root), self.private)
        with self.assertRaises(RuntimeError):
            policy.context(dict(self.env, GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA='different'), self.root)

    def test_private_native_directory_is_required(self):
        with patch.object(policy, 'context', return_value=self.private):
            policy.require_private_manifest(dict(native_build=str(self.private / 'native')))
            with self.assertRaises(RuntimeError):
                policy.require_private_manifest(dict(native_build=str(self.root / 'shared')))
            self.assertTrue(policy.owner_lock_name().endswith('.one'))
            self.assertEqual(policy.index_path(), self.private / 'launcher-session.json')
            self.assertEqual(policy.local_appdata(), self.private / 'LocalAppData')

    def test_missing_profile_directory_rejects_before_shared_game_checks_are_bypassed(self):
        (self.private / 'native').rmdir()
        with self.assertRaises(RuntimeError):
            policy.context(self.env, self.root)

    def test_only_exact_registered_private_process_is_excluded_from_normal_startup(self):
        executable=self.root/'GOWDE-Steam.exe'
        proof=json.loads((self.private/'instance.json').read_text())
        proof['game_path']=str(executable)
        (self.private/'instance.json').write_text(json.dumps(proof),encoding='utf-8')
        identity=dict(pid=10,creation_time=50,image=str(executable).casefold())
        (self.private/'launcher-session.json').write_text(json.dumps(dict(
            kind='repository_vr_launcher',game_identity=identity)),encoding='utf-8')
        self.assertEqual(policy.shared_game_pids([10,20],lambda pid:identity,self.root),[20])
        self.assertEqual(policy.shared_game_pids([10,20],lambda pid:dict(identity,creation_time=51),self.root),[10,20])
        self.assertEqual(policy.shared_game_pids([10,20],lambda pid:None,self.root),[10,20])

    def test_occlusion_group_includes_only_explicit_background_tests_with_current_identity(self):
        executable=self.root/'GOWDE-Steam.exe'
        proof=json.loads((self.private/'instance.json').read_text());proof['game_path']=str(executable)
        (self.private/'instance.json').write_text(json.dumps(proof),encoding='utf-8')
        identity=dict(pid=10,creation_time=50,image=str(executable).casefold())
        (self.private/'launcher-session.json').write_text(json.dumps(dict(kind='repository_vr_launcher',
            game_identity=identity)),encoding='utf-8')
        worker=self.private/'worker.json'
        for automatic,expected in [(False,set()),(True,{10})]:
            worker.write_text(json.dumps(dict(root=str(self.private),background_test=automatic)),encoding='utf-8')
            policy._background_cache.clear()
            self.assertEqual(policy.background_process_pids(lambda pid:identity,self.root),expected)
        self.assertEqual(policy.background_process_pids(lambda pid:dict(identity,creation_time=51),self.root),set())


if __name__ == '__main__':
    unittest.main()
