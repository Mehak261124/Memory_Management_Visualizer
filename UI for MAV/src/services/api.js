/**
 * API Service
 * Communicates with the C backend HTTP server
 * All functions return Promises (async)
 */

const API_BASE = 'http://localhost:8080';

/**
 * Generic fetch wrapper with error handling
 */
async function apiFetch(endpoint, options = {}) {
  try {
    const response = await fetch(`${API_BASE}${endpoint}`, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...options.headers,
      },
    });
    
    const data = await response.json();
    return data;
  } catch (error) {
    console.error(`[API] Error calling ${endpoint}:`, error);
    throw error;
  }
}

// ========== GET Endpoints ==========

/** Health check */
export async function getStatus() {
  return apiFetch('/api/status');
}

/** Get all memory blocks */
export async function getBlocks() {
  return apiFetch('/api/blocks');
}

/** Get memory statistics */
export async function getStats() {
  return apiFetch('/api/stats');
}

// ========== POST Endpoints ==========

/** Allocate memory
 * @param {number} size - Size in KB
 * @param {string} algorithm - 'first_fit', 'best_fit', or 'worst_fit'
 */
export async function allocateMemory(size, algorithm = 'first_fit') {
  return apiFetch('/api/allocate', {
    method: 'POST',
    body: JSON.stringify({ size, algorithm }),
  });
}

/** Deallocate a process
 * @param {number} processId - Numeric process ID (without 'P' prefix)
 */
export async function deallocateMemory(processId) {
  return apiFetch('/api/deallocate', {
    method: 'POST',
    body: JSON.stringify({ processId }),
  });
}

/** Run memory compaction */
export async function compactMemory() {
  return apiFetch('/api/compact', {
    method: 'POST',
  });
}

/** Auto-compact if fragmentation exceeds threshold */
export async function autoCompactMemory(threshold = 30) {
  return apiFetch('/api/autocompact', {
    method: 'POST',
    body: JSON.stringify({ threshold }),
  });
}

/** Convert to buddy system */
export async function convertToBuddySystem() {
  return apiFetch('/api/buddy/convert', {
    method: 'POST',
  });
}

/** Revert from buddy system */
export async function revertFromBuddySystem() {
  return apiFetch('/api/buddy/revert', {
    method: 'POST',
  });
}

/** Reset memory to initial state */
export async function resetMemory() {
  return apiFetch('/api/reset', {
    method: 'POST',
  });
}
