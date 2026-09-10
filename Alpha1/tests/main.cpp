/**
 * Alpha1 Test Suite
 * 
 * Comprehensive tests for all framework components.
 */

#include <alpha1/alpha1.h>
#include <iostream>
#include <cassert>

using namespace Alpha1;

void TestCoreTypes() {
    std::cout << "Testing Core Types..." << std::endl;
    
    // Test EntityID
    EntityID id = 42;
    assert(id == 42);
    assert(INVALID_ENTITY == Core::INVALID_ENTITY_ID);
    
    // Test Result<T>
    Result<int> successResult(42);
    assert(successResult.IsSuccess());
    assert(successResult.Value() == 42);
    
    Result<int> errorResult("Test error");
    assert(errorResult.IsError());
    assert(errorResult.Error() == "Test error");
    
    Result<void> voidResult;
    assert(voidResult.IsSuccess());
    
    Result<void> voidError("Void error");
    assert(voidError.IsError());
    
    std::cout << "  ✓ Core Types passed" << std::endl;
}

void TestMemoryAllocators() {
    std::cout << "Testing Memory Allocators..." << std::endl;
    
    using namespace Core;
    
    // Test Arena Allocator
    ArenaAllocator arena(1024);
    void* ptr1 = arena.Allocate(64);
    void* ptr2 = arena.Allocate(128);
    assert(ptr1 != nullptr);
    assert(ptr2 != nullptr);
    assert(arena.Used() >= 192);
    arena.Reset();
    assert(arena.Used() == 0);
    
    // Test Pool Allocator
    PoolAllocator<int, 32> pool;
    int* p1 = pool.Allocate();
    int* p2 = pool.Allocate();
    assert(p1 != nullptr);
    assert(p2 != nullptr);
    assert(p1 != p2);
    pool.Deallocate(p1);
    pool.Deallocate(p2);
    
    std::cout << "  ✓ Memory Allocators passed" << std::endl;
}

void TestECS() {
    std::cout << "Testing ECS..." << std::endl;
    
    using namespace ECS;
    
    // Define test components
    struct Position {
        float x, y, z;
        Position(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
    };
    
    struct Velocity {
        float dx, dy, dz;
        Velocity(float _dx = 0, float _dy = 0, float _dz = 0) : dx(_dx), dy(_dy), dz(_dz) {}
    };
    
    // Create world
    World world;
    
    // Create entities
    Entity e1 = world.CreateEntity();
    Entity e2 = world.CreateEntity();
    
    assert(e1.IsValid());
    assert(e2.IsValid());
    assert(e1.GetID() != e2.GetID());
    
    // Add components
    world.AddComponent<Position>(e1, 1.0f, 2.0f, 3.0f);
    world.AddComponent<Velocity>(e1, 0.1f, 0.2f, 0.3f);
    world.AddComponent<Position>(e2, 10.0f, 20.0f, 30.0f);
    
    // Check components exist
    assert(world.HasComponent<Position>(e1));
    assert(world.HasComponent<Velocity>(e1));
    assert(world.HasComponent<Position>(e2));
    assert(!world.HasComponent<Velocity>(e2));
    
    // Get components
    Position* pos1 = world.GetComponent<Position>(e1);
    assert(pos1 != nullptr);
    assert(pos1->x == 1.0f);
    assert(pos1->y == 2.0f);
    assert(pos1->z == 3.0f);
    
    Velocity* vel1 = world.GetComponent<Velocity>(e1);
    assert(vel1 != nullptr);
    assert(vel1->dx == 0.1f);
    
    // Remove component
    world.RemoveComponent<Velocity>(e1);
    assert(!world.HasComponent<Velocity>(e1));
    
    // Destroy entity
    world.DestroyEntity(e1);
    assert(!e1.IsValid());
    
    std::cout << "  ✓ ECS passed" << std::endl;
}

void TestThreading() {
    std::cout << "Testing Threading..." << std::endl;
    
    using namespace Core;
    
    // Test Thread Pool
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    
    // Submit tasks
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.Submit([&counter, i]() {
            ++counter;
            return i * 2;
        }));
    }
    
    // Wait for completion
    pool.WaitAll();
    assert(counter.load() == 100);
    
    // Check results
    for (int i = 0; i < 100; ++i) {
        assert(futures[i].get() == i * 2);
    }
    
    std::cout << "  ✓ Threading passed" << std::endl;
}

void TestFramework() {
    std::cout << "Testing Framework Initialization..." << std::endl;
    
    // Initialize
    auto result = Initialize(nullptr);
    assert(result.IsSuccess());
    assert(IsInitialized());
    
    // Run update loop
    for (int i = 0; i < 10; ++i) {
        Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    assert(GetFPS() > 0.0f);
    assert(GetFrameTime() > 0.0f);
    
    // Shutdown
    Shutdown();
    assert(!IsInitialized());
    
    std::cout << "  ✓ Framework passed" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Alpha1 Framework Test Suite v" << VERSION_STRING << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        TestCoreTypes();
        TestMemoryAllocators();
        TestECS();
        TestThreading();
        TestFramework();
        
        std::cout << "========================================" << std::endl;
        std::cout << "ALL TESTS PASSED ✓" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
