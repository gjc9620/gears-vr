#pragma once
#include <aaudio/AAudio.h>
#include <atomic>
#include <cstdint>

namespace workbench {
// Quiet procedural preview sounds. No microphone and no game audio assets.
class FeedbackAudio {
    AAudioStream* stream_{};std::atomic<float> gain_{.15f},motor_{},shot_request_{},click_request_{};
    float shot_{},click_{},phase_{};int rate_{48000},channels_{1};std::uint32_t noise_{0x6d2b79f5};
    static aaudio_data_callback_result_t callback(AAudioStream*,void*,void*,std::int32_t);
public:
    bool start();void stop();~FeedbackAudio(){stop();}
    void update(bool enabled,float gain,bool motor,bool shot,bool click){gain_=enabled?gain:0;motor_=motor?1.f:0.f;if(shot)shot_request_=1;if(click)click_request_=1;}
};
}
