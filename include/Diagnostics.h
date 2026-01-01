#pragma once

#include "Components.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>

// ============================================================================
// PROACTIVE VERIFICATION - "The Immune System"
// ============================================================================

namespace Diagnostics {

// ============================================================================
// DATA ARCHEOLOGY - Log state changes for deterministic replay
// ============================================================================
class StateLogger {
private:
    std::ofstream log_file;
    uint64_t frame_number = 0;
    
public:
    StateLogger(const std::string& filename) {
        log_file.open(filename, std::ios::binary);
    }
    
    ~StateLogger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    void LogFrame(const GameState& state) {
        if (!log_file.is_open()) return;
        
        // Write frame header
        log_file.write(reinterpret_cast<const char*>(&frame_number), sizeof(frame_number));
        log_file.write(reinterpret_cast<const char*>(&state.entity_count), sizeof(state.entity_count));
        
        // Log critical state (positions, actions, needs)
        for (EntityID i = 0; i < state.entity_count; ++i) {
            log_file.write(reinterpret_cast<const char*>(&state.transforms.position_x[i]), sizeof(float));
            log_file.write(reinterpret_cast<const char*>(&state.transforms.position_y[i]), sizeof(float));
            log_file.write(reinterpret_cast<const char*>(&state.actions.current_action[i]), sizeof(ActionType));
            log_file.write(reinterpret_cast<const char*>(&state.needs.hunger[i]), sizeof(float));
            log_file.write(reinterpret_cast<const char*>(&state.needs.energy[i]), sizeof(float));
        }
        
        frame_number++;
    }
    
    void LogEvent(const std::string& event_name, EntityID entity_id) {
        if (!log_file.is_open()) return;
        
        // Log event marker
        uint8_t marker = 0xFF;
        log_file.write(reinterpret_cast<const char*>(&marker), sizeof(marker));
        log_file.write(reinterpret_cast<const char*>(&frame_number), sizeof(frame_number));
        log_file.write(reinterpret_cast<const char*>(&entity_id), sizeof(entity_id));
        
        size_t name_len = event_name.length();
        log_file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        log_file.write(event_name.c_str(), name_len);
    }
};

// ============================================================================
// CHAOS MONKEY - Randomly corrupt data to test resilience
// ============================================================================
class ChaosMonkey {
private:
    std::mt19937 rng;
    float corruption_probability;
    bool enabled;
    
public:
    ChaosMonkey(float prob = 0.001f, bool enable = false) 
        : corruption_probability(prob), enabled(enable) {
        rng.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    void SetEnabled(bool enable) { enabled = enable; }
    
    void MaybeCorrupt(GameState& state) {
        if (!enabled) return;
        
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (EntityID i = 0; i < state.entity_count; ++i) {
            // Randomly delete entities
            if (dist(rng) < corruption_probability) {
                state.health.is_alive[i] = false;
                std::cout << "[CHAOS] Killed entity " << i << std::endl;
            }
            
            // Randomly corrupt positions
            if (dist(rng) < corruption_probability) {
                state.transforms.position_x[i] = dist(rng) * 1000.0f;
                state.transforms.position_y[i] = dist(rng) * 1000.0f;
                std::cout << "[CHAOS] Teleported entity " << i << std::endl;
            }
            
            // Randomly corrupt needs
            if (dist(rng) < corruption_probability) {
                state.needs.hunger[i] = dist(rng);
                state.needs.energy[i] = dist(rng);
                std::cout << "[CHAOS] Corrupted needs for entity " << i << std::endl;
            }
        }
    }
};

// ============================================================================
// PERFORMANCE PROFILER - Measure system execution times
// ============================================================================
class Profiler {
private:
    struct ProfileEntry {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;
        std::chrono::high_resolution_clock::time_point end;
        double duration_ms;
    };
    
    std::vector<ProfileEntry> entries;
    ProfileEntry* current_entry = nullptr;
    
public:
    void BeginProfile(const std::string& name) {
        entries.push_back({name, std::chrono::high_resolution_clock::now(), {}, 0.0});
        current_entry = &entries.back();
    }
    
    void EndProfile() {
        if (current_entry) {
            current_entry->end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                current_entry->end - current_entry->start);
            current_entry->duration_ms = duration.count() / 1000.0;
            current_entry = nullptr;
        }
    }
    
