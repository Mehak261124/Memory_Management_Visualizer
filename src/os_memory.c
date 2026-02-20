/*
================================================================================
FILE: os_memory.c
PURPOSE: Implement real OS memory operations using macOS-native system calls
DESCRIPTION:
    - Uses mmap() / munmap() for allocating/freeing real virtual memory pages
    - Uses sysconf() for page size, sysctl() for total RAM
    - All functions are macOS-compatible (no deprecated sbrk/brk)
    - This module is the bridge between our simulator and the real OS kernel
================================================================================
*/

#include <stdio.h>          // printf, snprintf
#include <string.h>         // memset
#include <sys/mman.h>       // mmap, munmap, PROT_READ, PROT_WRITE, MAP_PRIVATE, MAP_ANONYMOUS
#include <unistd.h>         // sysconf, _SC_PAGESIZE
#include <sys/types.h>      // size_t
#include <sys/sysctl.h>     // sysctl, HW_MEMSIZE (macOS)

#include "../include/os_memory.h"


/*
================================================================================
FUNCTION: os_get_page_size
================================================================================
PURPOSE: Return the system's virtual memory page size

HOW IT WORKS:
    sysconf(_SC_PAGESIZE) asks the OS kernel for the page size.
    On Intel Macs this is 4096 bytes (4 KB).
    On Apple Silicon (M1/M2/M3) this is 16384 bytes (16 KB).

WHY THIS MATTERS:
    mmap() allocates memory in whole pages. If you request 1 byte,
    you still get an entire page. We align our requests to page
    boundaries so we don't waste or misalign memory.
*/

size_t os_get_page_size(void) {
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) {
        // Fallback: assume 4096 if sysconf fails
        return 4096;
    }
    return (size_t)pageSize;
}


/*
================================================================================
FUNCTION: os_get_total_ram
================================================================================
PURPOSE: Query total physical RAM on this Mac using sysctl()

HOW IT WORKS:
    sysctl() with CTL_HW + HW_MEMSIZE queries the kernel for
    the total installed physical memory. This is a macOS-specific
    system call (Linux would use /proc/meminfo instead).

RETURNS: Total RAM in bytes (e.g., 17179869184 for 16 GB)
*/

size_t os_get_total_ram(void) {
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    int64_t memSize = 0;
    size_t len = sizeof(memSize);

    if (sysctl(mib, 2, &memSize, &len, NULL, 0) == 0) {
        return (size_t)memSize;
    }

    // Fallback if sysctl fails
    return 0;
}


/*
================================================================================
FUNCTION: os_region_alloc
================================================================================
PURPOSE: Allocate a region of real virtual memory from the OS kernel

DETAILED EXPLANATION:

    mmap() is the primary system call for memory allocation on macOS.
    It asks the kernel to map pages of memory into our process's
    virtual address space.

    Parameters we use:
    - NULL         → Let the kernel choose the virtual address
    - alignedSize  → How many bytes to allocate (page-aligned)
    - PROT_READ | PROT_WRITE → We want to read and write this memory
    - MAP_PRIVATE  → Changes are private to our process
    - MAP_ANONYMOUS → Not backed by any file (pure RAM/swap)
    - -1, 0        → No file descriptor, no offset (anonymous mapping)

    The kernel will:
    1. Find a free range in our virtual address space
    2. Create page table entries for the new pages
    3. Pages are initially zero-filled (demand-paged on first access)
    4. Return a pointer to the start of the mapped region

PAGE ALIGNMENT:
    If you request 1000 bytes and pages are 4096 bytes, we round up
    to 4096 bytes. This ensures we don't cross page boundaries in
    unexpected ways.

EXAMPLE:
    os_region_alloc(768 * 1024)  →  Allocates 768 KB of real memory
    The returned OSRegion.basePtr is a real virtual address like 0x104000000
*/

