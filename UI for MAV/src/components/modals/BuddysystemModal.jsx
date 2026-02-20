/**
 * BuddySystemModal Component
 * Convert current allocations to buddy system (power-of-2) allocation
 */

import React, { useState } from 'react';
import { Modal } from './Modal';

function nextPowerOf2(n) {
  if (n <= 0) return 1;
  return Math.pow(2, Math.ceil(Math.log2(n)));
}

function ConversionResult({ result }) {
  if (!result) return null;

  return (
    <div className="compaction-result">
      <div className="result-header">
        <span className="result-icon">✓</span>
        <h4>Buddy System Conversion Complete</h4>
      </div>

      <div className="result-stats">
        <div className="result-stat">
          <span className="stat-label">Processes Converted</span>
          <span className="stat-value">{result.successCount}</span>
        </div>
        {result.failedCount > 0 && (
          <div className="result-stat">
            <span className="stat-label">Failed</span>
            <span className="stat-value danger">{result.failedCount}</span>
          </div>
        )}
        <div className="result-stat">
          <span className="stat-label">Internal Fragmentation</span>
          <span className="stat-value warning">{result.totalWasted} KB wasted</span>
        </div>
        <div className="result-stat">
          <span className="stat-label">Ext. Fragmentation Before</span>
          <span className="stat-value danger">{result.fragmentationBefore}%</span>
        </div>
        <div className="result-stat">
          <span className="stat-label">Ext. Fragmentation After</span>
          <span className="stat-value success">{result.fragmentationAfter}%</span>
        </div>
      </div>

      {result.conversions && result.conversions.length > 0 && (
        <div style={{ marginTop: '16px' }}>
          <h4 style={{ color: 'var(--cyan)', marginBottom: '8px' }}>Process Details</h4>
          <table className="comparison-table">
            <thead>
              <tr>
                <th>Process</th>
                <th>Original Size</th>
                <th>Buddy Size (2^n)</th>
                <th>Wasted</th>
              </tr>
            </thead>
            <tbody>
              {result.conversions.map((c, i) => (
                <tr key={i}>
                  <td>{c.processId}</td>
                  <td>{c.originalSize} KB</td>
                  <td>{c.error ? <span className="danger">{c.error}</span> : `${c.buddySize} KB`}</td>
                  <td>{c.error ? '—' : `${c.wasted} KB`}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      <div className="result-summary">
        <p className="success-message">{result.message}</p>
      </div>
    </div>
  );
}

export function BuddySystemModal({ isOpen, onClose, manager, onConvert, onRevert, refresh }) {
  const [result, setResult] = useState(null);

  const allocatedBlocks = manager?.getAllocatedBlocks() || [];
  const stats = manager?.getStats();
  const isBuddyActive = manager?.useBuddySystem || false;

  const handleConvert = async () => {
    if (!onConvert) return;
    try {
      const conversionResult = await onConvert();
      setResult(conversionResult);
    } catch (err) {
      console.error('[BuddySystemModal] Convert error:', err);
    }
  };

  const handleRevert = async () => {
    if (!onRevert) return;
    try {
      await onRevert();
      setResult(null);
    } catch (err) {
      console.error('[BuddySystemModal] Revert error:', err);
    }
  };

  const handleClose = () => {
    setResult(null);
    onClose();
  };

  return (
    <Modal
      isOpen={isOpen}
      onClose={handleClose}
      title="Buddy System"
      wide={true}
      footer={
        <>
          <button className="modal-btn cancel" onClick={handleClose}>Close</button>
          {!result && !isBuddyActive && allocatedBlocks.length > 0 && (
            <button className="modal-btn confirm" onClick={handleConvert}>
              Apply Buddy System
            </button>
          )}
          {isBuddyActive && !result && (
            <button className="modal-btn danger" onClick={handleRevert}>
              Revert to Standard
            </button>
          )}
        </>
      }
    >
      <div className="buddy-content">
        {result ? (
          <ConversionResult result={result} />
        ) : (
          <>
            <div className="buddy-info">
              <h4>What is the Buddy System?</h4>
              <p>
                The <strong>buddy system</strong> is a memory allocation technique that
                divides memory into blocks of size 2^n (power of 2), splits blocks
                recursively to find optimal size, and merges "buddy" blocks when freed.
              </p>

              <div className="buddy-status">
                <div className="status-indicator">
                  <span className="status-dot" style={{
                    background: isBuddyActive ? 'var(--green)' : 'var(--red)'
                  }}></span>
                  <span className="status-text">
                    {isBuddyActive ? 'Buddy System Active' : 'Buddy System Inactive'}
                  </span>
                </div>
              </div>
            </div>

            {allocatedBlocks.length > 0 && !isBuddyActive && (
              <div className="buddy-stats">
                <h4>Current Processes → Buddy Conversion Preview</h4>
                <p style={{ color: 'var(--text-dim)', marginBottom: '12px' }}>
                  Clicking "Apply Buddy System" will re-allocate all current processes using power-of-2 block sizes:
                </p>
                <table className="comparison-table">
                  <thead>
                    <tr>
                      <th>Process</th>
                      <th>Current Size</th>
                      <th>Buddy Size (2^n)</th>
                      <th>Internal Waste</th>
                    </tr>
                  </thead>
                  <tbody>
                    {allocatedBlocks.map((block, i) => {
                      const buddySize = nextPowerOf2(block.size);
                      const wasted = buddySize - block.size;
                      return (
                        <tr key={i}>
                          <td>{block.processId}</td>
                          <td>{block.size} KB</td>
                          <td>{buddySize} KB</td>
                          <td style={{ color: wasted > 0 ? 'var(--yellow)' : 'var(--green)' }}>
                            {wasted > 0 ? `+${wasted} KB` : '0 KB'}
                          </td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
                <p style={{ color: 'var(--yellow)', marginTop: '8px', fontSize: '0.85em' }}>
                  ⚠️ Total internal fragmentation: {
                    allocatedBlocks.reduce((sum, b) => sum + (nextPowerOf2(b.size) - b.size), 0)
                  } KB will be wasted due to power-of-2 rounding.
                </p>
              </div>
            )}

            {allocatedBlocks.length === 0 && !isBuddyActive && (
              <div className="compaction-warning">
                <span className="warning-icon">ℹ️</span>
                <div className="warning-content">
                  <strong>No processes allocated.</strong> Allocate some processes first,
                  then use this to convert them to buddy system allocation.
                </div>
              </div>
            )}

            {isBuddyActive && (
              <div className="buddy-stats">
                <h4>Buddy System Active — Current Stats</h4>
                <div className="stats-grid">
                  <div className="stat-box">
                    <span className="stat-label">Free Memory</span>
                    <span className="stat-value">{stats?.freeMemory || 0} KB</span>
                  </div>
                  <div className="stat-box">
                    <span className="stat-label">Allocated</span>
                    <span className="stat-value">{stats?.usedMemory || 0} KB</span>
                  </div>
                  <div className="stat-box">
                    <span className="stat-label">Fragmentation</span>
                    <span className="stat-value">{stats?.fragmentation || 0}%</span>
                  </div>
                  <div className="stat-box">
                    <span className="stat-label">Blocks</span>
                    <span className="stat-value">{manager?.blocks.length || 0}</span>
                  </div>
                </div>
                <p style={{ color: 'var(--text-dim)', marginTop: '12px' }}>
                  Future allocations will use buddy (power-of-2) allocation. You can revert to standard allocation at any time.
                </p>
              </div>
            )}

            <div className="buddy-comparison">
              <h4>Buddy System vs Standard Allocation</h4>
              <table className="comparison-table">
                <thead>
                  <tr>
                    <th>Aspect</th>
                    <th>Standard</th>
                    <th>Buddy System</th>
                  </tr>
                </thead>
                <tbody>
                  <tr>
                    <td>External Fragmentation</td>
                    <td>High</td>
                    <td className="success">Low</td>
                  </tr>
                  <tr>
                    <td>Internal Fragmentation</td>
                    <td className="success">Low</td>
                    <td>Can be high</td>
                  </tr>
                  <tr>
                    <td>Allocation Speed</td>
                    <td>Slower</td>
                    <td className="success">Fast</td>
                  </tr>
                  <tr>
                    <td>Coalescing</td>
                    <td>Linear search</td>
                    <td className="success">O(log n)</td>
                  </tr>
                  <tr>
                    <td>Memory Utilization</td>
                    <td className="success">Better</td>
                    <td>Worse (power of 2)</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </>
        )}
      </div>
    </Modal>
  );
}