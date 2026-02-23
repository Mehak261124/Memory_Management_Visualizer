/**
 * Live Process API
 * Fetches real OS processes from the C backend
 * Used by Live System Mode
 */

const API_BASE = 'http://localhost:8080';

/**
 * Fetch top real OS processes sorted by RSS descending
 * @returns {Promise<{count: number, processes: Array, permissionWarning: boolean}>}
 */
export async function getProcesses() {
  const res = await fetch(`${API_BASE}/api/processes/top`);
  if (!res.ok) throw new Error('Failed to fetch processes');
  return res.json();
}
