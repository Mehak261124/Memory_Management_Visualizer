/*
================================================================================
FILE: memory_manager.h
PURPOSE: Declare memory management functions and allocation algorithms
DESCRIPTION: 
    - This file declares the "main" functions of our project
    - Allocation algorithms (First Fit, Best Fit, Worst Fit)
    - Memory operations (allocate, deallocate, display, etc.)
    - Defragmentation techniques (Compaction, Buddy System)
    - JSON output functions for the HTTP API
================================================================================
*/

// Header guard - prevents multiple inclusion
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

// Include our structures file
// We need this because our functions use MemoryManager and MemoryBlock
#include "memory_structures.h"


/*
================================================================================
ENUMERATION: AllocationAlgorithm
================================================================================
PURPOSE: Define the types of allocation algorithms we support

WHAT IS AN ENUMERATION (enum)?
- A way to create a list of named constants
- Instead of using numbers (0, 1, 2), we use meaningful names
- Makes code easier to read and understand

WHY USE THIS?
Instead of writing: allocateMemory(mm, pid, size, 0);  // What does 0 mean?
We write: allocateMemory(mm, pid, size, FIRST_FIT);    // Clear meaning!

HOW IT WORKS:
C automatically assigns numbers:
FIRST_FIT = 0
BEST_FIT = 1
WORST_FIT = 2
*/

typedef enum {
    FIRST_FIT,      // Allocate to first suitable hole found
    BEST_FIT,       // Allocate to smallest suitable hole
    WORST_FIT       // Allocate to largest hole
} AllocationAlgorithm;


/*
================================================================================
MAIN MEMORY MANAGEMENT FUNCTIONS
================================================================================
These are the core functions that manage memory allocation and deallocation
*/


/*
--------------------------------------------------------------------------------
FUNCTION: initializeMemory
--------------------------------------------------------------------------------
PURPOSE: Set up the memory system when program starts

WHAT IT DOES:
1. Sets up total memory size
2. Reserves space for OS
3. Calculates user memory
4. Creates one big initial hole (all memory is free at start)
5. Resets all statistics counters

PARAMETERS:
- mm: Pointer to MemoryManager structure (we'll fill this in)
- totalMem: Total memory size in KB (example: 1024 KB)
- osMem: Memory reserved for OS in KB (example: 256 KB)

RETURNS: Nothing (void)
*/
void initializeMemory(MemoryManager *mm, int totalMem, int osMem);


/*
--------------------------------------------------------------------------------
FUNCTION: allocateMemory
--------------------------------------------------------------------------------
PURPOSE: Allocate memory to a process using specified algorithm

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: ID of the process requesting memory
- size: How much memory the process needs (in KB)
- algo: Which algorithm to use (FIRST_FIT, BEST_FIT, or WORST_FIT)

RETURNS: 
- Starting address where process was allocated (success)
- -1 if allocation failed (no suitable hole found)
*/
int allocateMemory(MemoryManager *mm, int processID, int size, 
                   AllocationAlgorithm algo);


/*
--------------------------------------------------------------------------------
FUNCTION: deallocateMemory
--------------------------------------------------------------------------------
PURPOSE: Free memory when a process finishes

WHAT IT DOES:
1. Find the process in memory
2. Convert it to a hole (free space)
3. Merge with adjacent holes if they exist
4. Update statistics

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: ID of the process to remove

RETURNS:
- 1 if deallocation successful
- 0 if process not found
*/
int deallocateMemory(MemoryManager *mm, int processID);


/*
--------------------------------------------------------------------------------
FUNCTION: displayMemory
--------------------------------------------------------------------------------
PURPOSE: Show current state of memory on screen

PARAMETERS:
- mm: Pointer to MemoryManager

RETURNS: Nothing (void)
*/
void displayMemory(MemoryManager *mm);


/*
--------------------------------------------------------------------------------
FUNCTION: calculateFragmentation
--------------------------------------------------------------------------------
PURPOSE: Calculate external fragmentation percentage

HOW WE CALCULATE:
1. Find largest single hole
2. Calculate: (Total Free - Largest Hole) / User Memory * 100
3. This percentage shows how much free memory is unusable

PARAMETERS:
- mm: Pointer to MemoryManager

RETURNS: 
- Fragmentation percentage (float, 0.0 to 100.0)
*/
float calculateFragmentation(MemoryManager *mm);


/*
--------------------------------------------------------------------------------
FUNCTION: freeMemoryManager
--------------------------------------------------------------------------------
PURPOSE: Clean up and release all memory when program ends

PARAMETERS:
- mm: Pointer to MemoryManager

RETURNS: Nothing (void)
*/
void freeMemoryManager(MemoryManager *mm);


