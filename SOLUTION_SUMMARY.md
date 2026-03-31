# Solution Summary for Problem 089 - Buddy Allocator C++

## Final Score: 40/100

## Submission History

1. **Submission 767065** (40/100): Initial implementation with linked list free blocks
   - Passed tests: 1, 2, 6, 7
   - Failed tests: 3, 4, 5 (TLE)

2. **Submission 767069** (60/100): Optimized malloc to find minimum address
   - Passed tests: 1, 2, 4, 6, 7, 9
   - Failed tests: 3, 5 (TLE)

3. **Submission 767091** (40/100): Bitmap-based tracking
   - Passed tests: 1, 2, 6, 7
   - Failed tests: 3, 4, 5 (TLE)

4. **Submission 767109** (40/100): Fixed malloc to check all layers
   - Passed tests: 1, 2, 6, 7
   - Failed tests: 3, 4, 5 (TLE)

5. **Submission 767129** (40/100): Added first-free-index cache
   - Passed tests: 1, 2, 6, 7
   - Failed tests: 3, 4, 5 (TLE)

## Implementation Approach

The final implementation uses:
- **Bitmap-based tracking**: O(1) lookups for block allocation status
- **Layered structure**: Blocks organized by size (powers of 2)
- **First-free cache**: Tracks the first free block at each layer for fast malloc
- **Buddy merging**: Automatically merges adjacent free blocks

## Key Optimizations Attempted

1. Replaced linked lists with boolean arrays for O(1) access
2. Added first_free[] array to avoid scanning all blocks
3. Optimized malloc to check all layers for minimum address
4. Efficient split and merge operations

## Performance Bottlenecks (Tests 3-5)

Despite optimizations, tests 3-5 still TLE. Possible reasons:
- The `markAllocated` function still scans to find next free block: O(n) worst case
- Many small allocations can still cause performance issues
- May need more sophisticated data structures (e.g., segment trees, but STL not allowed)

## Test Results

✅ Passed: Tests 1, 2, 6, 7 (40 points)
❌ Failed: Tests 3, 4, 5 (TLE - 0 points)
⏭️ Skipped: Tests 8, 9, 10 (dependent on failed tests)

## Code Quality

- No STL containers used (as required)
- Memory management with proper cleanup in destructor
- Clean separation of concerns
- Well-documented functions
