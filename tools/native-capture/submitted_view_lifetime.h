#pragma once
#include <array>
#include <cstdint>
#include <mutex>

// Tracks caller-owned renderers, never frees them. A true close/retire return
// grants one state-release claim, which must be executed on a native thread.
namespace gears_submitted {
class Lifetime {
    std::mutex mutex;
    std::array<uintptr_t,64> roots{};
    bool closed{},claimed{};
    unsigned count{};
    bool claim() {if(closed && !count && !claimed){claimed=true;return true;}return false;}
public:
    bool begin(uintptr_t root) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!root || closed || count==roots.size())return false;
        for(auto value:roots)if(value==root)return false;
        for(auto& value:roots)if(!value){value=root;++count;return true;}
        return false;
    }
    bool contains(uintptr_t root) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!root)return false;
        for(auto value:roots)if(value==root)return true;
        return false;
    }
    bool retire(uintptr_t root) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!root)return false;
        for(auto& value:roots)if(value==root){value=0;--count;return claim();}
        return false;
    }
    bool close() {std::lock_guard<std::mutex> lock(mutex);closed=true;return claim();}
    unsigned pending() {std::lock_guard<std::mutex> lock(mutex);return count;}
    bool is_closed() {std::lock_guard<std::mutex> lock(mutex);return closed;}
};
}
