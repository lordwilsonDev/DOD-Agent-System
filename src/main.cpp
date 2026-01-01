#include "../include/Components.h"
#include "../include/Systems.h"
#include "../include/Diagnostics.h"
#include <iostream>
#include <random>
#include <chrono>

// ============================================================================
// THE GAME LOOP - "The Heartbeat"
// Linear pipeline of systems executing in sequence
// ============================================================================

void InitializeEntities(GameState& state, size_t count) {
    state.Initialize(count);
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> pos_dist(0.0f, 1000.0f);
    std::uniform_real_distribution<float> need_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    
    for (EntityID i = 0; i < count; ++i) {
        // Initialize transforms
        state.transforms.position_x[i] = pos_dist(rng);
        state.transforms.position_y[i] = pos_dist(rng);
        state.transforms.position_z[i] = 0.0f;
        state.transforms.velocity_x[i] = 0.0f;
        state.transforms.velocity_y[i] = 0.0f;
        state.transforms.velocity_z[i] = 0.0f;
        state.transforms.orientation[i] = angle_dist(rng);
        
        // Initialize perception
        state.perception.view_range[i] = 50.0f + (i % 50);
        state.perception.view_angle[i] = M_PI / 2.0f; // 90 degree FOV
        state.perception.visible_entity_count[i] = 0;
        
        // Initialize needs
        state.needs.hunger[i] = need_dist(rng);
        state.needs.energy[i] = need_dist(rng);
        state.needs.safety[i] = need_dist(rng);
        state.needs.curiosity[i] = need_dist(rng);
        
        // Initialize actions
        state.actions.current_action[i] = ActionType::IDLE;
        state.actions.action_utility[i] = 0.0f;
        state.actions.target_entity[i] = INVALID_ENTITY;
        state.actions.target_x[i] = 0.0f;
        state.actions.target_y[i] = 0.0f;
        state.actions.target_z[i] = 0.0f;
        
        // Initialize health
        state.health.health[i] = 100.0f;
        state.health.max_health[i] = 100.0f;
        state.health.armor_type[i] = i % 3;
        state.health.is_alive[i] = true;
    }
    
    std::cout << "Initialized " << count << " entities" << std::endl;
}

