#pragma once

#include "Components.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// SYSTEM DECLARATIONS
// All systems are stateless functions that transform data
// ============================================================================

namespace Systems {

// ============================================================================
// PERCEPTION SYSTEM - "The Eyes"
// Calculates what each entity can see in a single batched pass
// ============================================================================
class PerceptionSystem {
public:
    static void Update(GameState& state, float delta_time) {
        // Step 1: Build spatial partition
        state.spatial_grid.Clear();
        for (EntityID i = 0; i < state.entity_count; ++i) {
            if (state.health.is_alive[i]) {
                state.spatial_grid.Insert(i, 
                    state.transforms.position_x[i], 
                    state.transforms.position_y[i]);
            }
        }
        
        // Step 2: Clear previous stimulus
        state.stimulus_buffer.Clear();
        
        // Step 3: For each entity, query spatial grid for visible entities
        for (EntityID observer = 0; observer < state.entity_count; ++observer) {
            if (!state.health.is_alive[observer]) continue;
            
            float obs_x = state.transforms.position_x[observer];
            float obs_y = state.transforms.position_y[observer];
            float obs_orientation = state.transforms.orientation[observer];
            float view_range = state.perception.view_range[observer];
            float view_angle = state.perception.view_angle[observer];
            
            // Query nearby cells
            int grid_x = static_cast<int>(obs_x / GameState::SpatialGrid::CELL_SIZE);
            int grid_y = static_cast<int>(obs_y / GameState::SpatialGrid::CELL_SIZE);
            
            // Check 3x3 grid around observer
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int check_x = (grid_x + dx) % GameState::SpatialGrid::GRID_SIZE;
                    int check_y = (grid_y + dy) % GameState::SpatialGrid::GRID_SIZE;
                    
                    if (check_x < 0 || check_x >= GameState::SpatialGrid::GRID_SIZE ||
                        check_y < 0 || check_y >= GameState::SpatialGrid::GRID_SIZE) {
                        continue;
                    }
                    
                    for (EntityID target : state.spatial_grid.cells[check_x][check_y]) {
                        if (target == observer) continue;
                        if (!state.health.is_alive[target]) continue;
                        
                        float target_x = state.transforms.position_x[target];
                        float target_y = state.transforms.position_y[target];
                        
                        // Distance check
                        float dx_dist = target_x - obs_x;
                        float dy_dist = target_y - obs_y;
                        float distance_sq = dx_dist * dx_dist + dy_dist * dy_dist;
                        
                        if (distance_sq > view_range * view_range) continue;
                        
                        // Angle check (is target in FOV?)
                        float angle_to_target = std::atan2(dy_dist, dx_dist);
                        float angle_diff = std::abs(angle_to_target - obs_orientation);
                        
                        // Normalize angle difference to [-PI, PI]
                        while (angle_diff > M_PI) angle_diff -= 2.0f * M_PI;
                        while (angle_diff < -M_PI) angle_diff += 2.0f * M_PI;
                        
                        if (std::abs(angle_diff) <= view_angle / 2.0f) {
                            state.stimulus_buffer.visible_entities[observer].push_back(target);
                        }
                    }
                }
            }
            
            state.perception.visible_entity_count[observer] = 
                static_cast<uint32_t>(state.stimulus_buffer.visible_entities[observer].size());
        }
    }
};

// ============================================================================
// UTILITY SYSTEM - "The Brain"
// Uses Infinite Axis Utility System (IAUS) to select actions
// ============================================================================
class UtilitySystem {
public:
    // Response curves for utility calculations
    static float LinearCurve(float x) { return x; }
    static float QuadraticCurve(float x) { return x * x; }
    static float InverseLinearCurve(float x) { return 1.0f - x; }
    
    // Calculate utility for each action type
    static float CalculateEatUtility(const GameState& state, EntityID id) {
        // Higher hunger = higher utility to eat
        return state.needs.hunger[id] * QuadraticCurve(state.needs.hunger[id]);
    }
    
    static float CalculateSleepUtility(const GameState& state, EntityID id) {
        // Lower energy = higher utility to sleep
        float fatigue = 1.0f - state.needs.energy[id];
        return fatigue * QuadraticCurve(fatigue);
    }
    
