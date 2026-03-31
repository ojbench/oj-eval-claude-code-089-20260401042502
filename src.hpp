// Copyright (c) 2024 ACM Class, SJTU

namespace sjtu {

// Simple linked list node for free blocks (no STL)
struct FreeNode {
  int addr;
  FreeNode* next;

  FreeNode(int a) : addr(a), next(nullptr) {}
};

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

    // Initialize free lists for each layer
    free_lists_ = new FreeNode*[num_layers_];
    for (int i = 0; i < num_layers_; i++) {
      free_lists_[i] = nullptr;
    }

    // Add initial blocks to top layer
    int top_layer = num_layers_ - 1;
    int block_size = min_block_size;
    for (int i = 0; i < num_layers_ - 1; i++) {
      block_size *= 2;
    }

    // Add all top-level blocks as free
    for (int addr = 0; addr < ram_size; addr += block_size) {
      addFreeBlock(top_layer, addr);
    }
  }

  ~BuddyAllocator() {
    // Clean up free lists
    for (int i = 0; i < num_layers_; i++) {
      FreeNode* curr = free_lists_[i];
      while (curr) {
        FreeNode* next = curr->next;
        delete curr;
        curr = next;
      }
    }
    delete[] free_lists_;
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

    // Find minimum aligned address
    for (int addr = 0; addr < ram_size_; addr += size) {
      int result = malloc_at(addr, size);
      if (result != -1) {
        return result;
      }
    }

    return -1;
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

    // Try to allocate at this specific address
    if (allocateAt(addr, layer)) {
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

    // Add block back to free list
    addFreeBlock(layer, addr);

    // Try to merge with buddy
    mergeBlocks(layer, addr, size);
  }

private:
  int ram_size_;
  int min_block_size_;
  int num_layers_;
  FreeNode** free_lists_;  // Array of free lists for each layer

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

  // Add a free block to the free list
  void addFreeBlock(int layer, int addr) {
    FreeNode* node = new FreeNode(addr);

    // Insert in sorted order
    if (!free_lists_[layer] || free_lists_[layer]->addr > addr) {
      node->next = free_lists_[layer];
      free_lists_[layer] = node;
    } else {
      FreeNode* curr = free_lists_[layer];
      while (curr->next && curr->next->addr < addr) {
        curr = curr->next;
      }
      node->next = curr->next;
      curr->next = node;
    }
  }

  // Remove a free block from the free list
  bool removeFreeBlock(int layer, int addr) {
    if (!free_lists_[layer]) return false;

    if (free_lists_[layer]->addr == addr) {
      FreeNode* node = free_lists_[layer];
      free_lists_[layer] = node->next;
      delete node;
      return true;
    }

    FreeNode* curr = free_lists_[layer];
    while (curr->next && curr->next->addr != addr) {
      curr = curr->next;
    }

    if (curr->next && curr->next->addr == addr) {
      FreeNode* node = curr->next;
      curr->next = node->next;
      delete node;
      return true;
    }

    return false;
  }

  // Check if a block is free
  bool isFree(int layer, int addr) {
    FreeNode* curr = free_lists_[layer];
    while (curr) {
      if (curr->addr == addr) return true;
      curr = curr->next;
    }
    return false;
  }

  // Check if a range is completely covered by free blocks at a given layer
  bool canAllocateRange(int start_addr, int end_addr, int layer) {
    int block_size = getBlockSize(layer);
    for (int addr = start_addr; addr < end_addr; addr += block_size) {
      if (!isFree(layer, addr)) {
        return false;
      }
    }
    return true;
  }

  // Allocate at a specific address by recursively splitting
  bool allocateAt(int addr, int target_layer) {
    int size = getBlockSize(target_layer);

    // Check if already free at this layer
    if (isFree(target_layer, addr)) {
      removeFreeBlock(target_layer, addr);
      return true;
    }

    // Need to split from upper layers
    // Find which parent block contains this address
    for (int layer = target_layer + 1; layer < num_layers_; layer++) {
      int parent_size = getBlockSize(layer);
      int parent_addr = (addr / parent_size) * parent_size;

      if (isFree(layer, parent_addr)) {
        // Split this parent block down to target layer
        return splitToLayer(parent_addr, layer, target_layer, addr);
      }
    }

    return false;
  }

  // Split a block from upper layer down to target layer, ensuring addr is freed
  bool splitToLayer(int block_addr, int from_layer, int to_layer, int target_addr) {
    if (from_layer == to_layer) {
      if (block_addr == target_addr && isFree(from_layer, block_addr)) {
        removeFreeBlock(from_layer, block_addr);
        return true;
      }
      return false;
    }

    // Remove block from current layer
    if (!removeFreeBlock(from_layer, block_addr)) {
      return false;
    }

    // Split into two children
    int child_layer = from_layer - 1;
    int child_size = getBlockSize(child_layer);
    int left_child = block_addr;
    int right_child = block_addr + child_size;

    // Add both children as free
    addFreeBlock(child_layer, left_child);
    addFreeBlock(child_layer, right_child);

    // Continue splitting the child that contains target_addr
    int target_child = (target_addr >= right_child) ? right_child : left_child;

    if (child_layer == to_layer) {
      if (target_child == target_addr) {
        removeFreeBlock(child_layer, target_addr);
        return true;
      }
      return false;
    }

    return splitToLayer(target_child, child_layer, to_layer, target_addr);
  }

  // Merge with buddy block
  void mergeBlocks(int layer, int addr, int size) {
    if (layer >= num_layers_ - 1) return;

    // Calculate buddy address
    int buddy_addr = addr ^ size;

    // Check if buddy is also free and within bounds
    if (buddy_addr >= 0 && buddy_addr < ram_size_ && isFree(layer, buddy_addr)) {
      // Remove both blocks
      removeFreeBlock(layer, addr);
      removeFreeBlock(layer, buddy_addr);

      // Add merged block to upper layer
      int parent_addr = (addr < buddy_addr) ? addr : buddy_addr;
      int parent_layer = layer + 1;
      int parent_size = size * 2;
      addFreeBlock(parent_layer, parent_addr);

      // Recursively try to merge
      mergeBlocks(parent_layer, parent_addr, parent_size);
    }
  }
};

} // namespace sjtu
