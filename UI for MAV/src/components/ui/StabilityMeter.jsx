/**
 * StabilityMeter Component
 * System stability meter visualization
 */

import React from 'react';

export function StabilityMeter({ stability }) {
  let meterClass = '';
  if (stability < 50) {
    meterClass = 'danger';
  } else if (stability < 70) {
    meterClass = 'warning';
  }

  return (
    <div className="stability-meter">
      <span className="status-label">System Stability</span>
      <div className="meter-bar">
        <div 
          className={`meter-fill ${meterClass}`} 
          style={{ width: `${stability}%` }}
        />
      </div>
    </div>
  );
}
