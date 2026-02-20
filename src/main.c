/*
================================================================================
FILE: main.c
PURPOSE: Main program - Entry point for the Memory Allocation Visualizer
DESCRIPTION: 
    - Supports TWO modes:
      1. Interactive menu (default) - Text-based memory allocation visualizer
      2. HTTP server mode (--server flag) - JSON API for React frontend
    - Usage:
      ./memory_visualizer              → Interactive menu mode
      ./memory_visualizer --server 8080 → Start HTTP API server on port 8080
================================================================================
*/

// Include necessary headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/memory_manager.h"
#include "../include/http_server.h"
#include "../include/os_memory.h"


/*
================================================================================
FUNCTION: printMenu
================================================================================
PURPOSE: Display the main menu to the user

CREATES A NICE-LOOKING MENU:
╔════════════════════════════════════════╗
║  MEMORY ALLOCATION VISUALIZER          ║
╠════════════════════════════════════════╣
║  1. Allocate Memory (First Fit)        ║
║  2. Allocate Memory (Best Fit)         ║
... etc ...
*/

void printMenu() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  MEMORY ALLOCATION VISUALIZER          ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  1. Allocate Memory (First Fit)        ║\n");
    printf("║  2. Allocate Memory (Best Fit)         ║\n");
    printf("║  3. Allocate Memory (Worst Fit)        ║\n");
    printf("║  4. Deallocate Process                 ║\n");
    printf("║  5. Display Memory State               ║\n");
    printf("║  6. Show Fragmentation Analysis        ║\n");
    printf("║  7. Compare All Algorithms             ║\n");
    printf("║  8. Compact Memory                     ║\n");
    printf("║  9. Reset Memory                       ║\n");
    printf("║  0. Exit                               ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("Enter choice: ");
}


/*
================================================================================
FUNCTION: printWelcome
================================================================================
PURPOSE: Display welcome banner when program starts
*/

