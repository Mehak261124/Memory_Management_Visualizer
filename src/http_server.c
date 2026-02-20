/*
================================================================================
FILE: http_server.c
PURPOSE: Minimal HTTP server to expose memory management as JSON API
DESCRIPTION: 
    - Single-threaded HTTP server using POSIX sockets
    - Parses HTTP requests (GET and POST)
    - Routes requests to appropriate handlers
    - Returns JSON responses with CORS headers
    - No external dependencies - only standard C + POSIX
    - Designed to communicate with the React frontend
================================================================================
*/

// System headers for POSIX sockets and I/O
#include <stdio.h>           // printf, snprintf
#include <stdlib.h>          // atoi, malloc, free
#include <string.h>          // strlen, strcmp, strstr, memset
#include <unistd.h>          // close, read, write
#include <sys/socket.h>      // socket, bind, listen, accept
#include <netinet/in.h>      // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>       // inet_ntoa

// Our project headers
#include "../include/http_server.h"
#include "../include/memory_manager.h"
#include "../include/memory_structures.h"
#include "../include/os_memory.h"

// Buffer sizes for HTTP request/response handling
#define MAX_REQUEST_SIZE  8192    // Max size of incoming HTTP request (8 KB)
#define MAX_RESPONSE_SIZE 65536   // Max size of HTTP response body (64 KB)
#define MAX_HEADER_SIZE   1024    // Max size of HTTP response headers (1 KB)


/*
================================================================================
HELPER FUNCTION: sendResponse
================================================================================
PURPOSE: Send an HTTP response back to the client

WHAT IT DOES:
1. Builds HTTP response headers (status code, content type, CORS)
2. Appends the response body (JSON)
3. Sends everything through the socket
4. Closes the client connection

PARAMETERS:
- clientFd: The client's socket file descriptor
- statusCode: HTTP status code (200 for OK, 400 for bad request, etc.)
- statusText: Human-readable status ("OK", "Bad Request", etc.)
- contentType: MIME type of the response ("application/json")
- body: The response body (JSON string)

WHAT ARE CORS HEADERS?
CORS = Cross-Origin Resource Sharing
The React dev server runs on localhost:5173
The C server runs on localhost:8080
Without CORS headers, the browser blocks cross-origin requests.
We add "Access-Control-Allow-Origin: *" to allow any origin.
*/

void sendResponse(int clientFd, int statusCode, const char *statusText,
                  const char *contentType, const char *body) {
    
    // Build the full HTTP response
    char response[MAX_HEADER_SIZE + MAX_RESPONSE_SIZE];
    int bodyLen = (body != NULL) ? (int)strlen(body) : 0;
    
    // Format: HTTP/1.1 <code> <status>\r\n<headers>\r\n\r\n<body>
    int totalLen = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        statusCode, statusText,
        contentType,
        bodyLen,
        body ? body : ""
    );
    
    // Send the response through the socket
    write(clientFd, response, totalLen);
}


/*
================================================================================
HELPER FUNCTION: parseRequestBody
================================================================================
PURPOSE: Extract the body content from an HTTP request

HTTP requests look like:
POST /api/allocate HTTP/1.1
Host: localhost:8080
Content-Type: application/json
Content-Length: 35

{"size":100,"algorithm":"first_fit"}
                                    ← Body starts after blank line (\r\n\r\n)

We find the blank line and return everything after it.
*/

const char* parseRequestBody(const char *request) {
    
    // Find the blank line separator (\r\n\r\n)
    const char *body = strstr(request, "\r\n\r\n");
    
    if (body != NULL) {
        return body + 4;  // Skip past \r\n\r\n (4 characters)
    }
    
    return NULL;  // No body found
}


/*
================================================================================
HELPER FUNCTION: parseJSONInt
================================================================================
PURPOSE: Extract an integer value from a JSON string by key name

This is a simple JSON parser for our specific use case.
No external JSON library needed!

EXAMPLE:
Input JSON: {"size":100,"algorithm":"first_fit"}
parseJSONInt(json, "size") → 100

HOW IT WORKS:
1. Search for "keyName": in the string
2. Skip past the colon
3. Parse the integer value using atoi()
*/

int parseJSONInt(const char *json, const char *key) {
    
    // Build the search pattern: "key":
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    
    // Find the pattern in the JSON string
    const char *pos = strstr(json, pattern);
    if (pos == NULL) return -1;  // Key not found
    
    // Move past the pattern to the value
    pos += strlen(pattern);
    
    // Skip whitespace
    while (*pos == ' ' || *pos == '\t') pos++;
    
    // Parse integer
    return atoi(pos);
}


