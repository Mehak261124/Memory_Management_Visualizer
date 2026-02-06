/**
 * MemoryManager Class
 * Handles all memory allocation operations (matches C implementation)
 */

import { MemoryBlock } from './MemoryBlock';
import { CONFIG } from '../config/constants';

export class MemoryManager {
  constructor(totalMem = CONFIG.HEAP_SIZE, osMem = CONFIG.OS_MEMORY) {
    this.totalMemory = totalMem;
    this.osMemory = osMem;
    this.userMemory = totalMem - osMem;
    this.blocks = [];
    this.history = [];
    this.processCounter = 0;
    this.blockIdCounter = 0;
    this.totalAllocations = 0;
    this.totalDeallocations = 0;
    
    // Initialize with one free block (entire user memory)
    this.blocks.push(new MemoryBlock(
      this.blockIdCounter++,
      osMem,
      this.userMemory,
      null,
      false
    ));
    
    this.saveState('Initial State');
  }

  // Get all free blocks (holes)
  getFreeBlocks() {
    return this.blocks.filter(b => !b.isAllocated);
  }

  // Get all allocated blocks
  getAllocatedBlocks() {
    return this.blocks.filter(b => b.isAllocated);
  }

  // Calculate fragmentation percentage (MATCHES C CODE)
  calculateFragmentation() {
    const freeBlocks = this.getFreeBlocks();
    if (freeBlocks.length === 0) return 0;
    
    const totalFree = freeBlocks.reduce((sum, b) => sum + b.size, 0);
    if (totalFree === 0) return 0;
    
    const largestFree = Math.max(...freeBlocks.map(b => b.size));
    
    // Formula from C code: (totalFree - largestHole) / userMemory * 100
    const fragmentedMemory = totalFree - largestFree;
    return ((fragmentedMemory / this.userMemory) * 100).toFixed(1);
  }

  // Get statistics
  getStats() {
    const freeBlocks = this.getFreeBlocks();
    const allocatedBlocks = this.getAllocatedBlocks();
    
    const usedMemory = allocatedBlocks.reduce((sum, b) => sum + b.size, 0);
    const freeMemory = freeBlocks.reduce((sum, b) => sum + b.size, 0);
    const largestHole = freeBlocks.length > 0 
      ? Math.max(...freeBlocks.map(b => b.size)) 
      : 0;
    const avgHoleSize = freeBlocks.length > 0 
      ? (freeBlocks.reduce((sum, b) => sum + b.size, 0) / freeBlocks.length).toFixed(0)
      : 0;
    
    return {
      totalMemory: this.totalMemory,
      osMemory: this.osMemory,
      userMemory: this.userMemory,
      usedMemory,
      freeMemory,
      fragmentation: this.calculateFragmentation(),
      totalAllocations: this.totalAllocations,
      totalDeallocations: this.totalDeallocations,
      largestHole,
      numHoles: freeBlocks.length,
      avgHoleSize,
      numProcesses: allocatedBlocks.length
    };
  }

  // First Fit Algorithm (MATCHES C CODE)
  firstFit(size) {
    for (let block of this.blocks) {
      if (!block.isAllocated && block.size >= size) {
        return block;
      }
    }
    return null;
  }

  // Best Fit Algorithm (MATCHES C CODE)
  bestFit(size) {
    let bestBlock = null;
    let minSize = Infinity;
    
    for (let block of this.blocks) {
      if (!block.isAllocated && block.size >= size) {
        if (block.size < minSize) {
          minSize = block.size;
          bestBlock = block;
        }
      }
    }
    return bestBlock;
  }

  // Worst Fit Algorithm (MATCHES C CODE)
  worstFit(size) {
    let worstBlock = null;
    let maxSize = -1;
    
    for (let block of this.blocks) {
      if (!block.isAllocated && block.size >= size) {
        if (block.size > maxSize) {
          maxSize = block.size;
          worstBlock = block;
        }
      }
    }
    return worstBlock;
  }

