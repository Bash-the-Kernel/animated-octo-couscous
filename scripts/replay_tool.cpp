#include "../src/engine/matching_engine.hpp"
#include "../src/persistence/wal.hpp"
#include "../src/common/logging.hpp"
#include <iostream>
#include <iomanip>

using namespace hft;

// Compute hash of order book state for determinism verification
uint64_t compute_state_hash(const MatchingEngine& engine) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    
    // Hash would include all order book state
    // Simplified for demonstration
    
    return hash;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <wal_path>" << std::endl;
        return 1;
    }
    
    std::string wal_path = argv[1];
    
    LOG_INFO("Deterministic Replay Tool");
    LOG_INFO("========================");
    LOG_INFO("WAL Path: ", wal_path);
    
    // Open WAL
    WAL wal(wal_path);
    if (!wal.open()) {
        LOG_ERROR("Failed to open WAL");
        return 1;
    }
    
    LOG_INFO("WAL Entries: ", wal.entry_count());
    
    // Read all entries
    auto entries = wal.read_all();
    LOG_INFO("Read ", entries.size(), " entries");
    
    // Create matching engine
    MatchingEngine engine;
    
    std::vector<uint64_t> state_hashes;
    
    // Replay events
    engine.start();
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        
        // Deserialize and submit event
        // (Simplified - would need proper deserialization)
        
        if (i % 1000 == 0) {
            uint64_t hash = compute_state_hash(engine);
            state_hashes.push_back(hash);
            
            LOG_INFO("Progress: ", i, "/", entries.size(), 
                    " Hash: 0x", std::hex, hash, std::dec);
        }
    }
    
    engine.stop();
    
    // Final state
    uint64_t final_hash = compute_state_hash(engine);
    LOG_INFO("Final State Hash: 0x", std::hex, final_hash, std::dec);
    LOG_INFO("Processed: ", engine.processed_count(), " events");
    
    // Verify determinism by comparing with expected hash
    // In production, would store expected hash and compare
    
    LOG_INFO("Replay complete!");
    
    return 0;
}
