/**
 * Alpha1 Framework - Micro-Benchmark Harness
 * Task 0.2: Performance baseline measurement
 * 
 * This benchmark measures ECS performance across different entity counts
 * to establish a real, repeatable baseline for optimization comparisons.
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>

#include "alpha1/alpha1.h"
#include "alpha1/core/types.h"
#include "alpha1/core/memory.h"
#include "alpha1/ecs/entity.h"

using namespace alpha1;
using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double, std::milli>;

// Component types for realistic testing
struct Position {
    float x, y, z;
};

struct Velocity {
    float dx, dy, dz;
};

struct Renderable {
    int mesh_id;
    int material_id;
};

struct PhysicsBody {
    float mass;
    float friction;
    bool is_dynamic;
};

struct AIState {
    int current_state;
    float state_timer;
    int target_entity;
};

// Benchmark configuration
struct BenchmarkConfig {
    std::vector<size_t> entity_counts = {1000, 10000, 100000};
    int num_ticks = 60; // Simulate 60 frames at 1fps for measurement
    int num_runs = 3;   // Average over multiple runs
};

// Results structure
struct BenchmarkResult {
    size_t entity_count;
    double create_time_ms;
    double component_add_time_ms;
    double query_time_ms;
    double update_tick_time_avg_ms;
    double total_time_ms;
};

// Helper to format results as CSV
std::string ResultsToCSV(const std::vector<BenchmarkResult>& results) {
    std::ostringstream oss;
    oss << "EntityCount,CreateTime_ms,ComponentAddTime_ms,QueryTime_ms,UpdateTickAvg_ms,TotalTime_ms\n";
    
    for (const auto& r : results) {
        oss << r.entity_count << ","
            << std::fixed << std::setprecision(3) << r.create_time_ms << ","
            << r.component_add_time_ms << ","
            << r.query_time_ms << ","
            << r.update_tick_time_avg_ms << ","
            << r.total_time_ms << "\n";
    }
    
    return oss.str();
}

// Helper to format results as table
std::string ResultsToTable(const std::vector<BenchmarkResult>& results) {
    std::ostringstream oss;
    oss << "================================================================================\n";
    oss << "                    Alpha1 ECS Performance Benchmark Results\n";
    oss << "================================================================================\n\n";
    
    oss << std::left << std::setw(15) << "Entities"
        << std::right << std::setw(18) << "Create (ms)"
        << std::setw(18) << "Add Comp (ms)"
        << std::setw(18) << "Query (ms)"
        << std::setw(18) << "Update Avg (ms)"
        << std::setw(18) << "Total (ms)" << "\n";
    
    oss << std::string(105, '-') << "\n";
    
    for (const auto& r : results) {
        oss << std::left << std::setw(15) << r.entity_count
            << std::right << std::fixed << std::setprecision(3) << std::setw(18) << r.create_time_ms
            << std::setw(18) << r.component_add_time_ms
            << std::setw(18) << r.query_time_ms
            << std::setw(18) << r.update_tick_time_avg_ms
            << std::setw(18) << r.total_time_ms << "\n";
    }
    
    oss << "\n================================================================================\n";
    
    return oss.str();
}

// Run benchmark for a specific entity count
BenchmarkResult RunBenchmark(size_t entity_count, const BenchmarkConfig& config) {
    BenchmarkResult result;
    result.entity_count = entity_count;
    
    double total_create = 0.0;
    double total_component_add = 0.0;
    double total_query = 0.0;
    double total_update = 0.0;
    
    // Run multiple times and average
    for (int run = 0; run < config.num_runs; ++run) {
        ECSWorld world;
        
        // Phase 1: Entity creation
        auto create_start = Clock::now();
        std::vector<EntityID> entities;
        entities.reserve(entity_count);
        
        for (size_t i = 0; i < entity_count; ++i) {
            auto create_result = world.CreateEntity();
            if (create_result.IsOk()) {
                entities.push_back(create_result.Value());
            }
        }
        
        auto create_end = Clock::now();
        total_create += Duration(create_end - create_start).count();
        
        // Phase 2: Add components (realistic distribution)
        auto comp_start = Clock::now();
        
        size_t pos_count = entity_count;              // All entities have Position
        size_t vel_count = entity_count * 3 / 4;      // 75% have Velocity
        size_t render_count = entity_count / 2;       // 50% have Renderable
        size_t physics_count = entity_count / 4;      // 25% have PhysicsBody
        size_t ai_count = entity_count / 10;          // 10% have AIState
        
        for (size_t i = 0; i < pos_count && i < entities.size(); ++i) {
            world.AddComponent<Position>(entities[i], Position{0.0f, 0.0f, 0.0f});
        }
        
        for (size_t i = 0; i < vel_count && i < entities.size(); ++i) {
            world.AddComponent<Velocity>(entities[i], Velocity{1.0f, 0.0f, 0.0f});
        }
        
        for (size_t i = 0; i < render_count && i < entities.size(); ++i) {
            world.AddComponent<Renderable>(entities[i], Renderable{static_cast<int>(i), 0});
        }
        
        for (size_t i = 0; i < physics_count && i < entities.size(); ++i) {
            world.AddComponent<PhysicsBody>(entities[i], PhysicsBody{1.0f, 0.5f, true});
        }
        
        for (size_t i = 0; i < ai_count && i < entities.size(); ++i) {
            world.AddComponent<AIState>(entities[i], AIState{0, 0.0f, -1});
        }
        
        auto comp_end = Clock::now();
        total_component_add += Duration(comp_end - comp_start).count();
        
        // Phase 3: Query test (Position + Velocity)
        auto query_start = Clock::now();
        
        auto query_result = world.Query<Position, Velocity>();
        size_t matched_count = 0;
        if (query_result.IsOk()) {
            matched_count = query_result.Value().size();
        }
        
        auto query_end = Clock::now();
        total_query += Duration(query_end - query_start).count();
        
        // Phase 4: Update ticks (simulate game loop)
        auto update_start = Clock::now();
        
        double tick_total = 0.0;
        for (int tick = 0; tick < config.num_ticks; ++tick) {
            auto tick_start = Clock::now();
            
            // Get all entities with Position and Velocity
            auto update_query = world.Query<Position, Velocity>();
            if (update_query.IsOk()) {
                // Simulate simple movement update
                // Note: In real code, we'd iterate and modify components here
                volatile size_t dummy = update_query.Value().size();
                (void)dummy;
            }
            
            auto tick_end = Clock::now();
            tick_total += Duration(tick_end - tick_start).count();
        }
        
        total_update += tick_total;
        
        auto total_end = Clock::now();
        result.total_time_ms = Duration(total_end - create_start).count();
    }
    
    // Average over runs
    result.create_time_ms = total_create / config.num_runs;
    result.component_add_time_ms = total_component_add / config.num_runs;
    result.query_time_ms = total_query / config.num_runs;
    result.update_tick_time_avg_ms = (total_update / config.num_runs) / config.num_ticks;
    
    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "Alpha1 Framework - Micro-Benchmark Harness\n";
    std::cout << "Task 0.2: Performance Baseline Measurement\n\n";
    
    // Initialize framework
    FrameworkConfig fw_config;
    fw_config.log_level = LogLevel::WARNING; // Suppress info logs during benchmark
    
    auto init_result = Framework::Initialize(fw_config);
    if (!init_result.IsOk()) {
        std::cerr << "Failed to initialize framework: " << init_result.ErrorMessage() << "\n";
        return 1;
    }
    
    BenchmarkConfig config;
    std::vector<BenchmarkResult> all_results;
    
    std::cout << "Running benchmarks...\n";
    std::cout << "Entity counts: ";
    for (size_t count : config.entity_counts) {
        std::cout << count << " ";
        
        std::cout << "\n  Testing " << count << " entities...";
        std::cout.flush();
        
        auto result = RunBenchmark(count, config);
        all_results.push_back(result);
        
        std::cout << " done (" << std::fixed << std::setprecision(1) 
                  << result.total_time_ms << " ms total)\n";
    }
    std::cout << "\n";
    
    // Output results
    std::cout << ResultsToTable(all_results);
    
    // Save CSV results
    std::string csv_output = ResultsToCSV(all_results);
    
    // Generate timestamp for filename
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    gmtime_r(&time_t_now, &tm_now);
    
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d", &tm_now);
    
    std::string filename = std::string("bench/baseline_") + timestamp + ".txt";
    
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << "# Alpha1 Framework ECS Benchmark Baseline\n";
        outfile << "# Date: " << timestamp << "\n";
        outfile << "# Entity counts tested: ";
        for (size_t count : config.entity_counts) {
            outfile << count << " ";
        }
        outfile << "\n# Runs per test: " << config.num_runs << "\n";
        outfile << "# Ticks per run: " << config.num_ticks << "\n\n";
        outfile << csv_output;
        outfile.close();
        
        std::cout << "Results saved to: " << filename << "\n";
    } else {
        std::cerr << "Warning: Could not save results to file\n";
        std::cout << "\nCSV Output:\n" << csv_output;
    }
    
    // Shutdown framework
    auto shutdown_result = Framework::Shutdown();
    if (!shutdown_result.IsOk()) {
        std::cerr << "Warning: Framework shutdown reported error: " 
                  << shutdown_result.ErrorMessage() << "\n";
    }
    
    std::cout << "\nBenchmark complete.\n";
    
    return 0;
}
