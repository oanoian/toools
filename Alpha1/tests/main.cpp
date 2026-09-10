#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Alpha1 Framework Test Suite
// Task 0.1: Minimal unit test scaffold

#include "alpha1/alpha1.h"
#include "alpha1/core/types.h"
#include "alpha1/core/memory.h"
#include "alpha1/core/logging.h"
#include "alpha1/core/threading.h"
#include "alpha1/ecs/entity.h"

using namespace alpha1;

// ============================================================================
// TEST SUITE: Core Types
// ============================================================================

TEST_SUITE("CoreTypes") {
    TEST_CASE("Result<T> - Ok value") {
        auto result = Result<int>::Ok(42);
        CHECK(result.IsOk());
        CHECK(result.Value() == 42);
    }

    TEST_CASE("Result<T> - Error value") {
        auto result = Result<int>::Error("Test error message");
        CHECK(!result.IsOk());
        CHECK(result.ErrorMessage() == "Test error message");
    }

    TEST_CASE("EntityID - Valid construction") {
        EntityID id{1, 5};
        CHECK(id.index == 1);
        CHECK(id.generation == 5);
    }

    TEST_CASE("EntityID - Equality operator") {
        EntityID id1{1, 5};
        EntityID id2{1, 5};
        EntityID id3{2, 5};
        CHECK(id1 == id2);
        CHECK(id1 != id3);
    }
}

// ============================================================================
// TEST SUITE: Memory Allocators
// ============================================================================

TEST_SUITE("MemoryAllocators") {
    TEST_CASE("ArenaAllocator - Basic allocation") {
        ArenaAllocator arena(1024);
        void* ptr = arena.Allocate(64, 8);
        CHECK(ptr != nullptr);
        
        // Check alignment
        CHECK(reinterpret_cast<uintptr_t>(ptr) % 8 == 0);
    }

    TEST_CASE("ArenaAllocator - Multiple allocations") {
        ArenaAllocator arena(1024);
        void* ptr1 = arena.Allocate(128, 8);
        void* ptr2 = arena.Allocate(256, 8);
        
        CHECK(ptr1 != nullptr);
        CHECK(ptr2 != nullptr);
        CHECK(ptr1 != ptr2);
    }

    TEST_CASE("ArenaAllocator - Reset") {
        ArenaAllocator arena(1024);
        arena.Allocate(512, 8);
        size_t used_before = arena.GetUsedSize();
        arena.Reset();
        size_t used_after = arena.GetUsedSize();
        
        CHECK(used_before > 0);
        CHECK(used_after == 0);
    }

    TEST_CASE("PoolAllocator - Basic allocation") {
        PoolAllocator pool(64, 16); // Block size 64, 16 blocks
        void* ptr = pool.Allocate();
        CHECK(ptr != nullptr);
        
        pool.Deallocate(ptr);
    }

    TEST_CASE("PoolAllocator - Multiple allocations") {
        PoolAllocator pool(32, 8);
        std::vector<void*> ptrs;
        
        for (int i = 0; i < 8; ++i) {
            ptrs.push_back(pool.Allocate());
        }
        
        // All should be non-null and unique
        for (auto ptr : ptrs) {
            CHECK(ptr != nullptr);
        }
        
        // Deallocate all
        for (auto ptr : ptrs) {
            pool.Deallocate(ptr);
        }
    }
}

// ============================================================================
// TEST SUITE: ECS System
// ============================================================================

