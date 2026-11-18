// Included in boomshot_inspection_clip. Per-gun lifecycle, not an input writer.
enum class Result {rejected,pending,ready,recovery_required};
enum class Dispatch {not_dispatched,returned,unknown};
template<class Backend>class Owner {
    enum class Stage {idle,capturing,ready,finishing};
    Backend& backend_;Stage stage_{};std::uint64_t serial_{},last_serial_{};
    bool stop_sent_{},busy_{};
    struct Busy {bool& value;explicit Busy(bool& v):value(v){value=true;}~Busy(){value=false;}};
    bool drain_impl() noexcept {
        if(!active())return true;
        stage_=Stage::finishing;
        if(backend_.retire_if_replaced()){stage_=Stage::idle;serial_=0;stop_sent_=false;return true;}
        // A successful StopWeaponAnim can replace the selected sequence.
        // Its pending post-read must not recapture the old AR_Reload node.
        if(!backend_.finish_started()){
            if(!backend_.capture())return false;
            if(!stop_sent_){stop_sent_=true;if(backend_.stop()==Dispatch::not_dispatched)stop_sent_=false;}
        }
        if(!backend_.finish())return false;
        stage_=Stage::idle;serial_=0;stop_sent_=false;return true;
    }
    Result service_impl(gears_weapon_hand::Frame const& f) noexcept {
        if(!active())return Result::rejected;
        if(stage_==Stage::finishing)return drain_impl()?Result::rejected:Result::recovery_required;
        if(!backend_.capture())return Result::recovery_required;
        if(!stop_sent_){
            // Durable before the original call, including reentrant callbacks.
            stop_sent_=true;
            if(backend_.stop()==Dispatch::not_dispatched)stop_sent_=false;
        }
        if(!backend_.stopped())return Result::recovery_required;
        if(!backend_.controls(f))return Result::pending;
        stage_=Stage::ready;return Result::ready;
    }
public:
    explicit Owner(Backend& journal_backend):backend_(journal_backend){}
    Owner(Owner const&)=delete;Owner& operator=(Owner const&)=delete;
    bool active() const noexcept {return stage_!=Stage::idle;}
    std::uint64_t serial() const noexcept {return serial_;}
    std::uint64_t available_serial() const noexcept {return active()||last_serial_==UINT64_MAX?0:last_serial_+1;}
    Result service(gears_weapon_hand::Frame const& f) noexcept {
        if(busy_)return Result::recovery_required;Busy guard(busy_);return service_impl(f);
    }
    Result begin(gears_weapon_hand::Frame const& f,std::uint64_t serial) noexcept {
        if(busy_||active()||!serial||serial<=last_serial_)return Result::rejected;
        Busy guard(busy_);
        typename Backend::Prepared prepared{};
        if(!backend_.prepare(f,prepared)||!backend_.journal(f,serial,prepared))return Result::rejected;
        stage_=Stage::capturing;serial_=last_serial_=serial;stop_sent_=false;
        const auto dispatch=backend_.play(f,prepared);
        if(dispatch==Dispatch::not_dispatched){
            if(!backend_.drop_unstarted())return Result::recovery_required;
            stage_=Stage::idle;serial_=0;return Result::rejected;
        }
        // Returned/unknown both reconcile the actual post-Play selected node.
        // A getter or Stop failure cannot trigger another Play dispatch.
        return service_impl(f);
    }
    bool seek(gears_weapon_hand::Frame const& f,double q,gears_weapon::boomshot_barrel::Direction direction) noexcept {
        if(busy_||stage_!=Stage::ready)return false;Busy guard(busy_);return backend_.seek(f,q,direction);
    }
    bool drain() noexcept {
        if(busy_)return false;Busy guard(busy_);return drain_impl();
    }
};
