/**
 * StatsDashboard Component
 * Statistics grid display
 */

import React from 'react';

export function StatsDashboard({ stats }) {
  return (
    <div className="stats-dashboard">
      <div className="panel-header">
        <span className="panel-icon">◈</span>
        <h2>Statistics</h2>
      </div>
      <div className="stats-grid">
        <div className="stat-item">
          <span className="stat-value">{stats?.totalAllocations || 0}</span>
          <span className="stat-label">Total Allocations</span>
        </div>
        <div className="stat-item">
          <span className="stat-value">{stats?.totalDeallocations || 0}</span>
          <span className="stat-label">Total Deallocations</span>
        </div>
        <div className="stat-item">
          <span className="stat-value">{stats?.largestHole || 0} KB</span>
          <span className="stat-label">Largest Hole</span>
        </div>
        <div className="stat-item">
          <span className="stat-value">{stats?.numHoles || 0}</span>
          <span className="stat-label">Number of Holes</span>
        </div>
        <div className="stat-item">
          <span className="stat-value">{stats?.fragmentation || 0}%</span>
          <span className="stat-label">Fragmentation</span>
        </div>
        <div className="stat-item">
          <span className="stat-value">{stats?.avgHoleSize || 0} KB</span>
          <span className="stat-label">Avg Hole Size</span>
        </div>
      </div>
    </div>
  );
}
