// Included only by CoverBodyJournalTest. Uses its real protected wrapper and
// private allocation. No game, process injection, retail pose or side claim.
uint32_t mirror_word(){uint32_t value{};memcpy(&value,reinterpret_cast<void*>(records+5*0x44+0x3c),4);return value;}
void mirror_word(uint32_t value){put(records+5*0x44+0x3c,value);}
void mirror_tags(uint32_t node_tag,uint32_t mesh_tag){put(node+0xbc,node_tag);put(mesh+0x338,mesh_tag);}
void prepare_mirror_trial(uint32_t original=0xa0){
 prepare_round(0,b::PoseProfile::side);b::side_mirror_trial=true;b::layout.mirror_word=0x3c;b::layout.mirror_mask=1;
 mirror_word(original);mirror_tags(9,10);
}
void prime_mirror_trial(uint32_t original=0xa0){
 prepare_mirror_trial(original);invoke_round();balanced();auto const& m=b::journals[0].mirror;
 C(m.captured&&m.original_word==original&&m.mask==1&&m.native_bits==(original&1)&&m.installed_bits==((original&1)^1));
 C(m.initial_cache_seen&&m.initial_tags.node==9&&m.initial_tags.mesh==10);
 C(m.neutral_pending&&!m.neutral_verified&&!m.started&&m.neutral_epoch==10&&!m.owned&&!b::pending_count()&&!b::writes);
 // This explicitly simulates a completed neutral GetBoneAtoms evaluation and
 // a subsequent mesh epoch. A Tick alone never grants the fresh-zero proof.
 mirror_tags(10,11);b::step=2;
}
void mirror_first_observer(){C(weight(5)==.5f&&mirror_word()==0xa1&&b::pending.load()==3&&b::journals[0].round_open);mirror_word(0xb1);}
void mirror_post_observer(){C(weight(5)==0&&mirror_word()==0xb0&&b::pending.load()==3&&b::journals[0].mirror.native_visible);}
void mirror_conflict_first(){C(mirror_word()==0xa1);mirror_word(0xa0);}
void mirror_conflict_post(){C(mirror_word()==0xa0);mirror_word(0xa1);}
void mirror_external_side(){put(records+5*0x44+0x10,.33f);}
void mirror_epoch_during_original(){mirror_tags(10,12);}
void mirror_cache_during_original(){mirror_tags(11,11);}
void mirror_read_failure_during_original(){denied=mesh+0x338;}
void mirror_live_expiry(){live_time.qpc+=101;}
void mirror_positive_identity_loss(){
 gears_anchor::Registry registry{};memcpy(&registry,reinterpret_cast<void*>(b::metadata.objects_header),sizeof registry);
 put(registry.data+uintptr_t(b::journals[0].id.index)*8,uintptr_t(0));
}
void mirror_stop_test(unsigned phase_value,bool unknown_storage){
 prime_mirror_trial();barrier_phase=phase_value;barrier_unknown_header=unknown_storage;barrier_timeout=unknown_storage;
 phase_entered=CreateEventW(nullptr,FALSE,FALSE,nullptr);phase_release=CreateEventW(nullptr,FALSE,FALSE,nullptr);C(phase_entered&&phase_release);
 auto worker=CreateThread(nullptr,0,stop_at_phase,nullptr,0,nullptr);C(worker!=nullptr);invoke_round();C(WaitForSingleObject(worker,2000)==WAIT_OBJECT_0);
 C(barrier_wait_ok&&barrier_fields==3&&barrier_busy>3&&barrier_draining&&barrier_gate_blocked);
 C(CloseHandle(worker)!=0);C(CloseHandle(phase_entered)!=0);C(CloseHandle(phase_release)!=0);phase_entered=phase_release=nullptr;barrier_phase=0;
 balanced();C(b::stopping.load()&&b::journals[0].mirror.completed);
 if(unknown_storage){
  C(barrier_failed&&b::drain_failed()&&b::pending_count()==3&&b::journals[0].mirror.owned&&b::journals[0].mirror.native_visible);
  C(weight(0)==1&&weight(5)==0&&mirror_word()==0xa0);
  put(node+0xe4,b::journals[0].children);before_blend(other_node);
 }
 C(!b::pending_count()&&weight(0)==1&&weight(5)==0&&mirror_word()==0xa0);
}
void mirror_reflection_object(uintptr_t address,unsigned index,uintptr_t cls,uintptr_t outer,const wchar_t* label){
 gears_anchor::Registry objects{},names{};memcpy(&objects,reinterpret_cast<void*>(b::metadata.objects_header),sizeof objects);memcpy(&names,reinterpret_cast<void*>(b::metadata.names_header),sizeof names);
 put(address+8,index);put(address+72,index);put(address+76,0u);put(address+80,cls);put(address+64,outer);put(objects.data+uintptr_t(index)*8,address);
 auto entry=arena+0x20000+uintptr_t(index)*0x200;put(names.data+uintptr_t(index)*8,entry);put(entry,index);std::array<wchar_t,64> text{};std::copy_n(label,wcslen(label)+1,text.begin());put(entry+20,text);
}
void mirror_reflection_tests(){
 setup();b::layout={};b::pose_profile=b::PoseProfile::side;
 const auto class_class=arena+0x60000,struct_class=arena+0x60400,bool_class=arena+0x60800,float_class=arena+0x60c00,structure=arena+0x61000,prop=arena+0x61400;
 mirror_reflection_object(class_class,40,class_class,0,L"Class");mirror_reflection_object(struct_class,41,class_class,0,L"ScriptStruct");
 mirror_reflection_object(bool_class,42,class_class,0,L"BoolProperty");mirror_reflection_object(float_class,43,class_class,0,L"FloatProperty");
 mirror_reflection_object(structure,44,struct_class,0,L"AnimBlendChild");mirror_reflection_object(prop,45,bool_class,structure,L"bMirrorSkeleton");
 put(structure+0x80,prop);put(prop+0x70,1u);put(prop+0x74,4u);put(prop+0x94,0x3cu);put(prop+0xd0,1u);
 const auto old_base=base;base=arena;
 // Independent fixture bytes from the saved F7940 receipt. The production
 // helper validates both the reflected property and these consumer slices.
 const unsigned char cache_bytes[]={0x4c,0x8b,0x41,0x60,0x41,0x8b,0x80,0x38,0x03,0x00,0x00,0x39,0x81,0xbc,0x00,0x00,0x00,0x0f,0x84,0xf5,0x05,0x00,0x00};
 const unsigned char single[]={0x41,0xf6,0x44,0x11,0x3c,0x01},multiple[]={0x42,0xf6,0x44,0x1a,0x3c,0x01};
 memcpy(reinterpret_cast<void*>(base+0xf79a8),cache_bytes,sizeof cache_bytes);memcpy(reinterpret_cast<void*>(base+0xf7a47),single,sizeof single);memcpy(reinterpret_cast<void*>(base+0xf7bbe),multiple,sizeof multiple);
 C(b::prepare_mirror_layout(structure,0x44)&&b::layout.mirror_word==0x3c&&b::layout.mirror_mask==1&&b::layout.source_count>=35&&b::metadata_current());
 put(base+0xf79a8,static_cast<unsigned char>(0x90));C(!b::metadata_current());b::layout={};C(!b::prepare_mirror_layout(structure,0x44)&&!wcscmp(b::layout_failure,L"mirror_consumer_bytes"));put(base+0xf79a8,cache_bytes[0]);
 put(prop+80,float_class);b::layout={};C(!b::prepare_mirror_layout(structure,0x44));put(prop+80,bool_class);
 for(unsigned mask:{0u,2u,3u,0x80000000u}){put(prop+0xd0,mask);b::layout={};C(!b::prepare_mirror_layout(structure,0x44));}put(prop+0xd0,1u);
 for(unsigned offset:{0x38u,0x3du,0x44u}){put(prop+0x94,offset);b::layout={};C(!b::prepare_mirror_layout(structure,0x44));}put(prop+0x94,0x3cu);
 put(prop+0x74,1u);b::layout={};C(!b::prepare_mirror_layout(structure,0x44));put(prop+0x74,4u);
 put(prop+0x70,2u);b::layout={};C(!b::prepare_mirror_layout(structure,0x44));put(prop+0x70,1u);
 b::layout={};C(!b::prepare_mirror_layout(structure,0x3f));b::layout={};b::pose_profile=b::PoseProfile::top;C(!b::prepare_mirror_layout(structure,0x44));base=old_base;
}
void mirror_configuration_tests(){
 setup();b::restore_all();anchor_config=b::metadata;b::reserved=true;
 wchar_t directory[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,directory)>0);C(GetTempFileNameW(directory,L"gbm",0,ini)!=0);FILE* log{};C(tmpfile_s(&log)==0&&log);
 auto value=[&](const wchar_t* key,const wchar_t* text){C(WritePrivateProfileStringW(L"CoverBodyPrototype",key,text,ini)!=0);};
 value(L"enabled",L"1");value(L"pose_profile",L"side");bool parsed=true;C(b::read_mirror_trial(ini,parsed)&&!parsed);
 for(const wchar_t* text:{L"",L"2",L"-1",L"true",L"01",L"10",L"1extra"}){value(L"side_mirror_trial",text);C(!b::configure(ini,log)&&!b::enabled&&!b::advance_event&&!b::reset_event);}
 value(L"side_mirror_trial",L"1");value(L"pose_profile",L"top");C(!b::configure(ini,log)&&!b::enabled);
 value(L"pose_profile",L"side");C(b::configure(ini,log)&&b::side_mirror_trial&&b::pose_profile==b::PoseProfile::side);b::print(log,true);b::enabled=false;
 value(L"side_mirror_trial",L"0");value(L"pose_profile",L"top");C(b::configure(ini,log)&&!b::side_mirror_trial&&b::pose_profile==b::PoseProfile::top);b::print(log,true);b::enabled=false;
 C(fflush(log)==0&&fseek(log,0,SEEK_SET)==0);std::array<char,4096> diagnostic{};C(fread(diagnostic.data(),1,diagnostic.size()-1,log)>0);
 C(strstr(diagnostic.data(),"mirror_written=0 mirror_write_opt_in=1 side_mirror_trial=1")&&strstr(diagnostic.data(),"mirror_native_switch_certified=0 mirror_pose_concurrency_verified=0"));
 C(fclose(log)==0);C(DeleteFileW(ini)!=0);b::reserved=false;
}
void side_mirror_trial_tests(){
 // A real compare-exchange retry with a neighbour changed after the read.
 // A race changing the selected bit instead is a conflict, never a retry.
 prepare_mirror_trial();mask_race_read=records+5*0x44+0x3c;mask_race_xor=0x10;mask_races=1;
 C(b::exchange_mask(mask_race_read,1,0,1)==b::MaskExchange::changed&&mirror_word()==0xb1&&!mask_races);
 mirror_word(0xa0);mask_race_xor=1;mask_races=1;C(b::exchange_mask(mask_race_read,1,0,1)==b::MaskExchange::conflict&&mirror_word()==0xa1&&!mask_races);
 // No neutral pose/cache proof means no opt-in writes, even on a valid side
 // receiver. Missing tag reads, unchanged epochs and cache hits stay negative.
 prepare_mirror_trial();b::step=2;invoke_round();balanced();C(!b::writes&&!b::pending_count()&&!b::journals[0].mirror.started&&b::journals[0].mirror.reason==b::MirrorReason::await_neutral);
 prepare_mirror_trial();denied=mesh+0x338;invoke_round();balanced();C(!b::writes&&!b::journals[0].mirror.cache_read_ok&&b::journals[0].mirror.reason==b::MirrorReason::cache_unreadable);denied=0;
 prepare_mirror_trial();mirror_tags(10,10);invoke_round();balanced();C(!b::writes&&!b::journals[0].mirror.neutral_pending&&b::journals[0].mirror.reason==b::MirrorReason::already_cached);
 prime_mirror_trial();mirror_tags(9,10);invoke_round();balanced();C(!b::writes&&!b::journals[0].mirror.started&&b::journals[0].mirror.reason==b::MirrorReason::await_fresh_epoch);
 prime_mirror_trial();mirror_tags(9,11);invoke_round();balanced();C(!b::writes&&!b::journals[0].mirror.neutral_verified);
 prime_mirror_trial();b::step=0;invoke_round();balanced();C(b::journals[0].mirror.neutral_epoch==11&&!b::journals[0].mirror.neutral_verified);
 mirror_tags(10,12);b::step=2;invoke_round();balanced();C(!b::writes&&!b::journals[0].mirror.started&&b::journals[0].mirror.reason==b::MirrorReason::await_fresh_epoch);
 prime_mirror_trial();put(records+5*0x44+0x10,.5f);put(records+0x10,.5f);invoke_round();balanced();C(!b::writes&&mirror_word()==0xa0&&b::journals[0].mirror.reason==b::MirrorReason::native_side_nonzero);
 prime_mirror_trial();mirror_word(0xa1);invoke_round();balanced();C(!b::writes&&b::journals[0].mirror.reason==b::MirrorReason::baseline_changed);
 prime_mirror_trial();put(records+5*0x44+8,arena+0x3c800);invoke_round();balanced();C(!b::writes&&b::journals[0].mirror.reason==b::MirrorReason::child_changed);

 // A full round sees one frozen local bit for both propagations. Native
 // post sees the original bit and weights, with all three debts retained.
 prime_mirror_trial();round_after_first=mirror_first_observer;round_after_post=mirror_post_observer;
 std::array<unsigned char,0x400> pawn_before{},mesh_before{};memcpy(pawn_before.data(),reinterpret_cast<void*>(pawn),pawn_before.size());memcpy(mesh_before.data(),reinterpret_cast<void*>(mesh),mesh_before.size());
 invoke_round();balanced();C(mirror_word()==0xb1&&weight(5)==.5f&&weight(0)==.5f&&b::pending_count()==3);
 C(!memcmp(pawn_before.data(),reinterpret_cast<void*>(pawn),pawn_before.size())&&!memcmp(mesh_before.data(),reinterpret_cast<void*>(mesh),mesh_before.size()));
 auto const& m=b::journals[0].mirror;C(m.started&&!m.completed&&m.neutral_verified&&m.activation_epoch==11&&m.pose_epoch==11&&m.installs==2&&m.restores==1&&!m.native_visible);
 C(b::writes==6&&b::restores==3&&native_test_weight==1&&!native_test_transition&&total_first==.4f&&total_last==.4f);
 b::request_drain();before_blend(other_node);C(!b::pending_count()&&mirror_word()==0xb0&&weight(5)==0&&b::restores==6);
 // Toggling a native one to zero is supported; neither sign is named left.
 prime_mirror_trial(0xa1);invoke_round();balanced();C(mirror_word()==0xa0&&b::pending_count()==3);b::request_drain();before_blend(other_node);C(mirror_word()==0xa1&&!b::pending_count());

 // One direction is fixed for the lifetime. Same-epoch retargeting is refused
 // and a completed zero ends the trial; it cannot silently start again.
 prime_mirror_trial();invoke_round();balanced();const auto installs=b::journals[0].mirror.installs;b::step=3;invoke_round();balanced();
 C(b::journals[0].mirror.reason==b::MirrorReason::repeated_epoch&&b::journals[0].mirror.installs==installs&&!b::pending_count()&&mirror_word()==0xa0);
 prime_mirror_trial();invoke_round();mirror_tags(11,12);b::step=0;invoke_round();balanced();C(b::journals[0].mirror.completed&&!b::pending_count()&&mirror_word()==0xa0);
 mirror_tags(12,13);b::step=2;invoke_round();balanced();C(b::journals[0].mirror.reason==b::MirrorReason::completed&&!b::pending_count()&&mirror_word()==0xa0);
 prime_mirror_trial();invoke_round();b::target_source=b::TargetSource::live;before_blend(node);C(!b::pending_count()&&mirror_word()==0xa0&&b::journals[0].mirror.completed);

 // Cache changes/hits/read failures during the original prevent rearm and
 // still restore the exact owned fields. No epoch write is used to repair it.
 for(auto callback:{mirror_epoch_during_original,mirror_cache_during_original,mirror_read_failure_during_original}){
  prime_mirror_trial();round_after_post=callback;invoke_round();balanced();C(b::failed.load()&&b::stopping.load()&&!b::pending_count()&&mirror_word()==0xa0&&weight(5)==0);denied=0;
 }
 for(auto exceptional:{Mode::seh_after_first,Mode::seh_after_last}){prime_mirror_trial();mode=exceptional;C(catch_round());balanced();unlocked();C(b::failed.load()&&b::stopping.load()&&!b::pending_count()&&mirror_word()==0xa0&&weight(5)==0);}
 prime_mirror_trial();mode=Mode::own_post;invoke_round();balanced();unlocked();C(b::failed.load()&&b::pending_count()==3&&b::journals[0].mirror.native_visible&&mirror_word()==0xa0);before_blend(other_node);C(!b::pending_count());
 prime_mirror_trial();mode=Mode::expiry;invoke_round();balanced();C(b::timed_out.load()&&!b::pending_count()&&mirror_word()==0xa0&&b::journals[0].mirror.completed);
 prime_mirror_trial();mode=Mode::reset;b::reset_event=CreateEventW(nullptr,FALSE,FALSE,nullptr);C(b::reset_event!=nullptr);invoke_round();balanced();C(!b::pending_count()&&mirror_word()==0xa0&&b::journals[0].mirror.completed);C(CloseHandle(b::reset_event)!=0);b::reset_event=nullptr;

 // Packed bool neighbours survive; a conflict of the owned bit itself is
 // sticky and cannot make cleanup claim success merely because W is native.
 for(auto callback:{mirror_conflict_first,mirror_conflict_post}){
  prime_mirror_trial();round_after_first=callback==mirror_conflict_first?callback:nullptr;round_after_post=callback==mirror_conflict_post?callback:nullptr;invoke_round();balanced();
  C(b::stopping.load()&&b::journals[0].mirror.conflict&&b::journals[0].mirror.owned&&b::pending_count()==1&&weight(5)==0);
  b::request_drain();b::drain_tick=GetTickCount64()-5001;before_blend(other_node);C(b::drain_failed()&&b::pending_count()==1);
  mirror_positive_identity_loss();before_blend(other_node);C(!b::pending_count()&&b::retired==1);
 }
 prime_mirror_trial();round_after_first=mirror_external_side;invoke_round();balanced();C(b::pending_count()==1&&mirror_word()==0xa1&&weight(5)==.33f&&b::stopping.load());
 put(records+5*0x44+0x10,0.f);before_blend(other_node);C(!b::pending_count()&&mirror_word()==0xa0);
 // A failed mirror read after successful Weight restoration remains owed.
 prime_mirror_trial();invoke_round();denied=records+5*0x44+0x3c;b::request_drain();before_blend(other_node);C(b::pending_count()==1&&mirror_word()==0xa1&&weight(5)==0);
 denied=0;before_blend(other_node);C(!b::pending_count()&&mirror_word()==0xa0);
 prime_mirror_trial();invoke_round();DWORD old_protection{},ignored{};const auto page=records&~uintptr_t(4095);
 C(VirtualProtect(reinterpret_cast<void*>(page),4096,PAGE_READONLY,&old_protection)!=0);b::request_drain();before_blend(other_node);C(b::pending_count()==3&&mirror_word()==0xa1);
 C(VirtualProtect(reinterpret_cast<void*>(page),4096,old_protection,&ignored)!=0);before_blend(other_node);C(!b::pending_count()&&mirror_word()==0xa0);

 // Stop includes both open propagation and the native-visible gap. A world
 // generation change retains the mirror alongside the two Weight fields.
 mirror_stop_test(1,false);mirror_stop_test(2,false);mirror_stop_test(2,true);
 prime_mirror_trial();invoke_round();committed_world_generation=2;b::request_drain();before_blend(other_node);C(b::pending_count()==3&&mirror_word()==0xa1&&!b::retired);
 mirror_positive_identity_loss();before_blend(other_node);C(!b::pending_count()&&b::retired==3);
 // Live TTL expiry in the original cannot rearm either field. The exact
 // same production transaction is used; no second timer or input owner.
 prepare_live(0,b::PoseProfile::side);b::side_mirror_trial=true;b::layout.mirror_word=0x3c;b::layout.mirror_mask=1;mirror_word(0xa0);mirror_tags(9,10);invoke_round();balanced();
 mirror_tags(10,11);++live_time.qpc;C(b::publish_target(target(.5f,2)));round_after_post=mirror_live_expiry;invoke_round();balanced();
 C(b::live_expired.load()>0&&!b::pending_count()&&mirror_word()==0xa0&&b::journals[0].mirror.completed&&weight(5)==0);
 mirror_reflection_tests();mirror_configuration_tests();
}
