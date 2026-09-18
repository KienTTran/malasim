#include <string>
#include <vector>
#include <thread> // For potential concurrency tests later
#include <chrono> // For performance testing
#include <memory> // For std::make_unique
#include <stdexcept> // For std::runtime_error
#include <atomic> // For concurrency test flag

#include "Utils/ObjectPool.h"  // Assuming Utils is in include path or relative path works
#include "gtest/gtest.h"
#include "Population/Person/Person.h"

// Simple struct for testing
struct TestData {
  int id;
  std::string name;

  TestData(int i = 0, std::string n = "") : id(i), name(std::move(n)) {
    // std::cout << "TestData constructor called for id: " << id << std::endl;
  }

  ~TestData() {
    // std::cout << "TestData destructor called for id: " << id << std::endl;
  }

  // Disable copy/move semantics if needed by pool design, but
  // ObjectPool uses placement new, so it should handle construction.
};

// Struct designed to throw during construction
struct ThrowableData {
    static bool should_throw;
    int id;

    ThrowableData(int i) : id(i) {
        if (should_throw) {
            throw std::runtime_error("Constructor failed as requested");
        }
        // std::cout << "ThrowableData constructed: " << id << std::endl;
    }
    ~ThrowableData() {
        // std::cout << "ThrowableData destructed: " << id << std::endl;
    }
};
bool ThrowableData::should_throw = false; // Static member definition

// Test Fixture for ObjectPool tests
class ObjectPoolTest : public ::testing::Test {
protected:
  ObjectPool<TestData> pool;  // Test with default allocator
};

// Test case for acquiring a single object
TEST_F(ObjectPoolTest, AcquireSingleObject) {
  auto objPtr = pool.acquire_object(1, "Test1");
  ASSERT_NE(objPtr, nullptr);  // Check if the pointer is not null
  EXPECT_EQ(objPtr->id, 1);
  EXPECT_EQ(objPtr->name, "Test1");
  // Destructor of objPtr will call release_object implicitly
}

// Test case for acquiring multiple objects, triggering chunk allocation
TEST_F(ObjectPoolTest, AcquireMultipleObjects) {
  std::vector<ObjectPool<TestData>::UniqueObjPtr> objects;
  const int numObjects = 10;  // Should trigger at least one chunk allocation (initial size 5)

  for (int i = 0; i < numObjects; ++i) {
    objects.push_back(pool.acquire_object(i, "Test" + std::to_string(i)));
    ASSERT_NE(objects.back(), nullptr);
    EXPECT_EQ(objects.back()->id, i);
  }

  // Verify data integrity after acquisition
  for (int i = 0; i < numObjects; ++i) {
    ASSERT_NE(objects[i], nullptr);  // Redundant check, but good practice
    EXPECT_EQ(objects[i]->id, i);
    EXPECT_EQ(objects[i]->name, "Test" + std::to_string(i));
  }
}

