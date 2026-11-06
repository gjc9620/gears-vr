"""Prepare original geometry before starting game/XR; verify at handoff.

No game calls or resource-package copies. Generated data stays in the owner's
private cache. A failure disables the corresponding part input, not free hands.
"""
from pathlib import Path
import hashlib,os,stat
import weapon_part_cache as magazine
import lancer_handle_cache as handle

def _source(game_exe):
    game=Path(game_exe).resolve()
    return game,game.parent.parent/'WarGame/CookedPC/WarfareGame.u'

def _regular(path):
    for p in (path,*path.parents):
        info=p.lstat()
        if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
            raise ValueError('Reparse original-part input')
    if not path.is_file():raise ValueError('Missing original-part input')

def _digest(path,size,digest):
    _regular(path)
    with path.open('rb') as stream:
        if os.fstat(stream.fileno()).st_size!=size or hashlib.file_digest(stream,'sha256').hexdigest()!=digest:
            raise ValueError('Original-part input identity mismatch')

def prepare_inputs(game_exe,cache_root):
    game,package=_source(game_exe);cache=Path(cache_root).absolute()
    result=dict(schema=1,game_path=str(game),cache_root=str(cache),stage='before_game_reservation',parts={})
    for name,prepare in (('magazine',magazine.prepare),('lancer_handle',handle.prepare)):
        try:
            value=prepare(game,cache)
            result['parts'][name]=dict(prepared=True,source_sha256=value['source_sha256'])
        except (OSError,ValueError,MemoryError) as error:
            result['parts'][name]=dict(prepared=False,error=type(error).__name__+': '+str(error))
    return result

def verified_inputs(game_exe,cache_root,receipt):
    game,package=_source(game_exe);cache=Path(cache_root).absolute()
    if not isinstance(receipt,dict) or not isinstance(receipt.get('parts'),dict) or receipt.get('schema')!=1 or receipt.get('game_path')!=str(game) or \
       receipt.get('cache_root')!=str(cache) or receipt.get('stage')!='before_game_reservation':
        raise ValueError('Missing matching pre-launch part preparation')
    # Retain full pinned-source verification. Only the repeated LZ4 decode and
    # geometry construction are removed from the live VR session.
    _digest(package,magazine.PACKAGE_BYTES,magazine.PACKAGE_SHA256)
    out={}
    checks={
        'magazine':(magazine.CACHE_VERSION,magazine.EXPORT_SHA256,
            (('snub-magazine.vtx60',279*60,magazine.VERTEX_SHA256),
             ('snub-magazine.idx16',942*2,magazine.INDEX_SHA256))),
        'lancer_handle':(handle.CACHE_VERSION,handle.EXPORT_SHA256,
            (('pullA.delta20',handle.COUNT*20,handle.DELTA_SHA256),
             ('pullB.delta20',handle.COUNT*20,handle.DELTA_SHA256)))}
    for name,(version,source,files) in checks.items():
        item=receipt.get('parts',{}).get(name,{})
        if not isinstance(item,dict):raise ValueError('Malformed prepared part')
        if item.get('prepared') is not True:
            out[name]=dict(prepared=False,error=item.get('error','Part not prepared'));continue
        try:
            if item.get('source_sha256')!=source:raise ValueError('Prepared export identity mismatch')
            for filename,size,digest in files:_digest(cache/version/filename,size,digest)
            out[name]=dict(prepared=True,source_sha256=source)
        except (OSError,ValueError,MemoryError) as error:
            out[name]=dict(prepared=False,error=type(error).__name__+': '+str(error))
    return out
