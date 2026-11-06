"""Codec/recovery fixtures only; no game, native function or GPU execution."""
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import weapon_part_cache as cache

class PartCacheTests(unittest.TestCase):
    def test_raw_lz4_literals_extension_and_overlapping_match(self):
        self.assertEqual(cache.lz4_block(b'\x40test',4),b'test')
        self.assertEqual(cache.lz4_block(b'\xf0\x05'+b'x'*20,20),b'x'*20)
        self.assertEqual(cache.lz4_block(b'\x15a\x01\x00',10),b'a'*10)

    def test_malformed_compression_cannot_overrun_or_reference_unknown_output(self):
        for packed,size in [(b'\xf0',20),(b'\x40tes',4),(b'\x15a\x00\x00',10),
                            (b'\x15a\x02\x00',10),(b'\x15a\x01\x00',9),
                            (b'\x10x',2),(b'',1)]:
            with self.subTest(packed=packed,size=size),self.assertRaises(ValueError):
                cache.lz4_block(packed,size)

    def test_cache_recovery_preserves_corruption_and_source_is_revalidated(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);game=root/'game/Binaries_x64/GOWDE-Steam.exe'
            game.parent.mkdir(parents=True);game.write_bytes(b'fixture')
            output=root/'private';vertices=b'fixture-vertices';indices=b'fixture-indices'
            with mock.patch.object(cache,'read_export',return_value=b'original') as source, \
                 mock.patch.object(cache,'build_input',return_value=(vertices,indices)), \
                 mock.patch.object(cache,'VERTEX_SHA256',hashlib.sha256(vertices).hexdigest()), \
                 mock.patch.object(cache,'INDEX_SHA256',hashlib.sha256(indices).hexdigest()):
                result=cache.prepare(game,output);folder=Path(result['directory'])
                first=(folder/'snub-magazine.vtx60').read_bytes()
                cache.prepare(game,output);self.assertEqual(source.call_count,2)
                self.assertEqual((folder/'snub-magazine.vtx60').read_bytes(),first)
                bad=b'X'*len(vertices);(folder/'snub-magazine.vtx60').write_bytes(bad)
                cache.prepare(game,output)
                self.assertEqual((folder/'snub-magazine.vtx60').read_bytes(),vertices)
                self.assertEqual(next(folder.glob('*.rejected-*')).read_bytes(),bad)
                self.assertEqual(list(folder.glob('*.tmp')),[])
                with self.assertRaisesRegex(ValueError,'outside the game'):
                    cache.prepare(game,game.parent.parent/'cache')
            self.assertEqual(game.read_bytes(),b'fixture')

if __name__=='__main__':unittest.main()
