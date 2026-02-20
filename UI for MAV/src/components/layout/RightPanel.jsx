/**
 * RightPanel Component
 * Statistics dashboard and OS System Info panel
 */

import React, { useState, useEffect } from 'react';
import { StatsDashboard } from '../stats/StatsDashboard';

export function RightPanel({ stats }) {
  const [sysInfo, setSysInfo] = useState(null);

  useEffect(() => {
    fetch('http://localhost:8080/api/sysinfo')
      .then(r => r.json())
      .then(data => setSysInfo(data))
      .catch(() => setSysInfo(null));
  }, []);

  const formatBytes = (bytes) => {
    if (!bytes) return '—';
    const gb = bytes / (1024 * 1024 * 1024);
    return gb >= 1 ? `${gb.toFixed(0)} GB` : `${(bytes / (1024 * 1024)).toFixed(0)} MB`;
  };

  return (
    <aside className="right-panel">
      <StatsDashboard stats={stats} />

      {/* OS System Info Panel */}
      <div className="info-panel system-info-panel">
        <div className="panel-header">
          <span className="panel-icon">🖥</span>
          <h2>OS System Info</h2>
        </div>
        {sysInfo ? (
          <div className="info-content">
            <div className="sys-info-grid">
              <div className="sys-info-item">
                <span className="sys-label">Architecture</span>
                <span className="sys-value accent">{sysInfo.arch || '—'} {sysInfo.arch === 'arm64' ? '(Apple Silicon)' : ''}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">Physical RAM</span>
                <span className="sys-value accent">{formatBytes(sysInfo.totalRAM_bytes)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">Physical Pages</span>
                <span className="sys-value">{sysInfo.physicalPages?.toLocaleString() || '—'}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">Page Size</span>
                <span className="sys-value">{sysInfo.pageSize ? `${sysInfo.pageSize / 1024} KB` : '—'}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">mmap() Base</span>
                <span className="sys-value mono">{stats?.backingRegionBase || '—'}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">Backing Type</span>
                <span className="sys-value">{sysInfo.backingType || '—'}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">Pool Size</span>
                <span className="sys-value accent">{sysInfo.detectedPoolSize_KB ? `${sysInfo.detectedPoolSize_KB} KB` : '—'}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">OS Reserved</span>
                <span className="sys-value">{sysInfo.detectedOSReserved_KB ? `${sysInfo.detectedOSReserved_KB} KB` : '—'}</span>
              </div>
            </div>

            {sysInfo.systemCalls && (
              <div className="sys-calls">
                <span className="sys-label" style={{ marginBottom: '6px', display: 'block' }}>System Calls Used</span>
                <div className="sys-calls-list">
                  {sysInfo.systemCalls.map((call, i) => (
                    <span key={i} className="sys-call-tag">{call}</span>
                  ))}
                </div>
              </div>
            )}
          </div>
        ) : (
          <div className="info-content">
            <p style={{ color: 'var(--text-dim)' }}>Connecting to backend...</p>
          </div>
        )}
      </div>
    </aside>
  );
}

