/*
================================================================================
FILE: memory_structures.c
PURPOSE: Implement basic operations for memory structures
DESCRIPTION: 
    - This file contains the actual CODE for the functions we declared
    - We create blocks, display blocks, etc.
================================================================================
*/

// STEP 1: Include necessary header files
// #include tells C to "bring in" code from other files

// stdio.h = Standard Input/Output - gives us printf(), scanf()
#include <stdio.h>

// stdlib.h = Standard Library - gives us malloc(), free()
// malloc = memory allocation (reserve space)
// free = release memory
#include <stdlib.h>

// Our own header file - brings in structure definitions
// ../ means "go up one folder", then into include/
#include "../include/memory_structures.h"


/*
================================================================================
FUNCTION 1: createBlock
================================================================================
PURPOSE: Create a new memory block and return pointer to it

HOW IT WORKS (step by step):
1. Reserve memory for a new MemoryBlock structure
2. Fill in all the fields with the given values
3. Calculate the size
4. Return the pointer to this new block

PARAMETERS EXPLAINED:
- isHole: Is this a free space (1) or process (0)?
- start: Where does this block begin? (address in KB)
- end: Where does this block finish? (address in KB)
- pid: Process ID (if it's a process), or -1 if it's a hole

EXAMPLE USAGE:
createBlock(1, 256, 355, -1)
This creates: A hole (free space) from 256 to 355 KB (size = 100 KB)

createBlock(0, 256, 355, 5)
This creates: Process P5 from 256 to 355 KB (size = 100 KB)
*/

MemoryBlock* createBlock(int isHole, int start, int end, int pid) {
    
    // STEP 1: Allocate memory for the new block
    // malloc() asks the system for memory
    // sizeof(MemoryBlock) tells malloc how much space we need
    // (MemoryBlock*) converts the returned pointer to the right type
    MemoryBlock *newBlock = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    
    // STEP 2: Check if malloc was successful
    // malloc returns NULL if it couldn't allocate memory
    if (newBlock == NULL) {
        // If malloc failed, print error and return NULL
        printf("Error: Memory allocation failed!\n");
        printf("Cannot create new memory block.\n");
        return NULL;  // Return NULL to indicate failure
    }
    
    // STEP 3: Fill in the fields of our new block
    
    // Set whether this is a hole or process
    newBlock->isHole = isHole;
    // -> is used to access fields through a pointer
    // newBlock->isHole means "the isHole field of the block newBlock points to"
    
    // Set the starting address
    newBlock->startAddress = start;
    
    // Set the ending address
    newBlock->endAddress = end;
    
    // Calculate and set the size
    // Size = (end - start + 1)
    // Example: If start=256, end=355, then size = 355-256+1 = 100
    // The +1 is because both start and end are included
    newBlock->size = end - start + 1;
    
    // Set the process ID
    newBlock->processID = pid;
    
    // Set next pointer to NULL (no next block yet)
    // NULL means "points to nothing"
    newBlock->next = NULL;
    
    // STEP 4: Return pointer to the newly created block
    return newBlock;
}

/*
VISUAL EXAMPLE OF WHAT createBlock DOES:

Call: createBlock(0, 256, 355, 3)

Before call:
    [nothing exists yet]

After call:
    newBlock → [isHole: 0]
                [startAddress: 256]
                [endAddress: 355]
                [size: 100]
                [processID: 3]
                [next: NULL]

We return this pointer so other parts of the program can use this block!
*/


/*
================================================================================
FUNCTION 2: displayBlock
================================================================================
PURPOSE: Print information about a memory block to the screen

HOW IT WORKS:
1. Check if the block is a hole or process
2. Print differently based on what it is
3. Show address range and size

PARAMETER:
- block: Pointer to the block we want to display

EXAMPLE OUTPUT (if it's a process):
| P3     | 256 - 355  | Size: 100  KB |

EXAMPLE OUTPUT (if it's a hole):
| HOLE   | 356 - 455  | Size: 100  KB |
*/

void displayBlock(MemoryBlock *block) {
    
    // STEP 1: Check what type of block this is
    if (block->isHole) {
        // This is a HOLE (free space)
        
        // printf() prints text to the screen
        // Format specifiers:
        //   %4d = print an integer, use at least 4 characters, right-aligned
        // Example: If number is 5, it prints "   5" (3 spaces + 5)
        
        printf("| HOLE   | %4d - %4d | Size: %4d KB |\n", 
               block->startAddress,    // First %4d
               block->endAddress,      // Second %4d
               block->size);           // Third %4d
        
        // \n at the end means "new line" (move to next line)
        
    } else {
        // This is a PROCESS (occupied space)
        
        // %-5d means left-aligned, at least 5 characters
        // Example: processID=3 prints "P3   " (P3 + 3 spaces)
        
        printf("| P%-5d | %4d - %4d | Size: %4d KB |\n", 
               block->processID,       // First %d (for P_)
               block->startAddress,    // First %4d
               block->endAddress,      // Second %4d
               block->size);           // Third %4d
    }
}

/*
VISUAL EXAMPLE OF displayBlock OUTPUT:

Example 1: Hole from 256 to 355
displayBlock(holeBlock);
Output: | HOLE   |  256 -  355 | Size:  100 KB |

Example 2: Process P3 from 256 to 355
displayBlock(processBlock);
Output: | P3     |  256 -  355 | Size:  100 KB |

Example 3: Process P12 from 0 to 99
displayBlock(processBlock);
Output: | P12    |    0 -   99 | Size:  100 KB |

The formatting makes it look nice and aligned!
*/


/*
================================================================================
END OF FILE: memory_structures.c
================================================================================

WHAT WE IMPLEMENTED:
1. createBlock() - Creates and initializes a new MemoryBlock
2. displayBlock() - Prints block information in formatted way

KEY C CONCEPTS USED:
- malloc() - Allocate memory dynamically
- sizeof() - Get size of a data type
- Pointers (*) - Variables that store memory addresses
- Arrow operator (->) - Access fields through pointer
- NULL - Special pointer value meaning "points to nothing"
- printf() - Print formatted text to screen
- Format specifiers (%d, %4d, %-5d) - Control how numbers are printed

NEXT FILE: include/memory_manager.h
This will declare our main allocation algorithms!
================================================================================
*/