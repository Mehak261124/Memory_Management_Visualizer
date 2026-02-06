/**
 * RightPanel Component
 * Statistics dashboard and info panel
 */

import React from 'react';
import { StatsDashboard } from '../stats/StatsDashboard';

export function RightPanel({ stats }) {
  return (
    <aside className="right-panel">
      <StatsDashboard stats={stats} />

      <div className="info-panel">
        <div className="panel-header">
          <span className="panel-icon">ℹ</span>
          <h2>About</h2>
        </div>
        <div className="info-content">
          <p><strong>NeonHeap</strong> visualizes dynamic memory allocation algorithms.</p>
          <p><strong>Memory Layout:</strong></p>
          <ul>
            <li>OS Memory: 256 KB (Reserved)</li>
            <li>User Memory: 768 KB (Available)</li>
            <li>Total: 1024 KB</li>
          </ul>
        </div>
      </div>
    </aside>
  );
}
