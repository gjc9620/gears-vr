"""Pure serializer for a separately owned, read-only CloseScene request.

No change to the position schema. Native startup certifies original full bodies;
the loaded request is recertified through its exact sealed own hook prefix.
"""
import struct

import path_choice_position_config as position
import path_choice_trial_config as owner


def build(sample, panelproof, close_metadata, *, observer_duration_ms=8000, **session):
    """Use fresh same-process CloseScene metadata, never a historical heap address.

    close_metadata uses the existing bounded CloseScene probe receipt shape:
    read_only/consistent_final_reread/process_owner_verified, pid/creation_time,
    module_base, client/function/owner_class/header_hex and session_binding.
    Native pointer and all code/slot certifications are performed again natively.
    The caller final-rereads this metadata in its pre-sidecar acquisition, publishes
    both owned sidecars for the SAME nonce, and preserves them through drain.
    """
    require=owner._require;integer=owner._int
    ini=position.build(sample,panelproof,**session)
    fields=dict(line.split('=',1) for line in ini.splitlines()[1:] if line)
    try:
        meta=close_metadata;snapshot=sample['scene_owner'];base=int(fields['base'],0)
        require(meta['read_only'] is True and meta['consistent_final_reread'] is True and
                meta['process_owner_verified'] is True and meta['native_called'] is False and
                meta['game_memory_written'] is False,'CloseScene metadata is not a fresh read-only receipt')
        binding=meta['session_binding'];game=binding['game']
        require(meta['pid']==sample['pid']==int(fields['pid'])==game['pid'] and
                meta['creation_time']==game['creation_time']==int(fields['creation_time']) and
                int(meta['module_base'],0)==base and binding['phase']=='running' and
                int(binding['native_started_qpc'])==int(fields['started_qpc']),'CloseScene session mismatch')
        client=snapshot['roles']['client'];scene=snapshot['roles']['scene'];function=meta['function'];declaring=meta['owner_class']
        require(all(meta['client'][k]==client[k] for k in ('address','internal_index','name','number','cls','outer')),
                'CloseScene client identity mismatch')
        classes={row['identity']['address']:row['identity'] for row in snapshot['classes']}
        require(declaring['name']=='UISceneClient' and declaring['address'] in classes and
                all(declaring[k]==classes[declaring['address']][k] for k in ('internal_index','name','number','cls','outer')),
                'CloseScene defining class mismatch')
        owned=[row for row in meta['owned_field_chain'] if row['address']==function['address']]
        require(function['name']=='CloseScene' and function['number']==0 and function['outer']==declaring['address'] and
                len(owned)==1 and all(owned[0][k]==function[k] for k in ('internal_index','name','number','cls','outer')),
                'CloseScene function membership mismatch')
        raw=bytes.fromhex(meta['header_hex']);require(len(raw)==304,'CloseScene complete metadata header required')
        integer(function['internal_index'],0,(1<<32)-1);integer(function['number'],0,0)
        integer(function['cls'],1,owner.U64-112);integer(function['outer'],1,owner.U64-144)
        require(struct.unpack_from('<I',raw,8)[0]==function['internal_index'] and
                struct.unpack_from('<Q',raw,64)[0]==function['outer'] and
                struct.unpack_from('<I',raw,76)[0]==0 and struct.unpack_from('<Q',raw,80)[0]==function['cls'] and
                struct.unpack_from('<Q',raw,0x114)[0]==base+0x34698c,'CloseScene raw metadata mismatch')
        objects=snapshot['evidence']['registry_headers']['objects'];names=snapshot['evidence']['registry_headers']['names']
        name_index=struct.unpack_from('<I',raw,72)[0]
        require(0<=function['internal_index']<objects['count'] and 0<=name_index<names['count'],
                'CloseScene identity outside initial fixed prefix')
        active=snapshot['arrays']['active_scenes']
        props=[p for p in snapshot['properties'] if p['identity']['address']==active['property']]
        require(len(props)==1 and props[0]['identity']['name']=='ActiveScenes' and props[0]['owner']['name']=='GameUISceneClient',
                'ActiveScenes descriptor must derive from the authenticated property')
        header=client['address']+props[0]['offset']
        duration=integer(observer_duration_ms,100,8000)
        result={k:fields[k] for k in ('nonce','pid','creation_time','started_qpc','world_generation','base','client','scene','panel')}
        result=dict(schema=1,**result,duration_ms=duration,active_header=hex(header),active_data=hex(active['data']),
                    active_num=active['count'],active_max=active['capacity'],function=hex(integer(function['address'],1,owner.U64-0x11c)),
                    function_index=function['internal_index'],function_outer=hex(function['outer']),function_class=hex(function['cls']),
                    function_name_index=name_index,function_number=0)
        require(active['members'].count(scene['address'])==1 and len(result)==21,'Unexpected CloseScene descriptor')
        lines=['[PathChoiceCloseObserver]']+[key+'='+str(value) for key,value in result.items()]
        lines+=['member_%d=%s'%(i,hex(member)) for i,member in enumerate(active['members'])]
        text='\n'.join(lines)+'\n';require(len(text)<=32768,'CloseScene request size');text.encode('ascii');return text
    except (KeyError,TypeError,AttributeError,UnicodeError,struct.error,OverflowError) as error:
        raise ValueError('Malformed CloseScene metadata') from error
