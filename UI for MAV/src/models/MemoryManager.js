/**
 * ENHANCED MemoryManager Class
 * Added Defragmentation Techniques:
 * 1. Coalescing (already exists - merging adjacent holes)
 * 2. Memory Compaction (sliding all processes to eliminate holes)
 * 3. Buddy System (power-of-2 allocation with splitting/merging)
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
    this.totalCompactions = 0;
    
    // Buddy System tracking
    this.useBuddySystem = false;
    this.buddyBlocks = new Map(); // Track buddy pairs
    
    // Initialize with one free block
    this.blocks.push(new MemoryBlock(
      this.blockIdCounter++,
      osMem,
      this.userMemory,
      null,
      false
    ));
    
    this.saveState('Initial State');
  }

  getFreeBlocks() {
    return this.blocks.filter(b => !b.isAllocated);
  }

  getAllocatedBlocks() {
    return this.blocks.filter(b => b.isAllocated);
  }

  calculateFragmentation() {
    const freeBlocks = this.getFreeBlocks();
    if (freeBlocks.length === 0) return 0;
    
    const totalFree = freeBlocks.reduce((sum, b) => sum + b.size, 0);
    if (totalFree === 0) return 0;
    
    const largestFree = Math.max(...freeBlocks.map(b => b.size));
    const fragmentedMemory = totalFree - largestFree;
    return ((fragmentedMemory / this.userMemory) * 100).toFixed(1);
  }

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
      totalCompactions: this.totalCompactions,
      largestHole,
      numHoles: freeBlocks.length,
      avgHoleSize,
      numProcesses: allocatedBlocks.length
    };
  }

  firstFit(size) {
    for (let block of this.blocks) {
      if (!block.isAllocated && block.size >= size) {
        return block;
      }
    }
    return null;
  }

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

  allocate(size, processName, algorithm) {
    if (size <= 0) {
      return { success: false, message: 'Invalid process size' };
    }

    const freeMemory = this.getFreeBlocks().reduce((sum, b) => sum + b.size, 0);
    if (size > freeMemory) {
      return { success: false, message: `Not enough free memory (need ${size} KB, have ${freeMemory} KB)` };
    }

    let targetBlock = null;
    
    // Use buddy system if enabled
    if (this.useBuddySystem) {
      return this.buddyAllocate(size, processName);
    }
    
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
      targetBlock.isAllocated = true;
      targetBlock.processId = processId;
    } else {
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

  deallocate(processId) {
    const blockIndex = this.blocks.findIndex(b => b.processId === processId);
    
    if (blockIndex === -1) {
      return { success: false, message: 'Process not found' };
    }
    
    const block = this.blocks[blockIndex];
    
    // Use buddy system deallocation if enabled
    if (this.useBuddySystem && this.buddyBlocks.has(block.id)) {
      return this.buddyDeallocate(processId);
    }
    
    block.isAllocated = false;
    block.processId = null;
    
    // TECHNIQUE 1: COALESCING - Merge adjacent free blocks
    this.mergeAdjacentFreeBlocks();
    
    this.totalDeallocations++;
    this.saveState(`Deallocated ${processId}`);
    
    return { success: true, processId };
  }

  // TECHNIQUE 1: COALESCING
  // Merges adjacent free blocks to reduce external fragmentation
  mergeAdjacentFreeBlocks() {
    let merged = true;
    let mergeCount = 0;
    
    while (merged) {
      merged = false;
      for (let i = 0; i < this.blocks.length - 1; i++) {
        const current = this.blocks[i];
        const next = this.blocks[i + 1];
        
        // If both are free and adjacent, merge them
        if (!current.isAllocated && !next.isAllocated) {
          current.size += next.size;
          this.blocks.splice(i + 1, 1);
          merged = true;
          mergeCount++;
          break;
        }
      }
    }
    
    return mergeCount;
  }

  // ============================================
  // TECHNIQUE 2: MEMORY COMPACTION
  // ============================================
  
  /**
   * Sliding Compaction
   * Moves all allocated blocks to the beginning of memory,
   * leaving one large hole at the end
   */
  compact() {
    const allocatedBlocks = this.getAllocatedBlocks();
    
    if (allocatedBlocks.length === 0) {
      return {
        success: false,
        message: 'No processes to compact'
      };
    }
    
    // Calculate metrics before compaction
    const fragBefore = this.calculateFragmentation();
    const holesBefore = this.getFreeBlocks().length;
    
    const newBlocks = [];
    let currentAddress = this.osMemory;
    let totalMoved = 0;
    let totalBytesMoved = 0;
    
    // Move all allocated blocks to the beginning
    allocatedBlocks.forEach(block => {
      const oldAddress = block.startAddress;
      const newAddress = currentAddress;
      
      if (oldAddress !== newAddress) {
        totalMoved++;
        totalBytesMoved += Math.abs(newAddress - oldAddress);
      }
      
      const compactedBlock = new MemoryBlock(
        this.blockIdCounter++,
        newAddress,
        block.size,
        block.processId,
        true
      );
      
      newBlocks.push(compactedBlock);
      currentAddress += block.size;
    });
    
    // Create single large hole at the end
    const remainingSize = this.totalMemory - currentAddress;
    if (remainingSize > 0) {
      newBlocks.push(new MemoryBlock(
        this.blockIdCounter++,
        currentAddress,
        remainingSize,
        null,
        false
      ));
    }
    
    this.blocks = newBlocks;
    this.totalCompactions++;
    
    const fragAfter = this.calculateFragmentation();
    const holesAfter = this.getFreeBlocks().length;
    
    this.saveState(`Compaction: ${totalMoved} processes moved, fragmentation ${fragBefore}% → ${fragAfter}%`);
    
    return {
      success: true,
      processesMovedCount: totalMoved,
      totalBytesMoved,
      fragmentationBefore: fragBefore,
      fragmentationAfter: fragAfter,
      holesBefore,
      holesAfter,
      message: `Compaction complete: Moved ${totalMoved} processes, reduced fragmentation from ${fragBefore}% to ${fragAfter}%`
    };
  }

  /**
   * Check if compaction is needed based on fragmentation threshold
   */
  shouldCompact(threshold = 30) {
    const frag = parseFloat(this.calculateFragmentation());
    return frag > threshold;
  }

  /**
   * Auto-compact if fragmentation exceeds threshold
   */
  autoCompact(threshold = 30) {
    if (this.shouldCompact(threshold)) {
      return this.compact();
    }
    return {
      success: false,
      message: `Fragmentation (${this.calculateFragmentation()}%) is below threshold (${threshold}%)`
    };
  }

  // ============================================
  // TECHNIQUE 3: BUDDY SYSTEM
  // ============================================
  
  /**
   * Enable/Disable Buddy System
   */
  setBuddySystem(enabled) {
    if (enabled && !this.useBuddySystem) {
      // Initialize buddy system
      this.initializeBuddySystem();
    }
    this.useBuddySystem = enabled;
  }

  /**
   * Initialize memory for buddy system
   * Rounds user memory to nearest power of 2
   */
  initializeBuddySystem() {
    // Find nearest power of 2
    const maxPower = Math.floor(Math.log2(this.userMemory));
    const buddySize = Math.pow(2, maxPower);
    
    this.blocks = [new MemoryBlock(
      this.blockIdCounter++,
      this.osMemory,
      buddySize,
      null,
      false
    )];
    
    this.buddyBlocks.clear();
    this.saveState('Initialized Buddy System');
  }

  /**
   * Round up to next power of 2
   */
  nextPowerOf2(size) {
    if (size <= 0) return 1;
    return Math.pow(2, Math.ceil(Math.log2(size)));
  }

  /**
   * Buddy System Allocation
   */
  buddyAllocate(size, processName) {
    const allocSize = this.nextPowerOf2(size);
    const processId = processName || `P${++this.processCounter}`;
    
    // Find suitable block
    let targetBlock = null;
    for (let block of this.blocks) {
      if (!block.isAllocated && block.size >= allocSize) {
        targetBlock = block;
        break;
      }
    }
    
    if (!targetBlock) {
      return { success: false, message: 'No suitable buddy block found' };
    }
    
    // Split block until we get the right size
    while (targetBlock.size > allocSize) {
      const halfSize = targetBlock.size / 2;
      const blockIndex = this.blocks.indexOf(targetBlock);
      
      // Create two buddy blocks
      const buddy1 = new MemoryBlock(
        this.blockIdCounter++,
        targetBlock.startAddress,
        halfSize,
        null,
        false
      );
      
      const buddy2 = new MemoryBlock(
        this.blockIdCounter++,
        targetBlock.startAddress + halfSize,
        halfSize,
        null,
        false
      );
      
      // Track buddy relationship
      this.buddyBlocks.set(buddy1.id, buddy2.id);
      this.buddyBlocks.set(buddy2.id, buddy1.id);
      
      // Replace original with buddies
      this.blocks.splice(blockIndex, 1, buddy1, buddy2);
      targetBlock = buddy1;
    }
    
    // Allocate the block
    targetBlock.isAllocated = true;
    targetBlock.processId = processId;
    
    this.totalAllocations++;
    this.saveState(`Buddy allocated ${processId} (requested: ${size} KB, allocated: ${allocSize} KB)`);
    
    return {
      success: true,
      processId,
      requestedSize: size,
      allocatedSize: allocSize,
      wastedSpace: allocSize - size,
      startAddress: targetBlock.startAddress
    };
  }

  /**
   * Buddy System Deallocation with Coalescing
   */
  buddyDeallocate(processId) {
    const blockIndex = this.blocks.findIndex(b => b.processId === processId);
    
    if (blockIndex === -1) {
      return { success: false, message: 'Process not found' };
    }
    
    const block = this.blocks[blockIndex];
    block.isAllocated = false;
    block.processId = null;
    
    // Try to merge with buddy
    this.mergeBuddies(block);
    
    this.totalDeallocations++;
    this.saveState(`Buddy deallocated ${processId}`);
    
    return { success: true, processId };
  }

  /**
   * Merge buddy blocks recursively
   */
  mergeBuddies(block) {
    const buddyId = this.buddyBlocks.get(block.id);
    if (!buddyId) return;
    
    const buddyBlock = this.blocks.find(b => b.id === buddyId);
    if (!buddyBlock || buddyBlock.isAllocated) return;
    
    // Both buddies are free - merge them
    const mergedSize = block.size + buddyBlock.size;
    const mergedStart = Math.min(block.startAddress, buddyBlock.startAddress);
    
    const mergedBlock = new MemoryBlock(
      this.blockIdCounter++,
      mergedStart,
      mergedSize,
      null,
      false
    );
    
    // Remove old buddies
    this.blocks = this.blocks.filter(b => b.id !== block.id && b.id !== buddyId);
    this.buddyBlocks.delete(block.id);
    this.buddyBlocks.delete(buddyId);
    
    // Add merged block
    this.blocks.push(mergedBlock);
    this.blocks.sort((a, b) => a.startAddress - b.startAddress);
    
    // Try to merge again (recursive)
    this.mergeBuddies(mergedBlock);
  }

  /**
   * Convert existing allocations to Buddy System
   * Saves current processes, reinitializes with buddy system, and re-allocates
   */
  convertToBuddySystem() {
    const currentAllocations = this.getAllocatedBlocks().map(b => ({
      processId: b.processId,
      size: b.size
    }));

    if (currentAllocations.length === 0) {
      return { success: false, message: 'No processes to convert. Allocate some processes first.' };
    }

    // Save before stats
    const fragBefore = this.calculateFragmentation();
    const holesBefore = this.getFreeBlocks().length;

    // Enable buddy system and reinitialize memory
    this.useBuddySystem = true;
    this.initializeBuddySystem();

    // Re-allocate each process using buddy allocation
    const conversions = [];
    let totalWasted = 0;
    let failedCount = 0;

    for (const alloc of currentAllocations) {
      const result = this.buddyAllocate(alloc.size, alloc.processId);
      if (result.success) {
        const wasted = result.allocatedSize - result.requestedSize;
        totalWasted += wasted;
        conversions.push({
          processId: alloc.processId,
          originalSize: alloc.size,
          buddySize: result.allocatedSize,
          wasted
        });
      } else {
        failedCount++;
        conversions.push({
          processId: alloc.processId,
          originalSize: alloc.size,
          buddySize: null,
          error: result.message
        });
      }
    }

    const fragAfter = this.calculateFragmentation();
    const successCount = conversions.filter(c => !c.error).length;

    this.saveState(`Converted ${successCount} processes to Buddy System`);

    return {
      success: true,
      conversions,
      totalWasted,
      successCount,
      failedCount,
      fragmentationBefore: fragBefore,
      fragmentationAfter: fragAfter,
      holesBefore,
      message: `Converted ${successCount}/${currentAllocations.length} processes to buddy system allocation. Internal fragmentation: ${totalWasted} KB wasted.`
    };
  }

  /**
   * Revert from Buddy System back to standard allocation
   * Saves current processes and re-allocates using standard first-fit
   */
  revertFromBuddySystem() {
    const currentAllocations = this.getAllocatedBlocks().map(b => ({
      processId: b.processId,
      size: b.size
    }));

    this.useBuddySystem = false;
    this.buddyBlocks.clear();
    
    // Reset blocks to single free block
    this.blocks = [new MemoryBlock(
      this.blockIdCounter++,
      this.osMemory,
      this.userMemory,
      null,
      false
    )];

    // Re-allocate using standard first-fit
    for (const alloc of currentAllocations) {
      this.allocate(alloc.size, alloc.processId, 'firstFit');
    }

    this.saveState('Reverted from Buddy System to standard allocation');
    return { success: true, message: 'Reverted to standard allocation.' };
  }

  // ============================================
  // HELPER METHODS
  // ============================================

  saveState(action) {
    this.history.push({
      timestamp: new Date(),
      action,
      blocks: this.blocks.map(b => b.clone()),
      stats: this.getStats()
    });
  }

  restoreState(index) {
    if (index < 0 || index >= this.history.length) return false;
    
    const state = this.history[index];
    this.blocks = state.blocks.map(b => b.clone());
    this.history = this.history.slice(0, index + 1);
    
    return true;
  }

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
    this.totalCompactions = 0;
    this.useBuddySystem = false;
    this.buddyBlocks.clear();
    
    this.saveState('Memory Reset');
  }

  clone() {
    const cloned = new MemoryManager(this.totalMemory, this.osMemory);
    cloned.blocks = this.blocks.map(b => b.clone());
    cloned.processCounter = this.processCounter;
    cloned.totalAllocations = this.totalAllocations;
    cloned.totalDeallocations = this.totalDeallocations;
    cloned.totalCompactions = this.totalCompactions;
    return cloned;
  }
}