    void PrintReport() {
        std::cout << "\n=== PERFORMANCE REPORT ===" << std::endl;
        double total_time = 0.0;
        
        for (const auto& entry : entries) {
            std::cout << entry.name << ": " << entry.duration_ms << " ms" << std::endl;
            total_time += entry.duration_ms;
        }
        
        std::cout << "TOTAL: " << total_time << " ms" << std::endl;
        std::cout << "FPS: " << (1000.0 / total_time) << std::endl;
        std::cout << "=========================\n" << std::endl;
    }
    
    void Clear() {
        entries.clear();
        current_entry = nullptr;
    }
};

// ============================================================================
// RAII Profile Scope Helper
// ============================================================================
class ProfileScope {
private:
    Profiler& profiler;
    
public:
    ProfileScope(Profiler& p, const std::string& name) : profiler(p) {
        profiler.BeginProfile(name);
    }
    
    ~ProfileScope() {
        profiler.EndProfile();
    }
};

// ============================================================================
// SYSTEM VALIDATOR - Verify data integrity
// ============================================================================
class SystemValidator {
public:
    static bool ValidateState(const GameState& state) {
        bool valid = true;
        
        // Check array sizes match
        if (state.transforms.Size() != state.entity_count) {
            std::cerr << "[VALIDATION ERROR] Transform size mismatch!" << std::endl;
            valid = false;
        }
        
        if (state.perception.Size() != state.entity_count) {
            std::cerr << "[VALIDATION ERROR] Perception size mismatch!" << std::endl;
            valid = false;
        }
        
        if (state.needs.Size() != state.entity_count) {
            std::cerr << "[VALIDATION ERROR] Needs size mismatch!" << std::endl;
            valid = false;
        }
        
        if (state.actions.Size() != state.entity_count) {
            std::cerr << "[VALIDATION ERROR] Actions size mismatch!" << std::endl;
            valid = false;
        }
        
        // Check for NaN/Inf values
        for (EntityID i = 0; i < state.entity_count; ++i) {
            if (std::isnan(state.transforms.position_x[i]) || 
                std::isinf(state.transforms.position_x[i])) {
                std::cerr << "[VALIDATION ERROR] Invalid position_x for entity " << i << std::endl;
                valid = false;
            }
            
            if (std::isnan(state.needs.hunger[i]) || 
                state.needs.hunger[i] < 0.0f || 
                state.needs.hunger[i] > 1.0f) {
                std::cerr << "[VALIDATION ERROR] Invalid hunger for entity " << i << std::endl;
                valid = false;
            }
        }
        
        return valid;
    }
    
    static void PrintStateSnapshot(const GameState& state, EntityID entity_id) {
        if (entity_id >= state.entity_count) {
            std::cerr << "Invalid entity ID" << std::endl;
            return;
        }
        
        std::cout << "\n=== ENTITY " << entity_id << " SNAPSHOT ===" << std::endl;
        std::cout << "Position: (" 
                  << state.transforms.position_x[entity_id] << ", "
                  << state.transforms.position_y[entity_id] << ")" << std::endl;
        std::cout << "Velocity: (" 
                  << state.transforms.velocity_x[entity_id] << ", "
                  << state.transforms.velocity_y[entity_id] << ")" << std::endl;
        std::cout << "Orientation: " << state.transforms.orientation[entity_id] << std::endl;
        std::cout << "Action: " << static_cast<int>(state.actions.current_action[entity_id]) << std::endl;
        std::cout << "Hunger: " << state.needs.hunger[entity_id] << std::endl;
        std::cout << "Energy: " << state.needs.energy[entity_id] << std::endl;
        std::cout << "Safety: " << state.needs.safety[entity_id] << std::endl;
        std::cout << "Visible Entities: " << state.perception.visible_entity_count[entity_id] << std::endl;
        std::cout << "Health: " << state.health.health[entity_id] << "/" 
                  << state.health.max_health[entity_id] << std::endl;
        std::cout << "Alive: " << (state.health.is_alive[entity_id] ? "Yes" : "No") << std::endl;
        std::cout << "==============================\n" << std::endl;
    }
};

} // namespace Diagnostics
