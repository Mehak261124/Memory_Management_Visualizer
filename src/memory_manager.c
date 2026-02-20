/*
================================================================================
FILE: memory_manager.c
PURPOSE: Implement memory management algorithms
DESCRIPTION: 
    - This is the CORE file of our project
    - Contains First Fit, Best Fit, Worst Fit implementations
    - Handles allocation, deallocation, fragmentation calculation
    - Implements Compaction (sliding defragmentation)
    - Implements Buddy System (power-of-2 allocation)
    - Provides JSON output functions for the HTTP API
    - MOST IMPORTANT FILE - read comments carefully!
================================================================================
*/

// Include necessary headers
#include <stdio.h>      // For printf, snprintf
#include <stdlib.h>     // For malloc, free
#include <string.h>     // For strlen, strcpy, memset, memcpy, memmove
#include "../include/memory_manager.h"
#include "../include/os_memory.h"


/*
================================================================================
FUNCTION: initializeMemory
================================================================================
PURPOSE: Set up the memory system when program starts

DETAILED EXPLANATION:
When the program starts, memory is completely free. We need to:
1. Set up total memory size
2. Reserve space for OS (Operating System)
3. Calculate how much is left for user processes
4. Create one big hole representing all free user memory
5. Reset all statistics counters

THINK OF IT LIKE:
Starting a new neighborhood:
- Total land: 1024 square meters
- Town hall (OS): 256 square meters
- Available for houses (user): 768 square meters
- Initially: One big empty lot of 768 square meters
*/

void initializeMemory(MemoryManager *mm, int totalMem, int osMem) {
    
    // STEP 1: Set basic memory sizes
    mm->totalMemory = totalMem;
    mm->osMemory = osMem;
    
    // STEP 2: Calculate user memory
    // User memory = Total - OS
    // Example: 1024 - 256 = 768 KB for users
    mm->userMemory = totalMem - osMem;
    
    // STEP 3: Initially, all user memory is free
    mm->freeMemory = mm->userMemory;
    
    // STEP 4: No processes loaded yet
    mm->numProcesses = 0;
    
    // STEP 5: One big hole (all user memory is free)
    mm->numHoles = 1;
    
    // STEP 6: Initialize counters
    mm->processCounter = 0;       // No processes assigned yet
    mm->nextBlockID = 1;          // Start block IDs at 1 (0 is reserved for OS)
    mm->useBuddySystem = 0;      // Standard allocation by default
    mm->totalAllocations = 0;
    mm->totalDeallocations = 0;
    mm->totalCompactions = 0;
    
    // STEP 7: Allocate REAL OS memory via mmap()
    // This is the key upgrade: we now have a real memory region from the OS!
    // userMemory is in KB, so we multiply by 1024 to get bytes.
    size_t backingSizeBytes = (size_t)mm->userMemory * 1024;
    mm->backingRegion = os_region_alloc(backingSizeBytes);
    
    if (mm->backingRegion.basePtr == NULL) {
        printf("WARNING: Failed to allocate real OS memory backing region!\n");
        printf("         Falling back to simulated mode (realPtr will be NULL)\n");
    } else {
        printf("[REAL OS MEMORY] Backing region allocated at %p (%zu bytes)\n",
               mm->backingRegion.basePtr, mm->backingRegion.size);
        printf("[REAL OS MEMORY] System page size: %zu bytes\n", os_get_page_size());
    }
    
    // STEP 8: Create the initial hole
    // This hole starts where OS ends and goes to the end of memory
    // Example: If OS is 0-255, user memory hole is 256-1023
    mm->head = createBlock(
        mm,                       // MemoryManager pointer for block ID
        1,                        // isHole = 1 (this is a hole, not a process)
        osMem,                    // startAddress = where OS ends
        totalMem - 1,             // endAddress = end of total memory
        -1                        // processID = -1 (it's a hole, no process)
    );
    
    // STEP 9: Wire the initial hole to real OS memory
    if (mm->backingRegion.basePtr != NULL) {
        mm->head->realPtr = mm->backingRegion.basePtr;
        mm->head->realSize = mm->backingRegion.size;
    }
    
    // STEP 10: Print confirmation message
    printf("\n=== Memory Initialized ===\n");
    printf("Total Memory: %d KB\n", mm->totalMemory);
    printf("OS Memory: %d KB\n", mm->osMemory);
    printf("User Memory: %d KB\n", mm->userMemory);
    if (mm->backingRegion.basePtr != NULL) {
        printf("Backing: mmap() at %p\n", mm->backingRegion.basePtr);
    }
    printf("==========================\n\n");
}

/*
VISUAL EXAMPLE:
initializeMemory(&mm, 1024, 256);

Before call:
[Nothing exists]

After call:
Memory layout:
[OS: 0-255 (256 KB)][HOLE: 256-1023 (768 KB)]

MemoryManager state:
totalMemory = 1024
osMemory = 256
userMemory = 768
freeMemory = 768
numProcesses = 0
numHoles = 1
processCounter = 0
nextBlockID = 2 (1 used for initial hole)
useBuddySystem = 0
*/


/*
================================================================================
FUNCTION: firstFit
================================================================================
PURPOSE: Allocate memory using First Fit algorithm

ALGORITHM EXPLANATION:
1. Start from the beginning of memory (head of linked list)
2. Look at each block one by one
3. If you find a HOLE that is big enough → USE IT
4. If hole is exactly the right size → convert entire hole to process
5. If hole is bigger → split it (process + remaining hole)
6. STOP searching as soon as you find a suitable hole

WHY IT'S CALLED "FIRST FIT":
Uses the FIRST hole that is large enough. Doesn't look for better options.

SPEED: FASTEST (stops at first match)
*/

