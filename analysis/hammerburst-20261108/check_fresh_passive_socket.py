from pathlib import Path
import json,subprocess,re,hashlib
import prepare_fresh_passive_socket as p
A=p.A;PG=p.PG;N=p.N;S=N/'source/tools/native-capture';T=PG/'source/tools/playground'
jobs=[]
def job(label,source,include,extra='',arguments=''):
    target=N/(label+'.cpp');target.write_text(source)
    exe=N/(label+'.exe');cmd=N/(label+'.cmd')
    cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{include}" {extra} "{target}" /Fo"{N / (label+".obj")}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}" {arguments}\nexit /b %errorlevel%\n')
    with (N/(label+'.log')).open('wb') as log:result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    text=(N/(label+'.log')).read_text(errors='replace');print(text[-1800:]);assert result.returncode==0,(label,result.returncode)
    jobs.append(dict(label=label,exit_code=0,source_sha256=hashlib.sha256(source.encode()).hexdigest()))

# The actual sender already covers an expired historical Scene paired with a
# distinct fresh40ms acquisition; V1 still rejects that old tracking.
source=(T/'body_main_mount_sender_test.cpp').read_text()
needle='    std::printf("BodyMainMountSenderTest:';assert source.count(needle)==1
at=source.index(needle)
source=source[:at]+'''    reject([](auto& f){f.request.passive_socket_allowed=2;});
    reject([](auto& f){f.request.passive_reserved=1;});
    {BodyFixture f;f.request.passive_socket_allowed=1;bodywire::Response out{};
     CHECK(f.respond(out)==bodywire::Status::ready&&gears_fresh_support_socket::shape(out.passive_socket));}
'''+source[at:]
job('fresh-sender-test',source,T)

part=(S/'body_main_mount_broker_test.inl').read_text()
needle='    return unsigned(bm::Status::ready);';assert part.count(needle)==1
part=part.replace(needle,'''    if(body_mount_mode>=10){
        out->passive_socket={1,1,{{0,.45,0},{0,0,0,1}}};
        if(body_mount_mode==11)out->passive_socket.kind=2;
        if(body_mount_mode==13)out->passive_socket.relative.position.x=std::numeric_limits<double>::quiet_NaN();
        if(body_mount_mode==16)const_cast<bm::Request*>(q)->passive_socket_allowed^=1;
    }
'''+needle)
needle='    printf("body_mount V3 broker seam complete';assert part.count(needle)==1
at=part.index(needle)
part=part[:at]+'''    for(unsigned mode=10;mode<=17;++mode){
        Fixture f;f.attach();body_attach(f);auto x=mount_committed(f);
        if(mode==17){auto frame=f.frame(2);frame.weapon=x.rig.weapon;w::Proposal proposal{};
            C(f.broker.prepare(frame,proposal)==w::Reply::accepted);
            C(f.broker.commit(frame,proposal,f.decision(frame,proposal))==w::Reply::accepted);x=mount_input(frame);}
        auto s=body_source(f,x);auto& hands=s.tracking.controllers.hands;
        for(auto& hand:hands){hand.grip_active=1;hand.grip_flags=15;hand.squeeze_active=hand.trigger_active=1;}
        hands[1].grip={{0,0,0},{}};hands[0].grip={{.45,0,0},{}};
        if(mode==12)hands[0].squeeze=.8f;
        if(mode==14)hands[0].grip.position.x=5;
        if(mode==15)s.tracking.controllers.sync_ns=f.fake.stamp.load()-40000001;
        body_mount_mode=mode;bm::Response out{};const auto result=f.broker.query_body_mount(s,out);
        if(mode==10)C(result.code==MC::ready&&out.passive_socket.selected==1);
        else C(result.code!=MC::ready&&body_empty(out));
        C(!f.broker.pending_callbacks()&&!f.broker.pending_transactions());body_mount_mode=0;f.finish();
    }
'''+part[at:]
(S/'body_main_mount_fresh_test.inl').write_text(part)
source=(S/'weapon_hand_broker_test.cpp').read_text().replace('#include "body_main_mount_broker_test.inl"','#include "body_main_mount_fresh_test.inl"')
assert 'body_main_mount_fresh_test.inl' in source
job('fresh-broker-test',source,S)

source=(S/'body_c0_hands_test.cpp').read_text()
needle='            C(bytes(actual)==input_before&&bytes(work->eye_targets[0].grip_world)==bytes(moved_left));';assert source.count(needle)==1
source=source.replace(needle,needle+r'''
            // Fixed socket geometry is reused; every WORLD goal is assembled
            // from this fresh right grip, not an expired prior world request.
            for(unsigned step=1;step<=3;++step){
                auto fresh_right=right;fresh_right.position.x+=step*.1;
                fresh_right.orientation=mul(turn(2,step*.001),right.orientation);
                v::Pose wanted{};C(gears_fresh_support_socket::world(fresh_right,geometry,frame->units_per_metre,wanted));
                auto const& lc=work->bindings[0].authored_mount.calibration;
                auto const& rc=work->bindings[1].authored_mount.calibration;
                const v::Pose expected_left{add(wanted.position,rotate(wanted.orientation,scale(lc.grip_to_wrist_metres,frame->units_per_metre))),mul(wanted.orientation,lc.grip_to_wrist_rotation)};
                const v::Pose expected_right{add(fresh_right.position,rotate(fresh_right.orientation,scale(rc.grip_to_wrist_metres,frame->units_per_metre))),mul(fresh_right.orientation,rc.grip_to_wrist_rotation)};
                work->left_visual={gears_hand_object_grasp::Kind::support,geometry,1,true,true};
                work->left_blend.kind=gears_hand_object_grasp::Kind::support;work->left_blend.weight=1;
                adjusted=body_only;
                const auto fresh_prepare=[&](auto const&,unsigned side){
                    work->computed_wrist[side]=side?expected_right:physical_targets[0];
                    work->eye_targets[side].grip_world=side?fresh_right:moved_left;
                    return a::adapter::Status::okay;
                };
                C(a::compose_common(*model,*frame,actual,asset,base,now+step*10000000ll,sim,*work,adjusted,
                    a::adapter::MountKind::held_lancer,fresh_prepare,true).status==a::Status::okay);
                const auto seen=fk(*model,*frame,adjusted);
                C(work->left_visual_applied&&work->right_blend.weight==0);
                C(distance(seen[11].position,expected_left.position)<.002&&qerror(seen[11].orientation,expected_left.orientation)<1e-5);
                C(distance(seen[35].position,expected_right.position)<.002&&qerror(seen[35].orientation,expected_right.orientation)<1e-5);
                C(bytes(actual)==input_before);
                for(unsigned i=0;i<b::bones;++i)C(!std::memcmp(adjusted.local[i].translation,body_only.local[i].translation,12)&&adjusted.local[i].scale==body_only.local[i].scale);
            }
''')
fixture=A.parent/'solo-m1-20261028/observed-backend-fixture'
job('fresh-C0-test',source,S,'/DGEARS_C0_CONTEXT_GEOMETRY_ONLY bcrypt.lib',f'"{fixture}"')
proof=dict(passed=True,jobs=jobs,actual_sender=True,actual_broker=True,actual_C0=True,independent_FK=True,synthetic_input_seam=True,original_age_limits_preserved=True,old_world_request_not_extended=True,game_started=False)
(N/'fresh-socket-test-result.json').write_text(json.dumps(proof,indent=2));print(json.dumps(proof))
