/*
================================================================================
FILE: memory_structures.c
PURPOSE: Implement basic operations for memory structures
DESCRIPTION: 
    - This file contains the actual CODE for the functions we declared
    - We create blocks, display blocks, and serialize them to JSON
    - JSON serialization is needed for the HTTP API to communicate
      with the React frontend
================================================================================
*/

// STEP 1: Include necessary header files
// #include tells C to "bring in" code from other files

// stdio.h = Standard Input/Output - gives us printf(), snprintf(), sprintf()
#include <stdio.h>

// stdlib.h = Standard Library - gives us malloc(), free()
// malloc = memory allocation (reserve space)
// free = release memory
#include <stdlib.h>

// string.h = String operations - gives us strlen(), strcat(), strcpy()
#include <string.h>

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
4. Assign a unique blockID from the manager
5. Return the pointer to this new block

PARAMETERS EXPLAINED:
- mm: Pointer to MemoryManager (used to get the next block ID)
- isHole: Is this a free space (1) or process (0)?
- start: Where does this block begin? (address in KB)
- end: Where does this block finish? (address in KB)
- pid: Process ID (if it's a process), or -1 if it's a hole

EXAMPLE USAGE:
createBlock(&mm, 1, 256, 355, -1)
This creates: A hole (free space) from 256 to 355 KB (size = 100 KB)

createBlock(&mm, 0, 256, 355, 5)
This creates: Process P5 from 256 to 355 KB (size = 100 KB)
*/

MemoryBlock* createBlock(MemoryManager *mm, int isHole, int start, int end, int pid) {
    
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
    
    // STEP 4: Assign a unique block ID from the manager
    // This is used by the buddy system to track buddy pairs
    // If mm is NULL (used during initialization), use 0
    if (mm != NULL) {
        newBlock->blockID = mm->nextBlockID++;
    } else {
        newBlock->blockID = 0;
    }
    
    // No buddy pair yet (-1 means no buddy)
    newBlock->buddyID = -1;
    
    // Real OS memory fields — initialized to NULL/0
    // These will be set by the memory manager when backed by mmap()
    newBlock->realPtr = NULL;
    newBlock->realSize = 0;
    
    // Set next pointer to NULL (no next block yet)
    // NULL means "points to nothing"
    newBlock->next = NULL;
    
    // STEP 5: Return pointer to the newly created block
    return newBlock;
}

/*
VISUAL EXAMPLE OF WHAT createBlock DOES:

Call: createBlock(&mm, 0, 256, 355, 3)

Before call:
    [nothing exists yet]

After call:
    newBlock → [isHole: 0]
                [startAddress: 256]
                [endAddress: 355]
                [size: 100]
                [processID: 3]
                [blockID: auto-assigned]
                [buddyID: -1]
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
================================================================================
FUNCTION 3: blockToJSON
================================================================================
PURPOSE: Convert a single memory block to a JSON string

WHY DO WE NEED THIS?
The React frontend communicates with our C backend via HTTP.
HTTP sends text — and JSON is the standard text format for structured data.
So we convert our C struct into a JSON string that JavaScript can understand.

HOW IT WORKS:
1. Build a JSON object string using snprintf()
2. Include all relevant fields: id, startAddress, size, isHole, processId
3. Write the JSON into the provided buffer

PARAMETER:
- block: Pointer to the block to serialize
- buffer: Character array to write JSON into
- bufferSize: Maximum size of the buffer (to prevent overflow)

EXAMPLE OUTPUT:
If block is Process P3 at 256-355 (100 KB):
{"id":7,"startAddress":256,"endAddress":355,"size":100,"isHole":false,"processId":"P3"}

If block is a Hole at 356-455 (100 KB):
{"id":8,"startAddress":356,"endAddress":455,"size":100,"isHole":true,"processId":null}
*/

void blockToJSON(MemoryBlock *block, char *buffer, int bufferSize) {
    
    // Format the real address as a hex string (or "null" if not backed)
    char realAddrStr[32];
    if (block->realPtr != NULL) {
        snprintf(realAddrStr, sizeof(realAddrStr), "\"0x%lx\"", (unsigned long)block->realPtr);
    } else {
        snprintf(realAddrStr, sizeof(realAddrStr), "null");
    }
    
    // Use snprintf to safely write into buffer (prevents buffer overflow)
    // snprintf(buffer, maxSize, formatString, values...)
    
    if (block->isHole) {
        // Hole block: processId is null (JSON null, not a string)
        snprintf(buffer, bufferSize,
            "{\"id\":%d,\"startAddress\":%d,\"endAddress\":%d,"
            "\"size\":%d,\"isHole\":true,\"processId\":null,"
            "\"blockID\":%d,\"buddyID\":%d,"
            "\"realAddress\":%s,\"realSize\":%zu}",
            block->blockID,
            block->startAddress,
            block->endAddress,
            block->size,
            block->blockID,
            block->buddyID,
            realAddrStr,
            block->realSize
        );
    } else {
        // Process block: processId is a string like "P3"
        snprintf(buffer, bufferSize,
            "{\"id\":%d,\"startAddress\":%d,\"endAddress\":%d,"
            "\"size\":%d,\"isHole\":false,\"processId\":\"P%d\","
            "\"blockID\":%d,\"buddyID\":%d,"
            "\"realAddress\":%s,\"realSize\":%zu}",
            block->blockID,
            block->startAddress,
            block->endAddress,
            block->size,
            block->processID,
            block->blockID,
            block->buddyID,
            realAddrStr,
            block->realSize
        );
    }
}


/*
================================================================================
FUNCTION 4: blocksToJSON
================================================================================
PURPOSE: Convert ALL memory blocks (entire linked list) to a JSON array string

HOW IT WORKS:
1. Start with "[" (JSON array open bracket)
2. Walk through linked list, converting each block to JSON
3. Add commas between blocks
4. End with "]" (JSON array close bracket)

PARAMETER:
- mm: Pointer to MemoryManager (we traverse from mm->head)
- buffer: Character array to write JSON array into
- bufferSize: Maximum size of the buffer

EXAMPLE OUTPUT:
[
  {"id":1,"startAddress":256,"size":100,"isHole":false,"processId":"P1"},
  {"id":2,"startAddress":356,"size":100,"isHole":true,"processId":null}
]

NOTE: We also include the OS block at the beginning (address 0 to osMemory-1)
*/

void blocksToJSON(MemoryManager *mm, char *buffer, int bufferSize) {
    
    // STEP 1: Start the JSON array
    // Begin with the OS block (always at address 0)
    int written = snprintf(buffer, bufferSize,
        "[{\"id\":0,\"startAddress\":0,\"endAddress\":%d,"
        "\"size\":%d,\"isHole\":false,\"processId\":\"OS\","
        "\"blockID\":0,\"buddyID\":-1}",
        mm->osMemory - 1,
        mm->osMemory
    );
    
    // STEP 2: Walk through linked list and add each block
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        
        // Convert current block to JSON
        char blockJSON[512];
        blockToJSON(current, blockJSON, sizeof(blockJSON));
        
        // Append comma + block JSON to the array
        // Check we have enough buffer space remaining
        int remaining = bufferSize - written;
        if (remaining > (int)strlen(blockJSON) + 3) {
            written += snprintf(buffer + written, remaining, ",%s", blockJSON);
        }
        
        // Move to next block in linked list
        current = current->next;
    }
    
    // STEP 3: Close the JSON array
    if (written < bufferSize - 1) {
        buffer[written] = ']';
        buffer[written + 1] = '\0';
    }
}


/*
================================================================================
END OF FILE: memory_structures.c
================================================================================

WHAT WE IMPLEMENTED:
1. createBlock() - Creates and initializes a new MemoryBlock (with blockID)
2. displayBlock() - Prints block information in formatted way
3. blockToJSON() - Converts single block to JSON string
4. blocksToJSON() - Converts all blocks to JSON array string

KEY C CONCEPTS USED:
- malloc() - Allocate memory dynamically
- sizeof() - Get size of a data type
- Pointers (*) - Variables that store memory addresses
- Arrow operator (->) - Access fields through pointer
- NULL - Special pointer value meaning "points to nothing"
- printf() - Print formatted text to screen
- snprintf() - Safely write formatted text to buffer
- strlen() - Get length of a string
- Format specifiers (%d, %4d, %-5d) - Control how numbers are printed

NEXT FILE: src/memory_manager.c
This implements the allocation algorithms and defragmentation!
================================================================================
*/