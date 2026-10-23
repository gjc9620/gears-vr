import json
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
from performance_guard import blockers,process_snapshot,WINDOWS_EPOCH_100NS

class PerformanceGuardTests(unittest.TestCase):
    def test_raw_instances_and_compiler_are_seen_without_shared_index(self):
        with tempfile.TemporaryDirectory()as directory:
            result=blockers(directory,{10:{'name':'gowde-steam.exe'},20:{'name':'goweday-steam.exe'},
                30:{'name':'cl.exe'}},lambda pid:None,exclude={10})
            self.assertEqual({(x['kind'],x['pid'])for x in result},{('game',20),('compiler',30)})

    def test_live_worker_blocks_even_between_game_launches(self):
        with tempfile.TemporaryDirectory()as directory:
            root=Path(directory);path=root/'analysis/instances/test/worker.json';path.parent.mkdir(parents=True)
            path.write_text(json.dumps({'worker_pid':10}),encoding='utf-8')
            owner={'pid':10,'creation_time':WINDOWS_EPOCH_100NS+int(time.time()*10_000_000)-10_000_000,'image':'python.exe'}
            self.assertEqual(blockers(root,{10:{'name':'python.exe'}},lambda pid:owner)[0]['kind'],'worker')
            self.assertEqual(blockers(root,{},lambda pid:None),[])
            self.assertEqual(blockers(root,{10:{'name':'python.exe'}},lambda pid:owner,exclude={10}),[])

    def test_reused_worker_pid_is_not_treated_as_live_registration(self):
        with tempfile.TemporaryDirectory()as directory:
            root=Path(directory);path=root/'analysis/instances/test/worker.json';path.parent.mkdir(parents=True)
            path.write_text(json.dumps({'worker_pid':10}),encoding='utf-8');old=time.time()-100;os.utime(path,(old,old))
            owner={'pid':10,'creation_time':WINDOWS_EPOCH_100NS+int(time.time()*10_000_000),'image':'python.exe'}
            self.assertEqual(blockers(root,{10:{'name':'python.exe'}},lambda pid:owner),[])

    def test_private_launcher_needs_exact_creation_identity(self):
        with tempfile.TemporaryDirectory()as directory:
            root=Path(directory);path=root/'analysis/instances/test/launcher-session.json';path.parent.mkdir(parents=True)
            owner={'pid':10,'creation_time':1,'image':'python.exe'}
            path.write_text(json.dumps({'owner_identity':owner}),encoding='utf-8')
            processes={10:{'name':'python.exe'}}
            self.assertEqual(blockers(root,processes,lambda pid:owner)[0]['kind'],'launcher')
            self.assertEqual(blockers(root,processes,lambda pid:dict(owner,creation_time=2)),[])

    @unittest.skipUnless(sys.platform=='win32','native Windows snapshot')
    def test_native_snapshot_reports_current_process_and_parent(self):
        snapshot=process_snapshot();row=snapshot[os.getpid()]
        self.assertEqual(row['name'],Path(sys.executable).name.casefold())
        self.assertEqual(row['parent'],os.getppid())

if __name__=='__main__':unittest.main()
