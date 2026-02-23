/**
 * useLiveProcesses Hook
 * Polls the backend for real OS process data every 3 seconds
 * Also fetches system memory pressure in parallel
 * Used by Live System Mode
 */

import { useState, useEffect, useRef, useCallback } from 'react';
import { getProcesses } from '../api/getProcesses';
import { getMemoryPressure } from '../api/getMemoryPressure';

/**
 * @param {Object} options
 * @param {boolean} options.enabled - Whether polling is active
 * @returns {{ processes: Array, loading: boolean, error: string|null, lastUpdated: Date|null, permissionWarning: boolean, pressure: Object|null }}
 */
export function useLiveProcesses({ enabled = false } = {}) {
  const [processes, setProcesses] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [lastUpdated, setLastUpdated] = useState(null);
  const [permissionWarning, setPermissionWarning] = useState(false);
  const [pressure, setPressure] = useState(null);
  const intervalRef = useRef(null);
  const enabledRef = useRef(enabled);
  const initialLoadDone = useRef(false);

  // Keep ref in sync so the interval callback sees the latest value
  enabledRef.current = enabled;

  const fetchProcesses = useCallback(async () => {
    // Double-check enabled via ref (interval may fire during state transition)
    if (!enabledRef.current) return;

    // Only show loading spinner on first fetch (no data yet)
    // Prevents distracting loading flash on subsequent 3s polls
    if (!initialLoadDone.current) {
      setLoading(true);
    }
    setError(null);

    try {
      // Fetch processes and memory pressure in parallel
      // If one fails, the other still works
      const [procData, pressureData] = await Promise.allSettled([
        getProcesses(),
        getMemoryPressure()
      ]);

      // Only update if still enabled (could have toggled off during fetch)
      if (enabledRef.current) {
        initialLoadDone.current = true;

        // Handle process data
        if (procData.status === 'fulfilled') {
          setProcesses(procData.value.processes || []);
          setPermissionWarning(procData.value.permissionWarning || false);
        }

        // Handle pressure data (independent — failure doesn't affect processes)
        if (pressureData.status === 'fulfilled') {
          setPressure(pressureData.value);
        }

        setLastUpdated(new Date());
        setLoading(false);
      }
    } catch (err) {
      if (enabledRef.current) {
        setError(err.message || 'Failed to fetch processes');
        setLoading(false);
      }
    }
  }, []);

  useEffect(() => {
    if (enabled) {
      // Fetch immediately on enable
      fetchProcesses();

      // Start polling every 3 seconds
      intervalRef.current = setInterval(fetchProcesses, 3000);
    } else {
      // IMMEDIATELY stop polling when disabled
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
      // Reset state when switching away from live mode
      initialLoadDone.current = false;
      setProcesses([]);
      setError(null);
      setLoading(false);
      setPermissionWarning(false);
      setLastUpdated(null);
      setPressure(null);
    }

    // Cleanup on unmount
    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };
  }, [enabled, fetchProcesses]);

  return { processes, loading, error, lastUpdated, permissionWarning, pressure };
}

