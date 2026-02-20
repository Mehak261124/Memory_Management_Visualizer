/*
================================================================================
FILE: http_server.h
PURPOSE: HTTP server to expose memory management as a JSON API
DESCRIPTION: 
    - Declares a minimal HTTP server using POSIX sockets
    - The server handles API requests from the React frontend
    - All responses are JSON format with CORS headers
    - No external dependencies (only standard C + POSIX)
================================================================================
*/

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "memory_manager.h"

/*
--------------------------------------------------------------------------------
FUNCTION: startServer
--------------------------------------------------------------------------------
PURPOSE: Start the HTTP API server on the given port

WHAT IT DOES:
1. Creates a TCP socket using POSIX socket API
2. Binds to the specified port (e.g., 8080)
3. Listens for incoming HTTP connections
4. For each request, parses the URL and method
5. Routes the request to the appropriate handler
6. Sends back a JSON response with CORS headers
7. Loops forever (until program is terminated)

PARAMETERS:
- mm: Pointer to MemoryManager (shared state for all requests)
- port: Port number to listen on (e.g., 8080)

RETURNS: 
- 0 on normal exit
- -1 on error (socket creation failed, etc.)

EXAMPLE USAGE:
MemoryManager mm;
initializeMemory(&mm, 1024, 256);
startServer(&mm, 8080);  // Blocks here, handles requests forever
*/
int startServer(MemoryManager *mm, int port);


#endif

/*
================================================================================
END OF FILE: http_server.h
================================================================================

API ENDPOINTS (handled by the server):
GET  /api/status      → Server health check
GET  /api/blocks      → All memory blocks as JSON array
GET  /api/stats       → Memory statistics as JSON
POST /api/allocate    → Allocate memory (body: size, algorithm)
POST /api/deallocate  → Deallocate a process (body: processId)
POST /api/compact     → Run compaction
POST /api/autocompact → Auto-compact with threshold
POST /api/buddy/convert → Convert to buddy system
POST /api/buddy/revert  → Revert from buddy system
POST /api/reset       → Reset memory

All responses include CORS headers for cross-origin requests
from the React dev server (localhost:5173).
================================================================================
*/
