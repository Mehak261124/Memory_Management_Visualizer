/**
 * Process Detail API
 * Fetches extended info about a single process by PID
 * Used by ProcessDetailModal in Live System Mode
 */

const API_BASE = 'http://localhost:8080';

/**
 * Fetch detailed process info by PID
 * @param {number} pid - Process ID to look up
 * @returns {Promise<{found: boolean, pid: number, name?: string, vmRSS_KB?: number, vmSize_KB?: number, execPath?: string, threads?: number, pageFaults?: number, pageins?: number, cowFaults?: number, startTime?: string}>}
 */
export async function getProcessDetail(pid) {
  const res = await fetch(`${API_BASE}/api/process/${pid}`);
  if (!res.ok) throw new Error('Failed to fetch process detail');
  return res.json();
}
