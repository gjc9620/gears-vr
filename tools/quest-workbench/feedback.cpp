#include "feedback.h"
#include <algorithm>
#include <cmath>
namespace workbench {
aaudio_data_callback_result_t FeedbackAudio::callback(AAudioStream*,void* user,void* output,std::int32_t frames){
    auto& s=*static_cast<FeedbackAudio*>(user);auto* p=static_cast<float*>(output);if(s.shot_request_.exchange(0)>0)s.shot_=1;if(s.click_request_.exchange(0)>0)s.click_=1;
    float gain=s.gain_.load(),motor=s.motor_.load(),shot_decay=std::exp(-1.f/(s.rate_*.035f)),click_decay=std::exp(-1.f/(s.rate_*.012f));
    for(int i=0;i<frames;++i){s.noise_^=s.noise_<<13;s.noise_^=s.noise_>>17;s.noise_^=s.noise_<<5;float noise=float(s.noise_&65535)/32768.f-1;s.phase_+=125.f/s.rate_;if(s.phase_>=1)s.phase_-=1;
        float saw=(2*s.phase_-1)*.13f+noise*.1f;float value=std::clamp(gain*(noise*s.shot_*.7f+(noise*.6f+saw*.5f)*s.click_+saw*motor),-.8f,.8f);for(int c=0;c<s.channels_;++c)*p++=value;s.shot_*=shot_decay;s.click_*=click_decay;}
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
bool FeedbackAudio::start(){if(stream_)return true;AAudioStreamBuilder* b{};if(AAudio_createStreamBuilder(&b)!=AAUDIO_OK)return false;AAudioStreamBuilder_setDirection(b,AAUDIO_DIRECTION_OUTPUT);AAudioStreamBuilder_setFormat(b,AAUDIO_FORMAT_PCM_FLOAT);AAudioStreamBuilder_setChannelCount(b,1);AAudioStreamBuilder_setPerformanceMode(b,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);AAudioStreamBuilder_setUsage(b,AAUDIO_USAGE_GAME);AAudioStreamBuilder_setDataCallback(b,callback,this);auto result=AAudioStreamBuilder_openStream(b,&stream_);AAudioStreamBuilder_delete(b);if(result!=AAUDIO_OK){stream_=nullptr;return false;}rate_=AAudioStream_getSampleRate(stream_);channels_=AAudioStream_getChannelCount(stream_);if(AAudioStream_requestStart(stream_)!=AAUDIO_OK){AAudioStream_close(stream_);stream_=nullptr;return false;}return true;}
void FeedbackAudio::stop(){gain_=0;motor_=0;if(stream_){AAudioStream_requestStop(stream_);AAudioStream_close(stream_);stream_=nullptr;}}
}
