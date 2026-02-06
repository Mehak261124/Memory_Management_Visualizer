/**
 * ComparisonModal Component
 * Algorithm comparison with mini city visualizations
 */

import React, { useState } from 'react';
import { Modal } from './Modal';
import { MemoryManager } from '../../models/MemoryManager';
import { CONFIG } from '../../config/constants';

function MiniCity({ blocks, osMemory, totalMemory }) {
  return (
    <div className="mini-city">
      <div 
        className="mini-block os"
        style={{ 
          flex: `${(osMemory / totalMemory) * 100} 0 0`,
          height: '40px',
          background: 'linear-gradient(180deg, #4a4a5a, #2a2a3a)'
        }}
      />
      {blocks.map(block => (
        <div
          key={block.id}
          className={`mini-block ${block.isAllocated ? 'allocated' : 'free'}`}
          style={{
            flex: `${(block.size / totalMemory) * 100} 0 0`,
            height: `${Math.max(20, (block.size / CONFIG.MAX_BLOCK_SIZE) * 100)}px`
          }}
        />
      ))}
    </div>
  );
}

export function ComparisonModal({ isOpen, onClose, manager }) {
  const [results, setResults] = useState(null);

  const runComparison = () => {
    const testData = {
      phase1: [
        { size: 100, name: 'P1' },
        { size: 200, name: 'P2' },
        { size: 150, name: 'P3' },
        { size: 50, name: 'P4' },
        { size: 100, name: 'P5' }
      ],
      phase2: ['P2', 'P4'],
      phase3: [
        { size: 80, name: 'P6' },
        { size: 120, name: 'P7' },
        { size: 40, name: 'P8' }
      ]
    };

    const algorithms = ['firstFit', 'bestFit', 'worstFit'];
    const newResults = {};

    algorithms.forEach(algo => {
      const mm = manager.clone();
      const startTime = performance.now();

      testData.phase1.forEach(test => {
        mm.allocate(test.size, test.name, algo);
      });

      testData.phase2.forEach(processId => {
        mm.deallocate(processId);
      });

      testData.phase3.forEach(test => {
        mm.allocate(test.size, test.name, algo);
      });

      const endTime = performance.now();
      const stats = mm.getStats();

      newResults[algo] = {
        avgTime: ((endTime - startTime) / (testData.phase1.length + testData.phase3.length)).toFixed(2),
        fragmentation: stats.fragmentation,
        freeMemory: stats.freeMemory,
        holes: stats.numHoles,
        blocks: mm.blocks,
        mm
      };
    });

    setResults(newResults);
  };

  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title="Algorithm Comparison"
      wide={true}
      footer={
        <>
          <button className="modal-btn confirm" onClick={runComparison}>Run Comparison</button>
          <button className="modal-btn cancel" onClick={onClose}>Close</button>
        </>
      }
    >
      <div className="comparison-info">
        <p><strong>Test Scenario:</strong></p>
        <ol>
          <li>Allocate 5 processes (P1-P5)</li>
          <li>Deallocate P2 and P4 (creates holes)</li>
          <li>Allocate 3 new processes (P6-P8)</li>
        </ol>
        <p>This tests how each algorithm handles fragmented memory.</p>
      </div>
      
      <div className="comparison-grid">
        {['firstFit', 'bestFit', 'worstFit'].map(algo => (
          <div key={algo} className="comparison-card">
            <h4 className={`comparison-title ${algo === 'firstFit' ? 'cyan' : algo === 'bestFit' ? 'magenta' : 'purple'}`}>
              {algo === 'firstFit' ? 'First Fit' : algo === 'bestFit' ? 'Best Fit' : 'Worst Fit'}
            </h4>
            {results?.[algo] && (
              <>
                <MiniCity 
                  blocks={results[algo].blocks} 
                  osMemory={manager.osMemory} 
                  totalMemory={manager.totalMemory} 
                />
                <div className="comparison-stats">
                  <div className="comp-stat">
                    <span className="comp-label">Avg Time</span>
                    <span className="comp-value">{results[algo].avgTime}ms</span>
                  </div>
                  <div className="comp-stat">
                    <span className="comp-label">Fragmentation</span>
                    <span className="comp-value">{results[algo].fragmentation}%</span>
                  </div>
                  <div className="comp-stat">
                    <span className="comp-label">Free Memory</span>
                    <span className="comp-value">{results[algo].freeMemory} KB</span>
                  </div>
                  <div className="comp-stat">
                    <span className="comp-label">Holes</span>
                    <span className="comp-value">{results[algo].holes}</span>
                  </div>
                </div>
              </>
            )}
            {!results && (
              <div className="comparison-stats">
                <div className="comp-stat"><span className="comp-label">Avg Time</span><span className="comp-value">--</span></div>
                <div className="comp-stat"><span className="comp-label">Fragmentation</span><span className="comp-value">--</span></div>
                <div className="comp-stat"><span className="comp-label">Free Memory</span><span className="comp-value">--</span></div>
                <div className="comp-stat"><span className="comp-label">Holes</span><span className="comp-value">--</span></div>
              </div>
            )}
          </div>
        ))}
      </div>
    </Modal>
  );
}