    static float CalculateFleeUtility(const GameState& state, EntityID id) {
        // Lower safety = higher utility to flee
        float danger = 1.0f - state.needs.safety[id];
        return danger * QuadraticCurve(danger) * 1.5f; // Prioritize survival
    }
    
    static float CalculateExploreUtility(const GameState& state, EntityID id) {
        // High curiosity + high energy = explore
        return state.needs.curiosity[id] * state.needs.energy[id];
    }
    
    static float CalculateAttackUtility(const GameState& state, EntityID id) {
        // Attack if hungry and see potential food
        if (state.stimulus_buffer.visible_entities[id].empty()) return 0.0f;
        return state.needs.hunger[id] * state.needs.energy[id] * 0.8f;
    }
    
    static void Update(GameState& state, float delta_time) {
        // For each entity, calculate utility for all actions and pick best
        for (EntityID i = 0; i < state.entity_count; ++i) {
            if (!state.health.is_alive[i]) continue;
            
            // Calculate utilities
            float eat_utility = CalculateEatUtility(state, i);
            float sleep_utility = CalculateSleepUtility(state, i);
            float flee_utility = CalculateFleeUtility(state, i);
            float explore_utility = CalculateExploreUtility(state, i);
            float attack_utility = CalculateAttackUtility(state, i);
            
            // Find max utility action
            float max_utility = 0.0f;
            ActionType best_action = ActionType::IDLE;
            
            if (eat_utility > max_utility) {
                max_utility = eat_utility;
                best_action = ActionType::EAT;
            }
            if (sleep_utility > max_utility) {
                max_utility = sleep_utility;
                best_action = ActionType::SLEEP;
            }
            if (flee_utility > max_utility) {
                max_utility = flee_utility;
                best_action = ActionType::FLEE;
            }
            if (explore_utility > max_utility) {
                max_utility = explore_utility;
                best_action = ActionType::EXPLORE;
            }
            if (attack_utility > max_utility) {
                max_utility = attack_utility;
                best_action = ActionType::ATTACK;
            }
            
            // Write decision
            state.actions.current_action[i] = best_action;
            state.actions.action_utility[i] = max_utility;
            
            // Set target based on action
            if (best_action == ActionType::ATTACK && !state.stimulus_buffer.visible_entities[i].empty()) {
                EntityID target = state.stimulus_buffer.visible_entities[i][0];
                state.actions.target_entity[i] = target;
                state.actions.target_x[i] = state.transforms.position_x[target];
                state.actions.target_y[i] = state.transforms.position_y[target];
            } else if (best_action == ActionType::EXPLORE) {
                // Random exploration target
                state.actions.target_x[i] = state.transforms.position_x[i] + (rand() % 20 - 10);
                state.actions.target_y[i] = state.transforms.position_y[i] + (rand() % 20 - 10);
            }
        }
    }
};

// ============================================================================
// KINETIC SYSTEM - "The Body"
// Handles movement and physics integration
// ============================================================================
class KineticSystem {
public:
    static constexpr float MAX_SPEED = 5.0f;
    static constexpr float ACCELERATION = 2.0f;
    