OSRegion os_region_alloc(size_t sizeBytes) {
    OSRegion region;
    region.basePtr = NULL;
    region.size = 0;

    if (sizeBytes == 0) {
        printf("[os_memory] Error: Cannot allocate 0 bytes\n");
        return region;
    }

    // STEP 1: Align size to page boundary
    size_t pageSize = os_get_page_size();
    size_t alignedSize = ((sizeBytes + pageSize - 1) / pageSize) * pageSize;

    // STEP 2: Call mmap() — the real OS system call!
    //
    // This is where the magic happens:
    // We're asking the macOS kernel to give us REAL virtual memory pages.
    //
    // MAP_ANON is the macOS name for MAP_ANONYMOUS
    void *ptr = mmap(
        NULL,                          // Let kernel choose address
        alignedSize,                   // Size (page-aligned)
        PROT_READ | PROT_WRITE,        // Read + Write permissions
        MAP_PRIVATE | MAP_ANON,        // Private anonymous mapping
        -1,                            // No file descriptor
        0                              // No offset
    );

    // STEP 3: Check if mmap succeeded
    if (ptr == MAP_FAILED) {
        perror("[os_memory] mmap() failed");
        return region;
    }

    // STEP 4: Fill in the region struct
    region.basePtr = ptr;
    region.size = alignedSize;

    printf("[os_memory] mmap() allocated %zu bytes at address %p (requested %zu)\n",
           alignedSize, ptr, sizeBytes);

    return region;
}


/*
================================================================================
FUNCTION: os_region_free
================================================================================
PURPOSE: Release a memory region back to the OS kernel using munmap()

DETAILED EXPLANATION:

    munmap() is the counterpart to mmap(). It tells the kernel:
    "I'm done with these pages, you can reclaim them."

    The kernel will:
    1. Remove the page table entries for this range
    2. Mark the physical frames as available
    3. Any future access to these addresses will cause a segfault

    This is the OS-level equivalent of free(), but at the page level.

EXAMPLE:
    os_region_free(&region);
    // After this, region.basePtr is NULL and the memory is gone
*/

void os_region_free(OSRegion *region) {
    if (region == NULL || region->basePtr == NULL) {
        return;
    }

    // Call munmap() — the real OS system call!
    int result = munmap(region->basePtr, region->size);

    if (result == 0) {
        printf("[os_memory] munmap() freed %zu bytes at address %p\n",
               region->size, region->basePtr);
    } else {
        perror("[os_memory] munmap() failed");
    }

    // Clear the region to prevent use-after-free
    region->basePtr = NULL;
    region->size = 0;
}


/*
================================================================================
FUNCTION: os_get_system_info_json
================================================================================
PURPOSE: Generate a JSON string with real system memory information

WHAT IT QUERIES:
1. Page size via sysconf(_SC_PAGESIZE)
2. Total physical RAM via sysctl(HW_MEMSIZE)
3. Architecture detected at compile time

OUTPUT FORMAT:
{
    "pageSize": 16384,
    "totalRAM_bytes": 17179869184,
    "totalRAM_MB": 16384,
    "backingType": "mmap/munmap",
    "arch": "arm64",
    "osName": "macOS"
}
*/

void os_get_system_info_json(char *buffer, int bufferSize) {
    size_t pageSize = os_get_page_size();
    size_t totalRAM = os_get_total_ram();
    size_t totalRAM_MB = totalRAM / (1024 * 1024);
    
    // Get physical pages using sysconf
    long physPages = sysconf(_SC_PHYS_PAGES);
    if (physPages < 0) physPages = 0;
    
    // Get dynamically detected pool sizes
    int detectedTotal = 0, detectedOS = 0;
    os_detect_memory_sizes(&detectedTotal, &detectedOS);

    // Detect architecture at compile time
    const char *arch = "unknown";
#if defined(__aarch64__) || defined(__arm64__)
    arch = "arm64 (Apple Silicon)";
#elif defined(__x86_64__)
    arch = "x86_64 (Intel)";
#elif defined(__i386__)
    arch = "i386";
#endif

    snprintf(buffer, bufferSize,
        "{\"pageSize\":%zu,"
        "\"totalRAM_bytes\":%zu,"
        "\"totalRAM_MB\":%zu,"
        "\"physicalPages\":%ld,"
        "\"detectedPoolSize_KB\":%d,"
        "\"detectedOSReserved_KB\":%d,"
        "\"backingType\":\"mmap/munmap\","
        "\"arch\":\"%s\","
        "\"osName\":\"macOS\","
        "\"systemCalls\":[\"mmap()\",\"munmap()\",\"sysconf(_SC_PHYS_PAGES)\",\"sysconf(_SC_PAGE_SIZE)\",\"sysctl(HW_MEMSIZE)\"]}",
        pageSize,
        totalRAM,
        totalRAM_MB,
        physPages,
        detectedTotal,
        detectedOS,
        arch
    );
}


