// Copyright (c) 2024 ACM Class, SJTU

namespace sjtu {

class BuddyAllocator {
public:
  /**
   * @brief Construct a new Buddy Allocator object with the given RAM size and
   * minimum block size.
   *
   * @param ram_size Size of the RAM. The address space is 0 ~ ram_size - 1.
   * @param min_block_size Minimum size of a block. The block size is 2^k where
   * k >= min_block_size.
   */
  BuddyAllocator(int ram_size, int min_block_size)
    : ram_size_(ram_size), min_block_size_(min_block_size) {
    // Calculate number of layers
    num_layers_ = 0;
    int size = min_block_size;
    while (size <= ram_size) {
      num_layers_++;
      size *= 2;
    }

    // Initialize allocation bitmap for each layer
    allocated_ = new bool*[num_layers_];
    num_blocks_ = new int[num_layers_];

    for (int i = 0; i < num_layers_; i++) {
      int block_size = getBlockSize(i);
      num_blocks_[i] = ram_size / block_size;
      allocated_[i] = new bool[num_blocks_[i]];
      for (int j = 0; j < num_blocks_[i]; j++) {
        allocated_[i][j] = false;
      }
    }

    // Mark all blocks except top layer as allocated initially
    for (int layer = 0; layer < num_layers_ - 1; layer++) {
      for (int idx = 0; idx < num_blocks_[layer]; idx++) {
        allocated_[layer][idx] = true;
      }
    }
  }

  ~BuddyAllocator() {
    for (int i = 0; i < num_layers_; i++) {
      delete[] allocated_[i];
    }
    delete[] allocated_;
    delete[] num_blocks_;
  }

  /**
   * @brief Allocate a block with the given size at the minimum available
   * address.
   *
   * @param size The size of the block.
   * @return int The address of the block. Return -1 if the block cannot be
   * allocated.
   */
  int malloc(int size) {
    int layer = getLayer(size);
    if (layer == -1) return -1;

    int block_size = getBlockSize(layer);
    int min_addr = -1;
    int min_layer = -1;

    // Check current layer for minimum free address
    for (int idx = 0; idx < num_blocks_[layer]; idx++) {
      if (!allocated_[layer][idx]) {
        min_addr = idx * block_size;
        min_layer = layer;
        break; // Found minimum at this layer
      }
    }

    // Check upper layers for potentially smaller addresses
    for (int upper_layer = layer + 1; upper_layer < num_layers_; upper_layer++) {
      int upper_block_size = getBlockSize(upper_layer);
      for (int idx = 0; idx < num_blocks_[upper_layer]; idx++) {
        if (!allocated_[upper_layer][idx]) {
          int addr = idx * upper_block_size;
          if (min_addr == -1 || addr < min_addr) {
            min_addr = addr;
            min_layer = upper_layer;
          }
          break; // Found minimum at this layer
        }
      }
    }

    if (min_addr == -1) {
      return -1;
    }

    // Allocate from the found layer
    if (min_layer == layer) {
      markAllocated(min_addr, layer);
      return min_addr;
    } else {
      // Split from upper layer
      splitDown(min_addr, min_layer, layer, min_addr);
      return min_addr;
    }
  }

  /**
   * @brief Allocate a block with the given size at the given address.
   *
   * @param addr The address of the block.
   * @param size The size of the block.
   * @return int The address of the block. Return -1 if the block cannot be
   * allocated.
   */
  int malloc_at(int addr, int size) {
    int layer = getLayer(size);
    if (layer == -1) return -1;

    if (tryAllocate(addr, layer)) {
      return addr;
    }

    return -1;
  }

  /**
   * @brief Deallocate a block with the given size at the given address.
   *
   * @param addr The address of the block. It is ensured that the block is
   * allocated before.
   * @param size The size of the block.
   */
  void free_at(int addr, int size) {
    int layer = getLayer(size);
    if (layer == -1) return;

    int block_size = getBlockSize(layer);
    int idx = addr / block_size;
    allocated_[layer][idx] = false;

    // Try to merge with buddy
    tryMerge(addr, layer);
  }

private:
  int ram_size_;
  int min_block_size_;
  int num_layers_;
  bool** allocated_;  // allocated_[layer][block_index]
  int* num_blocks_;   // number of blocks at each layer

  // Get layer index for a given size
  int getLayer(int size) {
    if (size < min_block_size_) return -1;

    int layer = 0;
    int block_size = min_block_size_;
    while (block_size < size) {
      block_size *= 2;
      layer++;
    }

    if (layer >= num_layers_) return -1;
    return layer;
  }

  // Get block size for a given layer
  int getBlockSize(int layer) {
    int size = min_block_size_;
    for (int i = 0; i < layer; i++) {
      size *= 2;
    }
    return size;
  }

  // Check if a block is free (not allocated)
  bool isFree(int addr, int layer) {
    int block_size = getBlockSize(layer);
    int idx = addr / block_size;
    if (idx >= num_blocks_[layer]) return false;
    return !allocated_[layer][idx];
  }

  // Mark a block as allocated
  void markAllocated(int addr, int layer) {
    int block_size = getBlockSize(layer);
    int idx = addr / block_size;
    allocated_[layer][idx] = true;
  }

  // Mark a block as free
  void markFree(int addr, int layer) {
    int block_size = getBlockSize(layer);
    int idx = addr / block_size;
    allocated_[layer][idx] = false;
  }

  // Try to allocate at a specific address
  bool tryAllocate(int addr, int target_layer) {
    // Check if already free at this layer
    if (isFree(addr, target_layer)) {
      markAllocated(addr, target_layer);
      return true;
    }

    // Need to split from upper layers
    for (int layer = target_layer + 1; layer < num_layers_; layer++) {
      int parent_size = getBlockSize(layer);
      int parent_addr = (addr / parent_size) * parent_size;

      if (isFree(parent_addr, layer)) {
        // Split down to target layer
        splitDown(parent_addr, layer, target_layer, addr);
        return true;
      }
    }

    return false;
  }

  // Split a block from upper layer down to target layer
  void splitDown(int block_addr, int from_layer, int to_layer, int target_addr) {
    if (from_layer == to_layer) {
      markAllocated(target_addr, to_layer);
      return;
    }

    // Mark current block as allocated (no longer free)
    markAllocated(block_addr, from_layer);

    // Split into two children
    int child_layer = from_layer - 1;
    int child_size = getBlockSize(child_layer);
    int left_child = block_addr;
    int right_child = block_addr + child_size;

    // Mark both children as free
    markFree(left_child, child_layer);
    markFree(right_child, child_layer);

    // Continue splitting the child that contains target_addr
    int target_child = (target_addr >= right_child) ? right_child : left_child;

    if (child_layer == to_layer) {
      markAllocated(target_addr, to_layer);
    } else {
      splitDown(target_child, child_layer, to_layer, target_addr);
    }
  }

  // Try to merge with buddy
  void tryMerge(int addr, int layer) {
    if (layer >= num_layers_ - 1) return;

    int size = getBlockSize(layer);
    int buddy_addr = addr ^ size;

    // Check if buddy is free and in bounds
    if (buddy_addr >= 0 && buddy_addr < ram_size_ && isFree(buddy_addr, layer)) {
      // Mark both as allocated at current layer (removing them)
      markAllocated(addr, layer);
      markAllocated(buddy_addr, layer);

      // Mark parent as free
      int parent_addr = (addr < buddy_addr) ? addr : buddy_addr;
      int parent_layer = layer + 1;
      markFree(parent_addr, parent_layer);

      // Recursively try to merge
      tryMerge(parent_addr, parent_layer);
    }
  }
};

} // namespace sjtu