void printWelcome() {
    printf("\n\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║                                               ║\n");
    printf("║    MEMORY ALLOCATION VISUALIZER               ║\n");
    printf("║    Dynamic Partitioning Simulator             ║\n");
    printf("║                                               ║\n");
    printf("║    Demonstrates:                              ║\n");
    printf("║    • First Fit Algorithm                      ║\n");
    printf("║    • Best Fit Algorithm                       ║\n");
    printf("║    • Worst Fit Algorithm                      ║\n");
    printf("║    • Memory Compaction                        ║\n");
    printf("║    • Buddy System                             ║\n");
    printf("║    • External Fragmentation                   ║\n");
    printf("║                                               ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    printf("\n");
}


/*
================================================================================
FUNCTION: drawMemoryVisualization
================================================================================
PURPOSE: Create a visual ASCII representation of memory

EXAMPLE OUTPUT:
Memory Visualization:
[OS ][P1 ][==][P2      ][====]
0   256 356 456      656  1023

Legend: [Process] [==Hole==]
*/

void drawMemoryVisualization(MemoryManager *mm) {
    
    printf("\n");
    printf("┌─────────────────────────────────────────────────────────┐\n");
    printf("│              MEMORY VISUALIZATION                       │\n");
    printf("└─────────────────────────────────────────────────────────┘\n");
    
    // Print memory blocks as ASCII bar
    printf("Memory: ");
    
    // Print OS
    printf("[OS]");
    
    // Print user memory blocks
    MemoryBlock *current = mm->head;
    while (current != NULL) {
        if (current->isHole) {
            // Print hole
            // Size determines how many = signs
            int numSigns = (current->size / 50) + 1;  // 1 sign per 50KB
            if (numSigns > 10) numSigns = 10;  // Max 10 signs
            
            printf("[");
            for (int i = 0; i < numSigns; i++) {
                printf("=");
            }
            printf("]");
        } else {
            // Print process
            printf("[P%d]", current->processID);
        }
        current = current->next;
    }
    printf("\n");
    
    // Print legend
    printf("\nLegend: [Pn]=Process  [==]=Hole(Free Space)\n");
}


/*
================================================================================
FUNCTION: compareAlgorithms
================================================================================
PURPOSE: Run all three algorithms with same test data and compare results

WHAT IT DOES:
1. Creates identical test scenarios
2. Runs First Fit, Best Fit, Worst Fit
3. Shows fragmentation for each
4. Displays comparison table
*/

void compareAlgorithms(MemoryManager *mm) {
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║        ALGORITHM COMPARISON TEST              ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    printf("\nThis will test all algorithms with identical input.\n");
    printf("Press Enter to continue...");
    getchar();
    getchar();  // Clear input buffer
    
    // Test scenario: 5 processes of different sizes
    int testSizes[] = {100, 200, 150, 50, 300};
    int numTests = 5;
    
    printf("\nTest Scenario:\n");
    printf("Allocating 5 processes: 100KB, 200KB, 150KB, 50KB, 300KB\n");
    printf("\n");
    
    // ============ TEST 1: FIRST FIT ============
    printf("═══════════════════════════════════════════════\n");
    printf("        TEST 1: FIRST FIT ALGORITHM\n");
    printf("═══════════════════════════════════════════════\n");
    
    MemoryManager mm1;
    int t1_total, t1_os;
    os_detect_memory_sizes(&t1_total, &t1_os);
    initializeMemory(&mm1, t1_total, t1_os);
    
    for (int i = 0; i < numTests; i++) {
        int result = allocateMemory(&mm1, i + 1, testSizes[i], FIRST_FIT);
        if (result != -1) {
            printf("✓ Process P%d (%d KB) allocated at address %d\n", 
                   i + 1, testSizes[i], result);
        } else {
            printf("✗ Process P%d (%d KB) allocation FAILED\n", 
                   i + 1, testSizes[i]);
        }
    }
    
    displayMemory(&mm1);
    drawMemoryVisualization(&mm1);
    float frag1 = calculateFragmentation(&mm1);
    printf("\nFirst Fit Fragmentation: %.2f%%\n", frag1);
    
    printf("\nPress Enter to continue to Best Fit test...");
    getchar();
    
    
    // ============ TEST 2: BEST FIT ============
    printf("\n\n═══════════════════════════════════════════════\n");
    printf("        TEST 2: BEST FIT ALGORITHM\n");
    printf("═══════════════════════════════════════════════\n");
    
    MemoryManager mm2;
    int t2_total, t2_os;
    os_detect_memory_sizes(&t2_total, &t2_os);
    initializeMemory(&mm2, t2_total, t2_os);
    
    for (int i = 0; i < numTests; i++) {
        int result = allocateMemory(&mm2, i + 1, testSizes[i], BEST_FIT);
        if (result != -1) {
            printf("✓ Process P%d (%d KB) allocated at address %d\n", 
                   i + 1, testSizes[i], result);
        } else {
            printf("✗ Process P%d (%d KB) allocation FAILED\n", 
                   i + 1, testSizes[i]);
        }
    }
    
    displayMemory(&mm2);
    drawMemoryVisualization(&mm2);
    float frag2 = calculateFragmentation(&mm2);
    printf("\nBest Fit Fragmentation: %.2f%%\n", frag2);
    
    printf("\nPress Enter to continue to Worst Fit test...");
    getchar();
    
    
    // ============ TEST 3: WORST FIT ============
    printf("\n\n═══════════════════════════════════════════════\n");
    printf("        TEST 3: WORST FIT ALGORITHM\n");
    printf("═══════════════════════════════════════════════\n");
    
    MemoryManager mm3;
    int t3_total, t3_os;
    os_detect_memory_sizes(&t3_total, &t3_os);
    initializeMemory(&mm3, t3_total, t3_os);
    
    for (int i = 0; i < numTests; i++) {
        int result = allocateMemory(&mm3, i + 1, testSizes[i], WORST_FIT);
        if (result != -1) {
            printf("✓ Process P%d (%d KB) allocated at address %d\n", 
                   i + 1, testSizes[i], result);
        } else {
            printf("✗ Process P%d (%d KB) allocation FAILED\n", 
                   i + 1, testSizes[i]);
        }
    }
    
    displayMemory(&mm3);
    drawMemoryVisualization(&mm3);
    float frag3 = calculateFragmentation(&mm3);
    printf("\nWorst Fit Fragmentation: %.2f%%\n", frag3);
    
    
    // ============ COMPARISON SUMMARY ============
    printf("\n\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║          COMPARISON SUMMARY                   ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    printf("\n");
    printf("┌──────────────┬────────────────┬────────────┐\n");
    printf("│  Algorithm   │  Fragmentation │   Speed    │\n");
    printf("├──────────────┼────────────────┼────────────┤\n");
    printf("│  First Fit   │     %.2f%%     │   Fastest  │\n", frag1);
    printf("│  Best Fit    │     %.2f%%     │   Slowest  │\n", frag2);
    printf("│  Worst Fit   │     %.2f%%     │   Slow     │\n", frag3);
    printf("└──────────────┴────────────────┴────────────┘\n");
    
    // Analysis
    printf("\nAnalysis:\n");
    if (frag1 < frag2 && frag1 < frag3) {
        printf("• First Fit had the LEAST fragmentation for this workload\n");
    } else if (frag2 < frag1 && frag2 < frag3) {
        printf("• Best Fit had the LEAST fragmentation for this workload\n");
    } else if (frag3 < frag1 && frag3 < frag2) {
        printf("• Worst Fit had the LEAST fragmentation for this workload\n");
    }
    
    printf("• First Fit is fastest (stops at first match)\n");
    printf("• Best Fit and Worst Fit are slower (check all holes)\n");
    printf("• Results vary depending on process arrival patterns\n");
    
    // Cleanup
    freeMemoryManager(&mm1);
    freeMemoryManager(&mm2);
    freeMemoryManager(&mm3);
    
    printf("\nPress Enter to return to main menu...");
    getchar();
}


/*
================================================================================
FUNCTION: main
================================================================================
PURPOSE: Main program entry point

SUPPORTS TWO MODES:
1. Interactive menu (default):
   ./memory_visualizer
   
2. HTTP server mode:
   ./memory_visualizer --server 8080

HOW THE --server FLAG WORKS:
We check command-line arguments (argc/argv):
- argc = count of arguments
- argv = array of argument strings
Example: "./memory_visualizer --server 8080"
  argv[0] = "./memory_visualizer"
  argv[1] = "--server"
  argv[2] = "8080"
  argc = 3
*/

int main(int argc, char *argv[]) {
    
    // Variables
    MemoryManager mm;           // Memory manager structure
    int choice;                 // User menu choice
    int processID;              // Process ID for allocation/deallocation
    int size;                   // Process size
    int result;                 // Result of operations
    int nextProcessID = 1;      // Next available process ID
    char algoName[20] = "NONE"; // Current algorithm name
    
    // Dynamically detect system memory using OS system calls
    // Uses sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE) to detect real RAM
    int detectedTotal, detectedOS;
    os_detect_memory_sizes(&detectedTotal, &detectedOS);
    initializeMemory(&mm, detectedTotal, detectedOS);
    
    // ========== CHECK FOR --server FLAG ==========
    // If user passed "--server", start the HTTP API server
    // instead of the interactive menu
    if (argc >= 2 && strcmp(argv[1], "--server") == 0) {
        
        // Get port number (default: 8080)
        int port = 8080;
        if (argc >= 3) {
            port = atoi(argv[2]);
            if (port <= 0 || port > 65535) {
                printf("Error: Invalid port number. Using default 8080.\n");
                port = 8080;
            }
        }
        
        // Start the HTTP server (this blocks until Ctrl+C)
        printf("Starting HTTP API server...\n");
        startServer(&mm, port);
        
        // Cleanup (only reached if server stops)
        freeMemoryManager(&mm);
        return 0;
    }
    
    // ========== INTERACTIVE MENU MODE ==========
    // Display welcome banner
    printWelcome();
    
    printf("Memory initialized successfully!\n");
    printf("Press Enter to start...");
    getchar();  // Wait for user to press Enter
    
    // Main program loop
    while (1) {
        
        // Display menu
        printMenu();
        
        // Get user choice
        if (scanf("%d", &choice) != 1) {
            // Invalid input (not a number)
            printf("Invalid input! Please enter a number.\n");
            // Clear input buffer
            while (getchar() != '\n');
            continue;
        }
        
        // Process user choice
        switch (choice) {
            
            // ========== CASE 1: FIRST FIT ==========
            case 1:
                printf("\n--- FIRST FIT ALLOCATION ---\n");
                printf("Enter process size (KB): ");
                scanf("%d", &size);
                
                result = allocateMemory(&mm, nextProcessID, size, FIRST_FIT);
                
                if (result != -1) {
                    printf("\n✓ SUCCESS!\n");
                    printf("Process P%d (%d KB) allocated at address %d\n", 
                           nextProcessID, size, result);
                    printf("Algorithm: First Fit\n");
                    strcpy(algoName, "FIRST FIT");
                    nextProcessID++;
                    
                    // Show updated memory
                    displayMemory(&mm);
                    drawMemoryVisualization(&mm);
                } else {
                    printf("\n✗ ALLOCATION FAILED!\n");
                    printf("Not enough contiguous memory available.\n");
                    printf("Requested: %d KB, Free: %d KB\n", size, mm.freeMemory);
                }
                break;
            
            
            // ========== CASE 2: BEST FIT ==========
            case 2:
                printf("\n--- BEST FIT ALLOCATION ---\n");
                printf("Enter process size (KB): ");
                scanf("%d", &size);
                
                result = allocateMemory(&mm, nextProcessID, size, BEST_FIT);
                
                if (result != -1) {
                    printf("\n✓ SUCCESS!\n");
                    printf("Process P%d (%d KB) allocated at address %d\n", 
                           nextProcessID, size, result);
                    printf("Algorithm: Best Fit\n");
                    strcpy(algoName, "BEST FIT");
                    nextProcessID++;
                    
                    displayMemory(&mm);
                    drawMemoryVisualization(&mm);
                } else {
                    printf("\n✗ ALLOCATION FAILED!\n");
                    printf("Not enough contiguous memory available.\n");
                    printf("Requested: %d KB, Free: %d KB\n", size, mm.freeMemory);
                }
                break;
            
            
            // ========== CASE 3: WORST FIT ==========
            case 3:
                printf("\n--- WORST FIT ALLOCATION ---\n");
                printf("Enter process size (KB): ");
                scanf("%d", &size);
                
                result = allocateMemory(&mm, nextProcessID, size, WORST_FIT);
                
                if (result != -1) {
                    printf("\n✓ SUCCESS!\n");
                    printf("Process P%d (%d KB) allocated at address %d\n", 
                           nextProcessID, size, result);
                    printf("Algorithm: Worst Fit\n");
                    strcpy(algoName, "WORST FIT");
                    nextProcessID++;
                    
                    displayMemory(&mm);
                    drawMemoryVisualization(&mm);
                } else {
                    printf("\n✗ ALLOCATION FAILED!\n");
                    printf("Not enough contiguous memory available.\n");
                    printf("Requested: %d KB, Free: %d KB\n", size, mm.freeMemory);
                }
                break;
            
            
            // ========== CASE 4: DEALLOCATE ==========
            case 4:
                printf("\n--- DEALLOCATE PROCESS ---\n");
                
                // Show current processes
                printf("Current processes in memory:\n");
                {
                    MemoryBlock *current = mm.head;
                    int hasProcesses = 0;
                    while (current != NULL) {
                        if (!current->isHole) {
                            printf("  P%d (%d KB at address %d)\n", 
                                   current->processID, current->size, 
                                   current->startAddress);
                            hasProcesses = 1;
                        }
                        current = current->next;
                    }
                    
                    if (!hasProcesses) {
                        printf("  No processes in memory.\n");
                        break;
                    }
                }
                
                printf("\nEnter process ID to deallocate: ");
                scanf("%d", &processID);
                
                result = deallocateMemory(&mm, processID);
                
                if (result) {
                    printf("\n✓ SUCCESS!\n");
                    printf("Process P%d has been deallocated\n", processID);
                    printf("Memory freed and holes merged (if adjacent)\n");
                    
                    displayMemory(&mm);
                    drawMemoryVisualization(&mm);
                } else {
                    printf("\n✗ DEALLOCATION FAILED!\n");
                    printf("Process P%d not found in memory.\n", processID);
                }
                break;
            
            
            // ========== CASE 5: DISPLAY MEMORY ==========
            case 5:
                printf("\n--- CURRENT MEMORY STATE ---\n");
                printf("Current Algorithm: %s\n", algoName);
                displayMemory(&mm);
                drawMemoryVisualization(&mm);
                break;
            
            
            // ========== CASE 6: FRAGMENTATION ==========
            case 6:
                {
                    float frag = calculateFragmentation(&mm);
                    
                    printf("\n");
                    printf("╔═══════════════════════════════════════╗\n");
                    printf("║    FRAGMENTATION ANALYSIS             ║\n");
                    printf("╚═══════════════════════════════════════╝\n");
                    printf("\n");
                    printf("External Fragmentation: %.2f%%\n", frag);
                    printf("Total Holes: %d\n", mm.numHoles);
                    printf("Total Free Memory: %d KB\n", mm.freeMemory);
                    printf("Used Memory: %d KB\n", mm.userMemory - mm.freeMemory);
                    printf("Memory Utilization: %.2f%%\n", 
                           ((float)(mm.userMemory - mm.freeMemory) / mm.userMemory) * 100);
                    
                    printf("\nWhat is fragmentation?\n");
                    printf("Fragmentation occurs when free memory is scattered\n");
                    printf("in small holes that cannot be used effectively.\n");
                    
                    if (frag < 10) {
                        printf("\n✓ Low fragmentation - Memory is well utilized\n");
                    } else if (frag < 30) {
                        printf("\n⚠ Moderate fragmentation - Some memory waste\n");
                    } else {
                        printf("\n✗ High fragmentation - Consider compaction\n");
                    }
                }
                break;
            
            
            // ========== CASE 7: COMPARE ALGORITHMS ==========
            case 7:
                compareAlgorithms(&mm);
                break;
            
            
            // ========== CASE 8: COMPACT MEMORY ==========
            case 8:
                printf("\n--- COMPACT MEMORY ---\n");
                {
                    char resultBuf[1024];
                    int compacted = compact(&mm, resultBuf, sizeof(resultBuf));
                    
                    if (compacted) {
                        printf("\n✓ Compaction complete!\n");
                        displayMemory(&mm);
                        drawMemoryVisualization(&mm);
                    } else {
                        printf("\nNothing to compact (no processes in memory).\n");
                    }
                }
                break;
            
            
            // ========== CASE 9: RESET MEMORY ==========
            case 9:
                printf("\n--- RESET MEMORY ---\n");
                printf("Are you sure? This will remove all processes. (y/n): ");
                {
                    char confirm;
                    scanf(" %c", &confirm);
                    
                    if (confirm == 'y' || confirm == 'Y') {
                        resetMemory(&mm);
                        nextProcessID = 1;
                        strcpy(algoName, "NONE");
                        printf("\n✓ Memory reset successfully!\n");
                        displayMemory(&mm);
                    } else {
                        printf("\nReset cancelled.\n");
                    }
                }
                break;
            
            
            // ========== CASE 0: EXIT ==========
            case 0:
                printf("\n");
                printf("╔═══════════════════════════════════════╗\n");
                printf("║  Thank you for using                  ║\n");
                printf("║  MEMORY ALLOCATION VISUALIZER         ║\n");
                printf("╚═══════════════════════════════════════╝\n");
                printf("\n");
                
                // Cleanup
                freeMemoryManager(&mm);
                
                printf("Goodbye!\n\n");
                return 0;
            
            
            // ========== DEFAULT: INVALID CHOICE ==========
            default:
                printf("\n✗ Invalid choice! Please enter 0-9.\n");
        }
        
        // Pause before showing menu again
        printf("\nPress Enter to continue...");
        getchar();  // Clear newline from previous input
        getchar();  // Wait for Enter
    }
    
    return 0;
}


/*
================================================================================
END OF FILE: main.c
================================================================================

WHAT WE IMPLEMENTED:
1. printMenu() - Display interactive menu (updated with compaction option)
2. printWelcome() - Welcome banner (updated with compaction/buddy info)
3. drawMemoryVisualization() - ASCII art memory representation
4. compareAlgorithms() - Test and compare all three algorithms
5. main() - Main program with TWO modes:
   - Interactive menu mode (default)
   - HTTP server mode (--server flag)

FEATURES:
✓ Interactive menu (10 options: 0-9)
✓ All three allocation algorithms
✓ Deallocation with hole merging
✓ Memory visualization (text-based)
✓ Fragmentation analysis
✓ Algorithm comparison
✓ Memory compaction
✓ Memory reset
✓ Input validation
✓ Error handling
✓ HTTP server mode for React frontend

HOW TO USE:
  Interactive:  ./memory_visualizer
  HTTP Server:  ./memory_visualizer --server 8080

COMPLETE PROJECT - READY TO COMPILE AND RUN!
================================================================================
*/
