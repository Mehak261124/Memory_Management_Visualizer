/*
================================================================================
FILE: memory_manager.c
PURPOSE: Implement memory management algorithms
DESCRIPTION: 
    - This is the CORE file of our project
    - Contains First Fit, Best Fit, Worst Fit implementations
    - Handles allocation, deallocation, fragmentation calculation
    - MOST IMPORTANT FILE - read comments carefully!
================================================================================
*/

// Include necessary headers
#include <stdio.h>      // For printf, scanf
#include <stdlib.h>     // For malloc, free
#include "../include/memory_manager.h"


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
    
    // STEP 6: Create the initial hole
    // This hole starts where OS ends and goes to the end of memory
    // Example: If OS is 0-255, user memory hole is 256-1023
    mm->head = createBlock(
        1,                    // isHole = 1 (this is a hole, not a process)
        osMem,                // startAddress = where OS ends
        totalMem - 1,         // endAddress = end of total memory
        -1                    // processID = -1 (it's a hole, no process)
    );
    
    // STEP 7: Print confirmation message
    printf("\n=== Memory Initialized ===\n");
    printf("Total Memory: %d KB\n", mm->totalMemory);
    printf("OS Memory: %d KB\n", mm->osMemory);
    printf("User Memory: %d KB\n", mm->userMemory);
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
head → [isHole:1, start:256, end:1023, size:768, pid:-1, next:NULL]
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
                
                // MODIFY current block to become the process
                current->endAddress = newStart - 1;  // Process ends before new hole
                current->size = size;                 // Process size
                current->isHole = 0;                  // It's a process now
                current->processID = processID;       // Assign process ID
                
                // CREATE new hole for the remaining space
                MemoryBlock *newHole = createBlock(
                    1,              // isHole = 1
                    newStart,       // starts after the process
                    oldEnd,         // ends where old hole ended
                    -1              // processID = -1 (it's a hole)
                );
                
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

Notice: Didn't check third hole - stopped at first match!
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

WHY IT'S CALLED "BEST FIT":
Finds the "best" hole - the one that wastes the least space.

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
        mm->numHoles--;
    } 
    // Split hole
    else {
        int newStart = bestBlock->startAddress + size;
        int oldEnd = bestBlock->endAddress;
        
        bestBlock->endAddress = newStart - 1;
        bestBlock->size = size;
        bestBlock->isHole = 0;
        bestBlock->processID = processID;
        
        // Create new hole for remaining space
        MemoryBlock *newHole = createBlock(1, newStart, oldEnd, -1);
        newHole->next = bestBlock->next;
        bestBlock->next = newHole;
    }
    
    // Update statistics
    mm->numProcesses++;
    mm->freeMemory -= size;
    
    return startAddr;
}

/*
VISUAL EXAMPLE OF BEST FIT:

Initial memory:
[HOLE: 50KB][HOLE: 200KB][HOLE: 100KB]

Request: bestFit(&mm, 1, 80);  // Process P1 needs 80KB

Step-by-step:
1. Check all holes:
   - 50KB: Too small
   - 200KB: Suitable (size: 200)
   - 100KB: Suitable (size: 100) ← SMALLEST suitable!
2. Use 100KB hole (best fit)
3. Split: [80KB for process][20KB remaining hole]

Result:
[HOLE: 50KB][HOLE: 200KB][P1: 80KB][HOLE: 20KB]
Returns: starting address of P1

Notice: The 20KB remaining hole might be too small to use later!
This is the problem with Best Fit - creates tiny holes.
*/


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

WHY IT'S CALLED "WORST FIT":
Uses the "worst" (largest) hole, leaving bigger remaining space.

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
        mm->numHoles--;
    } 
    // Split hole
    else {
        int newStart = worstBlock->startAddress + size;
        int oldEnd = worstBlock->endAddress;
        
        worstBlock->endAddress = newStart - 1;
        worstBlock->size = size;
        worstBlock->isHole = 0;
        worstBlock->processID = processID;
        
        // Create new hole
        MemoryBlock *newHole = createBlock(1, newStart, oldEnd, -1);
        newHole->next = worstBlock->next;
        worstBlock->next = newHole;
    }
    
    // Update statistics
    mm->numProcesses++;
    mm->freeMemory -= size;
    
    return startAddr;
}

/*
VISUAL EXAMPLE OF WORST FIT:

Initial memory:
[HOLE: 50KB][HOLE: 200KB][HOLE: 100KB]

Request: worstFit(&mm, 1, 80);  // Process P1 needs 80KB

Step-by-step:
1. Check all holes:
   - 50KB: Too small
   - 200KB: Suitable (size: 200) ← LARGEST suitable!
   - 100KB: Suitable (size: 100)
2. Use 200KB hole (worst fit)
3. Split: [80KB for process][120KB remaining hole]

Result:
[HOLE: 50KB][P1: 80KB][HOLE: 120KB][HOLE: 100KB]
Returns: starting address of P1

Notice: The 120KB remaining hole is still quite large and usable!
This is the advantage of Worst Fit.
*/