int firstFit(MemoryManager *mm, int processID, int size) {
    
    // STEP 1: Start at the beginning of memory
    // 'current' is a pointer that will "walk" through our linked list
    MemoryBlock *current = mm->head;
    
    // STEP 2: Walk through the linked list looking for a suitable hole
    // Loop continues while current is not NULL (not at end of list)
    while (current != NULL) {
        
        // STEP 3: Check if this block is a hole AND big enough
        // We need TWO conditions to be true:
        //   1. current->isHole == 1 (it's free space)
        //   2. current->size >= size (it's big enough)
        if (current->isHole && current->size >= size) {
            
            // WE FOUND A SUITABLE HOLE! Now allocate it.
            
            // Save the starting address (we'll return this)
            int startAddr = current->startAddress;
            
            // CASE 1: Exact fit (hole size equals process size)
            // Example: Need 100 KB, hole is exactly 100 KB
            if (current->size == size) {
                
                // Convert the entire hole to a process
                current->isHole = 0;           // No longer a hole
                current->processID = processID; // Assign process ID
                
                // realPtr stays the same (inherited from the hole)
                current->realSize = (size_t)size * 1024;
                
                // Write a pattern byte into REAL memory to prove it's real!
                if (current->realPtr != NULL) {
                    memset(current->realPtr, processID & 0xFF, current->realSize);
                }
                
                // Update statistics
                mm->numHoles--;  // One less hole
                // (numProcesses updated at end of function)
            } 
            // CASE 2: Hole is bigger than needed - must split it
            // Example: Need 100 KB, hole is 200 KB
            else {
                
                // Calculate where the new hole should start
                // If process uses 0-99, new hole starts at 100
                int newStart = current->startAddress + size;
                
                // Save the original end address of the hole
                int oldEnd = current->endAddress;
                
                // Save the old realPtr before modifying current
                void *oldRealPtr = current->realPtr;
                
                // MODIFY current block to become the process
                current->endAddress = newStart - 1;  // Process ends before new hole
                current->size = size;                 // Process size
                current->isHole = 0;                  // It's a process now
                current->processID = processID;       // Assign process ID
                current->realSize = (size_t)size * 1024;
                
                // Write pattern byte into REAL memory
                if (current->realPtr != NULL) {
                    memset(current->realPtr, processID & 0xFF, current->realSize);
                }
                
                // CREATE new hole for the remaining space
                MemoryBlock *newHole = createBlock(
                    mm,             // MemoryManager for block ID
                    1,              // isHole = 1
                    newStart,       // starts after the process
                    oldEnd,         // ends where old hole ended
                    -1              // processID = -1 (it's a hole)
                );
                
                // Wire the new hole's realPtr to the offset portion
                if (oldRealPtr != NULL) {
                    newHole->realPtr = (char *)oldRealPtr + (size_t)size * 1024;
                    newHole->realSize = (size_t)(oldEnd - newStart + 1) * 1024;
                }
                
                // INSERT new hole into linked list
                // Put it right after current block
                newHole->next = current->next;  // New hole points to what current pointed to
                current->next = newHole;         // Current now points to new hole
                
                // Note: numHoles stays same (replaced one hole with process + new hole)
            }
            
            // STEP 4: Update statistics
            mm->numProcesses++;         // One more process
            mm->freeMemory -= size;     // Less free memory
            
            // STEP 5: Return starting address (SUCCESS!)
            return startAddr;
        }
        
        // STEP 6: This block didn't work, move to next block
        current = current->next;
    }
    
    // STEP 7: We went through entire list and found no suitable hole
    // Return -1 to indicate FAILURE
    return -1;
}

/*
VISUAL EXAMPLE OF FIRST FIT:

Initial memory:
[HOLE: 50KB][HOLE: 200KB][HOLE: 100KB]

Request: firstFit(&mm, 1, 80);  // Process P1 needs 80KB

Step-by-step:
1. Check first hole (50KB) → Too small, skip
2. Check second hole (200KB) → Big enough! USE IT
3. Split: [80KB for process][120KB remaining hole]

Result:
[HOLE: 50KB][P1: 80KB][HOLE: 120KB][HOLE: 100KB]
Returns: starting address of P1
*/


/*
================================================================================
FUNCTION: bestFit
================================================================================
PURPOSE: Allocate memory using Best Fit algorithm

ALGORITHM EXPLANATION:
1. Look through ALL holes in memory
2. Among holes that are big enough, find the SMALLEST one
3. Use that smallest suitable hole
4. Split if necessary

ADVANTAGE: Minimizes wasted space per allocation
DISADVANTAGE: Creates many tiny unusable holes over time
SPEED: SLOW (must check all holes)
*/

int bestFit(MemoryManager *mm, int processID, int size) {
    
    // STEP 1: Initialize search variables
    MemoryBlock *current = mm->head;     // Current block being checked
    MemoryBlock *bestBlock = NULL;       // Best hole found so far
    int minSize = mm->totalMemory + 1;   // Smallest suitable size found
                                         // Start with impossible value
    
    // STEP 2: Search through ALL blocks to find smallest suitable hole
    while (current != NULL) {
        
        // Check if this is a suitable hole
        if (current->isHole && current->size >= size) {
            
            // Is this smaller than our current best?
            if (current->size < minSize) {
                minSize = current->size;      // Update minimum
                bestBlock = current;          // This is new best
            }
        }
        
        // Move to next block
        current = current->next;
    }
    
    // STEP 3: Check if we found any suitable hole
    if (bestBlock == NULL) {
        return -1;  // No suitable hole found
    }
    
    // STEP 4: We found the best hole - now allocate it
    // (Same allocation logic as First Fit)
    
    int startAddr = bestBlock->startAddress;
    
    // Exact fit
    if (bestBlock->size == size) {
        bestBlock->isHole = 0;
        bestBlock->processID = processID;
        bestBlock->realSize = (size_t)size * 1024;
        if (bestBlock->realPtr != NULL) {
            memset(bestBlock->realPtr, processID & 0xFF, bestBlock->realSize);
        }
        mm->numHoles--;
    } 
    // Split hole
    else {
        int newStart = bestBlock->startAddress + size;
        int oldEnd = bestBlock->endAddress;
        void *oldRealPtr = bestBlock->realPtr;
        
        bestBlock->endAddress = newStart - 1;
        bestBlock->size = size;
        bestBlock->isHole = 0;
        bestBlock->processID = processID;
        bestBlock->realSize = (size_t)size * 1024;
        if (bestBlock->realPtr != NULL) {
            memset(bestBlock->realPtr, processID & 0xFF, bestBlock->realSize);
        }
        
        // Create new hole for remaining space
        MemoryBlock *newHole = createBlock(mm, 1, newStart, oldEnd, -1);
        if (oldRealPtr != NULL) {
            newHole->realPtr = (char *)oldRealPtr + (size_t)size * 1024;
            newHole->realSize = (size_t)(oldEnd - newStart + 1) * 1024;
        }
        newHole->next = bestBlock->next;
        bestBlock->next = newHole;
    }
    
    // Update statistics
    mm->numProcesses++;
    mm->freeMemory -= size;
    
    return startAddr;
}


