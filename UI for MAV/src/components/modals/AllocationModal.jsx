/**
 * AllocationModal Component
 * Modal for memory allocation with algorithm definitions and selection
 */

import React, { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { ALGORITHM_NAMES } from '../../config/constants';

const ALGO_DEFINITIONS = {
  firstFit: {
    name: 'First Fit',
    emoji: '▶',
    color: 'var(--cyan)',
    summary: 'Fast — takes the first available hole that is large enough.',
    steps: [
      'Start scanning from the beginning of memory',
      'Check if current hole ≥ requested size',
      'If YES → allocate here immediately (stop searching)',
      'If NO → move to the next hole',
    ],
    pros: [
      'Fastest allocation — O(n) worst case but often stops early',
      'Simple implementation',
      'Good for quick allocations',
    ],
    cons: [
      'Causes fragmentation at the front of memory',
      'Larger holes at the end go unused',
      'Front of memory gets "crumbled" over time',
    ],
    complexity: 'O(n)',
    bestFor: 'General purpose — good default when speed matters',
  },
  bestFit: {
    name: 'Best Fit',
    emoji: '◆',
    color: 'var(--magenta)',
    summary: 'Memory-efficient — finds the smallest hole that fits.',
    steps: [
      'Scan ALL free holes in memory',
      'Track the smallest hole where size ≥ requested',
      'After full scan → allocate in that smallest suitable hole',
      'Minimizes leftover space in the chosen hole',
    ],
    pros: [
      'Minimizes wasted space per allocation',
      'Preserves large holes for big future requests',
      'Best memory utilization',
    ],
    cons: [
      'Must scan entire list — always O(n)',
      'Creates many tiny unusable fragments',
      'Slower than First Fit',
    ],
    complexity: 'O(n)',
    bestFor: 'Memory-constrained systems with varied allocation sizes',
  },
  worstFit: {
    name: 'Worst Fit',
    emoji: '◇',
    color: 'var(--purple)',
    summary: 'Anti-fragmentation — picks the largest available hole.',
    steps: [
      'Scan ALL free holes in memory',
      'Track the largest hole overall',
      'After full scan → allocate in the largest hole',
      'Leaves the biggest possible remainder',
    ],
    pros: [
      'Remainders are large enough to be useful',
      'Reduces tiny unusable fragments',
      'Avoids the "tiny scraps" problem of Best Fit',
    ],
    cons: [
      'Must scan entire list — always O(n)',
      'Quickly breaks up large holes',
      'Poor for systems needing big contiguous blocks later',
    ],
    complexity: 'O(n)',
    bestFor: 'When keeping large remainders is more important than tight packing',
  },
};

export function AllocationModal({ 
  isOpen, 
  onClose, 
  algorithm, 
  stats,
  onAllocate,
  getFreeBlocks
}) {
  const [processName, setProcessName] = useState('');
  const [memorySize, setMemorySize] = useState('');

  useEffect(() => {
    if (isOpen) {
      setProcessName('');
      setMemorySize('');
    }
  }, [isOpen]);

  const freeBlocks = getFreeBlocks ? getFreeBlocks() : [];
  const maxFree = freeBlocks.length > 0 ? Math.max(...freeBlocks.map(b => b.size)) : 0;
  const def = ALGO_DEFINITIONS[algorithm];

  const handleConfirm = async () => {
    const size = parseInt(memorySize);
    if (!size || size <= 0) {
      alert('Please enter a valid memory size');
      return;
    }
    
    try {
      const result = await onAllocate(size, processName, algorithm);
      if (result && result.success) {
        onClose();
      } else if (result) {
        alert(result.message);
      }
    } catch (err) {
      alert('Failed to allocate memory. Is the C backend running?');
    }
  };

  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title={`Allocate Memory — ${def?.name || algorithm}`}
      footer={
        <>
          <button className="modal-btn cancel" onClick={onClose}>Cancel</button>
          <button className="modal-btn confirm" onClick={handleConfirm}>Allocate</button>
        </>
      }
    >
      {/* Algorithm Definition Section */}
      {def && (
        <div className="algo-definition" style={{ borderColor: `${def.color}33` }}>
          <h3 style={{ color: def.color, marginBottom: '8px' }}>
            {def.emoji} What is {def.name}?
          </h3>
          <p style={{ color: 'var(--text-secondary)', marginBottom: '12px' }}>{def.summary}</p>

          <h4 style={{ color: 'var(--text-primary)', marginBottom: '6px' }}>🔍 How It Works:</h4>
          <ol style={{ color: 'var(--text-secondary)', paddingLeft: '20px', marginBottom: '12px' }}>
            {def.steps.map((step, i) => (
              <li key={i} style={{ marginBottom: '4px' }}>{step}</li>
            ))}
          </ol>

          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '12px', marginBottom: '12px' }}>
            <div>
              <h4 style={{ color: '#50fa7b', marginBottom: '4px' }}>✓ Advantages</h4>
              <ul style={{ color: 'var(--text-secondary)', paddingLeft: '16px', fontSize: '0.85rem' }}>
                {def.pros.map((p, i) => <li key={i} style={{ marginBottom: '3px' }}>{p}</li>)}
              </ul>
            </div>
            <div>
              <h4 style={{ color: '#ff5555', marginBottom: '4px' }}>✗ Disadvantages</h4>
              <ul style={{ color: 'var(--text-secondary)', paddingLeft: '16px', fontSize: '0.85rem' }}>
                {def.cons.map((c, i) => <li key={i} style={{ marginBottom: '3px' }}>{c}</li>)}
              </ul>
            </div>
          </div>

          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '12px' }}>
            <div style={{ background: 'var(--bg-tertiary)', padding: '8px 12px', borderRadius: '6px', textAlign: 'center' }}>
              <div style={{ fontSize: '0.7rem', color: 'var(--text-dim)', textTransform: 'uppercase' }}>Time Complexity:</div>
              <div style={{ fontSize: '1.1rem', fontWeight: 700, color: def.color, fontFamily: 'var(--font-mono)' }}>{def.complexity}</div>
            </div>
            <div style={{ background: 'var(--bg-tertiary)', padding: '8px 12px', borderRadius: '6px', textAlign: 'center' }}>
              <div style={{ fontSize: '0.7rem', color: 'var(--text-dim)', textTransform: 'uppercase' }}>Best For:</div>
              <div style={{ fontSize: '0.8rem', fontWeight: 600, color: def.color }}>{def.bestFor}</div>
            </div>
          </div>
        </div>
      )}

      {/* Allocation Form */}
      <h3 style={{ color: 'var(--cyan)', marginTop: '16px', marginBottom: '8px' }}>Allocate Process</h3>
      <div className="input-group">
        <label htmlFor="processName">Process Name</label>
        <input
          type="text"
          id="processName"
          placeholder="e.g., P1, Chrome, etc."
          value={processName}
          onChange={(e) => setProcessName(e.target.value)}
        />
      </div>
      <div className="input-group">
        <label htmlFor="memorySize">Memory Size (KB)</label>
        <input
          type="number"
          id="memorySize"
          placeholder="e.g., 128"
          min="1"
          max="768"
          value={memorySize}
          onChange={(e) => setMemorySize(e.target.value)}
        />
      </div>
    </Modal>
  );
}

