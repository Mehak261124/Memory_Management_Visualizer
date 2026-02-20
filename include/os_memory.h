/*
================================================================================
FILE: os_memory.h
PURPOSE: Real OS memory abstraction using macOS-native system calls
DESCRIPTION:
    - Provides a clean API for allocating/freeing real OS memory
    - Uses mmap() / munmap() — fully supported on macOS
    - Queries system info via sysctl (macOS) and sysconf (POSIX)
    - Each allocated region is tracked with base pointer and size
================================================================================
*/

#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#include <stddef.h>   // size_t


/*
================================================================================
STRUCTURE: OSRegion
================================================================================
PURPOSE: Represents one region of real OS-allocated memory

FIELDS:
- basePtr:  The real virtual address returned by mmap()
- size:     Size of the region in bytes (page-aligned)

EXAMPLE:
    OSRegion region;
    region.basePtr = 0x100000000;  // Actual address from the OS
    region.size    = 786432;       // 768 KB (768 * 1024 bytes)
*/

typedef struct {
    void   *basePtr;    // Real virtual address from mmap()
    size_t  size;       // Region size in bytes (page-aligned)
} OSRegion;


/*
================================================================================
FUNCTION DECLARATIONS
================================================================================
*/


/*
--------------------------------------------------------------------------------
FUNCTION: os_region_alloc
--------------------------------------------------------------------------------
PURPOSE: Allocate a region of real memory from the OS using mmap()

WHAT IT DOES:
1. Rounds the requested size UP to the nearest page boundary
2. Calls mmap() with MAP_PRIVATE | MAP_ANONYMOUS to get real pages
3. The OS kernel maps fresh pages into our address space
4. Returns an OSRegion with the real pointer and actual size

PARAMETERS:
- sizeBytes: Requested size in bytes (will be page-aligned upward)

RETURNS:
- Filled OSRegion on success (basePtr != NULL)
- OSRegion with basePtr = NULL on failure

SYSTEM CALL USED:
    mmap(NULL, alignedSize, PROT_READ | PROT_WRITE,
         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
*/
OSRegion os_region_alloc(size_t sizeBytes);


/*
--------------------------------------------------------------------------------
FUNCTION: os_region_free
--------------------------------------------------------------------------------
PURPOSE: Release a real memory region back to the OS using munmap()

WHAT IT DOES:
1. Calls munmap() to unmap the pages from our address space
2. The OS kernel reclaims those physical frames
3. Sets basePtr to NULL to prevent dangling pointer

PARAMETERS:
- region: Pointer to the OSRegion to free

SYSTEM CALL USED:
    munmap(region->basePtr, region->size)
*/
void os_region_free(OSRegion *region);


/*
--------------------------------------------------------------------------------
FUNCTION: os_get_page_size
--------------------------------------------------------------------------------
PURPOSE: Get the system page size (typically 4096 or 16384 on Apple Silicon)

RETURNS: Page size in bytes

SYSTEM CALL USED:
    sysconf(_SC_PAGESIZE)
*/
size_t os_get_page_size(void);


/*
--------------------------------------------------------------------------------
FUNCTION: os_get_total_ram
--------------------------------------------------------------------------------
PURPOSE: Get total physical RAM on this machine

RETURNS: Total RAM in bytes

SYSTEM CALL USED (macOS):
    sysctl(HW_MEMSIZE)
*/
size_t os_get_total_ram(void);


/*
--------------------------------------------------------------------------------
FUNCTION: os_get_system_info_json
--------------------------------------------------------------------------------
PURPOSE: Write a JSON string with system memory information

OUTPUT FORMAT:
{
    "pageSize": 16384,
    "totalRAM_MB": 16384,
    "backingType": "mmap",
    "arch": "arm64"
}

PARAMETERS:
- buffer: Output buffer for JSON string
- bufferSize: Size of the output buffer
*/
void os_get_system_info_json(char *buffer, int bufferSize);

/*
--------------------------------------------------------------------------------
FUNCTION: os_detect_memory_sizes
--------------------------------------------------------------------------------
PURPOSE: Dynamically detect system RAM and compute managed pool sizes

WHAT IT DOES:
1. Calls sysconf(_SC_PHYS_PAGES) to get total physical pages
2. Calls sysconf(_SC_PAGE_SIZE) to get page size
3. Computes total physical RAM = pages * page_size
4. Computes a managed pool size as a fraction of physical RAM
5. Returns total and OS memory sizes in KB for initializeMemory()

PARAMETERS:
- totalMemKB: Output - total managed memory in KB
- osMemKB:    Output - OS reserved memory in KB

SYSTEM CALLS USED:
    sysconf(_SC_PHYS_PAGES)  - total physical pages
    sysconf(_SC_PAGE_SIZE)   - size of each page
*/
void os_detect_memory_sizes(int *totalMemKB, int *osMemKB);


#endif /* OS_MEMORY_H */
