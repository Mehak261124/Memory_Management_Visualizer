/**
 * useMemoryManager Hook (C Backend Connected)
 * 
 * This hook communicates with the C backend HTTP API server
 * running on localhost:8080. All memory operations (allocate,
 * deallocate, compact, buddy system) are handled by the C server.
 * 
 * The hook fetches blocks and stats from the server after each
 * operation and exposes them to React components.
 * 
 * For ComparisonModal (which needs manager.clone()), we keep
 * the JS MemoryManager class as a fallback for read-only simulations.
 */

import { useState, useCallback, useEffect } from 'react';
import { MemoryManager } from '../models/MemoryManager';
import { CONFIG } from '../config/constants';
import * as api from '../services/api';

export function useMemoryManager() {
  // JS MemoryManager for clone()-based comparison simulations only
  const [jsManager] = useState(() => new MemoryManager(CONFIG.HEAP_SIZE, CONFIG.OS_MEMORY));

  // State synced from C backend
  const [blocks, setBlocks] = useState([]);
  const [stats, setStats] = useState({
    totalMemory: CONFIG.HEAP_SIZE,
    osMemory: CONFIG.OS_MEMORY,
    userMemory: CONFIG.HEAP_SIZE - CONFIG.OS_MEMORY,
    usedMemory: 0,
    freeMemory: CONFIG.HEAP_SIZE - CONFIG.OS_MEMORY,
    fragmentation: '0.0',
    totalAllocations: 0,
    totalDeallocations: 0,
    totalCompactions: 0,
    largestHole: CONFIG.HEAP_SIZE - CONFIG.OS_MEMORY,
    numHoles: 1,
    avgHoleSize: CONFIG.HEAP_SIZE - CONFIG.OS_MEMORY,
    numProcesses: 0,
  });
  const [history, setHistory] = useState([{ label: 'Initial State', time: Date.now() }]);
  const [useBuddySystem, setUseBuddySystemState] = useState(false);

  /**
   * Sync state from the C backend
   * Fetches blocks and stats, normalizes them for the frontend
   */
  const syncFromBackend = useCallback(async () => {
    try {
      const [backendBlocks, backendStats] = await Promise.all([
        api.getBlocks(),
        api.getStats(),
      ]);

      // Transform C backend blocks to match frontend MemoryBlock format
      // C returns: { id, startAddress, endAddress, size, isHole, processId, blockID, buddyID }
      // Frontend expects: { id, startAddress, size, processId, isAllocated }
      const normalizedBlocks = backendBlocks
        .filter(b => b.processId !== 'OS') // Skip OS block (rendered separately)
        .map(b => ({
          id: b.blockID || b.id,
          startAddress: b.startAddress,
          size: b.size,
          processId: b.processId,
          isAllocated: !b.isHole,
          // Computed property to match MemoryBlock class
          get endAddress() { return this.startAddress + this.size - 1; },
        }));

      setBlocks(normalizedBlocks);

      // Normalize stats
      const normalizedStats = {
        totalMemory: backendStats.totalMemory,
        osMemory: backendStats.osMemory,
        userMemory: backendStats.userMemory,
        usedMemory: backendStats.usedMemory,
        freeMemory: backendStats.freeMemory,
        fragmentation: Number(backendStats.fragmentation).toFixed(1),
        totalAllocations: backendStats.totalAllocations,
        totalDeallocations: backendStats.totalDeallocations,
        totalCompactions: backendStats.totalCompactions,
        largestHole: backendStats.largestHole,
        numHoles: backendStats.numHoles,
        avgHoleSize: backendStats.numHoles > 0
          ? Math.round(backendStats.freeMemory / backendStats.numHoles)
          : 0,
        numProcesses: backendStats.numProcesses,
      };

      setStats(normalizedStats);
      setUseBuddySystemState(backendStats.useBuddySystem || false);

      return { blocks: normalizedBlocks, stats: normalizedStats };
    } catch (err) {
      console.error('[useMemoryManager] Failed to sync from backend:', err);
      return null;
    }
  }, []);

  // Initial sync on mount
  useEffect(() => {
    syncFromBackend();
  }, [syncFromBackend]);

  /**
   * Add entry to history timeline
   */
  const addHistory = useCallback((label) => {
    setHistory(prev => [...prev, { label, time: Date.now() }]);
  }, []);

  // ========== Core Operations ==========

  const allocate = useCallback(async (size, processName, algorithm) => {
    // Map algorithm names: frontend uses camelCase, backend uses snake_case
    const algoMap = {
      firstFit: 'first_fit',
      bestFit: 'best_fit',
      worstFit: 'worst_fit',
    };
    const backendAlgo = algoMap[algorithm] || 'first_fit';

    try {
      const result = await api.allocateMemory(size, backendAlgo);
      await syncFromBackend();

      if (result.success) {
        // Also sync JS manager for comparison modal support
        jsManager.allocate(size, processName || result.processId, algorithm);
        addHistory(`Allocated ${processName || result.processId} (${size} KB) [${algorithm}]`);
      }

      return result;
    } catch (err) {
      console.error('[allocate] Error:', err);
      return { success: false, message: 'Server connection failed' };
    }
  }, [syncFromBackend, jsManager, addHistory]);

  const deallocate = useCallback(async (processId) => {
    // Extract numeric ID from "P3" → 3
    const numericId = typeof processId === 'string'
      ? parseInt(processId.replace(/^P/, ''), 10)
      : processId;

    try {
      const result = await api.deallocateMemory(numericId);
      await syncFromBackend();

      if (result.success) {
        jsManager.deallocate(processId);
        addHistory(`Deallocated ${processId}`);
      }

      return result;
    } catch (err) {
      console.error('[deallocate] Error:', err);
      return { success: false, message: 'Server connection failed' };
    }
  }, [syncFromBackend, jsManager, addHistory]);

  const reset = useCallback(async () => {
    try {
      await api.resetMemory();
      await syncFromBackend();
      jsManager.reset();
      setHistory([{ label: 'Initial State', time: Date.now() }]);
    } catch (err) {
      console.error('[reset] Error:', err);
    }
  }, [syncFromBackend, jsManager]);

  const compact = useCallback(async () => {
    try {
      const result = await api.compactMemory();
      await syncFromBackend();
      addHistory('Compaction performed');
      return result;
    } catch (err) {
      console.error('[compact] Error:', err);
      return null;
    }
  }, [syncFromBackend, addHistory]);

  const autoCompact = useCallback(async (threshold) => {
    try {
      const result = await api.autoCompactMemory(threshold);
      await syncFromBackend();
      addHistory(`Auto-compact (threshold: ${threshold}%)`);
      return result;
    } catch (err) {
      console.error('[autoCompact] Error:', err);
      return null;
    }
  }, [syncFromBackend, addHistory]);

  const convertToBuddySystem = useCallback(async () => {
    try {
      const result = await api.convertToBuddySystem();
      await syncFromBackend();
      addHistory('Converted to Buddy System');
      return result;
    } catch (err) {
      console.error('[convertToBuddySystem] Error:', err);
      return null;
    }
  }, [syncFromBackend, addHistory]);

  const revertFromBuddySystem = useCallback(async () => {
    try {
      const result = await api.revertFromBuddySystem();
      await syncFromBackend();
      addHistory('Reverted from Buddy System');
      return result;
    } catch (err) {
      console.error('[revertFromBuddySystem] Error:', err);
      return null;
    }
  }, [syncFromBackend, addHistory]);

  const setBuddySystem = useCallback(async (enabled) => {
    if (enabled) {
      await convertToBuddySystem();
    } else {
      await revertFromBuddySystem();
    }
  }, [convertToBuddySystem, revertFromBuddySystem]);

  const restoreState = useCallback((index) => {
    // History restore is only for the JS manager's timeline display
    // The C backend doesn't track history states
    console.log('[restoreState] Timeline marker:', index);
  }, []);

  // ========== Manager-compatible proxy object ==========
  // Components like ComparisonModal, BuddySystemModal, CompactionModal
  // access manager.osMemory, manager.totalMemory, manager.blocks,
  // manager.getAllocatedBlocks(), manager.getStats(), manager.clone()

  const manager = {
    // These values come from the C backend which dynamically detects
    // real system RAM using sysconf(_SC_PHYS_PAGES) × sysconf(_SC_PAGE_SIZE)
    get osMemory() { return stats.osMemory; },
    get totalMemory() { return stats.totalMemory; },
    get userMemory() { return stats.userMemory; },
    useBuddySystem,
    blocks,
    
    getAllocatedBlocks() {
      return blocks.filter(b => b.isAllocated);
    },
    
    getFreeBlocks() {
      return blocks.filter(b => !b.isAllocated);
    },
    
    getStats() {
      return stats;
    },

    // clone() returns the JS MemoryManager for comparison simulations
    // This runs entirely in JS — no backend calls needed
    clone() {
      return jsManager.clone();
    },
  };

  // ========== Return values matching original hook API ==========

  return {
    manager,
    blocks,
    history,
    stats,
    allocate,
    deallocate,
    reset,
    restoreState,
    getFreeBlocks: () => blocks.filter(b => !b.isAllocated),
    getAllocatedBlocks: () => blocks.filter(b => b.isAllocated),
    firstFit: (size) => jsManager.firstFit(size),
    bestFit: (size) => jsManager.bestFit(size),
    worstFit: (size) => jsManager.worstFit(size),
    setBuddySystem,
    compact,
    autoCompact,
    convertToBuddySystem,
    revertFromBuddySystem,
    refresh: syncFromBackend,
  };
}