/*
================================================================================
FUNCTION: worstFit
================================================================================
PURPOSE: Allocate memory using Worst Fit algorithm

ALGORITHM EXPLANATION:
1. Look through ALL holes in memory
2. Among holes that are big enough, find the LARGEST one
3. Use that largest hole
4. Split if necessary

ADVANTAGE: Leaves larger reusable holes
DISADVANTAGE: Wastes the biggest holes quickly
SPEED: SLOW (must check all holes)
*/

int worstFit(MemoryManager *mm, int processID, int size) {
    
    // STEP 1: Initialize search variables
    MemoryBlock *current = mm->head;
    MemoryBlock *worstBlock = NULL;   // Largest hole found
    int maxSize = -1;                 // Largest suitable size found
    
    // STEP 2: Search through ALL blocks to find largest suitable hole
    while (current != NULL) {
        
        // Check if this is a suitable hole
        if (current->isHole && current->size >= size) {
            
            // Is this larger than our current worst?
            if (current->size > maxSize) {
                maxSize = current->size;       // Update maximum
                worstBlock = current;          // This is new worst
            }
        }
        
        // Move to next block
        current = current->next;
    }
    
    // STEP 3: Check if we found any suitable hole
    if (worstBlock == NULL) {
        return -1;  // No suitable hole found
    }
    
    // STEP 4: Allocate using the worst (largest) hole
    
    int startAddr = worstBlock->startAddress;
    
    // Exact fit
    if (worstBlock->size == size) {
        worstBlock->isHole = 0;
        worstBlock->processID = processID;
        worstBlock->realSize = (size_t)size * 1024;
        if (worstBlock->realPtr != NULL) {
            memset(worstBlock->realPtr, processID & 0xFF, worstBlock->realSize);
        }
        mm->numHoles--;
    } 
    // Split hole
    else {
        int newStart = worstBlock->startAddress + size;
        int oldEnd = worstBlock->endAddress;
        void *oldRealPtr = worstBlock->realPtr;
        
        worstBlock->endAddress = newStart - 1;
        worstBlock->size = size;
        worstBlock->isHole = 0;
        worstBlock->processID = processID;
        worstBlock->realSize = (size_t)size * 1024;
        if (worstBlock->realPtr != NULL) {
            memset(worstBlock->realPtr, processID & 0xFF, worstBlock->realSize);
        }
        
        // Create new hole
        MemoryBlock *newHole = createBlock(mm, 1, newStart, oldEnd, -1);
        if (oldRealPtr != NULL) {
            newHole->realPtr = (char *)oldRealPtr + (size_t)size * 1024;
            newHole->realSize = (size_t)(oldEnd - newStart + 1) * 1024;
        }
        newHole->next = worstBlock->next;
        worstBlock->next = newHole;
    }
    
    // Update statistics
    mm->numProcesses++;
    mm->freeMemory -= size;
    
    return startAddr;
}


/*
================================================================================
FUNCTION: allocateMemory
================================================================================
PURPOSE: Main allocation function - calls appropriate algorithm

WHAT IT DOES:
This is a "wrapper" function. It:
1. Validates input (size, free memory)
2. Auto-assigns a process ID using the counter
3. Calls the appropriate algorithm based on 'algo' parameter
4. Updates total allocation counter on success
5. Returns result
*/

int allocateMemory(MemoryManager *mm, int processID, int size, 
                   AllocationAlgorithm algo) {
    
    // STEP 1: Validate process size
    if (size <= 0) {
        printf("Error: Invalid process size!\n");
        return -1;
    }
    
    // STEP 2: Check if enough free memory exists
    if (size > mm->freeMemory) {
        printf("Error: Not enough free memory!\n");
        printf("Requested: %d KB, Available: %d KB\n", size, mm->freeMemory);
        return -1;
    }
    
    // STEP 3: Call appropriate algorithm based on 'algo' parameter
    int result;
    
    // Switch statement - like multiple if-else
    // Checks the value of 'algo' and runs matching case
    switch (algo) {
        case FIRST_FIT:
            result = firstFit(mm, processID, size);
            break;  // Exit switch after this case
            
        case BEST_FIT:
            result = bestFit(mm, processID, size);
            break;
            
        case WORST_FIT:
            result = worstFit(mm, processID, size);
            break;
            
        default:
            // This shouldn't happen, but just in case
            return -1;
    }
    
    // STEP 4: If allocation succeeded, update the total counter
    if (result != -1) {
        mm->totalAllocations++;
    }
    
    // STEP 5: Return result from the algorithm
    return result;
}


/*
================================================================================
FUNCTION: deallocateMemory
================================================================================
PURPOSE: Free memory when a process finishes

ALGORITHM EXPLANATION:
1. Find the process in memory
2. Convert it to a hole
3. Try to merge with adjacent holes
4. Update statistics
*/

