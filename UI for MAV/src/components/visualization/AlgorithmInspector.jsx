/**
 * AlgorithmInspector Component
 * Shows algorithm selection details and block highlights
 */

import React from 'react';

export function AlgorithmInspector({ selectedBlock }) {
  return (
    <div className="algorithm-inspector">
      <div className="inspector-header">
        <span className="inspector-icon">◉</span>
        <span className="inspector-title">Algorithm Inspector</span>
      </div>
      <div className="inspector-content">
        {selectedBlock ? (
          <div className="inspector-highlight">
            <div className="inspector-item">
              <strong>Block ID:</strong> {selectedBlock.id}
            </div>
            <div className="inspector-item">
              <strong>Status:</strong> {selectedBlock.isAllocated ? 'Allocated' : 'Free'}
            </div>
            <div className="inspector-item">
              <strong>Size:</strong> {selectedBlock.size} KB
            </div>
            <div className="inspector-item">
              <strong>Address:</strong> 0x{selectedBlock.startAddress.toString(16).padStart(4, '0')} - 0x{selectedBlock.endAddress.toString(16).padStart(4, '0')}
            </div>
            {selectedBlock.processId && (
              <div className="inspector-item">
                <strong>Process:</strong> {selectedBlock.processId}
              </div>
            )}
          </div>
        ) : (
          <p className="inspector-hint">
            Select an allocation method to see how it chooses holes
          </p>
        )}
      </div>
    </div>
  );
}
