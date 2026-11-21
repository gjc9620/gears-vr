"""Cache identity/lifecycle fixtures; no engine calls or copied game geometry."""
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import boomshot_carrier_cache as cache


class BoomshotCarrierCacheTests(unittest.TestCase):
    def test_selected_map_is_revalidated_and_existing_bytes_are_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);game=root/'game/Binaries_x64/GOWDE-Steam.exe'
            game.parent.mkdir(parents=True);game.write_bytes(b'owned executable fixture')
            with mock.patch.object(cache,'read_export',return_value=b'original') as source, \
                 mock.patch.object(cache,'build_input',return_value=(b'vertices',b'indices')):
                first=cache.prepare(game,root/'private')
                second=cache.prepare(game,root/'private')
                self.assertEqual(first,second);self.assertEqual(source.call_count,2)
                self.assertEqual(source.call_args.kwargs['package_sha256'],cache.PACKAGE_SHA256)
                self.assertEqual(source.call_args.kwargs['export_sha256'],cache.EXPORT_SHA256)
                self.assertEqual(source.call_args.args[0],game.parent.parent/'WarGame/CookedPC/Maps/MP_Maps/MP_WarMachine.war')
                folder=Path(first['directory']);bad=folder/'boomshot-carrier.vtx60'
                bad.write_bytes(b'preserve corrupted evidence')
                with self.assertRaisesRegex(ValueError,'Different existing'):
                    cache.prepare(game,root/'private')
                self.assertEqual(bad.read_bytes(),b'preserve corrupted evidence')
                self.assertEqual(list(folder.glob('*.tmp')),[])
                with self.assertRaisesRegex(ValueError,'outside game'):
                    cache.prepare(game,game.parent.parent/'cache')
            self.assertEqual(game.read_bytes(),b'owned executable fixture')

    def test_unknown_source_does_not_create_cache(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);destination=root/'private'
            with mock.patch.object(cache,'read_export',side_effect=ValueError('source mismatch')):
                with self.assertRaisesRegex(ValueError,'source mismatch'):
                    cache.prepare(root/'game/Binaries_x64/GOWDE-Steam.exe',destination)
            self.assertFalse(destination.exists())

    def test_arbitrary_or_truncated_export_cannot_be_converted(self):
        for data in (b'',b'\0'*cache.EXPORT_BYTES,b'\0'*(cache.EXPORT_BYTES-1)):
            with self.assertRaisesRegex(ValueError,'Unverified'):
                cache.build_input(data)


if __name__=='__main__':unittest.main()