int deallocateMemory(MemoryManager *mm, int processID) {
    
    // STEP 1: Set up pointers to traverse list
    MemoryBlock *current = mm->head;  // Block we're checking
    MemoryBlock *prev = NULL;         // Previous block (needed for merging)
    
    // STEP 2: Find the process
    while (current != NULL) {
        
        // Check if this is the process we're looking for
        if (!current->isHole && current->processID == processID) {
            
            // FOUND IT! Now deallocate.
            
            // STEP 3: Convert process to hole
            current->isHole = 1;           // Mark as hole
            current->processID = -1;       // No process ID
            
            // Clear the REAL memory (zero it out like the OS does)
            if (current->realPtr != NULL) {
                memset(current->realPtr, 0, current->realSize);
            }
            
            // STEP 4: Update statistics
            mm->numProcesses--;
            mm->numHoles++;                // One more hole (for now)
            mm->freeMemory += current->size;  // More free memory
            mm->totalDeallocations++;      // Increment deallocation counter
            
            // STEP 5: Try to merge with NEXT block (if it's a hole)
            if (current->next != NULL && current->next->isHole) {
                
                MemoryBlock *nextHole = current->next;
                
                // Extend current block to include next hole
                current->endAddress = nextHole->endAddress;
                current->size = current->endAddress - current->startAddress + 1;
                
                // Merge real memory: keep current's realPtr, extend realSize
                current->realSize = (size_t)current->size * 1024;
                
                // Remove next hole from list
                current->next = nextHole->next;
                
                // Free the merged hole's memory
                free(nextHole);
                
                // One less hole (merged two into one)
                mm->numHoles--;
            }
            
            // STEP 6: Try to merge with PREVIOUS block (if it's a hole)
            if (prev != NULL && prev->isHole) {
                
                // Extend previous block to include current
                prev->endAddress = current->endAddress;
                prev->size = prev->endAddress - prev->startAddress + 1;
                
                // Merge real memory: prev keeps its realPtr, extend realSize
                prev->realSize = (size_t)prev->size * 1024;
                
                // Remove current from list
                prev->next = current->next;
                
                // Free current block's memory
                free(current);
                
                // One less hole (merged two into one)
                mm->numHoles--;
            }
            
            // SUCCESS!
            return 1;
        }
        
        // STEP 7: Move to next block
        prev = current;
        current = current->next;
    }
    
    // STEP 8: Process not found
    return 0;
}


/*
================================================================================
FUNCTION: displayMemory
================================================================================
PURPOSE: Show current state of memory on the screen
*/

void displayMemory(MemoryManager *mm) {
    
    // Print header
    printf("\n========== MEMORY STATE ==========\n");
    printf("|  Type  |   Address   |    Size     |\n");
    printf("==================================\n");
    
    // Display OS memory
    printf("| OS     | %4d - %4d | Size: %4d KB |\n", 
           0, mm->osMemory - 1, mm->osMemory);
    printf("----------------------------------\n");
    
    // Display user memory blocks
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        displayBlock(current);  // Use our displayBlock function
        current = current->next;
    }
    
    // Print footer with statistics
    printf("==================================\n");
    printf("Free Memory: %d KB\n", mm->freeMemory);
    printf("Processes: %d | Holes: %d\n", mm->numProcesses, mm->numHoles);
    printf("==================================\n\n");
}


/*
================================================================================
FUNCTION: calculateFragmentation
================================================================================
PURPOSE: Calculate external fragmentation percentage

FORMULA:
Fragmentation% = (Total Free - Largest Hole) / User Memory * 100
*/

float calculateFragmentation(MemoryManager *mm) {
    
    // If no free memory, no fragmentation
    if (mm->freeMemory == 0) {
        return 0.0;
    }
    
    // Find the largest hole
    int largestHole = 0;
    MemoryBlock *current = mm->head;
    
    while (current != NULL) {
        if (current->isHole && current->size > largestHole) {
            largestHole = current->size;
        }
        current = current->next;
    }
    
    // Calculate fragmented memory
    // This is the free memory that's NOT in the largest hole
    int fragmentedMemory = mm->freeMemory - largestHole;
    
    // Calculate percentage
    float fragmentation = (float)fragmentedMemory / mm->userMemory * 100;
    
    return fragmentation;
}


/*
================================================================================
FUNCTION: freeMemoryManager
================================================================================
PURPOSE: Clean up all allocated memory blocks
*/

void freeMemoryManager(MemoryManager *mm) {
    
    MemoryBlock *current = mm->head;
    
    // Walk through list and free each block
    while (current != NULL) {
        MemoryBlock *temp = current;     // Save current
        current = current->next;          // Move to next
        free(temp);                       // Free saved block
    }
    
    // Set head to NULL (list is now empty)
    mm->head = NULL;
    
    // NOTE: We do NOT free backingRegion here because compact()
    // calls freeMemoryManager but needs the backing region to survive.
    // The backing region is freed in resetMemory() and at program exit.
}


/*
================================================================================
FUNCTION: compact
================================================================================
PURPOSE: Perform sliding compaction on memory

DETAILED EXPLANATION:
Compaction moves all allocated processes to the beginning of user memory
and creates one large hole at the end. This eliminates ALL external 
fragmentation.

THINK OF IT LIKE:
Books on a shelf: [Book1][gap][Book2][gap][Book3][gap]
After compaction:  [Book1][Book2][Book3][     big gap    ]

ALGORITHM:
1. Record fragmentation before compaction
2. Walk through blocks, collecting all processes
3. Rebuild the linked list: all processes first, then one hole
4. Record fragmentation after compaction
5. Return statistics about what was done

WHY IS THIS IMPORTANT?
Without compaction, free memory gets fragmented into many small holes.
Even if total free memory is enough, no single hole may be big enough
for a new process. Compaction solves this by consolidating free space.

Before: [OS][P1:100][HOLE:50][P2:200][HOLE:30][P3:100][HOLE:288]
Frag = (368-288)/768 * 100 = 10.4%

After:  [OS][P1:100][P2:200][P3:100][HOLE:368]
Frag = 0% (all free memory in one hole!)
*/

