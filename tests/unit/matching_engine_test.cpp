#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include <chrono>
#include <atomic>

using namespace hft;

TEST(MatchingEngineTest, BasicCreation) {
    MatchingEngine engine;
    EXPECT_EQ(engine.processed_count(), 0);
}

TEST(MatchingEngineTest, GetBook) {
    MatchingEngine engine;
    auto* book = engine.get_book(Symbol("BTCUSD"));
    EXPECT_EQ(book, nullptr);
}