/*
================================================================================
ALLOCATION ALGORITHM IMPLEMENTATIONS
================================================================================
These are the actual implementations of the three placement strategies
*/


/*
--------------------------------------------------------------------------------
FUNCTION: firstFit
--------------------------------------------------------------------------------
PURPOSE: Allocate using First Fit algorithm

ALGORITHM:
1. Start from beginning of memory
2. Check each hole in order
3. Use the FIRST hole that is big enough
4. Stop searching immediately

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process requesting memory
- size: Size needed in KB

RETURNS:
- Starting address if successful
- -1 if no suitable hole found
*/
int firstFit(MemoryManager *mm, int processID, int size);


/*
--------------------------------------------------------------------------------
FUNCTION: bestFit
--------------------------------------------------------------------------------
PURPOSE: Allocate using Best Fit algorithm

ALGORITHM:
1. Search through ALL holes
2. Find the SMALLEST hole that fits
3. Use that hole

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process requesting memory
- size: Size needed in KB

RETURNS:
- Starting address if successful
- -1 if no suitable hole found
*/
int bestFit(MemoryManager *mm, int processID, int size);


/*
--------------------------------------------------------------------------------
FUNCTION: worstFit
--------------------------------------------------------------------------------
PURPOSE: Allocate using Worst Fit algorithm

ALGORITHM:
1. Search through ALL holes
2. Find the LARGEST hole
3. Use that hole

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process requesting memory
- size: Size needed in KB

RETURNS:
- Starting address if successful
- -1 if no suitable hole found
*/
int worstFit(MemoryManager *mm, int processID, int size);


/*
================================================================================
DEFRAGMENTATION TECHNIQUES
================================================================================
Functions to reduce external fragmentation:
1. Compaction - slide processes to eliminate holes
2. Buddy System - power-of-2 allocation with split/merge
*/