  // Allocate memory using specified algorithm (MATCHES C CODE)
  allocate(size, processName, algorithm) {
    if (size <= 0) {
      return { success: false, message: 'Invalid process size' };
    }

    const freeMemory = this.getFreeBlocks().reduce((sum, b) => sum + b.size, 0);
    if (size > freeMemory) {
      return { success: false, message: `Not enough free memory (need ${size} KB, have ${freeMemory} KB)` };
    }

    let targetBlock = null;
    
    switch (algorithm) {
      case 'firstFit':
        targetBlock = this.firstFit(size);
        break;
      case 'bestFit':
        targetBlock = this.bestFit(size);
        break;
      case 'worstFit':
        targetBlock = this.worstFit(size);
        break;
      default:
        targetBlock = this.firstFit(size);
    }
    
    if (!targetBlock) {
      return { success: false, message: 'No suitable hole found (fragmentation)' };
    }
    
    const blockIndex = this.blocks.indexOf(targetBlock);
    const processId = processName || `P${++this.processCounter}`;
    
    if (targetBlock.size === size) {
      // Perfect fit - just mark as allocated
      targetBlock.isAllocated = true;
      targetBlock.processId = processId;
    } else {
      // Split the block
      const allocatedBlock = new MemoryBlock(
        this.blockIdCounter++,
        targetBlock.startAddress,
        size,
        processId,
        true
      );
      
      const remainingBlock = new MemoryBlock(
        this.blockIdCounter++,
        targetBlock.startAddress + size,
        targetBlock.size - size,
        null,
        false
      );
      
      this.blocks.splice(blockIndex, 1, allocatedBlock, remainingBlock);
    }
    
    this.totalAllocations++;
    this.saveState(`Allocated ${processId} (${size} KB) using ${algorithm}`);
    
    return { 
      success: true, 
      processId, 
      size, 
      algorithm,
      startAddress: targetBlock.startAddress
    };
  }

  // Deallocate memory (MATCHES C CODE - with merging)
  deallocate(processId) {
    const blockIndex = this.blocks.findIndex(b => b.processId === processId);
    
    if (blockIndex === -1) {
      return { success: false, message: 'Process not found' };
    }
    
    const block = this.blocks[blockIndex];
    block.isAllocated = false;
    block.processId = null;
    
    // Merge adjacent free blocks (MATCHES C CODE LOGIC)
    this.mergeAdjacentFreeBlocks();
    
    this.totalDeallocations++;
    this.saveState(`Deallocated ${processId}`);
    
    return { success: true, processId };
  }

  // Merge adjacent free blocks (MATCHES C CODE)
  mergeAdjacentFreeBlocks() {
    let merged = true;
    
    while (merged) {
      merged = false;
      for (let i = 0; i < this.blocks.length - 1; i++) {
        const current = this.blocks[i];
        const next = this.blocks[i + 1];
        
        if (!current.isAllocated && !next.isAllocated) {
          current.size += next.size;
          this.blocks.splice(i + 1, 1);
          merged = true;
          break;
        }
      }
    }
  }

  // Save current state to history
  saveState(action) {
    this.history.push({
      timestamp: new Date(),
      action,
      blocks: this.blocks.map(b => b.clone()),
      stats: this.getStats()
    });
  }

  // Restore state from history
  restoreState(index) {
    if (index < 0 || index >= this.history.length) return false;
    
    const state = this.history[index];
    this.blocks = state.blocks.map(b => b.clone());
    this.history = this.history.slice(0, index + 1);
    
    return true;
  }

  // Reset to initial state
  reset() {
    this.blocks = [new MemoryBlock(
      this.blockIdCounter++,
      this.osMemory,
      this.userMemory,
      null,
      false
    )];
    this.history = [];
    this.processCounter = 0;
    this.totalAllocations = 0;
    this.totalDeallocations = 0;
    
    this.saveState('Memory Reset');
  }

  // Clone manager for comparison
  clone() {
    const cloned = new MemoryManager(this.totalMemory, this.osMemory);
    cloned.blocks = [];
    cloned.history = [];
    cloned.processCounter = 0;
    cloned.totalAllocations = 0;
    cloned.totalDeallocations = 0;
    cloned.blocks.push(new MemoryBlock(0, this.osMemory, this.userMemory, null, false));
    return cloned;
  }
}
