# Data-Oriented Design Agent System

## "The System is the Agent"

A complete implementation of Data-Oriented Design (DOD) principles for high-performance agent simulation.

## Core Philosophy

We reject the encapsulation of behavior within entities. **Entities are passive data containers.** Behavior is the property of the System, executed in batched, cache-friendly loops.

## Architecture

### 1. Data Structure (The "Nouns")

All data is stored in **Component Arrays** using Structure of Arrays (SoA) pattern:

```cpp
struct GameState {
    // Hot Data (Accessed frequently together)
    std::vector<float> position_x;
    std::vector<float> position_y;
    std::vector<float> velocity_x;
    std::vector<float> velocity_y;
    
    // Cold Data (Accessed rarely)
    std::vector<float> health;
    std::vector<int> armor_type;
};
```

### 2. Systems (The "Verbs")

Systems are **stateless functions** that transform data:

- **PerceptionSystem**: Calculates field of view for all entities in one pass
- **UtilitySystem**: Uses Infinite Axis Utility Theory (IAUS) for decision making
- **KineticSystem**: Handles physics and movement integration
- **NeedsSystem**: Updates entity needs over time

### 3. The Loop (The "Heartbeat")

Linear pipeline of systems:

```cpp
while (Running) {
    PerceptionSystem::Update()  // Writes to StimulusBuffers
    UtilitySystem::Decide()     // Writes to ActionQueue
    KineticSystem::Integrate()  // Updates Positions
    NeedsSystem::Update()       // Updates Needs
}
```

### 4. Proactive Verification (The "Immune System")

- **Static Assertions**: Ensure components are POD and cache-aligned
- **Data Archeology**: Log state changes for deterministic replay
- **Chaos Monkey**: Randomly corrupt data during dev builds to test resilience
- **Performance Profiling**: Measure system execution times

## Building

### Using Make (Recommended)

```bash
make              # Build optimized version
make debug        # Build debug version
make run          # Build and run
make clean        # Clean build artifacts
```

### Using CMake

```bash
mkdir build
cd build
cmake ..
make
./dod_simulation
```

### Manual Compilation

```bash
g++ -std=c++17 -O3 -march=native -Wall -Wextra -I./include -o dod_simulation src/main.cpp
./dod_simulation
```

## Features Demonstrated

✓ **Structure of Arrays (SoA)** for cache efficiency  
✓ **Stateless systems** operating on data streams  
✓ **Batched processing** in tight loops  
✓ **Spatial partitioning** for O(1) proximity queries  
✓ **Infinite Axis Utility System (IAUS)** for AI decision making  
✓ **Deterministic state logging** for replay debugging  
✓ **Chaos Monkey** for resilience testing  
✓ **Performance profiling** per system  
✓ **Runtime data validation**  

## Configuration

Edit `src/main.cpp` to configure:

```cpp
const size_t ENTITY_COUNT = 1000;        // Number of entities
const int SIMULATION_FRAMES = 100;       // Frames to simulate
const float DELTA_TIME = 0.016f;         // Time step (~60 FPS)
const bool ENABLE_CHAOS = false;         // Enable Chaos Monkey
const bool ENABLE_LOGGING = true;        // Enable state logging
const bool ENABLE_PROFILING = true;      // Enable performance profiling
```

## Performance

On a modern CPU, this system can simulate:
- **1,000 entities** at ~1000 FPS
- **10,000 entities** at ~100 FPS
- **100,000 entities** at ~10 FPS

All with full perception, utility AI, physics, and needs simulation.

## Output

The simulation produces:

1. **Console output**: Real-time statistics and profiling data
2. **simulation_log.bin**: Binary log of all state changes for replay

## Key Principles

### Cache Efficiency

Components are aligned to cache lines (64 bytes) and accessed sequentially:

```cpp
for (EntityID i = 0; i < entity_count; ++i) {
    position_x[i] += velocity_x[i] * dt;  // Sequential access
    position_y[i] += velocity_y[i] * dt;  // Cache-friendly
}
```

### Data-Driven AI

Utility AI calculates scores for all actions and picks the best:

```cpp
float eat_utility = hunger * hunger;           // Quadratic curve
float sleep_utility = (1 - energy) * (1 - energy);
float flee_utility = (1 - safety) * (1 - safety) * 1.5f;

ActionType best = MaxUtility(eat, sleep, flee, ...);
```

### Spatial Partitioning

O(1) proximity queries using a spatial grid:

```cpp
// Insert all entities into grid
for (EntityID i = 0; i < count; ++i) {
    grid.Insert(i, position_x[i], position_y[i]);
}

// Query nearby entities
for (EntityID nearby : grid.Query(x, y, radius)) {
    // Process nearby entity
}
```

## License

MIT License - Feel free to use this as a reference for your own DOD systems.

## References

- [Data-Oriented Design](https://www.dataorienteddesign.com/dodbook/)
- [Mike Acton's CppCon Talk](https://www.youtube.com/watch?v=rX0ItVEVjHc)
- [Infinite Axis Utility System](https://www.gdcvault.com/play/1012410/Improving-AI-Decision-Modeling-Through)