/*
================================================================================
HELPER FUNCTION: parseJSONString
================================================================================
PURPOSE: Extract a string value from a JSON string by key name

EXAMPLE:
Input JSON: {"size":100,"algorithm":"first_fit"}
parseJSONString(json, "algorithm", buffer, 64) → writes "first_fit" into buffer

HOW IT WORKS:
1. Search for "keyName": in the string
2. Find the opening quote of the value
3. Copy characters until closing quote
*/

void parseJSONString(const char *json, const char *key, char *value, int valueSize) {
    
    // Initialize value to empty string
    value[0] = '\0';
    
    // Build search pattern: "key":
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    
    // Find the pattern
    const char *pos = strstr(json, pattern);
    if (pos == NULL) return;
    
    // Move past the pattern
    pos += strlen(pattern);
    
    // Skip whitespace
    while (*pos == ' ' || *pos == '\t') pos++;
    
    // Check for opening quote
    if (*pos != '"') return;
    pos++;  // Skip opening quote
    
    // Copy characters until closing quote
    int i = 0;
    while (*pos != '"' && *pos != '\0' && i < valueSize - 1) {
        value[i++] = *pos++;
    }
    value[i] = '\0';
}


/*
================================================================================
FUNCTION: handleRequest
================================================================================
PURPOSE: Route an HTTP request to the appropriate handler

WHAT IT DOES:
1. Parse the HTTP method (GET, POST, OPTIONS)
2. Parse the request URL path
3. Route to the appropriate handler based on method + path
4. Send back the JSON response

SUPPORTED ROUTES:
GET  /api/status        → Health check
GET  /api/blocks        → Get all memory blocks
GET  /api/stats         → Get memory statistics
POST /api/allocate      → Allocate memory
POST /api/deallocate    → Deallocate a process
POST /api/compact       → Run compaction
POST /api/autocompact   → Auto-compact
POST /api/buddy/convert → Convert to buddy system
POST /api/buddy/revert  → Revert from buddy system
POST /api/reset         → Reset memory
OPTIONS *               → CORS preflight response
*/

