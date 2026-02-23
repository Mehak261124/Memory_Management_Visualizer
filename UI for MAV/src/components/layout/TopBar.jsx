/**
 * TopBar Component
 * Header with logo, status indicators, and mode toggle
 */

import React from 'react';
import { StatusItem } from '../ui/StatusItem';
import { StabilityMeter } from '../ui/StabilityMeter';

export function TopBar({ stats, isLiveMode = false, onToggleMode, liveProcessCount = 0 }) {
  const stability = 100 - parseFloat(stats?.fragmentation || 0);

  return (
    <header className="top-bar">
      <div className="logo-section">
        <div className="logo-icon">
          <div className="logo-cube" />
        </div>
        <h1 className="app-title">
          <span className="neon-text cyan">Neon</span>
          <span className="neon-text magenta">Heap</span>
          <span className="subtitle">Memory Allocation Visualizer</span>
        </h1>
      </div>

      <div className="status-indicators">
        {!isLiveMode && (
          <>
            <StatusItem label="User Memory" value={`${stats?.userMemory || 768} KB`} />
            <StatusItem label="Used" value={`${stats?.usedMemory || 0} KB`} variant="used" />
            <StatusItem label="Free" value={`${stats?.freeMemory || 768} KB`} variant="free" />
            <StatusItem label="Fragmentation" value={`${stats?.fragmentation || 0}%`} variant="frag" />
            <StabilityMeter stability={stability} />
          </>
        )}

        {isLiveMode && (
          <>
            <StatusItem
              label="Live Processes"
              value={liveProcessCount || '—'}
              variant="free"
            />
            <StatusItem
              label="Mode"
              value="KERNEL"
              variant="used"
            />
          </>
        )}

        {/* Mode Toggle Button */}
        <button
          className={`mode-toggle-btn ${isLiveMode ? 'mode-toggle-btn--live' : 'mode-toggle-btn--edu'}`}
          onClick={onToggleMode}
          title={isLiveMode ? 'Switch to Educational Mode' : 'Switch to Live System Mode'}
        >
          <span className="mode-toggle-btn__indicator" />
          <span className="mode-toggle-btn__label">
            {isLiveMode ? '⚡ Live System' : '📚 Educational'}
          </span>
        </button>
      </div>

      {/* Mode Banner */}
      <div className={`mode-banner ${isLiveMode ? 'mode-banner--live' : 'mode-banner--edu'}`}>
        {isLiveMode
          ? '🔴 Live Mode — Showing real kernel-managed processes via proc_pidinfo()'
          : '🟢 Simulation Mode — Teaching allocation algorithms (First Fit / Best Fit / Worst Fit)'}
      </div>
    </header>
  );
}