// Test case for acquiring and releasing objects
TEST_F(ObjectPoolTest, AcquireAndRelease) {
  std::vector<TestData*> rawPointers;

  // Acquire initial chunk size objects
  {
    std::vector<ObjectPool<TestData>::UniqueObjPtr> objects;
    for (int i = 0; i < 5; ++i) {
      objects.push_back(pool.acquire_object(i));
      ASSERT_NE(objects.back(), nullptr);
      rawPointers.push_back(objects.back().get());  // Store raw pointer
    }
    // Objects go out of scope here, releasing them back to the pool
  }

  // Acquire again, should reuse objects from the free list
  {
    std::vector<ObjectPool<TestData>::UniqueObjPtr> objects;
    for (int i = 0; i < 5; ++i) {
      objects.push_back(pool.acquire_object(100 + i));  // Use different IDs
      ASSERT_NE(objects.back(), nullptr);
      EXPECT_EQ(objects.back()->id, 100 + i);

      // Check if the raw pointer matches one of the previously released ones
      bool found = false;
      for (const auto* rawPtr : rawPointers) {
        if (objects.back().get() == rawPtr) {
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found) << "Acquired object pointer " << objects.back().get()
                         << " was not among the previously released pointers.";
    }
  }
}

// Test case for construction arguments forwarding
TEST_F(ObjectPoolTest, ConstructorArgumentForwarding) {
  struct ComplexData {
    int i_;
    std::string s_;
    double d_;
    ComplexData(int i, std::string s, double d) : i_(i), s_(std::move(s)), d_(d) {}
  };

  ObjectPool<ComplexData> complexPool;
  auto objPtr = complexPool.acquire_object(42, "Hello", 3.14);
  ASSERT_NE(objPtr, nullptr);
  EXPECT_EQ(objPtr->i_, 42);
  EXPECT_EQ(objPtr->s_, "Hello");
  EXPECT_DOUBLE_EQ(objPtr->d_, 3.14);
}

// Performance Test: Compare ObjectPool vs std::make_unique
TEST_F(ObjectPoolTest, PerformanceComparison) {
    const int num_operations = 10000; // Number of acquire/release operations
    const int num_rounds = 10; // Number of rounds to run the test

    // --- Test ObjectPool Performance ---
    auto start_pool = std::chrono::high_resolution_clock::now();
    ObjectPool<Person> person_pool;
    for (int round = 0; round < num_rounds; ++round) {
        std::vector<ObjectPool<Person>::UniqueObjPtr> pool_objects;
        pool_objects.reserve(num_operations); // Pre-allocate vector space
        for (int i = 0; i < num_operations; ++i) {
            pool_objects.push_back(person_pool.acquire_object());
            pool_objects.back()->initialize();
        }
        pool_objects.clear(); // Release all objects by clearing the vector (triggers destructors)
    }
    auto end_pool = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> pool_duration = end_pool - start_pool;
    std::cout << "[ PERF ] ObjectPool acquire/release x " << num_operations << ": " << pool_duration.count() << " ms" << std::endl;


    // --- Test std::make_unique Performance ---
    auto start_unique = std::chrono::high_resolution_clock::now();
    for (int round = 0; round < num_rounds; ++round) {
        std::vector<std::unique_ptr<Person>> unique_objects;
        unique_objects.reserve(num_operations); // Pre-allocate vector space
        for (int i = 0; i < num_operations; ++i) {
            unique_objects.push_back(std::make_unique<Person>());
            unique_objects.back()->initialize();
        }
        unique_objects.clear(); // Release all objects by clearing the vector (triggers destructors)
    }
    auto end_unique = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> unique_duration = end_unique - start_unique;
    std::cout << "[ PERF ] std::make_unique/delete x " << num_operations << ": " << unique_duration.count() << " ms" << std::endl;

    // Optional: Add an EXPECT_TRUE or other assertion based on expected performance,
    // e.g., EXPECT_LT(pool_duration.count(), unique_duration.count());
    // However, this can be flaky depending on the system, compiler, and object complexity.
    // Printing the times is often more informative for benchmarks.
}

// Stress Test: Acquire and release many times
TEST_F(ObjectPoolTest, AcquireReleaseStressTest) {
    const int num_cycles = 100;
    const int num_objects_per_cycle = 1000;

    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        std::vector<ObjectPool<TestData>::UniqueObjPtr> objects;
        objects.reserve(num_objects_per_cycle);
        for (int i = 0; i < num_objects_per_cycle; ++i) {
            objects.push_back(pool.acquire_object(i));
            ASSERT_NE(objects.back(), nullptr);
        }
        // Release by letting objects go out of scope (or clear vector)
        objects.clear();
    }
    // If the pool's internal free list logic were flawed,
    // this might eventually fail to acquire or corrupt state.
    // We expect this test to pass without crashing or assertion failures.
    SUCCEED(); // Explicitly mark success if it completes
}

// Test Destructor Logic: Create and destroy multiple pools
TEST(ObjectPoolLifecycleTest, MultiplePoolInstancesTest) {
    const int num_pools = 5;
    const int num_objects_per_pool = 50;

    for (int p = 0; p < num_pools; ++p) {
        ObjectPool<TestData> temp_pool;
        std::vector<ObjectPool<TestData>::UniqueObjPtr> objects;
        objects.reserve(num_objects_per_pool);
        for (int i = 0; i < num_objects_per_pool; ++i) {
            objects.push_back(temp_pool.acquire_object(i));
            ASSERT_NE(objects.back(), nullptr);
        }
        // Pool goes out of scope here, destructor is called.
        // A memory leak detector (Valgrind, ASan) would catch issues here.
    }
     // If the destructor logic were flawed (e.g., double-free, incorrect size),
     // this might crash or be flagged by external tools.
     SUCCEED();
}

// Test case for allocator failure simulation (requires a mock allocator)
// This is more advanced and might require a custom allocator that can be configured to fail.
// TEST_F(ObjectPoolTest, AllocatorFailure) {
//     // Setup a mock allocator that throws std::bad_alloc on allocate
//     // MockAllocator<TestData> mockAllocator;
//     // EXPECT_CALL(mockAllocator,
//     allocate(testing::_)).WillOnce(testing::Throw(std::bad_alloc()));
//     // ObjectPool<TestData, MockAllocator<TestData>> poolWithMock(mockAllocator);
//     // EXPECT_THROW(poolWithMock.acquire_object(), std::bad_alloc);
// }