/*
================================================================================
FUNCTION: allocateMemory
================================================================================
PURPOSE: Main allocation function - calls appropriate algorithm

WHAT IT DOES:
This is a "wrapper" function. It:
1. Validates input (size, free memory)
2. Calls the appropriate algorithm based on 'algo' parameter
3. Returns result

THINK OF IT LIKE:
A restaurant menu selector:
- Customer says: "I want Chinese food"
- This function says: "Okay, let me call the Chinese chef"
- Calls firstFit/bestFit/worstFit based on choice
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
    
    // STEP 4: Return result from the algorithm
    return result;
}

/*
EXAMPLE USAGE:

// Allocate 100KB to Process P1 using First Fit
int addr = allocateMemory(&mm, 1, 100, FIRST_FIT);

// Allocate 200KB to Process P2 using Best Fit
int addr = allocateMemory(&mm, 2, 200, BEST_FIT);

// Allocate 150KB to Process P3 using Worst Fit
int addr = allocateMemory(&mm, 3, 150, WORST_FIT);
*/


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

WHY MERGE HOLES?
Without merging:
[P1][HOLE:50KB][HOLE:50KB][HOLE:50KB] = Can't fit 120KB process!

With merging:
[P1][HOLE:150KB] = Can fit 120KB process!

Merging prevents fragmentation from getting worse.
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
            
            // STEP 4: Update statistics
            mm->numProcesses--;
            mm->numHoles++;                // One more hole (for now)
            mm->freeMemory += current->size;  // More free memory
            
            // STEP 5: Try to merge with NEXT block (if it's a hole)
            if (current->next != NULL && current->next->isHole) {
                
                MemoryBlock *nextHole = current->next;
                
                // Extend current block to include next hole
                current->endAddress = nextHole->endAddress;
                current->size = current->endAddress - current->startAddress + 1;
                
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
VISUAL EXAMPLE OF DEALLOCATION WITH MERGING:

Scenario 1: Merge with next hole
Before: [P1][P2:100KB][HOLE:50KB][P3]
Deallocate P2:
After:  [P1][HOLE:150KB][P3]

Scenario 2: Merge with previous hole
Before: [P1][HOLE:50KB][P2:100KB][P3]
Deallocate P2:
After:  [P1][HOLE:150KB][P3]

Scenario 3: Merge with both
Before: [P1][HOLE:50KB][P2:100KB][HOLE:50KB][P3]
Deallocate P2:
After:  [P1][HOLE:200KB][P3]

Scenario 4: No merging (processes on both sides)
Before: [P1][P2:100KB][P3]
Deallocate P2:
After:  [P1][HOLE:100KB][P3]
*/


/*
================================================================================
FUNCTION: displayMemory
================================================================================
PURPOSE: Show current state of memory on the screen

WHAT IT DISPLAYS:
1. OS region
2. All memory blocks (processes and holes)
3. Statistics (free memory, processes, holes)
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

LOGIC:
- If all free memory is in one hole → 0% fragmentation (perfect!)
- If free memory is scattered → high fragmentation (bad!)
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
EXAMPLE:

User Memory: 1000 KB
Free Memory: 500 KB
Holes: [50KB][200KB][100KB][150KB]

Largest hole = 200 KB
Fragmented memory = 500 - 200 = 300 KB
Fragmentation = (300 / 1000) * 100 = 30%

Interpretation: 30% of total memory is wasted in unusable fragments.
*/


/*
================================================================================
FUNCTION: freeMemoryManager
================================================================================
PURPOSE: Clean up all allocated memory blocks

WHY WE NEED THIS:
Every malloc() must have a matching free(), otherwise we have a memory leak.
This function walks through the entire linked list and frees each block.
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
}

/*
VISUAL EXAMPLE:

Before freeMemoryManager:
head → [Block1] → [Block2] → [Block3] → NULL

After freeMemoryManager:
head → NULL
(All blocks freed, memory returned to system)
*/


/*
================================================================================
END OF FILE: memory_manager.c
================================================================================

WHAT WE IMPLEMENTED:
1. initializeMemory() - Set up initial memory state
2. firstFit() - First Fit allocation algorithm
3. bestFit() - Best Fit allocation algorithm  
4. worstFit() - Worst Fit allocation algorithm
5. allocateMemory() - Main allocation function (wrapper)
6. deallocateMemory() - Free memory and merge holes
7. displayMemory() - Show memory state
8. calculateFragmentation() - Measure fragmentation
9. freeMemoryManager() - Clean up memory

THIS IS THE CORE OF YOUR PROJECT!
All the OS concepts you learned are implemented here.

NEXT FILES:
- graphics_display.h and graphics_display.c (visualization)
- main.c (user interface)

These are simpler - the hard part is done!
================================================================================
*/