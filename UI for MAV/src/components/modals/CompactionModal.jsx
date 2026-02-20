/**
 * CompactionModal Component
 * Performs compaction on current processes and shows before/after results
 */

import React, { useState } from 'react';
import { Modal } from './Modal';

function CompactionResult({ result, beforeBlocks }) {
  if (!result) return null;

  // Auto compaction skipped — fragmentation below threshold
  if (!result.success) {
    return (
      <div className="compaction-result">
        <div className="result-header">
          <span className="result-icon" style={{ color: 'var(--yellow)' }}>ℹ️</span>
          <h4>Compaction Skipped</h4>
        </div>
        <div className="compaction-warning" style={{ marginTop: '16px' }}>
          <span className="warning-icon">⚡</span>
          <div className="warning-content">
            <strong>{result.message}</strong>
            <p style={{ marginTop: '8px', color: 'var(--text-dim)' }}>
              No changes were made. Lower the threshold or use Sliding Compaction to force it.
            </p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="compaction-result">
      <div className="result-header">
        <span className="result-icon">✓</span>
        <h4>Compaction Complete</h4>
      </div>
      
      <div className="result-stats">
        <div className="result-stat">
          <span className="stat-label">Processes Moved</span>
          <span className="stat-value">{result.processesMovedCount}</span>
        </div>
        
        <div className="result-stat">
          <span className="stat-label">Data Moved</span>
          <span className="stat-value">{result.totalBytesMoved} KB</span>
        </div>
        
        <div className="result-stat">
          <span className="stat-label">Fragmentation Before</span>
          <span className="stat-value danger">{result.fragmentationBefore}%</span>
        </div>
        
        <div className="result-stat">
          <span className="stat-label">Fragmentation After</span>
          <span className="stat-value success">{result.fragmentationAfter}%</span>
        </div>
        
        <div className="result-stat">
          <span className="stat-label">Holes Before</span>
          <span className="stat-value">{result.holesBefore}</span>
        </div>
        
        <div className="result-stat">
          <span className="stat-label">Holes After</span>
          <span className="stat-value success">{result.holesAfter}</span>
        </div>
      </div>

      {/* Before / After visualization using actual blocks */}
      {beforeBlocks && beforeBlocks.length > 0 && (
        <div style={{ marginTop: '16px' }}>
          <h4 style={{ color: 'var(--cyan)', marginBottom: '8px' }}>Before Compaction</h4>
          <div className="option-diagram">
            <div className="diagram-before" style={{ display: 'flex', gap: '2px', flexWrap: 'wrap' }}>
              {beforeBlocks.map((b, i) => (
                <span key={i} className={`block ${b.isAllocated ? 'alloc' : 'free'}`}>
                  {b.isAllocated ? `${b.processId} (${b.size})` : `Free (${b.size})`}
                </span>
              ))}
            </div>
          </div>
        </div>
      )}
      
      <div className="result-summary">
        <p className="success-message">{result.message}</p>
        <p style={{ color: 'var(--text-dim)', marginTop: '8px', fontSize: '0.85em' }}>
          ✓ The main visualization has been updated. Close this modal to see the results.
        </p>
      </div>
    </div>
  );
}

export function CompactionModal({ isOpen, onClose, manager, onCompact, onAutoCompact }) {
  const [result, setResult] = useState(null);
  const [beforeBlocks, setBeforeBlocks] = useState(null);
  const [compactionType, setCompactionType] = useState('sliding');
  const [autoThreshold, setAutoThreshold] = useState(30);

  const stats = manager?.getStats();
  const currentFrag = parseFloat(stats?.fragmentation || 0);
  const currentHoles = stats?.numHoles || 0;
  const allocatedBlocks = manager?.getAllocatedBlocks() || [];
  const freeBlocks = manager?.getFreeBlocks() || [];
  const hasFragmentation = freeBlocks.length > 0 && allocatedBlocks.length > 0;

  const handleCompact = async () => {
    if (!manager) return;

    // Save snapshot of blocks before compaction
    setBeforeBlocks(manager.blocks.map(b => ({
      processId: b.processId,
      size: b.size,
      isAllocated: b.isAllocated,
      startAddress: b.startAddress
    })));

    try {
      let compactResult;
      if (compactionType === 'auto') {
        compactResult = onAutoCompact ? await onAutoCompact(autoThreshold) : manager.autoCompact(autoThreshold);
      } else {
        compactResult = onCompact ? await onCompact() : manager.compact();
      }
      setResult(compactResult);
    } catch (err) {
      console.error('[CompactionModal] Error:', err);
    }
  };

  const handleClose = () => {
    setResult(null);
    setBeforeBlocks(null);
    onClose();
  };

  return (
    <Modal
      isOpen={isOpen}
      onClose={handleClose}
      title="Memory Compaction"
      wide={true}
      footer={
        <>
          <button className="modal-btn cancel" onClick={handleClose}>Close</button>
          {!result && hasFragmentation && (
            <button className="modal-btn confirm" onClick={handleCompact}>
              Run Compaction
            </button>
          )}
        </>
      }
    >
      <div className="compaction-content">
        {!result ? (
          <>
            <div className="compaction-info">
              <h4>What is Memory Compaction?</h4>
              <p>
                Memory compaction eliminates <strong>external fragmentation</strong> by 
                moving all allocated blocks together, creating one large contiguous free space.
              </p>
              
              <div className="current-status">
                <div className="status-item">
                  <span className="status-label">Current Fragmentation:</span>
                  <span className={`status-value ${currentFrag > 30 ? 'danger' : currentFrag > 15 ? 'warning' : 'success'}`}>
                    {currentFrag}%
                  </span>
                </div>
                
                <div className="status-item">
                  <span className="status-label">Current Holes:</span>
                  <span className="status-value">{currentHoles}</span>
                </div>
                
                <div className="status-item">
                  <span className="status-label">Recommendation:</span>
                  <span className="status-value">
                    {allocatedBlocks.length === 0 ? 'ℹ️ No processes to compact' :
                     currentFrag > 30 ? '⚠️ Compaction Recommended' : 
                     currentFrag > 15 ? '⚡ Monitor' : 
                     currentHoles > 1 ? '⚡ Can be compacted' :
                     '✓ No Action Needed'}
                  </span>
                </div>
              </div>
            </div>

            {/* Show actual current memory layout */}
            {allocatedBlocks.length > 0 && (
              <div style={{ marginBottom: '16px' }}>
                <h4 style={{ color: 'var(--cyan)', marginBottom: '8px' }}>Current Memory Layout</h4>
                <div className="option-diagram">
                  <div className="diagram-before" style={{ display: 'flex', gap: '2px', flexWrap: 'wrap' }}>
                    {manager.blocks.map((b, i) => (
                      <span key={i} className={`block ${b.isAllocated ? 'alloc' : 'free'}`}>
                        {b.isAllocated ? `${b.processId} (${b.size})` : `Free (${b.size})`}
                      </span>
                    ))}
                  </div>
                </div>
                {hasFragmentation && (
                  <div className="diagram-arrow" style={{ textAlign: 'center', margin: '8px 0', color: 'var(--cyan)' }}>↓ After Compaction ↓</div>
                )}
                {hasFragmentation && (
                  <div className="diagram-after" style={{ display: 'flex', gap: '2px', flexWrap: 'wrap' }}>
                    {allocatedBlocks.map((b, i) => (
                      <span key={i} className="block alloc">
                        {b.processId} ({b.size})
                      </span>
                    ))}
                    <span className="block free">
                      Free ({freeBlocks.reduce((s, b) => s + b.size, 0)})
                    </span>
                  </div>
                )}
              </div>
            )}

            <div className="compaction-options">
              <h4>Compaction Type</h4>
              
              <div className="option-card">
                <label className="option-label">
                  <input
                    type="radio"
                    name="compaction"
                    value="sliding"
                    checked={compactionType === 'sliding'}
                    onChange={(e) => setCompactionType(e.target.value)}
                  />
                  <div className="option-content">
                    <strong>Sliding Compaction</strong>
                    <p>Moves all processes to the beginning of memory, leaving one large hole at the end.</p>
                  </div>
                </label>
              </div>

              <div className="option-card">
                <label className="option-label">
                  <input
                    type="radio"
                    name="compaction"
                    value="auto"
                    checked={compactionType === 'auto'}
                    onChange={(e) => setCompactionType(e.target.value)}
                  />
                  <div className="option-content">
                    <strong>Auto Compaction</strong>
                    <p>Only compact if fragmentation exceeds threshold.</p>
                    <div className="threshold-control">
                      <label>
                        Threshold: <strong>{autoThreshold}%</strong>
                        <input
                          type="range"
                          min="10"
                          max="50"
                          step="5"
                          value={autoThreshold}
                          onChange={(e) => setAutoThreshold(parseInt(e.target.value))}
                          disabled={compactionType !== 'auto'}
                        />
                      </label>
                    </div>
                  </div>
                </label>
              </div>
            </div>

            {!hasFragmentation && allocatedBlocks.length > 0 && (
              <div className="compaction-warning">
                <span className="warning-icon">ℹ️</span>
                <div className="warning-content">
                  <strong>Memory is already contiguous.</strong> There are no holes to compact.
                  Deallocate a process to create fragmentation first.
                </div>
              </div>
            )}

            {allocatedBlocks.length === 0 && (
              <div className="compaction-warning">
                <span className="warning-icon">ℹ️</span>
                <div className="warning-content">
                  <strong>No processes allocated.</strong> Allocate some processes and deallocate 
                  a middle one to create holes, then use compaction.
                </div>
              </div>
            )}

            {hasFragmentation && (
              <div className="compaction-warning">
                <span className="warning-icon">⚠️</span>
                <div className="warning-content">
                  <strong>Note:</strong> Compaction requires pausing all processes and updating 
                  their addresses. In real systems, this is a costly operation.
                </div>
              </div>
            )}
          </>
        ) : (
          <CompactionResult result={result} beforeBlocks={beforeBlocks} />
        )}
      </div>
    </Modal>
  );
}