    static void Update(GameState& state, float delta_time) {
        // Process all entities in a tight loop (cache-friendly)
        for (EntityID i = 0; i < state.entity_count; ++i) {
            if (!state.health.is_alive[i]) continue;
            
            ActionType action = state.actions.current_action[i];
            
            // Apply action-based movement
            if (action == ActionType::MOVE_TO_TARGET || 
                action == ActionType::ATTACK || 
                action == ActionType::EXPLORE) {
                
                float target_x = state.actions.target_x[i];
                float target_y = state.actions.target_y[i];
                float current_x = state.transforms.position_x[i];
                float current_y = state.transforms.position_y[i];
                
                // Calculate direction to target
                float dx = target_x - current_x;
                float dy = target_y - current_y;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance > 0.1f) {
                    // Normalize and apply acceleration
                    float dir_x = dx / distance;
                    float dir_y = dy / distance;
                    
                    state.transforms.velocity_x[i] += dir_x * ACCELERATION * delta_time;
                    state.transforms.velocity_y[i] += dir_y * ACCELERATION * delta_time;
                    
                    // Update orientation
                    state.transforms.orientation[i] = std::atan2(dy, dx);
                }
            } else if (action == ActionType::FLEE) {
                // Flee from nearest threat
                if (!state.stimulus_buffer.visible_entities[i].empty()) {
                    EntityID threat = state.stimulus_buffer.visible_entities[i][0];
                    float threat_x = state.transforms.position_x[threat];
                    float threat_y = state.transforms.position_y[threat];
                    float current_x = state.transforms.position_x[i];
                    float current_y = state.transforms.position_y[i];
                    
                    // Move away from threat
                    float dx = current_x - threat_x;
                    float dy = current_y - threat_y;
                    float distance = std::sqrt(dx * dx + dy * dy);
                    
                    if (distance > 0.1f) {
                        float dir_x = dx / distance;
                        float dir_y = dy / distance;
                        
                        state.transforms.velocity_x[i] += dir_x * ACCELERATION * 1.5f * delta_time;
                        state.transforms.velocity_y[i] += dir_y * ACCELERATION * 1.5f * delta_time;
                    }
                }
            } else if (action == ActionType::SLEEP || action == ActionType::IDLE) {
                // Decelerate
                state.transforms.velocity_x[i] *= 0.9f;
                state.transforms.velocity_y[i] *= 0.9f;
            }
            
            // Clamp velocity to max speed
            float speed_sq = state.transforms.velocity_x[i] * state.transforms.velocity_x[i] +
                           state.transforms.velocity_y[i] * state.transforms.velocity_y[i];
            
            if (speed_sq > MAX_SPEED * MAX_SPEED) {
                float speed = std::sqrt(speed_sq);
                state.transforms.velocity_x[i] = (state.transforms.velocity_x[i] / speed) * MAX_SPEED;
                state.transforms.velocity_y[i] = (state.transforms.velocity_y[i] / speed) * MAX_SPEED;
            }
            
            // Integrate position
            state.transforms.position_x[i] += state.transforms.velocity_x[i] * delta_time;
            state.transforms.position_y[i] += state.transforms.velocity_y[i] * delta_time;
            
            // Simple world bounds
            state.transforms.position_x[i] = std::max(0.0f, std::min(1000.0f, state.transforms.position_x[i]));
            state.transforms.position_y[i] = std::max(0.0f, std::min(1000.0f, state.transforms.position_y[i]));
        }
    }
};

// ============================================================================
// NEEDS SYSTEM - Updates entity needs over time
// ============================================================================
class NeedsSystem {
public:
    static void Update(GameState& state, float delta_time) {
        for (EntityID i = 0; i < state.entity_count; ++i) {
            if (!state.health.is_alive[i]) continue;
            
            ActionType action = state.actions.current_action[i];
            
            // Hunger increases over time
            state.needs.hunger[i] = std::min(1.0f, state.needs.hunger[i] + 0.01f * delta_time);
            
            // Energy decreases when active, increases when sleeping
            if (action == ActionType::SLEEP) {
                state.needs.energy[i] = std::min(1.0f, state.needs.energy[i] + 0.1f * delta_time);
            } else {
                state.needs.energy[i] = std::max(0.0f, state.needs.energy[i] - 0.02f * delta_time);
            }
            
            // Eating reduces hunger
            if (action == ActionType::EAT) {
                state.needs.hunger[i] = std::max(0.0f, state.needs.hunger[i] - 0.15f * delta_time);
            }
            
            // Safety based on nearby entities
            if (state.perception.visible_entity_count[i] > 3) {
                state.needs.safety[i] = std::max(0.0f, state.needs.safety[i] - 0.05f * delta_time);
            } else {
                state.needs.safety[i] = std::min(1.0f, state.needs.safety[i] + 0.03f * delta_time);
            }
            
            // Curiosity fluctuates
            state.needs.curiosity[i] += (rand() % 100 - 50) * 0.001f * delta_time;
            state.needs.curiosity[i] = std::max(0.0f, std::min(1.0f, state.needs.curiosity[i]));
        }
    }
};

} // namespace Systems