/*
================================================================================
FUNCTION: os_detect_memory_sizes
================================================================================
PURPOSE: Dynamically detect system RAM and compute managed pool sizes

DETAILED EXPLANATION:

    Real operating systems don't have hardcoded memory sizes. They query
    the hardware at boot to find out how much RAM is installed. We do
    the same thing here using POSIX system calls:

    1. sysconf(_SC_PHYS_PAGES) returns the total number of physical
       memory pages on this machine.

    2. sysconf(_SC_PAGE_SIZE) returns the size of each page in bytes.

    3. Total Physical RAM = pages × page_size

    For our visualizer, we use a FRACTION of the real RAM as the
    managed pool. This keeps the visualization manageable while still
    being dynamically detected from the real system.

    Formula:
    - Managed pool = Total RAM / 8192 (gives ~1 MB per 8 GB of RAM)
    - OS reserved  = 25% of managed pool
    - Minimum pool = 512 KB, Maximum pool = 8192 KB

EXAMPLE (on your 8 GB M1 Mac):
    Physical pages = 524288
    Page size      = 16384 bytes
    Total RAM      = 524288 × 16384 = 8,589,934,592 bytes = 8 GB
    Managed pool   = 8,589,934,592 / (8192 × 1024) = 1024 KB
    OS reserved    = 1024 × 0.25 = 256 KB
    User memory    = 1024 - 256 = 768 KB
*/

void os_detect_memory_sizes(int *totalMemKB, int *osMemKB) {
    
    // STEP 1: Get total physical pages using sysconf
    // This is the POSIX way to query physical memory
    long physPages = sysconf(_SC_PHYS_PAGES);
    long pageSize  = sysconf(_SC_PAGE_SIZE);
    
    if (physPages <= 0 || pageSize <= 0) {
        // Fallback to sysctl if sysconf fails
        size_t totalRAM = os_get_total_ram();
        if (totalRAM > 0) {
            physPages = (long)(totalRAM / (pageSize > 0 ? pageSize : 4096));
        } else {
            // Ultimate fallback: assume 4 GB
            printf("[os_memory] WARNING: Could not detect system memory, using 4 GB default\n");
            *totalMemKB = 1024;
            *osMemKB = 256;
            return;
        }
    }
    
    // STEP 2: Calculate total physical RAM
    size_t totalRAM_bytes = (size_t)physPages * (size_t)pageSize;
    size_t totalRAM_KB = totalRAM_bytes / 1024;
    
    printf("\n[SYSTEM DETECTION] Using sysconf(_SC_PHYS_PAGES) × sysconf(_SC_PAGE_SIZE)\n");
    printf("[SYSTEM DETECTION] Physical pages: %ld\n", physPages);
    printf("[SYSTEM DETECTION] Page size: %ld bytes\n", pageSize);
    printf("[SYSTEM DETECTION] Total physical RAM: %zu KB (%zu MB)\n", 
           totalRAM_KB, totalRAM_KB / 1024);
    
    // STEP 3: Compute managed pool size
    // Use 1/8192 of physical RAM as the managed pool
    // This gives a reasonable visualization size:
    //   4 GB RAM → 512 KB pool
    //   8 GB RAM → 1024 KB pool  
    //  16 GB RAM → 2048 KB pool
    //  32 GB RAM → 4096 KB pool
    int poolKB = (int)(totalRAM_KB / 8192);
    
    // Clamp to reasonable range [512 KB, 8192 KB]
    if (poolKB < 512)  poolKB = 512;
    if (poolKB > 8192) poolKB = 8192;
    
    // STEP 4: OS reserved = 25% of pool
    int osReservedKB = poolKB / 4;
    
    printf("[SYSTEM DETECTION] Managed pool: %d KB (1/%d of physical RAM)\n", 
           poolKB, (int)(totalRAM_KB / poolKB));
    printf("[SYSTEM DETECTION] OS reserved: %d KB (25%%)\n", osReservedKB);
    printf("[SYSTEM DETECTION] User memory: %d KB\n", poolKB - osReservedKB);
    
    *totalMemKB = poolKB;
    *osMemKB = osReservedKB;
}
