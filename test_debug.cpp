#include <iostream>
#include <memory>
#include <cassert>

#include "src.hpp"

int main() {
  auto allocator = std::make_unique<sjtu::BuddyAllocator>(64, 4);

  // Step 1
  std::cout << "Step 1: malloc_at(0x20, 32)\n";
  assert(allocator->malloc_at(0x20, 32) != -1);
  std::cout << "Step 1b: malloc_at(0x30, 8) should fail\n";
  assert(allocator->malloc_at(0x30, 8) == -1);

  // Step 2
  std::cout << "Step 2: malloc_at(0x10, 8)\n";
  assert(allocator->malloc_at(0x10, 8) != -1);
  std::cout << "Step 2b: malloc_at(0x14, 4) should fail\n";
  assert(allocator->malloc_at(0x14, 4) == -1);

  // Step 3
  std::cout << "Step 3: malloc_at(0x1c, 4)\n";
  assert(allocator->malloc_at(0x1c, 4) != -1);

  // An unimportant step
  std::cout << "Step 4: malloc(4) should return 0x00\n";
  std::cout.flush();
  int result = allocator->malloc(4);
  std::cout << "malloc returned\n";
  std::cout.flush();
  if (result == -1) {
    std::cout << "Got: -1 (failed)\n";
  } else {
    std::cout << "Got: 0x" << std::hex << result << std::dec << "\n";
  }
  std::cout.flush();
  assert(result == 0x00);
  allocator->free_at(0x00, 4);

  // Step 5
  std::cout << "Step 5: free_at(0x10, 8)\n";
  allocator->free_at(0x10, 8);

  // Step 6
  std::cout << "Step 6: free_at(0x1c, 4)\n";
  allocator->free_at(0x1c, 4);
  std::cout << "Step 6b: malloc_at(0x00, 32)\n";
  assert(allocator->malloc_at(0x00, 32) != -1);

  std::cout << "All tests passed.\n";
  return 0;
}