void PrintSimulationStats(const GameState& state, int frame) {
    // Count entities by action
    int idle_count = 0;
    int move_count = 0;
    int eat_count = 0;
    int sleep_count = 0;
    int flee_count = 0;
    int attack_count = 0;
    int explore_count = 0;
    int alive_count = 0;
    
    for (EntityID i = 0; i < state.entity_count; ++i) {
        if (!state.health.is_alive[i]) continue;
        alive_count++;
        
        switch (state.actions.current_action[i]) {
            case ActionType::IDLE: idle_count++; break;
            case ActionType::MOVE_TO_TARGET: move_count++; break;
            case ActionType::EAT: eat_count++; break;
            case ActionType::SLEEP: sleep_count++; break;
            case ActionType::FLEE: flee_count++; break;
            case ActionType::ATTACK: attack_count++; break;
            case ActionType::EXPLORE: explore_count++; break;
            default: break;
        }
    }
    
    std::cout << "\n=== FRAME " << frame << " STATS ===" << std::endl;
    std::cout << "Alive: " << alive_count << "/" << state.entity_count << std::endl;
    std::cout << "Actions - Idle: " << idle_count 
              << " | Move: " << move_count
              << " | Eat: " << eat_count
              << " | Sleep: " << sleep_count
              << " | Flee: " << flee_count
              << " | Attack: " << attack_count
              << " | Explore: " << explore_count << std::endl;
    std::cout << "============================\n" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << "  DATA-ORIENTED DESIGN AGENT SYSTEM" << std::endl;
    std::cout << "  'The System is the Agent'" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Configuration
    const size_t ENTITY_COUNT = 1000;
    const int SIMULATION_FRAMES = 100;
    const float DELTA_TIME = 0.016f; // ~60 FPS
    const bool ENABLE_CHAOS = false; // Set to true to test resilience
    const bool ENABLE_LOGGING = true;
    const bool ENABLE_PROFILING = true;
    
    // Initialize game state
    GameState state;
    InitializeEntities(state, ENTITY_COUNT);
    
    // Initialize diagnostics
    Diagnostics::StateLogger logger("simulation_log.bin");
    Diagnostics::ChaosMonkey chaos(0.001f, ENABLE_CHAOS);
    Diagnostics::Profiler profiler;
    
    std::cout << "\nStarting simulation with " << ENTITY_COUNT << " entities..." << std::endl;
    std::cout << "Chaos Monkey: " << (ENABLE_CHAOS ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "Logging: " << (ENABLE_LOGGING ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "Profiling: " << (ENABLE_PROFILING ? "ENABLED" : "DISABLED") << std::endl;
    
    // Validate initial state
    if (!Diagnostics::SystemValidator::ValidateState(state)) {
        std::cerr << "Initial state validation failed!" << std::endl;
        return 1;
    }
    
    // Print initial snapshot of first entity
    Diagnostics::SystemValidator::PrintStateSnapshot(state, 0);
    
    // ========================================================================
    // THE MAIN LOOP - Linear pipeline execution
    // ========================================================================
    
    auto simulation_start = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < SIMULATION_FRAMES; ++frame) {
        if (ENABLE_PROFILING) profiler.Clear();
        
        // System Pipeline
        {
            if (ENABLE_PROFILING) {
                Diagnostics::ProfileScope scope(profiler, "PerceptionSystem");
                Systems::PerceptionSystem::Update(state, DELTA_TIME);
            } else {
                Systems::PerceptionSystem::Update(state, DELTA_TIME);
            }
        }
        
        {
            if (ENABLE_PROFILING) {
                Diagnostics::ProfileScope scope(profiler, "UtilitySystem");
                Systems::UtilitySystem::Update(state, DELTA_TIME);
            } else {
                Systems::UtilitySystem::Update(state, DELTA_TIME);
            }
        }
        
        {
            if (ENABLE_PROFILING) {
                Diagnostics::ProfileScope scope(profiler, "KineticSystem");
                Systems::KineticSystem::Update(state, DELTA_TIME);
            } else {
                Systems::KineticSystem::Update(state, DELTA_TIME);
            }
        }
        
        {
            if (ENABLE_PROFILING) {
                Diagnostics::ProfileScope scope(profiler, "NeedsSystem");
                Systems::NeedsSystem::Update(state, DELTA_TIME);
            } else {
                Systems::NeedsSystem::Update(state, DELTA_TIME);
            }
        }
        
        // Chaos Monkey (if enabled)
        if (ENABLE_CHAOS) {
            chaos.MaybeCorrupt(state);
        }
        
        // Validation
        if (!Diagnostics::SystemValidator::ValidateState(state)) {
            std::cerr << "State validation failed at frame " << frame << "!" << std::endl;
            Diagnostics::SystemValidator::PrintStateSnapshot(state, 0);
            return 1;
        }
        
        // Logging
        if (ENABLE_LOGGING) {
            logger.LogFrame(state);
        }
        
        // Print stats every 10 frames
        if (frame % 10 == 0) {
            PrintSimulationStats(state, frame);
            
            if (ENABLE_PROFILING) {
                profiler.PrintReport();
            }
        }
    }
    
    auto simulation_end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        simulation_end - simulation_start);
    
    // Final report
    std::cout << "\n==================================================" << std::endl;
    std::cout << "  SIMULATION COMPLETE" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Total frames: " << SIMULATION_FRAMES << std::endl;
    std::cout << "Total time: " << total_duration.count() << " ms" << std::endl;
    std::cout << "Average frame time: " << (total_duration.count() / (float)SIMULATION_FRAMES) << " ms" << std::endl;
    std::cout << "Average FPS: " << (SIMULATION_FRAMES * 1000.0f / total_duration.count()) << std::endl;
    std::cout << "Entities processed: " << ENTITY_COUNT << std::endl;
    std::cout << "Total entity-frames: " << (ENTITY_COUNT * SIMULATION_FRAMES) << std::endl;
    
    // Print final snapshot
    std::cout << "\nFinal state of entity 0:" << std::endl;
    Diagnostics::SystemValidator::PrintStateSnapshot(state, 0);
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "  DATA-ORIENTED DESIGN PRINCIPLES DEMONSTRATED:" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "✓ Structure of Arrays (SoA) for cache efficiency" << std::endl;
    std::cout << "✓ Stateless systems operating on data streams" << std::endl;
    std::cout << "✓ Batched processing in tight loops" << std::endl;
    std::cout << "✓ Spatial partitioning for O(1) queries" << std::endl;
    std::cout << "✓ Infinite Axis Utility System (IAUS) for AI" << std::endl;
    std::cout << "✓ Deterministic state logging for replay" << std::endl;
    std::cout << "✓ Chaos Monkey for resilience testing" << std::endl;
    std::cout << "✓ Performance profiling per system" << std::endl;
    std::cout << "✓ Data validation at runtime" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    return 0;
}
