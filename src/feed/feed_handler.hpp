#pragma once

#include "../engine/matching_engine.hpp"
#include "parser.hpp"
#include <string>
#include <atomic>
#include <thread>

namespace hft {

class FeedHandler {
public:
    explicit FeedHandler(MatchingEngine& engine);
    ~FeedHandler();
    
    void start(const std::string& url);
    void stop();
    
    [[nodiscard]] uint64_t messages_received() const noexcept { return msg_count_; }
    [[nodiscard]] uint64_t messages_dropped() const noexcept { return dropped_; }
    [[nodiscard]] uint64_t sequence_gaps() const noexcept { return gaps_; }
    
private:
    void run(const std::string& url);
    void handle_message(std::string_view msg);
    
    MatchingEngine& engine_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    
    uint64_t msg_count_{0};
    uint64_t dropped_{0};
    uint64_t gaps_{0};
    SequenceNum expected_seq_{1};
};

} // namespace hft
