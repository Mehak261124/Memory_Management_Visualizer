/**
 * LiveCityViewport Component
 * City skyline visualization for real OS processes (Live System Mode)
 * Shows top 10 processes as buildings with height proportional to RSS
 */

import React, { useState } from 'react';
import { LiveProcessBlock } from './LiveProcessBlock';
import { ProcessDetailModal } from './ProcessDetailModal';
import { MemoryPressureBar } from './MemoryPressureBar';

export function LiveCityViewport({
  processes = [],
  loading = false,
  error = null,
  permissionWarning = false,
  lastUpdated = null,
  zoomLevel = 1,
  pressure = null,
}) {
  // State for process detail modal
  const [selectedProcess, setSelectedProcess] = useState(null);

  // Find max RSS for scaling
  const maxRSS = processes.length > 0
    ? Math.max(...processes.map(p => p.vmRSS_KB))
    : 1;

  // Format last updated time
  const lastUpdatedStr = lastUpdated
    ? lastUpdated.toLocaleTimeString()
    : '—';

  // Scale legend entries (based on maxRSS)
  const maxMB = (maxRSS / 1024).toFixed(0);
  const halfMB = (maxRSS / 2048).toFixed(0);

  return (
    <div className="city-viewport live-city-viewport">
      {/* Permission Warning Banner — OUTSIDE city-container so zoom doesn't affect it */}
      {permissionWarning && (
        <div className="permission-warning">
          ⚠ Limited visibility — run server with <code>sudo</code> for full process list
        </div>
      )}

      {/* Memory Pressure Bar — OUTSIDE city-container so zoom doesn't affect it */}
      <MemoryPressureBar pressure={pressure} />

      <div
        className="city-container"
        style={{ transform: `scale(${zoomLevel})` }}
      >
        {/* Loading State */}
        {loading && processes.length === 0 && (
          <div className="live-loading">
            <div className="live-loading__spinner" />
            <span>Loading real processes...</span>
          </div>
        )}

        {/* Error State */}
        {error && (
          <div className="live-error">
            <span className="live-error__icon">⚠</span>
            <span>Error fetching processes: {error}</span>
            <span className="live-error__hint">Is the C server running on port 8080?</span>
          </div>
        )}

        {/* No Processes State */}
        {!loading && !error && processes.length === 0 && (
          <div className="live-empty">
            <span>No processes visible.</span>
            <span className="live-empty__hint">Try running the server with <code>sudo</code></span>
          </div>
        )}

        {/* Street Layer */}
        <div className="street-layer">
          <div className="neon-road" />
        </div>

        {/* Buildings Layer (Live Processes) */}
        {processes.length > 0 && (
          <div className="buildings-layer live-buildings-layer">
            {processes.map((proc, index) => (
              <LiveProcessBlock
                key={proc.pid}
                process={proc}
                maxRSS={maxRSS}
                index={index}
                onClick={setSelectedProcess}
              />
            ))}
          </div>
        )}

        {/* Address Bar */}
        <div className="address-bar" />
      </div>

      {/* Scale Legend */}
      {processes.length > 0 && (
        <div className="scale-legend">
          <div className="scale-legend__title">RAM Scale</div>
          <div className="scale-legend__entry">
            <div className="scale-legend__bar scale-legend__bar--full" />
            <span>{maxMB} MB</span>
          </div>
          <div className="scale-legend__entry">
            <div className="scale-legend__bar scale-legend__bar--half" />
            <span>{halfMB} MB</span>
          </div>
          <div className="scale-legend__entry">
            <div className="scale-legend__bar scale-legend__bar--min" />
            <span>{'< 1'} MB</span>
          </div>
        </div>
      )}

      {/* Last Updated Timestamp */}
      {lastUpdated && (
        <div className="last-updated">
          Last updated: {lastUpdatedStr}
          {loading && <span className="last-updated__dot"> ●</span>}
        </div>
      )}

      {/* Process Detail Modal */}
      {selectedProcess && (
        <ProcessDetailModal
          process={selectedProcess}
          onClose={() => setSelectedProcess(null)}
        />
      )}
    </div>
  );
}