void handleRequest(int clientFd, const char *request, MemoryManager *mm) {
    
    // STEP 1: Parse the HTTP method and path
    // HTTP request first line: "GET /api/status HTTP/1.1"
    char method[16] = {0};
    char path[256] = {0};
    
    sscanf(request, "%15s %255s", method, path);
    
    // Log the request for debugging
    printf("[REQUEST] %s %s\n", method, path);
    
    
    // ========== HANDLE OPTIONS (CORS PREFLIGHT) ==========
    // Browsers send OPTIONS requests before POST requests
    // to check if the server allows cross-origin requests
    if (strcmp(method, "OPTIONS") == 0) {
        sendResponse(clientFd, 200, "OK", "text/plain", "");
        return;
    }
    
    
    // ========== GET /api/status ==========
    // Health check - confirms server is running
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/status") == 0) {
        
        sendResponse(clientFd, 200, "OK", "application/json",
            "{\"status\":\"running\",\"message\":\"Memory Management API Server\"}");
        return;
    }
    
    
    // ========== GET /api/blocks ==========
    // Returns all memory blocks as a JSON array
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/blocks") == 0) {
        
        char blocksJSON[MAX_RESPONSE_SIZE];
        blocksToJSON(mm, blocksJSON, sizeof(blocksJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", blocksJSON);
        return;
    }
    
    
    // ========== GET /api/stats ==========
    // Returns memory statistics as JSON
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/stats") == 0) {
        
        char statsJSON[MAX_RESPONSE_SIZE];
        getStatsJSON(mm, statsJSON, sizeof(statsJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", statsJSON);
        return;
    }
    
    // ========== GET /api/sysinfo ==========
    // Returns real OS system information (page size, total RAM, etc.)
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/sysinfo") == 0) {
        
        char sysInfoJSON[MAX_RESPONSE_SIZE];
        os_get_system_info_json(sysInfoJSON, sizeof(sysInfoJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", sysInfoJSON);
        return;
    }
    
    
    // ========== POST /api/allocate ==========
    // Allocate memory for a new process
    // Body: {"size": 100, "algorithm": "first_fit"}
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/allocate") == 0) {
        
        const char *body = parseRequestBody(request);
        if (body == NULL) {
            sendResponse(clientFd, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Missing request body\"}");
            return;
        }
        
        // Parse requested size
        int size = parseJSONInt(body, "size");
        if (size <= 0) {
            sendResponse(clientFd, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Invalid size\"}");
            return;
        }
        
        // Parse algorithm
        char algorithm[32] = "first_fit";
        parseJSONString(body, "algorithm", algorithm, sizeof(algorithm));
        
        // Determine allocation algorithm
        AllocationAlgorithm algo = FIRST_FIT;
        if (strcmp(algorithm, "best_fit") == 0) {
            algo = BEST_FIT;
        } else if (strcmp(algorithm, "worst_fit") == 0) {
            algo = WORST_FIT;
        }
        
        // Check if buddy system is active
        if (mm->useBuddySystem) {
            // Use buddy allocation instead
            char resultJSON[MAX_RESPONSE_SIZE];
            int result = buddyAllocate(mm, size, resultJSON, sizeof(resultJSON));
            sendResponse(clientFd, (result >= 0) ? 200 : 400, 
                        (result >= 0) ? "OK" : "Bad Request",
                        "application/json", resultJSON);
            return;
        }
        
        // Auto-assign process ID
        int processID = ++(mm->processCounter);
        
        // Allocate memory
        int startAddr = allocateMemory(mm, processID, size, algo);
        
        // Build response
        char resultJSON[512];
        if (startAddr >= 0) {
            snprintf(resultJSON, sizeof(resultJSON),
                "{\"success\":true,"
                "\"processId\":\"P%d\","
                "\"size\":%d,"
                "\"startAddress\":%d,"
                "\"algorithm\":\"%s\"}",
                processID, size, startAddr, algorithm
            );
            sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
        } else {
            snprintf(resultJSON, sizeof(resultJSON),
                "{\"success\":false,"
                "\"message\":\"Allocation failed. Requested: %d KB, Free: %d KB\"}",
                size, mm->freeMemory
            );
            sendResponse(clientFd, 400, "Bad Request", "application/json", resultJSON);
        }
        return;
    }
    
    
    // ========== POST /api/deallocate ==========
    // Deallocate a process
    // Body: {"processId": 3}
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/deallocate") == 0) {
        
        const char *body = parseRequestBody(request);
        if (body == NULL) {
            sendResponse(clientFd, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Missing request body\"}");
            return;
        }
        
        int processID = parseJSONInt(body, "processId");
        if (processID <= 0) {
            sendResponse(clientFd, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Invalid processId\"}");
            return;
        }
        
        // Check if buddy system is active
        if (mm->useBuddySystem) {
            char resultJSON[MAX_RESPONSE_SIZE];
            buddyDeallocate(mm, processID, resultJSON, sizeof(resultJSON));
            sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
            return;
        }
        
        // Standard deallocation
        int result = deallocateMemory(mm, processID);
        
        char resultJSON[256];
        if (result) {
            snprintf(resultJSON, sizeof(resultJSON),
                "{\"success\":true,\"processId\":\"P%d\"}", processID);
            sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
        } else {
            snprintf(resultJSON, sizeof(resultJSON),
                "{\"success\":false,\"message\":\"Process P%d not found\"}", processID);
            sendResponse(clientFd, 404, "Not Found", "application/json", resultJSON);
        }
        return;
    }
    
    
    // ========== POST /api/compact ==========
    // Run memory compaction
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/compact") == 0) {
        
        char resultJSON[MAX_RESPONSE_SIZE];
        compact(mm, resultJSON, sizeof(resultJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
        return;
    }
    
    
    // ========== POST /api/autocompact ==========
    // Auto-compact if fragmentation exceeds threshold
    // Body: {"threshold": 30}
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/autocompact") == 0) {
        
        int threshold = 30;  // Default threshold
        
        const char *body = parseRequestBody(request);
        if (body != NULL) {
            int parsed = parseJSONInt(body, "threshold");
            if (parsed > 0) threshold = parsed;
        }
        
        char resultJSON[MAX_RESPONSE_SIZE];
        autoCompact(mm, threshold, resultJSON, sizeof(resultJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
        return;
    }
    
    
    // ========== POST /api/buddy/convert ==========
    // Convert to buddy system
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/buddy/convert") == 0) {
        
        char resultJSON[MAX_RESPONSE_SIZE];
        convertToBuddySystem(mm, resultJSON, sizeof(resultJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
        return;
    }
    
    
    // ========== POST /api/buddy/revert ==========
    // Revert from buddy system
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/buddy/revert") == 0) {
        
        char resultJSON[MAX_RESPONSE_SIZE];
        revertFromBuddySystem(mm, resultJSON, sizeof(resultJSON));
        
        sendResponse(clientFd, 200, "OK", "application/json", resultJSON);
        return;
    }
    
    
    // ========== POST /api/reset ==========
    // Reset memory to initial state
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/reset") == 0) {
        
        resetMemory(mm);
        
        sendResponse(clientFd, 200, "OK", "application/json",
            "{\"success\":true,\"message\":\"Memory reset to initial state\"}");
        return;
    }
    
    
    // ========== 404 NOT FOUND ==========
    // No matching route found
    char notFound[256];
    snprintf(notFound, sizeof(notFound),
        "{\"error\":\"Not Found\",\"message\":\"Unknown endpoint: %s %s\"}", 
        method, path);
    
    sendResponse(clientFd, 404, "Not Found", "application/json", notFound);
}


/*
================================================================================
FUNCTION: startServer
================================================================================
PURPOSE: Start the HTTP API server

HOW SOCKETS WORK (simplified):
1. socket() → Create a communication endpoint (like getting a phone)
2. bind() → Assign a port number (like getting a phone number)
3. listen() → Start waiting for connections (like turning on the phone)
4. accept() → Accept a connection (like answering a call)
5. read() → Read the client's request (like listening to the caller)
6. handleRequest() → Process the request
7. close() → Hang up the call
8. Go back to step 4 (wait for next call)

THIS IS THE MAIN SERVER LOOP!
*/

int startServer(MemoryManager *mm, int port) {
    
    // STEP 1: Create a TCP socket
    // AF_INET = IPv4 internet
    // SOCK_STREAM = TCP (reliable, ordered)
    // 0 = auto-select protocol
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (serverFd < 0) {
        perror("Error: Could not create socket");
        return -1;
    }
    
    // STEP 2: Set socket options
    // SO_REUSEADDR allows reusing the port immediately after the server stops
    // Without this, you might get "Address already in use" error
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // STEP 3: Set up the server address structure
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));  // Clear to zeros
    serverAddr.sin_family = AF_INET;             // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;     // Accept from any IP
    serverAddr.sin_port = htons(port);           // Port number (network byte order)
    // htons = "Host TO Network Short" (converts endianness)
    
    // STEP 4: Bind socket to the port
    // This "claims" the port number for our server
    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error: Could not bind to port");
        close(serverFd);
        return -1;
    }
    
    // STEP 5: Start listening for connections
    // 10 = maximum queue length for pending connections
    if (listen(serverFd, 10) < 0) {
        perror("Error: Could not listen on socket");
        close(serverFd);
        return -1;
    }
    
    // STEP 6: Print server info
    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║  MEMORY MANAGEMENT API SERVER                    ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  Server running on http://localhost:%-5d        ║\n", port);
    printf("║                                                  ║\n");
    printf("║  API Endpoints:                                  ║\n");
    printf("║  GET  /api/status         Health check           ║\n");
    printf("║  GET  /api/blocks         Get memory blocks      ║\n");
    printf("║  GET  /api/stats          Get statistics         ║\n");
    printf("║  POST /api/allocate       Allocate memory        ║\n");
    printf("║  POST /api/deallocate     Free memory            ║\n");
    printf("║  POST /api/compact        Run compaction         ║\n");
    printf("║  POST /api/autocompact    Auto-compact           ║\n");
    printf("║  POST /api/buddy/convert  Enable buddy system    ║\n");
    printf("║  POST /api/buddy/revert   Disable buddy system   ║\n");
    printf("║  POST /api/reset          Reset memory           ║\n");
    printf("║                                                  ║\n");
    printf("║  Press Ctrl+C to stop the server                 ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    printf("\nWaiting for connections...\n\n");
    
    // STEP 7: Main server loop — handle requests forever
    while (1) {
        
        // Accept a new client connection
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientFd < 0) {
            perror("Error: Could not accept connection");
            continue;  // Try again
        }
        
        // Read the client's HTTP request
        char request[MAX_REQUEST_SIZE];
        memset(request, 0, sizeof(request));
        
        ssize_t bytesRead = read(clientFd, request, sizeof(request) - 1);
        
        if (bytesRead > 0) {
            // Handle the request
            handleRequest(clientFd, request, mm);
        }
        
        // Close the client connection
        close(clientFd);
    }
    
    // Close the server socket (we never actually reach here)
    close(serverFd);
    return 0;
}


/*
================================================================================
END OF FILE: http_server.c
================================================================================

WHAT WE IMPLEMENTED:
1. sendResponse() - Send HTTP response with CORS headers
2. parseRequestBody() - Extract body from HTTP request
3. parseJSONInt() - Parse integer from JSON string
4. parseJSONString() - Parse string from JSON string
5. handleRequest() - Route HTTP requests to handlers
6. startServer() - Main server loop with POSIX sockets

KEY NETWORKING CONCEPTS:
- Socket: Communication endpoint (like a phone)
- Bind: Assign a port number (like getting a phone number)
- Listen: Start accepting connections
- Accept: Accept one connection
- Read/Write: Send and receive data
- CORS: Allow cross-origin requests from React frontend

HOW TO USE:
1. Compile: gcc -o server src/*.c -I include
2. Run: ./server --server 8080
3. Test: curl http://localhost:8080/api/status
4. Frontend will call these endpoints via fetch()
================================================================================
*/
