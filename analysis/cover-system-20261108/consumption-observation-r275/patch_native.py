def patch_native(s):
    def replace(a,b):
        nonlocal s
        assert s.count(a)==1,a[:120]
        s=s.replace(a,b)
    replace('std::atomic<uint64_t> cover_pose_attempts{},cover_pose_accepted{},cover_pose_rejected{};',
        'std::atomic<uint64_t> cover_pose_attempts{},cover_pose_accepted{},cover_pose_rejected{};\n#include "cover_consumption_observation.inl"')
    replace('            if(state.cover_completed_valid)++cover_pose_accepted;else ++cover_pose_rejected;',
        '            if(state.cover_completed_valid)++cover_pose_accepted;else ++cover_pose_rejected;\n            cover_consumption_observation::capture(unsigned(result.code),returned,original_called,same_entry(e));')
    replace('        cover_pose_attempts.load(),cover_pose_accepted.load(),cover_pose_rejected.load());',
        '        cover_pose_attempts.load(),cover_pose_accepted.load(),cover_pose_rejected.load());\n    cover_consumption_observation::summary(f);')
    return s
