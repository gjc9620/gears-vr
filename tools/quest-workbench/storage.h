#pragma once
#include "core.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace workbench {
inline void write_json_atomic(std::string const& path,Json const& value,bool replace=true){
    namespace fs=std::filesystem;fs::path target=fs::u8path(path);fs::create_directories(target.parent_path());
    if(!replace&&fs::exists(target))throw std::runtime_error("Capture archive already exists");
    fs::path temp=target;temp+=".tmp";std::ofstream f(temp,std::ios::trunc);f<<value.dump(2);f.close();if(!f)throw std::runtime_error("Project write failed");
#ifdef _WIN32
    if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH|(replace?MOVEFILE_REPLACE_EXISTING:0)))throw std::runtime_error("Project publish failed");
#else
    fs::rename(temp,target);
#endif
}
// No GL, OpenXR or application state is touched on this worker.
class StorageWorker {
public:
    struct Result {std::string id,message;bool success{};};
private:
    struct Task {std::string id,message;std::function<void()> work;};
    std::mutex mutex_;std::condition_variable ready_;
    std::deque<Task> queue_;std::deque<Result> results_;bool stopping_{},busy_{};
    std::thread thread_;
    void run(){for(;;){Task t;{std::unique_lock<std::mutex> lock(mutex_);ready_.wait(lock,[&]{return stopping_||!queue_.empty();});if(queue_.empty()&&stopping_)return;t=std::move(queue_.front());queue_.pop_front();busy_=true;}Result r{t.id,t.message,true};try{t.work();}catch(std::exception const& e){r.success=false;r.message=e.what();}{std::lock_guard<std::mutex> lock(mutex_);results_.push_back(std::move(r));busy_=false;}}}
public:
    StorageWorker():thread_([this]{run();}){}
    ~StorageWorker(){finish();}
    bool submit(std::string id,std::string message,std::function<void()> work){std::lock_guard<std::mutex> lock(mutex_);if(stopping_||queue_.size()>=32)return false;queue_.push_back({std::move(id),std::move(message),std::move(work)});ready_.notify_one();return true;}
    std::vector<Result> poll(){std::lock_guard<std::mutex> lock(mutex_);std::vector<Result> out;while(!results_.empty()){out.push_back(std::move(results_.front()));results_.pop_front();}return out;}
    bool pending(){std::lock_guard<std::mutex> lock(mutex_);return busy_||!queue_.empty();}
    void finish(){{std::lock_guard<std::mutex> lock(mutex_);stopping_=true;}ready_.notify_one();if(thread_.joinable())thread_.join();}
};
}
