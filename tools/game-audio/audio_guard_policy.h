#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gears_test_audio {
struct Session {
    virtual ~Session() = default;
    virtual std::wstring key() const = 0;
    virtual bool mute(bool& value) = 0;
    virtual bool set_mute(bool value) = 0;
    virtual bool set_volume(float) { return false; }
};
struct Counts {
    std::uint64_t matched{}, muted{}, read_failed{}, mute_failed{}, restore_attempted{}, restored{};
};
// Keeps the first state and the exact session object. Restoration never performs
// another PID lookup (which could select a replacement process/session).
// Manual launch intent: apply once to each late-created, exclusive game session.
// Never restore or enforce after success; later user volume changes are respected.
class ManualPolicy {
    std::vector<std::wstring> completed_;
public:
    std::uint64_t failed{};
    bool observe(std::unique_ptr<Session> session, bool target_alive) {
        if (!target_alive || !session) return false;
        auto key=session->key(); if (key.empty()) return false;
        for (auto const& done:completed_) if (done==key) return true;
        if (completed_.size()>=512) { ++failed; return false; }
        if (!session->set_volume(1.0f) || !session->set_mute(false)) { ++failed; return false; }
        completed_.push_back(key); return true;
    }
    std::uint64_t configured() const noexcept { return completed_.size(); }
};
class Policy {
    struct Saved { std::unique_ptr<Session> session; bool original{}, pending{}, muted{}; };
    std::vector<Saved> saved_;
    Counts counts_;
public:
    bool observe(std::unique_ptr<Session> session, bool target_alive) {
        if (!target_alive || !session) return false;
        Saved* saved = nullptr;
        auto key = session->key();
        if (key.empty()) return false;
        for (auto& item : saved_) if (item.session->key() == key) { saved = &item; break; }
        bool current = false;
        if (!saved) {
            if (saved_.size() >= 512 || !session->mute(current)) { ++counts_.read_failed; return false; }
            saved_.push_back({std::move(session), current, false, false});
            saved = &saved_.back(); ++counts_.matched;
        } else if (!saved->session->mute(current)) { ++counts_.read_failed; return false; }
        if (current) return true;
        // Retain restoration evidence even if an API failure has uncertain effects.
        saved->pending = true;
        if (!saved->session->set_mute(true)) { ++counts_.mute_failed; return false; }
        if (!saved->muted) { saved->muted = true; ++counts_.muted; }
        return true;
    }
    bool restore() noexcept {
        for (auto& item : saved_) if (item.pending) {
            ++counts_.restore_attempted;
            if (item.session->set_mute(item.original)) { item.pending = false; ++counts_.restored; }
        }
        return pending() == 0;
    }
    std::uint64_t pending() const noexcept {
        std::uint64_t result = 0;
        for (auto const& item : saved_) if (item.pending) ++result;
        return result;
    }
    Counts counts() const noexcept { return counts_; }
};
}
