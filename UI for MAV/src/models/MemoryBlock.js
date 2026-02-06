/**
 * MemoryBlock Class
 * Represents a single block of memory (allocated or free)
 */

export class MemoryBlock {
  constructor(id, startAddress, size, processId = null, isAllocated = false) {
    this.id = id;
    this.startAddress = startAddress;
    this.size = size;
    this.processId = processId;
    this.isAllocated = isAllocated;
  }

  get endAddress() {
    return this.startAddress + this.size - 1;
  }

  clone() {
    return new MemoryBlock(
      this.id,
      this.startAddress,
      this.size,
      this.processId,
      this.isAllocated
    );
  }
}