int compact(MemoryManager *mm, char *resultBuffer, int bufferSize) {
    
    // STEP 1: Count allocated processes
    int processCount = 0;
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        if (!current->isHole) {
            processCount++;
        }
        current = current->next;
    }
    
    // If no processes, nothing to compact
    if (processCount == 0) {
        if (resultBuffer != NULL) {
            snprintf(resultBuffer, bufferSize,
                "{\"success\":false,\"message\":\"No processes to compact\"}");
        }
        return 0;
    }
    
    // STEP 2: Record metrics BEFORE compaction
    float fragBefore = calculateFragmentation(mm);
    int holesBefore = mm->numHoles;
    
    // STEP 3: Collect all process info (processID and size)
    // We need arrays to temporarily store process data
    int processIDs[100];     // Store up to 100 process IDs
    int processSizes[100];   // Store their sizes
    int processIdx = 0;
    int totalMoved = 0;
    int totalBytesMoved = 0;
    
    current = mm->head;
    int expectedAddr = mm->osMemory;  // Where the first process should be
    
    while (current != NULL) {
        if (!current->isHole) {
            processIDs[processIdx] = current->processID;
            processSizes[processIdx] = current->size;
            
            // Check if this process needs to move
            if (current->startAddress != expectedAddr) {
                totalMoved++;
                totalBytesMoved += (current->startAddress - expectedAddr);
            }
            expectedAddr += current->size;
            processIdx++;
        }
        current = current->next;
    }
    
    // STEP 4: Compact REAL memory using memmove
    // This is what a real OS does during compaction!
    // We move the actual bytes in the mmap'd region.
    if (mm->backingRegion.basePtr != NULL) {
        size_t destOffset = 0;
        current = mm->head;
        while (current != NULL) {
            if (!current->isHole && current->realPtr != NULL) {
                void *dest = (char *)mm->backingRegion.basePtr + destOffset;
                if (dest != current->realPtr) {
                    // memmove handles overlapping regions safely
                    memmove(dest, current->realPtr, current->realSize);
                    printf("[COMPACT] Moved P%d real memory: %p -> %p (%zu bytes)\n",
                           current->processID, current->realPtr, dest, current->realSize);
                }
                destOffset += current->realSize;
            }
            current = current->next;
        }
        // Zero out the freed space at the end
        size_t totalUsedBytes = destOffset;
        size_t remainingBytes = mm->backingRegion.size - totalUsedBytes;
        if (remainingBytes > 0) {
            memset((char *)mm->backingRegion.basePtr + totalUsedBytes, 0, remainingBytes);
        }
    }
    
    // STEP 5: Free old linked list (keeps backingRegion alive)
    freeMemoryManager(mm);
    
    // STEP 6: Rebuild linked list with compacted layout
    // All processes packed together starting at osMemory
    int currentAddr = mm->osMemory;
    MemoryBlock *prevBlock = NULL;
    size_t realOffset = 0;
    
    for (int i = 0; i < processIdx; i++) {
        int endAddr = currentAddr + processSizes[i] - 1;
        
        MemoryBlock *block = createBlock(mm, 0, currentAddr, endAddr, processIDs[i]);
        
        // Wire up realPtr to the compacted position in the backing region
        if (mm->backingRegion.basePtr != NULL) {
            block->realPtr = (char *)mm->backingRegion.basePtr + realOffset;
            block->realSize = (size_t)processSizes[i] * 1024;
            realOffset += block->realSize;
        }
        
        if (i == 0) {
            mm->head = block;  // First block becomes the head
        } else {
            prevBlock->next = block;  // Link to previous block
        }
        prevBlock = block;
        currentAddr = endAddr + 1;
    }
    
    // STEP 7: Create one big hole at the end (if there's space left)
    int remainingSpace = mm->totalMemory - currentAddr;
    if (remainingSpace > 0) {
        MemoryBlock *hole = createBlock(mm, 1, currentAddr, mm->totalMemory - 1, -1);
        
        // Wire the hole's realPtr to the remaining backing region
        if (mm->backingRegion.basePtr != NULL) {
            hole->realPtr = (char *)mm->backingRegion.basePtr + realOffset;
            hole->realSize = mm->backingRegion.size - realOffset;
        }
        
        if (prevBlock != NULL) {
            prevBlock->next = hole;
        } else {
            mm->head = hole;  // If no processes, hole is the head
        }
        mm->numHoles = 1;
    } else {
        mm->numHoles = 0;
    }
    
    // STEP 7: Update statistics
    mm->numProcesses = processIdx;
    mm->freeMemory = remainingSpace;
    mm->totalCompactions++;
    
    // STEP 8: Record metrics AFTER compaction
    float fragAfter = calculateFragmentation(mm);
    int holesAfter = mm->numHoles;
    
    // STEP 9: Write result JSON
    if (resultBuffer != NULL) {
        snprintf(resultBuffer, bufferSize,
            "{\"success\":true,"
            "\"processesMovedCount\":%d,"
            "\"totalBytesMoved\":%d,"
            "\"fragmentationBefore\":%.1f,"
            "\"fragmentationAfter\":%.1f,"
            "\"holesBefore\":%d,"
            "\"holesAfter\":%d,"
            "\"message\":\"Compaction complete: Moved %d processes\"}",
            totalMoved, totalBytesMoved,
            fragBefore, fragAfter,
            holesBefore, holesAfter,
            totalMoved
        );
    }
    
    printf("Compaction complete: Moved %d processes\n", totalMoved);
    printf("Fragmentation: %.1f%% → %.1f%%\n", fragBefore, fragAfter);
    
    return 1;
}


/*
================================================================================
FUNCTION: autoCompact
================================================================================
PURPOSE: Automatically compact memory if fragmentation exceeds threshold

ALGORITHM:
1. Calculate current fragmentation percentage
2. Compare against threshold
3. If above threshold → run compact()
4. If below threshold → skip and return info message
*/

int autoCompact(MemoryManager *mm, int threshold, char *resultBuffer, int bufferSize) {
    
    // Calculate current fragmentation
    float frag = calculateFragmentation(mm);
    
    // Check if fragmentation exceeds threshold
    if (frag > (float)threshold) {
        // Fragmentation is high — perform compaction
        return compact(mm, resultBuffer, bufferSize);
    }
    
    // Fragmentation is acceptable — skip compaction
    if (resultBuffer != NULL) {
        snprintf(resultBuffer, bufferSize,
            "{\"success\":false,"
            "\"message\":\"Fragmentation (%.1f%%) is below threshold (%d%%)\"}",
            frag, threshold
        );
    }
    
    return 0;
}