/*
--------------------------------------------------------------------------------
FUNCTION: compact
--------------------------------------------------------------------------------
PURPOSE: Perform sliding compaction on memory

WHAT IT DOES:
1. Moves all allocated processes to the beginning of user memory
2. Creates one large hole at the end
3. Eliminates external fragmentation

THINK OF IT LIKE:
Imagine books on a shelf with gaps between them.
Compaction slides all books to the left, creating one big space on the right.

Before: [OS][P1][HOLE][P2][HOLE][P3][HOLE]
After:  [OS][P1][P2][P3][    BIG HOLE    ]

PARAMETERS:
- mm: Pointer to MemoryManager
- resultBuffer: Buffer to write JSON result into
- bufferSize: Size of the result buffer

RETURNS: 
- 1 if compaction was performed
- 0 if no processes to compact

RESULT JSON FORMAT:
{
  "success": true,
  "processesMovedCount": 2,
  "totalBytesMoved": 200,
  "fragmentationBefore": 25.0,
  "fragmentationAfter": 0.0,
  "holesBefore": 3,
  "holesAfter": 1
}
*/
int compact(MemoryManager *mm, char *resultBuffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: autoCompact
--------------------------------------------------------------------------------
PURPOSE: Automatically compact memory if fragmentation exceeds threshold

WHAT IT DOES:
1. Calculate current fragmentation
2. If fragmentation > threshold → compact
3. If fragmentation <= threshold → skip (return info message)

PARAMETERS:
- mm: Pointer to MemoryManager
- threshold: Fragmentation % above which compaction triggers (e.g., 30)
- resultBuffer: Buffer to write JSON result into
- bufferSize: Size of the result buffer

RETURNS:
- 1 if compaction was performed
- 0 if fragmentation was below threshold (skipped)
*/
int autoCompact(MemoryManager *mm, int threshold, char *resultBuffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: buddyAllocate
--------------------------------------------------------------------------------
PURPOSE: Allocate memory using the Buddy System

ALGORITHM:
1. Round requested size UP to the nearest power of 2
2. Find a free block that is big enough
3. If block is too big, SPLIT it into two "buddies" (half-size each)
4. Keep splitting until we get the right size
5. Allocate the block

EXAMPLE:
Request: 50 KB
Rounded: 64 KB (next power of 2)
If we have a 256 KB block:
  Split → [128 KB][128 KB]
  Split → [64 KB][64 KB][128 KB]
  Allocate first 64 KB block → [P1:64KB][FREE:64KB][FREE:128KB]

PARAMETERS:
- mm: Pointer to MemoryManager
- size: Requested size in KB
- resultBuffer: Buffer for JSON result
- bufferSize: Size of result buffer

RETURNS:
- Starting address if successful
- -1 if no suitable block found
*/
int buddyAllocate(MemoryManager *mm, int size, char *resultBuffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: buddyDeallocate
--------------------------------------------------------------------------------
PURPOSE: Deallocate a process under buddy system and merge buddy pairs

WHAT IT DOES:
1. Find the process and mark it as free
2. Check if its buddy is also free
3. If yes → merge them back together (coalesce)
4. Recursively check if the merged block's buddy is also free

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process to deallocate
- resultBuffer: Buffer for JSON result
- bufferSize: Size of result buffer

RETURNS:
- 1 if deallocation successful
- 0 if process not found
*/
int buddyDeallocate(MemoryManager *mm, int processID, char *resultBuffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: convertToBuddySystem
--------------------------------------------------------------------------------
PURPOSE: Convert current memory layout to buddy system

WHAT IT DOES:
1. Save all current processes (their sizes)
2. Reinitialize memory for buddy system (round to power of 2)
3. Re-allocate each saved process using buddy allocation

PARAMETERS:
- mm: Pointer to MemoryManager
- resultBuffer: Buffer for JSON result
- bufferSize: Size of result buffer

RETURNS:
- 1 if conversion successful
- 0 if conversion failed
*/
int convertToBuddySystem(MemoryManager *mm, char *resultBuffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: revertFromBuddySystem
--------------------------------------------------------------------------------
PURPOSE: Revert from buddy system back to standard allocation

WHAT IT DOES:
1. Save all current processes
2. Reinitialize memory normally
3. Re-allocate each process using standard first-fit

PARAMETERS:
- mm: Pointer to MemoryManager
- resultBuffer: Buffer for JSON result
- bufferSize: Size of result buffer

RETURNS:
- 1 if revert successful
- 0 if revert failed
*/
int revertFromBuddySystem(MemoryManager *mm, char *resultBuffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: resetMemory
--------------------------------------------------------------------------------
PURPOSE: Reset memory to its initial state (like starting fresh)

WHAT IT DOES:
1. Free all memory blocks
2. Reinitialize memory with same total/OS sizes
3. Reset all counters and statistics

PARAMETERS:
- mm: Pointer to MemoryManager
*/
void resetMemory(MemoryManager *mm);


/*
================================================================================
JSON / API HELPER FUNCTIONS
================================================================================
These functions produce JSON strings for the HTTP API responses
*/


/*
--------------------------------------------------------------------------------
FUNCTION: getStatsJSON
--------------------------------------------------------------------------------
PURPOSE: Generate a JSON string with all memory statistics

OUTPUT FORMAT:
{
  "totalMemory": 1024,
  "osMemory": 256,
  "userMemory": 768,
  "usedMemory": 400,
  "freeMemory": 368,
  "numProcesses": 3,
  "numHoles": 2,
  "fragmentation": 15.5,
  "totalAllocations": 5,
  "totalDeallocations": 2,
  "totalCompactions": 1,
  "useBuddySystem": false
}

PARAMETERS:
- mm: Pointer to MemoryManager
- buffer: Output buffer for the JSON string
- bufferSize: Size of the output buffer
*/
void getStatsJSON(MemoryManager *mm, char *buffer, int bufferSize);


/*
--------------------------------------------------------------------------------
FUNCTION: nextPowerOf2
--------------------------------------------------------------------------------
PURPOSE: Round a number up to the next power of 2

EXAMPLES:
  nextPowerOf2(50) → 64
  nextPowerOf2(64) → 64
  nextPowerOf2(100) → 128
  nextPowerOf2(1) → 1

PARAMETERS:
- n: The number to round up

RETURNS: Next power of 2 >= n
*/
int nextPowerOf2(int n);


// End of header guard
#endif

/*
================================================================================
END OF FILE: memory_manager.h
================================================================================

WHAT WE DECLARED:
1.  AllocationAlgorithm enum (FIRST_FIT, BEST_FIT, WORST_FIT)
2.  initializeMemory() - Set up memory system
3.  allocateMemory() - Main allocation function (wrapper)
4.  deallocateMemory() - Free memory with hole merging
5.  displayMemory() - Show memory state
6.  calculateFragmentation() - Measure fragmentation
7.  freeMemoryManager() - Clean up
8.  firstFit() - First Fit algorithm
9.  bestFit() - Best Fit algorithm
10. worstFit() - Worst Fit algorithm
11. compact() - Sliding compaction
12. autoCompact() - Auto-compact based on threshold
13. buddyAllocate() - Buddy system allocation
14. buddyDeallocate() - Buddy system deallocation
15. convertToBuddySystem() - Switch to buddy system
16. revertFromBuddySystem() - Switch back to standard
17. resetMemory() - Reset to initial state
18. getStatsJSON() - Memory stats as JSON
19. nextPowerOf2() - Helper for buddy system

NEXT FILE: src/memory_manager.c
This will implement all these functions!
================================================================================
*/