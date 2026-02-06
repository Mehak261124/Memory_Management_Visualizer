/**
 * AllocationModal Component
 * Modal for memory allocation with algorithm selection
 */

import React, { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { ALGORITHM_NAMES } from '../../config/constants';

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

  const handleConfirm = () => {
    const size = parseInt(memorySize);
    if (!size || size <= 0) {
      alert('Please enter a valid memory size');
      return;
    }
    
    const result = onAllocate(size, processName, algorithm);
    if (result.success) {
      onClose();
    } else {
      alert(result.message);
    }
  };

  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title={`Allocate Memory (${ALGORITHM_NAMES[algorithm] || algorithm})`}
      footer={
        <>
          <button className="modal-btn cancel" onClick={onClose}>Cancel</button>
          <button className="modal-btn confirm" onClick={handleConfirm}>Allocate</button>
        </>
      }
    >
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
      <div className="algorithm-preview">
        <p><strong>Algorithm:</strong> {ALGORITHM_NAMES[algorithm] || algorithm}</p>
        <p><strong>Available Free Memory:</strong> {stats?.freeMemory || 0} KB</p>
        <p><strong>Largest Hole:</strong> {maxFree} KB</p>
        <p><strong>Number of Holes:</strong> {freeBlocks.length}</p>
      </div>
    </Modal>
  );
}