/*
================================================================================
FUNCTION: nextPowerOf2
================================================================================
PURPOSE: Round a number up to the nearest power of 2

WHY DO WE NEED THIS?
The buddy system only works with power-of-2 block sizes.
When a process requests 50 KB, we need to allocate 64 KB (next power of 2).

ALGORITHM:
We use bit manipulation to find the next power of 2:
1. Subtract 1 from n
2. OR with right-shifted versions to "fill in" all lower bits
3. Add 1 to get next power of 2

EXAMPLES:
  nextPowerOf2(50)  → 64
  nextPowerOf2(64)  → 64 (already a power of 2)
  nextPowerOf2(100) → 128
  nextPowerOf2(1)   → 1
*/

int nextPowerOf2(int n) {
    
    // Handle edge cases
    if (n <= 1) return 1;
    
    // Bit manipulation trick to find next power of 2
    n--;              // Subtract 1
    n |= n >> 1;     // OR with shifted versions
    n |= n >> 2;     // This "fills in" all bits below the highest set bit
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;              // Add 1 to get next power of 2
    
    return n;
}


/*
================================================================================
FUNCTION: buddyAllocate
================================================================================
PURPOSE: Allocate memory using the Buddy System

DETAILED EXPLANATION:
The buddy system divides memory into blocks that are powers of 2.
When we need to allocate, we:
1. Round the requested size up to the next power of 2
2. Find the smallest free block that is big enough
3. If the block is too big, split it into two "buddies"
4. Keep splitting until we get the right size
5. Allocate the block

EXAMPLE:
Request: 50 KB → Rounded to 64 KB (next power of 2)
Available: [256 KB free block]
Split 256 → [128 KB] + [128 KB]
Split 128 → [64 KB] + [64 KB] + [128 KB]
Allocate → [P1: 64 KB] + [FREE: 64 KB] + [FREE: 128 KB]

ADVANTAGES:
- Fast allocation and deallocation
- Efficient buddy merging when freeing
- No external fragmentation (but has internal fragmentation)

DISADVANTAGE:
- Internal fragmentation (wasted space within allocated blocks)
  Example: 50 KB request gets 64 KB → 14 KB wasted inside the block
*/

int buddyAllocate(MemoryManager *mm, int size, char *resultBuffer, int bufferSize) {
    
    // STEP 1: Round up requested size to next power of 2
    int allocSize = nextPowerOf2(size);
    
    // STEP 2: Auto-assign a process ID
    int processID = ++(mm->processCounter);
    
    // STEP 3: Find a free block that is big enough
    MemoryBlock *targetBlock = NULL;
    MemoryBlock *current = mm->head;
    
    while (current != NULL) {
        if (current->isHole && current->size >= allocSize) {
            targetBlock = current;
            break;  // Use first suitable block (for buddy system)
        }
        current = current->next;
    }
    
    // No suitable block found
    if (targetBlock == NULL) {
        if (resultBuffer != NULL) {
            snprintf(resultBuffer, bufferSize,
                "{\"success\":false,\"message\":\"No suitable buddy block found\"}");
        }
        return -1;
    }
    
    // STEP 4: Split the block until it's the right size
    // Each split creates two "buddy" blocks of half the size
    while (targetBlock->size > allocSize) {
        
        int halfSize = targetBlock->size / 2;
        
        // Create the second buddy (right half)
        MemoryBlock *buddy2 = createBlock(
            mm,
            1,                                    // isHole = 1 (free)
            targetBlock->startAddress + halfSize,  // Starts at halfway point
            targetBlock->endAddress,               // Ends where original ended
            -1                                     // No process
        );
        
        // Wire buddy2's realPtr to the right half of the parent's real memory
        if (targetBlock->realPtr != NULL) {
            buddy2->realPtr = (char *)targetBlock->realPtr + (size_t)halfSize * 1024;
            buddy2->realSize = (size_t)halfSize * 1024;
        }
        
        // Shrink the first buddy (left half) = targetBlock
        targetBlock->endAddress = targetBlock->startAddress + halfSize - 1;
        targetBlock->size = halfSize;
        if (targetBlock->realPtr != NULL) {
            targetBlock->realSize = (size_t)halfSize * 1024;
        }
        
        // Track buddy relationship using buddyID
        targetBlock->buddyID = buddy2->blockID;
        buddy2->buddyID = targetBlock->blockID;
        
        // Insert buddy2 into linked list after targetBlock
        buddy2->next = targetBlock->next;
        targetBlock->next = buddy2;
        
        // Update hole count (we split one hole into two smaller ones)
        mm->numHoles++;
    }
    
    // STEP 5: Allocate the target block
    targetBlock->isHole = 0;
    targetBlock->processID = processID;
    targetBlock->realSize = (size_t)allocSize * 1024;
    
    // Write pattern byte into REAL memory
    if (targetBlock->realPtr != NULL) {
        memset(targetBlock->realPtr, processID & 0xFF, targetBlock->realSize);
    }
    
    // Update statistics
    mm->numProcesses++;
    mm->numHoles--;
    mm->freeMemory -= targetBlock->size;
    mm->totalAllocations++;
    
    // STEP 6: Write result JSON
    if (resultBuffer != NULL) {
        snprintf(resultBuffer, bufferSize,
            "{\"success\":true,"
            "\"processId\":\"P%d\","
            "\"requestedSize\":%d,"
            "\"allocatedSize\":%d,"
            "\"wastedSpace\":%d,"
            "\"startAddress\":%d}",
            processID, size, allocSize,
            allocSize - size,
            targetBlock->startAddress
        );
    }
    
    return targetBlock->startAddress;
}


/*
================================================================================
FUNCTION: buddyDeallocate
================================================================================
PURPOSE: Deallocate a process under buddy system and merge buddy pairs

ALGORITHM:
1. Find the process block
2. Mark it as free
3. Check if its buddy is also free
4. If yes → merge them back together
5. Recursively check if the merged block's buddy is also free

EXAMPLE (Merging):
Before: [P1: 64KB][FREE: 64KB][FREE: 128KB]
Deallocate P1: [FREE: 64KB][FREE: 64KB][FREE: 128KB]
Merge buddies: [FREE: 128KB][FREE: 128KB]
Merge buddies: [FREE: 256KB]
*/

