/**
 * TopBar Component
 * Header with logo and status indicators
 */

import React from 'react';
import { StatusItem } from '../ui/StatusItem';
import { StabilityMeter } from '../ui/StabilityMeter';

export function TopBar({ stats }) {
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
        <StatusItem label="User Memory" value={`${stats?.userMemory || 768} KB`} />
        <StatusItem label="Used" value={`${stats?.usedMemory || 0} KB`} variant="used" />
        <StatusItem label="Free" value={`${stats?.freeMemory || 768} KB`} variant="free" />
        <StatusItem label="Fragmentation" value={`${stats?.fragmentation || 0}%`} variant="frag" />
        <StabilityMeter stability={stability} />
      </div>
    </header>
  );
}