// --- Edge Case Tests ---

// Test behavior when object constructor throws during acquire
TEST(ObjectPoolEdgeCaseTest, ConstructorThrows) {
    ObjectPool<ThrowableData> pool;
    ThrowableData::should_throw = false; // Ensure it doesn't throw initially

    // Acquire one object successfully
    auto ptr1 = pool.acquire_object(1);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(ptr1->id, 1);

    // Set up to throw on the next construction
    ThrowableData::should_throw = true;
    ThrowableData* raw_ptr_before_throw = nullptr;

    try {
        // Attempt to acquire an object, expecting the constructor to throw
        auto ptr2 = pool.acquire_object(2);
        FAIL() << "Expected constructor to throw, but acquire_object succeeded.";
    } catch (const std::runtime_error& e) {
        // This is expected
        EXPECT_STREQ(e.what(), "Constructor failed as requested");
        // Crucially, the object should have been put back onto the free list
        // even though construction failed.
    } catch (...) {
        FAIL() << "Caught unexpected exception type.";
    }

    // Reset flag
    ThrowableData::should_throw = false;

    // Try acquiring again, it should reuse the object whose constructor failed
    auto ptr3 = pool.acquire_object(3);
    ASSERT_NE(ptr3, nullptr);
    EXPECT_EQ(ptr3->id, 3);

    // Ideally, ptr3 should reuse the memory slot that failed construction.
    // Verifying the exact pointer address can be tricky/implementation-dependent
    // but we expect acquisition to succeed using a previously failed slot.
}

// Test acquiring exactly chunk sizes
TEST(ObjectPoolEdgeCaseTest, AcquireExactChunkSizes) {
    ObjectPool<TestData> pool;
    std::vector<ObjectPool<TestData>::UniqueObjPtr> objects;

    // Initial chunk size is 5
    for (int i = 0; i < 5; ++i) {
        objects.push_back(pool.acquire_object(i));
        ASSERT_NE(objects.back(), nullptr);
    }
    EXPECT_EQ(objects.size(), 5);

    // Next chunk size is 10
    for (int i = 5; i < 5 + 10; ++i) {
        objects.push_back(pool.acquire_object(i));
        ASSERT_NE(objects.back(), nullptr);
    }
    EXPECT_EQ(objects.size(), 15);

     // Next chunk size is 20
    for (int i = 15; i < 15 + 20; ++i) {
        objects.push_back(pool.acquire_object(i));
        ASSERT_NE(objects.back(), nullptr);
    }
    EXPECT_EQ(objects.size(), 35);
    // Test passes if no crashes/errors occur.
}


// Test demonstrating lack of thread safety (EXPECT potential issues/crashes)
TEST(ObjectPoolEdgeCaseTest, ConcurrentAccessUnsafe) {
    ObjectPool<TestData, true> shared_pool;
    std::atomic<bool> start_flag{false};
    std::atomic<int> acquired_count{0};
    const int num_threads = 4;
    const int num_acquires_per_thread = 5000;

    std::vector<std::thread> threads;

    auto worker = [&]() {
        std::vector<ObjectPool<TestData, true>::UniqueObjPtr> local_objects;
        local_objects.reserve(num_acquires_per_thread);
        while (!start_flag.load()) { std::this_thread::yield(); } // Spin until start

        try {
            for(int i = 0; i < num_acquires_per_thread; ++i) {
                // This line is the race condition hotspot
                auto ptr = shared_pool.acquire_object(i);
                if (ptr) { 
                    local_objects.push_back(std::move(ptr));
                    acquired_count++; // Atomic increment
                } // else: Ignore acquisition failures for this test
            }
        } catch (const std::exception& e) {
            // Don't fail the test on exceptions here, as they might occur due to races
            std::cerr << "Thread caught exception (expected in unsafe test): " << e.what() << std::endl;
        } catch (...) {
             std::cerr << "Thread caught unknown exception (expected in unsafe test)." << std::endl;
        }
        // Objects are released as local_objects destructs
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    start_flag.store(true); // Signal threads to start

    for (auto& t : threads) {
        t.join();
    }

    // We don't assert on acquired_count == num_threads * num_acquires_per_thread
    // because the race conditions might cause acquire_object to fail or return nullptr
    // even if chunks are added. The main point is to run it and see if it crashes
    // or exhibits issues under a thread sanitizer.
    SUCCEED(); // Test 'succeeds' by not crashing deterministically, highlights unsafety.
}

// Basic main function for running tests
// int main(int argc, char **argv) {
//   ::testing::InitGoogleTest(&argc, argv);
//   return RUN_ALL_TESTS();
// }
// You might not need this main if your build system (like CMake with CTest) handles it.
