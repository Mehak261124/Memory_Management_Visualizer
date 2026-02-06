/**
 * NeonHeap: Memory Allocation Visualizer
 * Configuration Constants
 */

export const CONFIG = {
  HEAP_SIZE: 1024,  // KB
  OS_MEMORY: 256,   // KB (reserved for OS)
  USER_MEMORY: 768, // KB (1024 - 256)
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
