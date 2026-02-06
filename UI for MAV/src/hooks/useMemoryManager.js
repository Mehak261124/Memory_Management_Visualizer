/**
 * useMemoryManager Hook
 * React hook wrapper for MemoryManager to trigger re-renders on state changes
 */

import { useState, useCallback } from 'react';
import { MemoryManager } from '../models/MemoryManager';
import { CONFIG } from '../config/constants';

export function useMemoryManager() {
  const [manager] = useState(() => new MemoryManager(CONFIG.HEAP_SIZE, CONFIG.OS_MEMORY));
  const [, forceUpdate] = useState(0);

  const refresh = useCallback(() => {
    forceUpdate(n => n + 1);
  }, []);

  const allocate = useCallback((size, processName, algorithm) => {
    const result = manager.allocate(size, processName, algorithm);
    refresh();
    return result;
  }, [manager, refresh]);

  const deallocate = useCallback((processId) => {
    const result = manager.deallocate(processId);
    refresh();
    return result;
  }, [manager, refresh]);

  const reset = useCallback(() => {
    manager.reset();
    refresh();
  }, [manager, refresh]);

  const restoreState = useCallback((index) => {
    const result = manager.restoreState(index);
    refresh();
    return result;
  }, [manager, refresh]);

  return {
    manager,
    blocks: manager.blocks,
    history: manager.history,
    stats: manager.getStats(),
    allocate,
    deallocate,
    reset,
    restoreState,
    getFreeBlocks: () => manager.getFreeBlocks(),
    getAllocatedBlocks: () => manager.getAllocatedBlocks(),
    firstFit: (size) => manager.firstFit(size),
    bestFit: (size) => manager.bestFit(size),
    worstFit: (size) => manager.worstFit(size),
  };
}