TEST_SUITE("ECSSystem") {
    TEST_CASE("EntityManager - Create entity") {
        ECSWorld world;
        auto entity = world.CreateEntity();
        CHECK(entity.IsOk());
        CHECK(entity.Value().index >= 0);
    }

    TEST_CASE("EntityManager - Destroy entity") {
        ECSWorld world;
        auto create_result = world.CreateEntity();
        CHECK(create_result.IsOk());
        
        EntityID entity = create_result.Value();
        auto destroy_result = world.DestroyEntity(entity);
        CHECK(destroy_result.IsOk());
    }

    TEST_CASE("ComponentManager - Add component") {
        ECSWorld world;
        auto create_result = world.CreateEntity();
        CHECK(create_result.IsOk());
        
        EntityID entity = create_result.Value();
        
        struct TestComponent {
            int value;
        };
        
        auto add_result = world.AddComponent<TestComponent>(entity, TestComponent{42});
        CHECK(add_result.IsOk());
    }

    TEST_CASE("ComponentManager - Remove component") {
        ECSWorld world;
        auto create_result = world.CreateEntity();
        CHECK(create_result.IsOk());
        
        EntityID entity = create_result.Value();
        
        struct TestComponent {
            int value;
        };
        
        world.AddComponent<TestComponent>(entity, TestComponent{42});
        auto remove_result = world.RemoveComponent<TestComponent>(entity);
        CHECK(remove_result.IsOk());
    }

    TEST_CASE("System - Signature matching") {
        ECSWorld world;
        
        // Create entities with different component combinations
        auto e1_result = world.CreateEntity();
        auto e2_result = world.CreateEntity();
        auto e3_result = world.CreateEntity();
        
        CHECK(e1_result.IsOk() && e2_result.IsOk() && e3_result.IsOk());
        
        EntityID e1 = e1_result.Value();
        EntityID e2 = e2_result.Value();
        EntityID e3 = e3_result.Value();
        
        struct Position { float x, y; };
        struct Velocity { float dx, dy; };
        struct Renderable { int mesh_id; };
        
        // e1: Position + Velocity
        world.AddComponent<Position>(e1, Position{0.0f, 0.0f});
        world.AddComponent<Velocity>(e1, Velocity{1.0f, 1.0f});
        
        // e2: Position + Renderable
        world.AddComponent<Position>(e2, Position{1.0f, 1.0f});
        world.AddComponent<Renderable>(e2, Renderable{1});
        
        // e3: Position + Velocity + Renderable
        world.AddComponent<Position>(e3, Position{2.0f, 2.0f});
        world.AddComponent<Velocity>(e3, Velocity{2.0f, 2.0f});
        world.AddComponent<Renderable>(e3, Renderable{2});
        
        // Query entities with Position + Velocity
        auto query_result = world.Query<Position, Velocity>();
        CHECK(query_result.IsOk());
        
        const auto& matched_entities = query_result.Value();
        CHECK(matched_entities.size() == 2); // e1 and e3
        
        // Verify correct entities matched
        bool found_e1 = false, found_e3 = false;
        for (const auto& eid : matched_entities) {
            if (eid == e1) found_e1 = true;
            if (eid == e3) found_e3 = true;
        }
        CHECK(found_e1);
        CHECK(found_e3);
    }
}

// ============================================================================
// TEST SUITE: Threading Primitives
// ============================================================================

TEST_SUITE("ThreadingPrimitives") {
    TEST_CASE("SpinLock - Basic lock/unlock") {
        SpinLock lock;
        int counter = 0;
        
        lock.lock();
        counter++;
        lock.unlock();
        
        CHECK(counter == 1);
    }

    TEST_CASE("ConcurrentQueue - Push/Pop") {
        ConcurrentQueue<int> queue;
        
        queue.Push(1);
        queue.Push(2);
        queue.Push(3);
        
        int value;
        CHECK(queue.Pop(value));
        CHECK(value == 1);
        
        CHECK(queue.Pop(value));
        CHECK(value == 2);
        
        CHECK(queue.Pop(value));
        CHECK(value == 3);
    }

    TEST_CASE("ThreadPool - Execute task") {
        ThreadPool pool(2);
        std::atomic<int> counter{0};
        
        pool.Enqueue([&counter]() {
            counter++;
        });
        
        // Give time for task execution
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        CHECK(counter.load() >= 1);
    }

    TEST_CASE("Barrier - Synchronize threads") {
        const int num_threads = 4;
        Barrier barrier(num_threads);
        std::atomic<int> counter{0};
        
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&barrier, &counter]() {
                counter++;
                barrier.Wait();
                // After barrier, all threads should have incremented
                CHECK(counter.load() == num_threads);
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
}

// ============================================================================
// TEST SUITE: Framework Lifecycle
// ============================================================================

TEST_SUITE("FrameworkLifecycle") {
    TEST_CASE("Framework - Initialize and shutdown") {
        FrameworkConfig config;
        config.log_level = LogLevel::INFO;
        
        auto init_result = Framework::Initialize(config);
        CHECK(init_result.IsOk());
        
        auto shutdown_result = Framework::Shutdown();
        CHECK(shutdown_result.IsOk());
    }

    TEST_CASE("Framework - Double initialization fails") {
        FrameworkConfig config;
        config.log_level = LogLevel::INFO;
        
        auto init1 = Framework::Initialize(config);
        CHECK(init1.IsOk());
        
        auto init2 = Framework::Initialize(config);
        CHECK(!init2.IsOk()); // Should fail - already initialized
        
        auto shutdown_result = Framework::Shutdown();
        CHECK(shutdown_result.IsOk());
    }
}
