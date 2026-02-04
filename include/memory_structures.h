/*
================================================================================
FILE: memory_structures.h
PURPOSE: Define data structures for memory management
DESCRIPTION: 
    - This file contains the "blueprints" for our data structures
    - Think of structures like a form/template with fields to fill in
    - We define 3 main structures: MemoryBlock, Process, and MemoryManager
================================================================================
*/

// Header guard - prevents this file from being included multiple times
// If MEMORY_STRUCTURES_H is not defined, define it and include the code
// This is a standard C practice
#ifndef MEMORY_STRUCTURES_H
#define MEMORY_STRUCTURES_H

/*
================================================================================
STRUCTURE 1: MemoryBlock
================================================================================
PURPOSE: Represents one block of memory (either a process or a hole/free space)

THINK OF IT LIKE:
Imagine memory as a street with houses. Each MemoryBlock is like one house 
or empty lot. Some houses have people living in them (processes), some are 
empty (holes).

EXAMPLE:
Street (Memory):
[House P1][Empty Lot][House P2][Empty Lot][House P3]

Each of these is a MemoryBlock.
*/

typedef struct MemoryBlock {
    // FIELD 1: isHole
    // Purpose: Tell us if this block is free (hole) or occupied (process)
    // Value: 1 = hole (free), 0 = process (occupied)
    // Example: isHole = 1 means this is empty space
    int isHole;
    
    // FIELD 2: startAddress
    // Purpose: Where does this block start in memory?
    // Value: Starting memory address (in KB)
    // Example: startAddress = 256 means this block starts at 256 KB
    int startAddress;
    
    // FIELD 3: endAddress
    // Purpose: Where does this block end in memory?
    // Value: Ending memory address (in KB)
    // Example: endAddress = 355 means this block ends at 355 KB
    int endAddress;
    
    // FIELD 4: size
    // Purpose: How big is this block?
    // Value: Size in KB (calculated as endAddress - startAddress + 1)
    // Example: If start=256, end=355, then size=100 KB
    int size;
    
    // FIELD 5: processID
    // Purpose: If this is a process, which process is it?
    // Value: Process ID number, or -1 if it's a hole
    // Example: processID = 3 means Process P3 is here
    //          processID = -1 means this is empty (hole)
    int processID;
    
    // FIELD 6: next
    // Purpose: Pointer to the next block in memory
    // Why? We're using a LINKED LIST to store blocks
    // Think of it like: Each block holds the address of the next block
    // Example: Block 1 → Block 2 → Block 3 → NULL (end)
    struct MemoryBlock *next;
    
} MemoryBlock;
// NOTE: The semicolon after } is important!


/*
================================================================================
STRUCTURE 2: Process
================================================================================
PURPOSE: Represents a process (program) that needs memory

THINK OF IT LIKE:
A process is like a person who wants to rent a house. They have:
- An ID (name)
- Size requirements (how many rooms they need)
- Status (have they found a house yet?)
- Address (if they found a house, where is it?)
*/

typedef struct Process {
    // FIELD 1: processID
    // Purpose: Unique identifier for this process
    // Value: Process number (1, 2, 3, etc.)
    // Example: processID = 5 means this is Process P5
    int processID;
    
    // FIELD 2: size
    // Purpose: How much memory does this process need?
    // Value: Memory size in KB
    // Example: size = 200 means this process needs 200 KB
    int size;
    
    // FIELD 3: isAllocated
    // Purpose: Has this process been given memory yet?
    // Value: 0 = waiting (no memory yet), 1 = allocated (has memory)
    // Example: isAllocated = 0 means still waiting for memory
    int isAllocated;
    
    // FIELD 4: startAddress
    // Purpose: If allocated, where is the process in memory?
    // Value: Starting address in KB, or -1 if not allocated
    // Example: startAddress = 256 means process starts at 256 KB
    //          startAddress = -1 means not allocated yet
    int startAddress;
    
} Process;


/*
================================================================================
STRUCTURE 3: MemoryManager
================================================================================
PURPOSE: Manages the entire memory system

THINK OF IT LIKE:
The MemoryManager is like a property manager who:
- Keeps track of all houses/lots on the street
- Knows total size of the street
- Knows how much is occupied vs free
- Maintains a list of all blocks
*/

typedef struct MemoryManager {
    // FIELD 1: head
    // Purpose: Pointer to the first block in our linked list
    // Think of it like: The address of the first house on the street
    // We start here and follow the "next" pointers to see all blocks
    MemoryBlock *head;
    
    // FIELD 2: totalMemory
    // Purpose: Total size of memory (OS + User memory)
    // Value: Total KB available
    // Example: totalMemory = 1024 means 1024 KB total
    int totalMemory;
    
    // FIELD 3: osMemory
    // Purpose: How much memory is reserved for Operating System?
    // Value: OS memory size in KB
    // Example: osMemory = 256 means first 256 KB reserved for OS
    int osMemory;
    
    // FIELD 4: userMemory
    // Purpose: How much memory is available for user processes?
    // Value: User memory size in KB
    // Calculation: userMemory = totalMemory - osMemory
    // Example: If total=1024, os=256, then user=768 KB
    int userMemory;
    
    // FIELD 5: freeMemory
    // Purpose: How much free space is currently available?
    // Value: Free memory size in KB
    // This decreases when we allocate, increases when we deallocate
    // Example: freeMemory = 500 means 500 KB currently free
    int freeMemory;
    
    // FIELD 6: numProcesses
    // Purpose: How many processes are currently in memory?
    // Value: Count of processes
    // Example: numProcesses = 3 means 3 processes loaded
    int numProcesses;
    
    // FIELD 7: numHoles
    // Purpose: How many holes (free blocks) exist?
    // Value: Count of holes
    // Example: numHoles = 2 means 2 separate free spaces
    int numHoles;
    
} MemoryManager;


/*
================================================================================
FUNCTION DECLARATIONS
================================================================================
These are like "promises" - we're telling C these functions will exist.
The actual code for these functions will be in memory_structures.c
*/

// FUNCTION 1: createBlock
// Purpose: Create a new memory block
// Parameters:
//   - isHole: 1 if hole, 0 if process
//   - start: starting address
//   - end: ending address
//   - pid: process ID (-1 for holes)
// Returns: Pointer to the newly created block
// Example: createBlock(1, 100, 199, -1) creates a 100KB hole at address 100
MemoryBlock* createBlock(int isHole, int start, int end, int pid);


// FUNCTION 2: displayBlock
// Purpose: Print information about a single block to the screen
// Parameters:
//   - block: pointer to the block to display
// Returns: Nothing (void)
// Example: displayBlock(myBlock) prints "| P1 | 100 - 199 | Size: 100 KB |"
void displayBlock(MemoryBlock *block);


// End of header guard
#endif

/*
================================================================================
END OF FILE: memory_structures.h
================================================================================

SUMMARY OF WHAT WE DEFINED:
1. MemoryBlock structure - represents one piece of memory
2. Process structure - represents a program needing memory
3. MemoryManager structure - manages all memory blocks
4. Two function declarations - createBlock() and displayBlock()

NEXT FILE: memory_structures.c (will implement these functions)
================================================================================
*/