int buddyDeallocate(MemoryManager *mm, int processID, char *resultBuffer, int bufferSize) {
    
    // STEP 1: Find the process block
    MemoryBlock *current = mm->head;
    
    while (current != NULL) {
        if (!current->isHole && current->processID == processID) {
            
            // FOUND IT!
            
            // STEP 2: Mark as free
            current->isHole = 1;
            current->processID = -1;
            
            // Clear REAL memory
            if (current->realPtr != NULL) {
                memset(current->realPtr, 0, current->realSize);
            }
            
            // Update statistics
            mm->numProcesses--;
            mm->numHoles++;
            mm->freeMemory += current->size;
            mm->totalDeallocations++;
            
            // STEP 3: Try to merge with buddy (recursively)
            // We keep trying to merge until no more merging is possible
            int merged = 1;
            while (merged) {
                merged = 0;
                
                // Walk through all blocks looking for buddy pairs to merge
                MemoryBlock *block = mm->head;
                MemoryBlock *prevBlock = NULL;
                
                while (block != NULL) {
                    // Check if this free block has a buddy
                    if (block->isHole && block->buddyID != -1) {
                        
                        // Find the buddy block
                        MemoryBlock *buddy = mm->head;
                        MemoryBlock *prevBuddy = NULL;
                        
                        while (buddy != NULL) {
                            if (buddy->blockID == block->buddyID) {
                                break;
                            }
                            prevBuddy = buddy;
                            buddy = buddy->next;
                        }
                        
                        // If buddy found and also free → MERGE
                        if (buddy != NULL && buddy->isHole) {
                            
                            // Determine which block comes first in memory
                            MemoryBlock *first = (block->startAddress < buddy->startAddress) ? block : buddy;
                            MemoryBlock *second = (first == block) ? buddy : block;
                            
                            // Extend first block to cover both
                            first->endAddress = second->endAddress;
                            first->size = first->endAddress - first->startAddress + 1;
                            first->buddyID = -1;  // Reset buddy (will find new buddy later)
                            
                            // Merge real memory: first keeps its realPtr, extend realSize
                            first->realSize = (size_t)first->size * 1024;
                            
                            // Remove second block from linked list
                            MemoryBlock *search = mm->head;
                            MemoryBlock *searchPrev = NULL;
                            while (search != NULL) {
                                if (search == second) {
                                    if (searchPrev != NULL) {
                                        searchPrev->next = search->next;
                                    } else {
                                        mm->head = search->next;
                                    }
                                    free(second);
                                    break;
                                }
                                searchPrev = search;
                                search = search->next;
                            }
                            
                            mm->numHoles--;
                            merged = 1;
                            break;  // Restart the merge scan
                        }
                    }
                    
                    prevBlock = block;
                    block = block->next;
                }
            }
            
            // STEP 4: Write result JSON
            if (resultBuffer != NULL) {
                snprintf(resultBuffer, bufferSize,
                    "{\"success\":true,\"processId\":\"P%d\"}",
                    processID
                );
            }
            
            return 1;
        }
        
        current = current->next;
    }
    
    // Process not found
    if (resultBuffer != NULL) {
        snprintf(resultBuffer, bufferSize,
            "{\"success\":false,\"message\":\"Process P%d not found\"}",
            processID
        );
    }
    
    return 0;
}


/*
================================================================================
FUNCTION: convertToBuddySystem
================================================================================
PURPOSE: Convert current memory layout to buddy system

ALGORITHM:
1. Save all current processes (their IDs and sizes)
2. Free the entire linked list
3. Reinitialize memory with power-of-2 size for buddy system
4. Re-allocate each saved process using buddyAllocate
*/

int convertToBuddySystem(MemoryManager *mm, char *resultBuffer, int bufferSize) {
    
    // STEP 1: Save current processes
    int savedIDs[100];
    int savedSizes[100];
    int savedCount = 0;
    
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        if (!current->isHole) {
            savedIDs[savedCount] = current->processID;
            savedSizes[savedCount] = current->size;
            savedCount++;
        }
        current = current->next;
    }
    
    // STEP 2: Free old memory and backing region
    freeMemoryManager(mm);
    os_region_free(&mm->backingRegion);
    
    // STEP 3: Round user memory to nearest power of 2
    int buddySize = 1;
    while (buddySize < mm->userMemory) {
        buddySize *= 2;
    }
    // Use the largest power of 2 that fits in user memory
    if (buddySize > mm->userMemory) {
        buddySize /= 2;
    }
    
    // STEP 4: Allocate new backing region for buddy system
    mm->backingRegion = os_region_alloc((size_t)buddySize * 1024);
    
    // STEP 5: Initialize buddy system
    mm->useBuddySystem = 1;
    mm->numProcesses = 0;
    mm->numHoles = 1;
    mm->freeMemory = buddySize;
    mm->processCounter = 0;
    
    // Create one big buddy block
    mm->head = createBlock(mm, 1, mm->osMemory, mm->osMemory + buddySize - 1, -1);
    if (mm->backingRegion.basePtr != NULL) {
        mm->head->realPtr = mm->backingRegion.basePtr;
        mm->head->realSize = mm->backingRegion.size;
    }
    
    // STEP 5: Re-allocate saved processes using buddy system
    int successCount = 0;
    char tempBuf[512];
    
    for (int i = 0; i < savedCount; i++) {
        mm->processCounter = savedIDs[i] - 1;  // Set counter so PID matches
        int result = buddyAllocate(mm, savedSizes[i], tempBuf, sizeof(tempBuf));
        if (result != -1) {
            successCount++;
        }
    }
    
    // STEP 6: Write result JSON
    if (resultBuffer != NULL) {
        snprintf(resultBuffer, bufferSize,
            "{\"success\":true,"
            "\"message\":\"Converted to buddy system. %d/%d processes re-allocated.\","
            "\"buddyMemorySize\":%d,"
            "\"processesConverted\":%d,"
            "\"totalProcesses\":%d}",
            successCount, savedCount,
            buddySize,
            successCount,
            savedCount
        );
    }
    
    return 1;
}


/*
================================================================================
FUNCTION: revertFromBuddySystem
================================================================================
PURPOSE: Revert from buddy system back to standard allocation

ALGORITHM:
1. Save all current processes
2. Free the entire linked list
3. Reinitialize memory normally (standard layout)
4. Re-allocate each saved process using first-fit
*/

