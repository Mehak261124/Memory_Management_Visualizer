/**
 * FragmentationDashboard Component
 * Fragmentation meter and warning display
 */

import React from 'react';
import { CONFIG } from '../../config/constants';

export function FragmentationDashboard({ stats }) {
  const frag = parseFloat(stats?.fragmentation || 0);
  const showWarning = frag > CONFIG.HIGH_FRAG_THRESHOLD;

  return (
    <div className="frag-dashboard">
      <div className="frag-header">
        <span className="frag-icon">⚠</span>
        <span className="frag-title">Fragmentation Status</span>
      </div>
      <div className="frag-content">
        <div className="frag-meter">
          <div className="frag-bar" style={{ width: `${frag}%` }} />
          <span className="frag-value">{stats?.fragmentation || 0}%</span>
        </div>
        <div className="frag-details">
          <span className="frag-detail">
            External: <span>{stats?.fragmentation || 0}%</span>
          </span>
          <span className="frag-detail">
            Holes: <span>{stats?.numHoles || 0}</span>
          </span>
          <span className="frag-detail">
            Largest: <span>{stats?.largestHole || 0} KB</span>
          </span>
        </div>
      </div>
      {showWarning && (
        <div className="frag-warning">
          <span className="warning-icon">⚡</span>
          <span className="warning-text">High Fragmentation Detected!</span>
        </div>
      )}
    </div>
  );
}
