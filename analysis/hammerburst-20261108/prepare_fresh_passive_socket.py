from pathlib import Path
import json,hashlib,shutil,difflib,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1]
PGBASE=A/'early-released-refire-r1';NBASE=A/'native-passive-rejection-r1'
PG=A/'fresh-passive-pg-r1';N=A/'fresh-passive-native-r1'
def main():
    assert not PG.exists() and not N.exists(),'Preserve candidate identities'
    for base,p in ((PGBASE,PG),(NBASE,N)):
        shutil.copytree(base/'source/tools',p/'source/tools')
        if (base/'dependency').exists():shutil.copytree(base/'dependency',p/'dependency')
    patch=''
    def edit(p,rel,fn):
        nonlocal patch
        path=p/'source'/rel;before=path.read_text();after=fn(before);assert before!=after,rel
        path.write_text(after);patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile=str(p.name)+'/a/'+rel,tofile=str(p.name)+'/b/'+rel))
    for p in (PG,N):
        shutil.copy2(A/'fresh_support_socket.h',p/'source/tools/vr-common/fresh_support_socket.h')
        def wire(s):
            s=s.replace('#include "weapon_main_mount_provider.h"','#include "weapon_main_mount_provider.h"\n#include "fresh_support_socket.h"')
            s=s.replace('version=5;', 'version=6;')
            needle='    Source source{};\n};';assert s.count(needle)==1
            s=s.replace(needle,'    std::uint32_t passive_socket_allowed{},passive_reserved{}; // Query-local neutral visual eligibility only.\n'+needle)
            needle='    mount::Response mount{};';assert s.count(needle)==1
            s=s.replace(needle,needle+'\n    gears_fresh_support_socket::Description passive_socket{};')
            s=s.replace('sizeof(Request)==4056','sizeof(Request)==4064').replace('sizeof(Response)==736','sizeof(Response)==800').replace('offsetof(Request,source)==56','offsetof(Request,source)==64')
            return s
        edit(p,'tools/vr-common/body_main_mount_provider.h',wire)
        edit(p,'tools/vr-common/hand_object_grasp.h',lambda s:s.replace('bool passive_socket{};};','bool passive_socket{};bool relative_socket{};};'))
    def geometry(s):
        start=s.index('bool support_geometry(hand::Frame const& f,');end=s.index('bool fill_support_mount(',start)
        old=s[start:end]
        new=old.replace('bool support_geometry(hand::Frame const& f,gears_vr::Pose& out)',
            'bool support_geometry_context(hand::ObjectIdentity const& weapon,std::uint64_t hold,\n    std::uint64_t session,std::uint64_t space,std::uint64_t input,std::uint64_t reference,std::uint64_t world,gears_vr::Pose& out)')
        for a,b in [('f.weapon','weapon'),('vrequip::model.hold_generation()','hold'),('f.stereo.session','session'),('f.stereo.space_epoch','space'),('f.controllers.input_epoch','input'),('f.reference_generation','reference'),('f.key.world_generation','world')]:new=new.replace(a,b)
        new+='''bool support_geometry(hand::Frame const& f,gears_vr::Pose& out) noexcept {
    return support_geometry_context(f.weapon,vrequip::model.hold_generation(),f.stereo.session,f.stereo.space_epoch,
        f.controllers.input_epoch,f.reference_generation,f.key.world_generation,out);
}
'''
        return s[:start]+new+s[end:]
    edit(PG,'tools/playground/weapon_native_lancer_follow.inl',geometry)
    def sender(s):
        needle='r.maximum_tracking_age_ns!=wire::tracking_age_ns';assert s.count(needle)==1
        return s.replace(needle,'r.passive_socket_allowed>1||r.passive_reserved||\n       '+needle)
    edit(PG,'tools/playground/body_main_mount_sender.h',sender)
    def provider(s):
        needle='    status=sender::response(r,parent,scratch.identity,scratch.observation,weaponhand::clock_ns(nullptr),scratch.response);';assert s.count(needle)==1
        return s.replace(needle,needle+'''
    // Existing fresh query already authenticates actual held asset/attachment.
    // Reuse only fixed mount geometry; no old Hand Frame/world pose is renewed.
    if(status==wire::Status::ready&&nativelancerfollow::bound_is_hammerburst&&
       nativelancerfollow::bound_key_known&&hand::same(nativelancerfollow::bound_key.weapon,scratch.identity.weapon)){
        auto& socket=scratch.response.passive_socket;socket.kind=1;
        auto const& tracking=r.source.tracking;
        if(r.passive_socket_allowed&&!hammerburst_runtime::blocks_fire(scratch.identity.weapon)&&
           hand::same(nativelancerfollow::bound_key.component,scratch.observation.component)&&
           hand::same(nativelancerfollow::bound_key.asset,scratch.observation.asset)&&
           nativelancerfollow::support_geometry_context(scratch.identity.weapon,scratch.observation.hold_generation,
               tracking.stereo.session,tracking.stereo.space_epoch,tracking.controllers.input_epoch,
               scratch.identity.reference_generation,scratch.identity.key.world_generation,socket.relative)&&
           gears_fresh_support_socket::contact(tracking.controllers,socket.relative))socket.selected=1;
    }
''')
    edit(PG,'tools/playground/body_main_mount_provider.inl',provider)
    def broker(s):
        needle='snapshot(w.request.source,source);snapshot(w.shadow,w.request);snapshot(w.committed,frozen_);w.response={};'
        assert s.count(needle)==1
        s=s.replace(needle,'''snapshot(w.request.source,source);
                    w.request.passive_socket_allowed=committed_.lease.kind==LeaseKind::none&&!committed_.release_required&&
                        gears_fresh_support_socket::neutral(source.tracking.controllers);
                    snapshot(w.shadow,w.request);snapshot(w.committed,frozen_);w.response={};''')
        needle='if(outer.query_id!=q.query_id||outer.hand_lifecycle!=q.hand_lifecycle||outer.action_generation!=q.action_generation||';assert s.count(needle)==1
        return s.replace(needle,'''if(!gears_fresh_support_socket::shape(outer.passive_socket)||
           (outer.passive_socket.selected&&(!q.passive_socket_allowed||
            !gears_fresh_support_socket::contact(q.source.tracking.controllers,outer.passive_socket.relative)))||
           outer.query_id!=q.query_id||outer.hand_lifecycle!=q.hand_lifecycle||outer.action_generation!=q.action_generation||''')
    edit(N,'tools/native-capture/weapon_hand_broker.h',broker)
    def native(s):
        needle='                lancer_grasp_live::target(source,current_input.controllers,state.hand_work.left_visual);';assert s.count(needle)==1
        return s.replace(needle,needle+'''
                auto const& socket=state.body_mount_response.passive_socket;
                if(body_tick_source_live::enabled&&socket.kind==1){
                    // Fresh query-local geometry, not the old world request.
                    if(state.hand_work.left_visual.passive_socket)state.hand_work.left_visual={};
                    if(socket.selected&&gears_fresh_support_socket::shape(socket)&&
                       gears_fresh_support_socket::contact(current_input.controllers,socket.relative))
                        state.hand_work.left_visual={gears_hand_object_grasp::Kind::support,socket.relative,1.,true,true};
                }
''')
    edit(N,'tools/native-capture/full_body_c0_native_live.inl',native)
    def provenance(s):
        s=s.replace('unsigned applied{},fitted{},solved_mask{},kind{};double weight{};','unsigned applied{},fitted{},solved_mask{},kind{};double weight{};uint64_t fresh_query{};')
        needle='    row.grip=w.left_visual.grip;row.physical=w.eye_targets[0].grip_world;';assert s.count(needle)==1
        s=s.replace(needle,needle+'''
    if(w.left_visual.relative_socket){
        row.fresh_query=state.body_mount_response.query_id;
        row.frame=state.body_mount_response.mount.key.frame; // Original metadata Scene, not a new Hand Frame.
        row.request_sequence=current.controllers.sequence;row.request_sync_ns=current.controllers.sync_ns;
        gears_fresh_support_socket::world(w.eye_targets[1].grip_world,w.left_visual.grip,state.source.units_per_metre,row.grip);
    }
''')
        needle='        const gears_vr::Pose poses[]={r.grip,r.physical,r.adjusted,r.solved,r.right};';assert s.count(needle)==1
        return s.replace(needle,'        fprintf(f,"body_passive_commit_%u_fresh_query=%llu ",i,r.fresh_query);\n'+needle)
    edit(N,'tools/native-capture/full_body_c0_native_live.inl',provenance)
    def hands(s):
        s=s.replace('#include "../vr-common/hand_object_grasp.h"','#include "../vr-common/hand_object_grasp.h"\n#include "../vr-common/fresh_support_socket.h"')
        needle='    gears_hand_object_grasp::Target visual_goals[2]{work.left_visual,{}};';assert s.count(needle)==1
        return s.replace(needle,needle+'''
    if(work.left_visual.relative_socket){
        visual_goals[0]={};gears_vr::Pose socket{};
        if(work.left_visual.passive_socket&&work.left_visual.kind==gears_hand_object_grasp::Kind::support&&
           independent&&side_prepared[0]&&side_prepared[1]&&right_mount==adapter::MountKind::held_lancer&&
           gears_fresh_support_socket::world(work.eye_targets[1].grip_world,work.left_visual.grip,frame.units_per_metre,socket))
            visual_goals[0]={gears_hand_object_grasp::Kind::support,socket,work.left_visual.weight,true};
    }
''')
    edit(N,'tools/native-capture/body_c0_hands.h',hands)
    # Sender and broker fixture layouts change explicitly with this private ABI.
    edit(PG,'tools/playground/body_main_mount_sender_test.cpp',lambda s:s.replace('bodywire::version==5','bodywire::version==6').replace('out.abi_version==5','out.abi_version==bodywire::version').replace('version:{1u,2u,3u,4u}','version:{1u,2u,3u,4u,5u}'))
    edit(N,'tools/native-capture/body_main_mount_broker_test.inl',lambda s:s.replace('bm::version==5','bm::version==6'))
    (R/'docs/hammerburst-fresh-passive-socket.patch').write_text(patch)
    pp=json.loads((PGBASE/'prepared.json').read_text());configure=list(pp['configure_command']);build=list(pp['build_command'])
    configure[configure.index('-S')+1]=str(PG/'source/tools/playground');configure[configure.index('-B')+1]=str(PG/'build');build[build.index('--build')+1]=str(PG/'build')
    np=json.loads((NBASE/'prepared.json').read_text());project=N/'project';project.mkdir();(project/'CMakeLists.txt').write_text((NBASE/'project/CMakeLists.txt').read_text().replace(str(NBASE).replace('\\','/'),str(N).replace('\\','/')))
    nc=[list(c) for c in np['commands']];nc[0][nc[0].index('-S')+1]=str(project);nc[0][nc[0].index('-B')+1]=str(N/'build');nc[1][nc[1].index('--build')+1]=str(N/'build')
    (PG/'prebuild.json').write_text(json.dumps(dict(parent=pp,configure_command=configure,build_command=build),indent=2));(N/'prebuild.json').write_text(json.dumps(dict(parent=np,commands=nc),indent=2))
    for p,commands in ((PG,[configure,build]),(N,nc)):
        for label,cmd in zip(('configure','build'),commands):
            with (p/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    pgdll=PG/'build/Release/GearsPlayground6DofUnified3.dll';ndll=N/'build/Release/GearsCapture6DofM1.dll'
    scope='Fresh query-local HB passive socket geometry through existing BodyMount6 and sole C0 solve, current neutral/contact/no incumbent and original40/50/50ms checks; old100ms world request remains rejected; no new tracking/Hand Frame/action/writer.'
    (PG/'prepared.json').write_text(json.dumps(dict(base=str(PGBASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(pgdll),dll_sha256=hashlib.sha256(pgdll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=True,xr_rebuilt=False,scope=scope),indent=2))
    (N/'prepared.json').write_text(json.dumps(dict(parent_native=np['native_sha256'],frozen_native=np['frozen_native'],native_sha256=hashlib.sha256(ndll.read_bytes()).hexdigest(),dll=str(ndll),full_native_compiled=True,commands=nc,original_compile_definitions=np['original_compile_definitions'],runtime_exercised=False,gameplay_accepted=False,root_native_modified=False,xr_changed=False,scope=scope),indent=2))
    print('Both private modules compiled')
if __name__=='__main__':main()
