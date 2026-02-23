/**
 * Memory Pressure API
 * Fetches system-wide memory pressure data
 * Used by MemoryPressureBar in Live System Mode
 */

const API_BASE = 'http://localhost:8080';

/**
 * Fetch current system memory pressure
 * @returns {Promise<{totalRAM_KB: number, usedRAM_KB: number, freeRAM_KB: number, usedPercent: number, pressureLevel: string, totalRAM_GB: string, usedRAM_GB: string}>}
 */
export async function getMemoryPressure() {
  const res = await fetch(`${API_BASE}/api/memory/pressure`);
  if (!res.ok) throw new Error('Failed to fetch memory pressure');
  return res.json();
}
