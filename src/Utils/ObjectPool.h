#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H
// Provides an object pool that can be used with any class.
//
// acquireObject() returns an object from the list of free objects. If
// there are no more free objects, acquireObject() creates a new chunk
// of objects.
// The pool only grows: objects are never removed from the pool, until
// the pool is destroyed.
// acquireObject() returns an std::shared_ptr with a custom deleter that
// automatically puts the object back into the object pool when the
// shared_ptr is destroyed and its reference count reaches 0.
#include <memory>
#include <mutex>    // For std::mutex and std::lock_guard
#include <utility>  // For std::pair
#include <vector>

// Add IsThreadSafe template parameter, defaulting to true
template <typename T, bool IsThreadSafe = false, typename Allocator = std::allocator<T>>
class ObjectPool final {
private:
  // Forward declare PoolDeleter
  struct PoolDeleter;

public:
  // Update UniqueObjPtr alias to use the correct PoolDeleter specialization
  using UniqueObjPtr = std::unique_ptr<T, PoolDeleter>;

  ObjectPool() = default;
  explicit ObjectPool(const Allocator &allocator);
  ~ObjectPool();

  // Prevent move construction and move assignment.
  ObjectPool(ObjectPool &&) = delete;
  ObjectPool &operator=(ObjectPool &&) = delete;

  // Prevent copy construction and copy assignment.
  ObjectPool(const ObjectPool &) = delete;
  ObjectPool &operator=(const ObjectPool &) = delete;

  // Modified acquire_object
  template <typename... Args>  // Use variadic template for args
  UniqueObjPtr acquire_object(Args &&... args) {
    T* obj_ptr = nullptr;

    // Lock acquisition logic only if thread-safe
    if constexpr (IsThreadSafe) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (free_objects_.empty()) {
        add_chunk();  // Called while lock is held
        if (free_objects_.empty()) { return UniqueObjPtr(nullptr, PoolDeleter(this)); }
      }
      obj_ptr = free_objects_.back();
      free_objects_.pop_back();
    } else {  // Non-thread-safe path
      if (free_objects_.empty()) {
        add_chunk();
        if (free_objects_.empty()) { return UniqueObjPtr(nullptr, PoolDeleter(this)); }
      }
      obj_ptr = free_objects_.back();
      free_objects_.pop_back();
    }

    // Placement new happens outside the main lock
    try {
      new (obj_ptr) T(std::forward<Args>(args)...);
    } catch (...) {
      // Return object slot to pool if construction fails - lock if needed
      if constexpr (IsThreadSafe) {
        std::lock_guard<std::mutex> lock(mutex_);
        free_objects_.push_back(obj_ptr);
      } else {
        free_objects_.push_back(obj_ptr);
      }
      throw;  // Rethrow the original exception
    }

    // Return unique_ptr with the custom deleter
    return UniqueObjPtr(obj_ptr, PoolDeleter(this));
  }

private:
  // Custom deleter struct - now needs the IsThreadSafe parameter implicitly via pool_ptr type
  struct PoolDeleter {
    ObjectPool* pool_ptr;  // Pointer type implicitly carries IsThreadSafe
    explicit PoolDeleter(ObjectPool* pool = nullptr) : pool_ptr(pool) {}
    void operator()(T* obj) const {
      if (pool_ptr && obj) { pool_ptr->release_object(obj); }
    }
  };

  // Method to return object to the pool
  void release_object(T* obj) {
    obj->~T();  // Call destructor explicitly BEFORE returning to pool

    // Lock release logic only if thread-safe
    if constexpr (IsThreadSafe) {
      std::lock_guard<std::mutex> lock(mutex_);
      free_objects_.push_back(obj);
    } else {
      free_objects_.push_back(obj);
    }
  }
  // Creates a new block of uninitialized memory, big enough to hold
  void add_chunk();
  // Contains chunks of memory in which instances of T will be created.
  // For each chunk, the pointer to its first object is stored.
  std::vector<std::pair<T*, std::size_t>> pool_;  // Store pointer and size
  // Contains pointers to all free instances of T that
  // are available in the pool.
  std::vector<T*> free_objects_;
  // The number of T instances that should fit in the first allocated chunk.
  static constexpr std::size_t INITIAL_CHUNK_SIZE{10000};
  // The number of T instances that should fit in a newly allocated chunk.
  // This value is doubled after each newly created chunk.
  std::size_t new_chunk_size_{INITIAL_CHUNK_SIZE};
  // The allocator to use for allocating and deallocating chunks.
  Allocator allocator_;
  // Mutex to protect shared data access
  std::mutex mutex_;
};

// Implementation of the destructor needs the template parameter now
template <typename T, bool IsThreadSafe, typename Allocator>
ObjectPool<T, IsThreadSafe, Allocator>::~ObjectPool() {
  // Iterate through all allocated chunks
  for (const auto &chunk_pair : pool_) {
    T* chunk_ptr = chunk_pair.first;
    std::size_t chunk_size = chunk_pair.second;
    if (chunk_ptr) {
      // Deallocate the raw memory chunk using the stored size
      allocator_.deallocate(chunk_ptr, chunk_size);
    }
  }
  // Vectors pool_ and free_objects_ will be cleared automatically
}

// Implementation of add_chunk needs the template parameter now
template <typename T, bool IsThreadSafe, typename Allocator>
void ObjectPool<T, IsThreadSafe, Allocator>::add_chunk() {
  T* new_chunk_ptr = nullptr;
  std::size_t current_chunk_size = new_chunk_size_;
  try {
    new_chunk_ptr = allocator_.allocate(current_chunk_size);
  } catch (...) { throw; }

  try {
    pool_.emplace_back(new_chunk_ptr, current_chunk_size);
  } catch (...) {
    allocator_.deallocate(new_chunk_ptr, current_chunk_size);
    throw;
  }

  auto old_free_objects_size = free_objects_.size();
  try {
    free_objects_.reserve(old_free_objects_size + current_chunk_size);
    free_objects_.resize(old_free_objects_size + current_chunk_size);
  } catch (...) { throw; }

  T* first_new_obj = new_chunk_ptr;
  for (size_t i = 0; i < current_chunk_size; ++i) {
    free_objects_[old_free_objects_size + i] = first_new_obj + i;
  }

  new_chunk_size_ *= 2;
}

#endif /* //OBJECTPOOL_H */