int revertFromBuddySystem(MemoryManager *mm, char *resultBuffer, int bufferSize) {
    
    // STEP 1: Save current processes
    int savedIDs[100];
    int savedSizes[100];
    int savedCount = 0;
    
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        if (!current->isHole) {
            savedIDs[savedCount] = current->processID;
            savedSizes[savedCount] = current->size;
            savedCount++;
        }
        current = current->next;
    }
    
    // STEP 2: Free old memory and backing region
    freeMemoryManager(mm);
    os_region_free(&mm->backingRegion);
    mm->useBuddySystem = 0;
    
    // Allocate new backing region (standard size)
    mm->backingRegion = os_region_alloc((size_t)mm->userMemory * 1024);
    
    // Reinitialize normally
    mm->numProcesses = 0;
    mm->numHoles = 1;
    mm->freeMemory = mm->userMemory;
    
    // Create initial hole (standard layout)
    mm->head = createBlock(mm, 1, mm->osMemory, mm->totalMemory - 1, -1);
    if (mm->backingRegion.basePtr != NULL) {
        mm->head->realPtr = mm->backingRegion.basePtr;
        mm->head->realSize = mm->backingRegion.size;
    }
    
    // STEP 3: Re-allocate saved processes using first-fit
    int successCount = 0;
    
    for (int i = 0; i < savedCount; i++) {
        int result = allocateMemory(mm, savedIDs[i], savedSizes[i], FIRST_FIT);
        if (result != -1) {
            successCount++;
        }
    }
    
    // STEP 4: Write result JSON
    if (resultBuffer != NULL) {
        snprintf(resultBuffer, bufferSize,
            "{\"success\":true,"
            "\"message\":\"Reverted to standard allocation. %d/%d processes re-allocated.\","
            "\"processesConverted\":%d,"
            "\"totalProcesses\":%d}",
            successCount, savedCount,
            successCount,
            savedCount
        );
    }
    
    return 1;
}


/*
================================================================================
FUNCTION: resetMemory
================================================================================
PURPOSE: Reset memory to its initial state (like pressing the reset button)

WHAT IT DOES:
1. Free all memory blocks
2. Reinitialize with the same total/OS memory sizes
3. All counters reset to zero
*/

void resetMemory(MemoryManager *mm) {
    
    // Save the original sizes
    int totalMem = mm->totalMemory;
    int osMem = mm->osMemory;
    
    // Free the linked list
    freeMemoryManager(mm);
    
    // Free the old OS backing region via munmap()
    os_region_free(&mm->backingRegion);
    
    // Reinitialize from scratch (will allocate new backing region)
    initializeMemory(mm, totalMem, osMem);
}


/*
================================================================================
FUNCTION: getStatsJSON
================================================================================
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
*/

void getStatsJSON(MemoryManager *mm, char *buffer, int bufferSize) {
    
    // Calculate current fragmentation
    float frag = calculateFragmentation(mm);
    
    // Calculate used memory
    int usedMemory = mm->userMemory - mm->freeMemory;
    
    // Find largest hole size
    int largestHole = 0;
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        if (current->isHole && current->size > largestHole) {
            largestHole = current->size;
        }
        current = current->next;
    }
    
    // Format backing region address as hex
    char backingAddrStr[32];
    if (mm->backingRegion.basePtr != NULL) {
        snprintf(backingAddrStr, sizeof(backingAddrStr), "\"0x%lx\"", (unsigned long)mm->backingRegion.basePtr);
    } else {
        snprintf(backingAddrStr, sizeof(backingAddrStr), "null");
    }
    
    // Build the JSON string (with real OS memory info)
    snprintf(buffer, bufferSize,
        "{\"totalMemory\":%d,"
        "\"osMemory\":%d,"
        "\"userMemory\":%d,"
        "\"usedMemory\":%d,"
        "\"freeMemory\":%d,"
        "\"numProcesses\":%d,"
        "\"numHoles\":%d,"
        "\"largestHole\":%d,"
        "\"fragmentation\":%.1f,"
        "\"totalAllocations\":%d,"
        "\"totalDeallocations\":%d,"
        "\"totalCompactions\":%d,"
        "\"useBuddySystem\":%s,"
        "\"backingType\":\"mmap/munmap\","
        "\"backingRegionBase\":%s,"
        "\"backingRegionSize\":%zu,"
        "\"systemPageSize\":%zu}",
        mm->totalMemory,
        mm->osMemory,
        mm->userMemory,
        usedMemory,
        mm->freeMemory,
        mm->numProcesses,
        mm->numHoles,
        largestHole,
        frag,
        mm->totalAllocations,
        mm->totalDeallocations,
        mm->totalCompactions,
        mm->useBuddySystem ? "true" : "false",
        backingAddrStr,
        mm->backingRegion.size,
        os_get_page_size()
    );
}


/*
================================================================================
END OF FILE: memory_manager.c
================================================================================

WHAT WE IMPLEMENTED:
1.  initializeMemory() - Set up initial memory state (with stats reset)
2.  firstFit() - First Fit allocation algorithm
3.  bestFit() - Best Fit allocation algorithm  
4.  worstFit() - Worst Fit allocation algorithm
5.  allocateMemory() - Main allocation function (wrapper)
6.  deallocateMemory() - Free memory and merge holes
7.  displayMemory() - Show memory state
8.  calculateFragmentation() - Measure fragmentation
9.  freeMemoryManager() - Clean up memory
10. compact() - Sliding compaction with JSON result
11. autoCompact() - Auto-compact based on threshold
12. nextPowerOf2() - Helper for buddy system
13. buddyAllocate() - Buddy system allocation with splitting
14. buddyDeallocate() - Buddy system deallocation with merging
15. convertToBuddySystem() - Switch to buddy system
16. revertFromBuddySystem() - Switch back to standard
17. resetMemory() - Reset to initial state
18. getStatsJSON() - Memory stats as JSON

THIS IS THE CORE OF YOUR PROJECT!
All the OS concepts you learned are implemented here.
================================================================================
*/