/*
================================================================================
FILE: memory_manager.h
PURPOSE: Declare memory management functions and allocation algorithms
DESCRIPTION: 
    - This file declares the "main" functions of our project
    - Allocation algorithms (First Fit, Best Fit, Worst Fit)
    - Memory operations (allocate, deallocate, display, etc.)
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
EXAMPLE USAGE:
AllocationAlgorithm myAlgo = FIRST_FIT;
if (myAlgo == FIRST_FIT) {
    printf("Using First Fit algorithm\n");
}
*/


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

PARAMETERS:
- mm: Pointer to MemoryManager structure (we'll fill this in)
- totalMem: Total memory size in KB (example: 1024 KB)
- osMem: Memory reserved for OS in KB (example: 256 KB)

RETURNS: Nothing (void)

EXAMPLE CALL:
MemoryManager mm;
initializeMemory(&mm, 1024, 256);
// This creates: 1024 KB total, 256 KB for OS, 768 KB for user

VISUAL REPRESENTATION:
Before: [nothing]
After:  [OS: 0-255][HOLE: 256-1023]
*/
void initializeMemory(MemoryManager *mm, int totalMem, int osMem);


/*
--------------------------------------------------------------------------------
FUNCTION: allocateMemory
--------------------------------------------------------------------------------
PURPOSE: Allocate memory to a process using specified algorithm

WHAT IT DOES:
1. Takes a process and its size
2. Uses the specified algorithm (First/Best/Worst Fit)
3. Finds a suitable hole
4. Assigns memory to the process

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: ID of the process requesting memory
- size: How much memory the process needs (in KB)
- algo: Which algorithm to use (FIRST_FIT, BEST_FIT, or WORST_FIT)

RETURNS: 
- Starting address where process was allocated (success)
- -1 if allocation failed (no suitable hole found)

EXAMPLE CALL:
int addr = allocateMemory(&mm, 1, 100, FIRST_FIT);
// Try to allocate 100 KB to Process P1 using First Fit
// If successful, addr will be the starting address (e.g., 256)
// If failed, addr will be -1

EXAMPLE SCENARIO:
Memory: [OS][HOLE: 256-455][P1][HOLE: 600-1023]
Call: allocateMemory(&mm, 2, 100, FIRST_FIT);
Result: Process P2 gets 256-355, returns 256
New Memory: [OS][P2: 256-355][HOLE: 356-455][P1][HOLE: 600-1023]
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

EXAMPLE CALL:
int result = deallocateMemory(&mm, 2);
// Remove Process P2 from memory
// result = 1 if successful, 0 if P2 not found

EXAMPLE SCENARIO:
Before: [OS][P1][P2: 256-355][HOLE: 356-455][P3]
Call: deallocateMemory(&mm, 2);
After:  [OS][P1][HOLE: 256-455][P3]
        // P2's space merged with adjacent hole!

WHY MERGE HOLES?
If we don't merge, we get many small holes that can't be used:
[HOLE:10KB][HOLE:10KB][HOLE:10KB] = Can't fit a 25KB process!
[HOLE:30KB] = Can fit a 25KB process!
*/
int deallocateMemory(MemoryManager *mm, int processID);


/*
--------------------------------------------------------------------------------
FUNCTION: displayMemory
--------------------------------------------------------------------------------
PURPOSE: Show current state of memory on screen

WHAT IT DOES:
1. Prints OS region
2. Prints all memory blocks (processes and holes)
3. Shows statistics (free memory, processes, holes)

PARAMETERS:
- mm: Pointer to MemoryManager

RETURNS: Nothing (void)

EXAMPLE OUTPUT:
========== MEMORY STATE ==========
|  Type  |   Address   |    Size     |
==================================
| OS     |    0 -  255 | Size:  256 KB |
----------------------------------
| P1     |  256 -  355 | Size:  100 KB |
| HOLE   |  356 -  455 | Size:  100 KB |
| P2     |  456 -  655 | Size:  200 KB |
| HOLE   |  656 - 1023 | Size:  368 KB |
==================================
Free Memory: 468 KB
Processes: 2 | Holes: 2
==================================
*/
void displayMemory(MemoryManager *mm);


/*
--------------------------------------------------------------------------------
FUNCTION: calculateFragmentation
--------------------------------------------------------------------------------
PURPOSE: Calculate external fragmentation percentage

WHAT IS EXTERNAL FRAGMENTATION?
When free memory exists but is scattered in small pieces that can't be used.

EXAMPLE:
Total free: 500 KB
But scattered as: [100KB][50KB][150KB][200KB] in different places
A 300KB process can't fit even though 500KB is free!
The 300KB of smaller holes is "fragmented" and wasted.

HOW WE CALCULATE:
1. Find largest single hole
2. Calculate: (Total Free - Largest Hole) / User Memory * 100
3. This percentage shows how much free memory is unusable

PARAMETERS:
- mm: Pointer to MemoryManager

RETURNS: 
- Fragmentation percentage (float, 0.0 to 100.0)

EXAMPLE:
User Memory = 1000 KB
Total Free = 500 KB distributed as:
  - Hole 1: 200 KB (largest)
  - Hole 2: 150 KB
  - Hole 3: 100 KB
  - Hole 4: 50 KB

Fragmented memory = 500 - 200 = 300 KB
Fragmentation % = (300 / 1000) * 100 = 30%

This means 30% of total memory is wasted in small unusable holes.
*/
float calculateFragmentation(MemoryManager *mm);


/*
--------------------------------------------------------------------------------
FUNCTION: freeMemoryManager
--------------------------------------------------------------------------------
PURPOSE: Clean up and release all memory when program ends

WHAT IT DOES:
1. Goes through entire linked list
2. Frees each MemoryBlock
3. Prevents memory leaks

WHY IS THIS IMPORTANT?
When we use malloc(), we must use free() to release memory.
If we don't, we have a "memory leak" - wasted memory that can't be reused.

PARAMETERS:
- mm: Pointer to MemoryManager

RETURNS: Nothing (void)

WHEN TO CALL:
- Before program exits
- When resetting memory
- When switching to a new configuration
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

ADVANTAGES:
- Fastest algorithm (stops at first match)
- Simple to implement

DISADVANTAGES:
- Creates small holes at the beginning
- These small holes accumulate and waste space

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process requesting memory
- size: Size needed in KB

RETURNS:
- Starting address if successful
- -1 if no suitable hole found

EXAMPLE:
Memory: [HOLE:50KB][HOLE:200KB][HOLE:100KB]
Request: 80 KB
Result: Uses second hole (200KB) - first one big enough
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

ADVANTAGES:
- Minimizes wasted space in each allocation
- Good for memory utilization

DISADVANTAGES:
- Slower (must check all holes)
- Creates many tiny holes over time
- These tiny holes are often unusable

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process requesting memory
- size: Size needed in KB

RETURNS:
- Starting address if successful
- -1 if no suitable hole found

EXAMPLE:
Memory: [HOLE:50KB][HOLE:200KB][HOLE:100KB]
Request: 80 KB
Result: Uses third hole (100KB) - smallest that fits
Remaining: [HOLE:50KB][HOLE:200KB][P1:80KB][HOLE:20KB]
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

ADVANTAGES:
- Leaves larger remaining holes
- Larger holes are more likely to be reusable

DISADVANTAGES:
- Slower (must check all holes)
- Wastes the biggest holes first
- May prevent large processes from allocating later

PARAMETERS:
- mm: Pointer to MemoryManager
- processID: Process requesting memory
- size: Size needed in KB

RETURNS:
- Starting address if successful
- -1 if no suitable hole found

EXAMPLE:
Memory: [HOLE:50KB][HOLE:200KB][HOLE:100KB]
Request: 80 KB
Result: Uses second hole (200KB) - largest
Remaining: [HOLE:50KB][P1:80KB][HOLE:120KB][HOLE:100KB]
Notice: Remaining hole (120KB) is still quite large and usable
*/
int worstFit(MemoryManager *mm, int processID, int size);


/*
================================================================================
ALGORITHM COMPARISON SUMMARY
================================================================================

SCENARIO: Memory has holes of: 50 KB, 200 KB, 100 KB
REQUEST: Allocate 80 KB

FIRST FIT:
- Checks: 50 KB (too small), 200 KB (fits!) → STOP
- Uses: 200 KB hole
- Speed: FASTEST ⚡⚡⚡
- Result: [50KB][P:80KB][HOLE:120KB][100KB]

BEST FIT:
- Checks: ALL holes → 50KB (no), 200KB (yes), 100KB (yes, SMALLEST!)
- Uses: 100 KB hole
- Speed: SLOW 🐌
- Result: [50KB][200KB][P:80KB][HOLE:20KB]

WORST FIT:
- Checks: ALL holes → 50KB (no), 200KB (LARGEST!), 100KB (yes)
- Uses: 200 KB hole
- Speed: SLOW 🐌
- Result: [50KB][P:80KB][HOLE:120KB][100KB]

WHICH IS BEST?
- Speed: First Fit wins
- Memory efficiency: Depends on workload
- Generally: First Fit is most commonly used in real systems
*/


// End of header guard
#endif

/*
================================================================================
END OF FILE: memory_manager.h
================================================================================

WHAT WE DECLARED:
1. AllocationAlgorithm enum (FIRST_FIT, BEST_FIT, WORST_FIT)
2. initializeMemory() - Set up memory system
3. allocateMemory() - Main allocation function
4. deallocateMemory() - Free memory
5. displayMemory() - Show memory state
6. calculateFragmentation() - Measure fragmentation
7. freeMemoryManager() - Clean up
8. firstFit() - First Fit algorithm
9. bestFit() - Best Fit algorithm
10. worstFit() - Worst Fit algorithm

NEXT FILE: src/memory_manager.c
This will be the BIGGEST and most important file!
It implements all these algorithms with the actual logic.
================================================================================
*/