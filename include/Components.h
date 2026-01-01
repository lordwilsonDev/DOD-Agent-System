#pragma once

#include <vector>
#include <cstdint>
#include <cassert>

// Cache line size for alignment
constexpr size_t CACHE_LINE_SIZE = 64;

// Entity is just an index
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = UINT32_MAX;

// ============================================================================
// COMPONENT ARRAYS (Structure of Arrays - SoA)
// ============================================================================

// Hot Data - Accessed every frame for movement/physics
struct alignas(CACHE_LINE_SIZE) TransformComponents {
    std::vector<float> position_x;
    std::vector<float> position_y;
    std::vector<float> position_z;
    
    std::vector<float> velocity_x;
    std::vector<float> velocity_y;
    std::vector<float> velocity_z;
    
    std::vector<float> orientation; // Radians
    
    void Resize(size_t count) {
        position_x.resize(count);
        position_y.resize(count);
        position_z.resize(count);
        velocity_x.resize(count);
        velocity_y.resize(count);
        velocity_z.resize(count);
        orientation.resize(count);
    }
    
    size_t Size() const { return position_x.size(); }
};

// Perception Data - What entities can "see"
struct alignas(CACHE_LINE_SIZE) PerceptionComponents {
    std::vector<float> view_range;
    std::vector<float> view_angle; // Field of view in radians
    std::vector<uint32_t> visible_entity_count;
    
    void Resize(size_t count) {
        view_range.resize(count);
        view_angle.resize(count);
        visible_entity_count.resize(count);
    }
    
    size_t Size() const { return view_range.size(); }
};

// Needs/Drives for Utility AI
struct alignas(CACHE_LINE_SIZE) NeedsComponents {
    std::vector<float> hunger;      // 0.0 = full, 1.0 = starving
    std::vector<float> energy;      // 0.0 = exhausted, 1.0 = full energy
    std::vector<float> safety;      // 0.0 = in danger, 1.0 = safe
    std::vector<float> curiosity;   // 0.0 = content, 1.0 = exploring
    
    void Resize(size_t count) {
        hunger.resize(count);
        energy.resize(count);
        safety.resize(count);
        curiosity.resize(count);
    }
    
    size_t Size() const { return hunger.size(); }
};

// Action State - What the entity is currently doing
enum class ActionType : uint8_t {
    IDLE = 0,
    MOVE_TO_TARGET,
    EAT,
    SLEEP,
    FLEE,
    ATTACK,
    EXPLORE,
    COUNT
};

struct alignas(CACHE_LINE_SIZE) ActionComponents {
    std::vector<ActionType> current_action;
    std::vector<float> action_utility;      // Score of current action
    std::vector<EntityID> target_entity;    // Target for action (if any)
    std::vector<float> target_x;            // Target position
    std::vector<float> target_y;
    std::vector<float> target_z;
    
    void Resize(size_t count) {
        current_action.resize(count, ActionType::IDLE);
        action_utility.resize(count);
        target_entity.resize(count, INVALID_ENTITY);
        target_x.resize(count);
        target_y.resize(count);
        target_z.resize(count);
    }
    
    size_t Size() const { return current_action.size(); }
};

// Cold Data - Rarely accessed (only when taking damage, etc.)
struct alignas(CACHE_LINE_SIZE) HealthComponents {
    std::vector<float> health;
    std::vector<float> max_health;
    std::vector<int> armor_type;
    std::vector<bool> is_alive;
    
    void Resize(size_t count) {
        health.resize(count);
        max_health.resize(count);
        armor_type.resize(count);
        is_alive.resize(count, true);
    }
    
    size_t Size() const { return health.size(); }
};

// ============================================================================
// GAME STATE - The Single Source of Truth
// ============================================================================

struct GameState {
    size_t entity_count = 0;
    
    // Component Arrays
    TransformComponents transforms;
    PerceptionComponents perception;
    NeedsComponents needs;
    ActionComponents actions;
    HealthComponents health;
    
    // Spatial Partition (for fast proximity queries)
    // Simple grid-based for now
    struct SpatialGrid {
        static constexpr int GRID_SIZE = 100;
        static constexpr float CELL_SIZE = 10.0f;
        std::vector<EntityID> cells[GRID_SIZE][GRID_SIZE];
        
        void Clear() {
            for (int x = 0; x < GRID_SIZE; ++x) {
                for (int y = 0; y < GRID_SIZE; ++y) {
                    cells[x][y].clear();
                }
            }
        }
        
        void Insert(EntityID id, float x, float y) {
            int grid_x = static_cast<int>(x / CELL_SIZE) % GRID_SIZE;
            int grid_y = static_cast<int>(y / CELL_SIZE) % GRID_SIZE;
            if (grid_x >= 0 && grid_x < GRID_SIZE && grid_y >= 0 && grid_y < GRID_SIZE) {
                cells[grid_x][grid_y].push_back(id);
            }
        }
    };
    
    SpatialGrid spatial_grid;
    
    // Stimulus Buffer - What each entity perceives
    struct StimulusBuffer {
        std::vector<std::vector<EntityID>> visible_entities;
        
        void Resize(size_t count) {
            visible_entities.resize(count);
        }
        
        void Clear() {
            for (auto& vec : visible_entities) {
                vec.clear();
            }
        }
    };
    
    StimulusBuffer stimulus_buffer;
    
    // Initialize with N entities
    void Initialize(size_t count) {
        entity_count = count;
        transforms.Resize(count);
        perception.Resize(count);
        needs.Resize(count);
        actions.Resize(count);
        health.Resize(count);
        stimulus_buffer.Resize(count);
    }
    
    // Add a new entity
    EntityID AddEntity() {
        EntityID id = static_cast<EntityID>(entity_count);
        entity_count++;
        
        transforms.Resize(entity_count);
        perception.Resize(entity_count);
        needs.Resize(entity_count);
        actions.Resize(entity_count);
        health.Resize(entity_count);
        stimulus_buffer.Resize(entity_count);
        
        return id;
    }
};

// Static assertions to ensure POD and alignment
static_assert(std::is_standard_layout<TransformComponents>::value, 
              "TransformComponents must be standard layout");
static_assert(alignof(TransformComponents) == CACHE_LINE_SIZE, 
              "TransformComponents must be cache-line aligned");
