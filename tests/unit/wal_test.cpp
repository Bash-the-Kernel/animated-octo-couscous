#include <gtest/gtest.h>
#include "persistence/wal.hpp"
#include <filesystem>

using namespace hft;

TEST(WALTest, CreateAndOpen) {
    std::filesystem::create_directories("test_data");
    std::string path = "test_data/test_wal.bin";
    
    {
        WAL wal(path);
        EXPECT_TRUE(wal.open());
        EXPECT_EQ(wal.entry_count(), 0);
    }
    
    std::filesystem::remove(path);
}

TEST(WALTest, AppendAndRead) {
    std::filesystem::create_directories("test_data");
    std::string path = "test_data/test_wal_append.bin";
    
    {
        WAL wal(path);
        ASSERT_TRUE(wal.open());
        
        uint64_t data = 12345;
        EXPECT_TRUE(wal.append(EventType::NEW_ORDER, 1, &data, sizeof(data)));
        EXPECT_TRUE(wal.sync());
        
        EXPECT_EQ(wal.entry_count(), 1);
    }
    
    {
        WAL wal(path);
        ASSERT_TRUE(wal.open());
        
        auto entries = wal.read_all();
        ASSERT_EQ(entries.size(), 1);
        EXPECT_EQ(entries[0].header.type, EventType::NEW_ORDER);
        EXPECT_EQ(entries[0].header.sequence, 1);
    }
    
    std::filesystem::remove(path);
}

TEST(WALTest, MultipleEntries) {
    std::filesystem::create_directories("test_data");
    std::string path = "test_data/test_wal_multi.bin";
    
    {
        WAL wal(path);
        ASSERT_TRUE(wal.open());
        
        for (uint64_t i = 0; i < 10; ++i) {
            EXPECT_TRUE(wal.append(EventType::NEW_ORDER, i, &i, sizeof(i)));
        }
        
        EXPECT_TRUE(wal.sync());
        EXPECT_EQ(wal.entry_count(), 10);
    }
    
    {
        WAL wal(path);
        ASSERT_TRUE(wal.open());
        
        auto entries = wal.read_all();
        EXPECT_EQ(entries.size(), 10);
    }
    
    std::filesystem::remove(path);
}
