from pathlib import Path
R=Path(__file__).resolve().parents[3]
p=R/'tools/native-capture/cover_physx_shape_owner.h';t=p.read_text()
start=t.index('// The overlap enumerator');end=t.index('\n}\n',start)+2
block=t[start:end];t=t[:start]+t[end:];last=t.rfind('\n}')
t=t[:last]+'\n'+block+t[last:];p.write_text(t)
p=R/'tools/native-capture/cover_physx_query.h';t=p.read_text()
start=t.index('    template<class Shape,class Read,class Guard,class Output> Code sweep_all(')
end=t.index('\n};\nusing Queries=',start);old=t[start:end];body=old
body=body.replace('template<class Shape,class Read,class Guard,class Output> Code sweep_all(', 'template<bool Enumerate,class Shape,class Read,class Guard,class Output> Code collect_all(',1)
body=body.replace('        static_assert(std::is_same_v<Shape,Box>||std::is_same_v<Shape,Capsule>);', '''        static_assert(Enumerate?std::is_same_v<Shape,Capsule>:(std::is_same_v<Shape,Box>||std::is_same_v<Shape,Capsule>));
        constexpr bool with_owners=std::is_same_v<Output,AllSweepOwners>||std::is_same_v<Output,AllOverlapOwners>;
        if constexpr(Enumerate)static_assert(std::is_same_v<Output,AllOverlap>||std::is_same_v<Output,AllOverlapOwners>);
        else static_assert(std::is_same_v<Output,AllSweep>||std::is_same_v<Output,AllSweepOwners>);
        auto& candidate=[this]() -> auto& {if constexpr(Enumerate)return overlap_candidate_;else return stream_candidate_;}();
        auto result_count=[&](){if constexpr(Enumerate)return candidate.shape_count;else return candidate.hit_count;};
        auto enumeration_code_current=[&](){if constexpr(Enumerate)return gears_cover_physx_overlap_code::current(image_.base(),image_.size(),read,detail::digest);else return true;};''',1)
body=body.replace('        static_assert(std::is_same_v<Output,AllSweep>||std::is_same_v<Output,AllSweepOwners>);\n','',1)
body=body.replace('if constexpr(std::is_same_v<Output,AllSweep>)','if constexpr(!with_owners)')
body=body.replace('if constexpr(std::is_same_v<Output,AllSweepOwners>','if constexpr(with_owners')
body=body.replace('!detail::sweep_motion(geometry,movement)||','(!Enumerate&&!detail::sweep_motion(geometry,movement))||')
body=body.replace('if(detail::active_stream_reporter)','if(detail::active_query_reporter)')
needle='try{const auto ready=begin(request,read,guard);if(ready!=Code::queried)return ready;'
assert body.count(needle)==1
body=body.replace(needle,needle+'''
            if constexpr(Enumerate){progress_.stage=QueryStage::code_begin;if(!enumeration_code_current())return Code::code;}''')
a=body.index('            stream_candidate_={};');b=body.index('            if constexpr(with_owners){',a)
body=body[:a]+'''            candidate={};std::uint32_t raw_count{};bool malformed{},truncated{},untrusted{};
            if constexpr(Enumerate){
                std::array<std::uintptr_t,stream_batch_hits> batch{};
                detail::OverlapReporter reporter{detail::overlap_vtable.data(),&candidate,batch.data(),GetCurrentThreadId(),budget};
                detail::OverlapScope callback(reporter);if(!callback.admitted())return Code::reentrant;
                progress_.stage=QueryStage::dispatch;++progress_.original_started;
                const bool returned=Calls::overlap_shapes(image_.base()+0x172160,before_.public_scene,geometry,batch.data(),reporter,raw_count);
                callback.close();if(!returned){fault();return Code::fault_unknown;}
                malformed=reporter.malformed;truncated=reporter.truncated;untrusted=reporter.untrusted_batch;
            }else{
                std::array<Hit,stream_batch_hits> batch{};
                detail::StreamReporter reporter{detail::stream_vtable.data(),&candidate,batch.data(),GetCurrentThreadId(),budget};
                detail::StreamScope callback(reporter);if(!callback.admitted())return Code::reentrant;
                constexpr auto rva=std::is_same_v<Shape,Box>?0x171770:0x1716f0;
                progress_.stage=QueryStage::dispatch;++progress_.original_started;
                const bool returned=Calls::sweep_stream(image_.base()+rva,before_.public_scene,geometry,movement,batch.data(),reporter,raw_count);
                callback.close();if(!returned){fault();return Code::fault_unknown;}
                malformed=reporter.malformed;truncated=reporter.truncated;
            }
            ++progress_.original_returned;progress_.stage=QueryStage::returned;
            const auto closed=end(request,read,guard);if(closed!=Code::queried)return closed;
            if constexpr(Enumerate){progress_.stage=QueryStage::code_end;if(!enumeration_code_current())return Code::code;}
            if(!source::same(request,s)||std::memcmp(&geometry,&shape,sizeof shape)||std::memcmp(&movement,&delta,sizeof delta)||
               std::memcmp(&budget,&limits,sizeof limits))return Code::changed;
            if(malformed||untrusted)return Code::callback_rejected;
            if(truncated)return Code::truncated;
            if(raw_count!=result_count())return Code::callback_rejected;
            candidate.raw_count=raw_count;
''' + body[b:]
body=body.replace('stream_candidate_.hit_count','result_count()').replace('stream_candidate_','candidate')
# The lambda's old member selector must keep selecting the member, not itself.
body=body.replace('else return candidate;}();','else return stream_candidate_;}();',1)
needle='''                    const auto owner=gears_cover_physx_shape_owner::observe(image_.base(),image_.size(),before_,
                        candidate.hits[i],read,current_owner,owners_candidate_[i]);'''
assert body.count(needle)==1
body=body.replace(needle,'''                    const auto owner=[&](){if constexpr(Enumerate)
                        return gears_cover_physx_shape_owner::observe_address(image_.base(),image_.size(),before_,candidate.shapes[i],read,current_owner,owners_candidate_[i]);
                        else return gears_cover_physx_shape_owner::observe(image_.base(),image_.size(),before_,candidate.hits[i],read,current_owner,owners_candidate_[i]);}();''')
needle='                const auto owner_closed=end(request,read,guard);if(owner_closed!=Code::queried)return owner_closed;'
assert body.count(needle)==1
body=body.replace(needle,needle+'''
                if constexpr(Enumerate){progress_.stage=QueryStage::code_end;if(!enumeration_code_current())return Code::code;}''')
wrappers='''    template<class Shape,class Read,class Guard,class Output> Code sweep_all(source::Source const& s,Shape const& shape,Vector const& delta,
        StreamLimits const& limits,Read read,Guard guard,Output& output,gears_cover_physx_collider::Source const* component_source=nullptr,
        gears_cover_physx_components::Source const* component_catalog=nullptr)noexcept{
        return collect_all<false>(s,shape,delta,limits,read,guard,output,component_source,component_catalog);
    }
    template<class Read,class Guard,class Output> Code overlap_all(source::Source const& s,Capsule const& shape,
        StreamLimits const& limits,Read read,Guard guard,Output& output,gears_cover_physx_collider::Source const* component_source=nullptr,
        gears_cover_physx_components::Source const* component_catalog=nullptr)noexcept{
        return collect_all<true>(s,shape,Vector{},limits,read,guard,output,component_source,component_catalog);
    }
private:
'''
t=t[:start]+wrappers+body+t[end:];p.write_text(t)
print('Shared bounded enumeration/sweep transaction written; inspect diff and compile')
