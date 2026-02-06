/**
 * AnalysisModal Component
 * Fragmentation analysis details
 */

import React from 'react';
import { Modal } from './Modal';
import { CONFIG } from '../../config/constants';

export function AnalysisModal({ isOpen, onClose, stats }) {
  const frag = parseFloat(stats?.fragmentation || 0);
  
  let fragClass = '';
  let fragStatus = 'Good';
  if (frag > CONFIG.CRITICAL_FRAG_THRESHOLD) {
    fragClass = 'danger';
    fragStatus = 'Critical - Consider Compaction';
  } else if (frag > CONFIG.HIGH_FRAG_THRESHOLD) {
    fragClass = 'warning';
    fragStatus = 'High - Monitor Carefully';
  }

  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title="Fragmentation Analysis"
      footer={
        <button className="modal-btn confirm" onClick={onClose}>Close</button>
      }
    >
      <div className="analysis-content">
        <div className="analysis-stat">
          <span className="analysis-label">External Fragmentation</span>
          <span className={`analysis-value ${fragClass}`}>{stats?.fragmentation || 0}%</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Fragmentation Status</span>
          <span className={`analysis-value ${fragClass}`}>{fragStatus}</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Total Memory</span>
          <span className="analysis-value">{stats?.totalMemory || 0} KB</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">OS Memory (Reserved)</span>
          <span className="analysis-value">{stats?.osMemory || 0} KB</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">User Memory</span>
          <span className="analysis-value">{stats?.userMemory || 0} KB</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Used Memory</span>
          <span className="analysis-value">
            {stats?.usedMemory || 0} KB ({((stats?.usedMemory / stats?.userMemory) * 100 || 0).toFixed(1)}%)
          </span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Free Memory</span>
          <span className="analysis-value">{stats?.freeMemory || 0} KB</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Number of Holes</span>
          <span className="analysis-value">{stats?.numHoles || 0}</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Largest Hole</span>
          <span className="analysis-value">{stats?.largestHole || 0} KB</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Average Hole Size</span>
          <span className="analysis-value">{stats?.avgHoleSize || 0} KB</span>
        </div>
        <div className="analysis-stat">
          <span className="analysis-label">Active Processes</span>
          <span className="analysis-value">{stats?.numProcesses || 0}</span>
        </div>
      </div>
    </Modal>
  );
}
