/**
 * MemoryPressureBar Component
 * Shows system-wide memory pressure in Live System Mode
 * Displays a horizontal bar with used/free/total RAM and pressure level
 */

import React from 'react';

export function MemoryPressureBar({ pressure }) {
  if (!pressure) {
    return (
      <div className="pressure-bar-container">
        <div className="pressure-bar-header">SYSTEM MEMORY PRESSURE</div>
        <div style={{ padding: '0.5rem 1rem', opacity: 0.5, fontSize: '0.85rem' }}>
          Loading...
        </div>
      </div>
    );
  }

  const {
    usedPercent = 0,
    pressureLevel = 'LOW',
    totalRAM_GB = '0',
    usedRAM_GB = '0',
    freeRAM_KB = 0,
    totalRAM_KB = 1,
  } = pressure;

  const freeGB = (freeRAM_KB / 1048576).toFixed(1);
  const levelClass = `pressure-level-${pressureLevel}`;

  return (
    <div className="pressure-bar-container">
      <div className="pressure-bar-header">SYSTEM MEMORY PRESSURE</div>

      <div className="pressure-bar-track">
        <div
          className={`pressure-bar-fill ${levelClass}`}
          style={{ width: `${Math.min(usedPercent, 100)}%` }}
        />
      </div>

      <div className={`pressure-bar-label ${levelClass}`}>
        {usedPercent.toFixed(1)}% — {pressureLevel}
      </div>

      <div className="pressure-bar-stats">
        <span>Used: {usedRAM_GB} GB</span>
        <span>Free: {freeGB} GB</span>
        <span>Total: {totalRAM_GB} GB</span>
      </div>
    </div>
  );
}
