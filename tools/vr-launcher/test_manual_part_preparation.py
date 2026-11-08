import copy,hashlib,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
import manual_part_preparation as p

class PartHandoffTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);self.exe=self.root/'Game/Binaries_x64/GOWDE-Steam.exe'
        self.exe.parent.mkdir(parents=True);self.exe.write_bytes(b'fixture executable')
        self.package=self.exe.parent.parent/'WarGame/CookedPC/WarfareGame.u'
        self.package.parent.mkdir(parents=True);self.package.write_bytes(b'pinned fixture package')
        self.cache=self.root/'private-cache';self.payloads={}
        def bind(obj,name,value):
            item=patch.object(obj,name,value);item.start();self.addCleanup(item.stop)
        bind(p.magazine,'PACKAGE_BYTES',self.package.stat().st_size)
        bind(p.magazine,'PACKAGE_SHA256',hashlib.sha256(self.package.read_bytes()).hexdigest())
        for directory,name,data,module,field in (
            (p.magazine.CACHE_VERSION,'snub-magazine.vtx60',b'v'*(279*60),p.magazine,'VERTEX_SHA256'),
            (p.magazine.CACHE_VERSION,'snub-magazine.idx16',b'i'*(942*2),p.magazine,'INDEX_SHA256'),
            (p.handle.CACHE_VERSION,'pullA.delta20',b'd'*(p.handle.COUNT*20),p.handle,'DELTA_SHA256'),
            (p.handle.CACHE_VERSION,'pullB.delta20',b'd'*(p.handle.COUNT*20),p.handle,'DELTA_SHA256'),
            (p.lancer_magazine.CACHE_VERSION,'lancer-magazine.vtx60',b'l'*(120*60),p.lancer_magazine,'VERTEX_SHA256'),
            (p.lancer_magazine.CACHE_VERSION,'lancer-magazine.idx16',b'm'*(468*2),p.lancer_magazine,'INDEX_SHA256')):
            dest=self.cache/directory/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
            self.payloads[name]=dest;bind(module,field,hashlib.sha256(data).hexdigest())
        self.receipt=dict(schema=1,game_path=str(self.exe.resolve()),cache_root=str(self.cache.absolute()),
            stage='before_game_reservation',parts=dict(magazine=dict(prepared=True,source_sha256=p.magazine.EXPORT_SHA256),
                lancer_handle=dict(prepared=True,source_sha256=p.handle.EXPORT_SHA256),
                lancer_magazine=dict(prepared=True,source_sha256=p.lancer_magazine.EXPORT_SHA256)))
    def verify(self,receipt=None):return p.verified_inputs(self.exe,self.cache,self.receipt if receipt is None else receipt)
    def test_live_handoff_checks_files_without_decoding(self):
        with patch.object(p.magazine,'prepare',side_effect=AssertionError('decode')),patch.object(p.handle,'prepare',side_effect=AssertionError('decode')):
            self.assertTrue(all(x['prepared'] for x in self.verify().values()))
    def test_changed_source_rejected_even_with_matching_cache(self):
        self.package.write_bytes(b'changed fixture source')
        with self.assertRaises(ValueError):self.verify()
    def test_same_size_source_corruption_rejected(self):
        self.package.write_bytes(b'x'*self.package.stat().st_size)
        with self.assertRaises(ValueError):self.verify()
    def test_corrupt_magazine_does_not_grant_original_parts(self):
        f=self.payloads['snub-magazine.vtx60'];f.write_bytes(b'x'*f.stat().st_size)
        result=self.verify();self.assertFalse(result['magazine']['prepared']);self.assertTrue(result['lancer_handle']['prepared'])
    def test_missing_handle_does_not_grant_handle(self):
        self.payloads['pullA.delta20'].unlink();self.assertFalse(self.verify()['lancer_handle']['prepared'])
    def test_lancer_magazine_corruption_does_not_affect_other_inputs(self):
        f=self.payloads['lancer-magazine.vtx60'];f.write_bytes(b'x'*f.stat().st_size)
        result=self.verify();self.assertFalse(result['lancer_magazine']['prepared'])
        self.assertTrue(result['magazine']['prepared']);self.assertTrue(result['lancer_handle']['prepared'])
    def test_older_receipt_does_not_claim_matching_magazine_appearance(self):
        r=copy.deepcopy(self.receipt);del r['parts']['lancer_magazine']
        result=self.verify(r);self.assertFalse(result['lancer_magazine']['prepared'])
        self.assertTrue(result['magazine']['prepared'])
    def test_wrong_game_receipt_rejected(self):
        r=copy.deepcopy(self.receipt);r['game_path']=str(self.exe.with_name('Other.exe'))
        with self.assertRaises(ValueError):self.verify(r)
    def test_missing_or_malformed_receipt_rejected(self):
        for value in ({},dict(self.receipt,parts=None),dict(self.receipt,stage='live_vr_decode')):
            with self.subTest(value=value),self.assertRaises(ValueError):self.verify(value)
    def test_old_export_identity_rejected(self):
        r=copy.deepcopy(self.receipt);r['parts']['lancer_handle']['source_sha256']='0'*64
        self.assertFalse(self.verify(r)['lancer_handle']['prepared'])
    def test_preparation_allocation_failure_is_reported(self):
        with patch.object(p.magazine,'prepare',side_effect=MemoryError()),patch.object(p.handle,'prepare',side_effect=MemoryError()),patch.object(p.lancer_magazine,'prepare',side_effect=MemoryError()):
            result=p.prepare_inputs(self.exe,self.cache)
        self.assertFalse(result['parts']['magazine']['prepared']);self.assertFalse(result['parts']['lancer_handle']['prepared'])

if __name__=='__main__':unittest.main()
