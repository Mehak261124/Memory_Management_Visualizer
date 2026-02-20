/**
 * NeonHeap: Memory Allocation Visualizer
 * Configuration Constants
 * 
 * NOTE: HEAP_SIZE, OS_MEMORY, USER_MEMORY are DEFAULT values.
 * They are overridden at runtime by the C backend which
 * dynamically detects real system RAM using:
 *   sysconf(_SC_PHYS_PAGES) × sysconf(_SC_PAGE_SIZE)
 * 
 * The actual values come from /api/stats after the first sync.
 */

export const CONFIG = {
  HEAP_SIZE: 1024,  // KB (default — overridden by backend detection)
  OS_MEMORY: 256,   // KB (default — overridden by backend detection)
  USER_MEMORY: 768, // KB (default — overridden by backend detection)
  MIN_BLOCK_SIZE: 16,
  MAX_BLOCK_SIZE: 512,
  WINDOW_FLICKER_CHANCE: 0.3,
  PARTICLE_COUNT: 30,
  HIGH_FRAG_THRESHOLD: 30,
  CRITICAL_FRAG_THRESHOLD: 50
};

export const ALGORITHM_NAMES = {
  firstFit: 'First Fit',
  bestFit: 'Best Fit',
  worstFit: 'Worst Fit'
};
