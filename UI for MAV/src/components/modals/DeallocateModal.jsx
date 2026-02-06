/**
 * DeallocateModal Component
 * Modal for process deallocation
 */

import React, { useState, useEffect } from 'react';
import { Modal } from './Modal';

export function DeallocateModal({ 
  isOpen, 
  onClose, 
  allocatedBlocks,
  onDeallocate 
}) {
  const [selectedProcess, setSelectedProcess] = useState('');
  const [previewBlock, setPreviewBlock] = useState(null);

  useEffect(() => {
    if (isOpen) {
      setSelectedProcess('');
      setPreviewBlock(null);
    }
  }, [isOpen]);

  const handleSelectChange = (e) => {
    const processId = e.target.value;
    setSelectedProcess(processId);
    if (processId) {
      const block = allocatedBlocks.find(b => b.processId === processId);
      setPreviewBlock(block);
    } else {
      setPreviewBlock(null);
    }
  };

  const handleConfirm = () => {
    if (!selectedProcess) {
      alert('Please select a process');
      return;
    }
    
    const result = onDeallocate(selectedProcess);
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
      title="Deallocate Process"
      footer={
        <>
          <button className="modal-btn cancel" onClick={onClose}>Cancel</button>
          <button className="modal-btn confirm danger" onClick={handleConfirm}>Free Memory</button>
        </>
      }
    >
      <div className="input-group">
        <label htmlFor="deallocProcess">Select Process</label>
        <select 
          id="deallocProcess" 
          value={selectedProcess}
          onChange={handleSelectChange}
        >
          <option value="">-- Select Process --</option>
          {allocatedBlocks.map(block => (
            <option key={block.processId} value={block.processId}>
              {block.processId} ({block.size} KB at 0x{block.startAddress.toString(16).padStart(4, '0')})
            </option>
          ))}
        </select>
      </div>
      <div className="dealloc-preview">
        {previewBlock ? (
          <>
            <p><strong>Process:</strong> {previewBlock.processId}</p>
            <p><strong>Size:</strong> {previewBlock.size} KB</p>
            <p><strong>Address Range:</strong> 0x{previewBlock.startAddress.toString(16).padStart(4, '0')} - 0x{previewBlock.endAddress.toString(16).padStart(4, '0')}</p>
          </>
        ) : (
          <p>Select a process to see details</p>
        )}
      </div>
    </Modal>
  );
}
