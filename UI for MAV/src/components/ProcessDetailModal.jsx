/**
 * ProcessDetailModal Component
 * Shows extended detail about a real OS process when a building is clicked
 * in Live System Mode. Fetches data from /api/process/<pid> on open.
 *
 * Uses existing modal and sys-info-grid CSS classes for cyberpunk theme.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { getProcessDetail } from '../api/getProcessDetail';

export function ProcessDetailModal({ process, onClose }) {
  const [detail, setDetail] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  // Fetch detail when modal opens
  useEffect(() => {
    if (!process) return;

    let cancelled = false;
    setLoading(true);
    setError(null);
    setDetail(null);

    getProcessDetail(process.pid)
      .then((data) => {
        if (!cancelled) {
          setDetail(data);
          setLoading(false);
        }
      })
      .catch((err) => {
        if (!cancelled) {
          setError(err.message || 'Failed to fetch');
          setLoading(false);
        }
      });

    return () => { cancelled = true; };
  }, [process]);

  // Close on Escape key
  useEffect(() => {
    const handleKey = (e) => {
      if (e.key === 'Escape') onClose();
    };
    document.addEventListener('keydown', handleKey);
    return () => document.removeEventListener('keydown', handleKey);
  }, [onClose]);

  // Close on overlay click
  const handleOverlayClick = useCallback((e) => {
    if (e.target === e.currentTarget) onClose();
  }, [onClose]);

  if (!process) return null;

  // Format helpers
  const fmt = (val) => val != null ? val.toLocaleString() : '—';
  const fmtMB = (kb) => kb != null ? (kb / 1024).toFixed(1) + ' MB' : '—';

  return (
    <div className="modal-overlay" onClick={handleOverlayClick}>
      <div className="modal glass-panel" style={{ maxWidth: '520px' }}>
        {/* Header */}
        <div className="modal-header">
          <h3>
            PROCESS DETAIL{' '}
            <span style={{ color: 'var(--cyan)' }}>PID {process.pid}</span>
          </h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>

        {/* Body */}
        <div className="modal-body">
          {loading && (
            <div style={{ textAlign: 'center', padding: '2rem' }}>
              <div className="live-loading__spinner" />
              <p style={{ marginTop: '1rem', color: 'var(--cyan)', opacity: 0.7 }}>
                Fetching process data...
              </p>
            </div>
          )}

          {error && (
            <div style={{ textAlign: 'center', padding: '2rem', color: 'var(--red, #ff4444)' }}>
              ⚠ {error}
            </div>
          )}

          {!loading && !error && detail && !detail.found && (
            <div style={{ textAlign: 'center', padding: '2rem', color: 'var(--yellow, #ffff00)' }}>
              Process PID {process.pid} not found or access denied.
            </div>
          )}

          {!loading && !error && detail && detail.found && (
            <div className="sys-info-grid">
              <div className="sys-info-item">
                <span className="sys-label">NAME</span>
                <span className="sys-value accent">{detail.name}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">PID</span>
                <span className="sys-value accent">{detail.pid}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">RSS MEMORY</span>
                <span className="sys-value">{fmtMB(detail.vmRSS_KB)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">VIRTUAL SIZE</span>
                <span className="sys-value">{fmtMB(detail.vmSize_KB)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">THREADS</span>
                <span className="sys-value">{fmt(detail.threads)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">PAGE FAULTS</span>
                <span className="sys-value">{fmt(detail.pageFaults)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">PAGE INS</span>
                <span className="sys-value">{fmt(detail.pageins)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">COW FAULTS</span>
                <span className="sys-value">{fmt(detail.cowFaults)}</span>
              </div>
              <div className="sys-info-item">
                <span className="sys-label">STARTED</span>
                <span className="sys-value">{detail.startTime || '—'}</span>
              </div>
              <div className="sys-info-item" style={{ gridColumn: '1 / -1' }}>
                <span className="sys-label">EXECUTABLE</span>
                <span
                  className="sys-value"
                  style={{ fontSize: '0.75rem', wordBreak: 'break-all' }}
                  title={detail.execPath}
                >
                  {detail.execPath || '—'}
                </span>
              </div>
            </div>
          )}
        </div>

        {/* Footer */}
        <div className="modal-footer">
          <button className="modal-btn cancel" onClick={onClose}>Close</button>
        </div>
      </div>
    </div>
  );
}
