/**
 * DisplayModal Component
 * Memory state table display
 */

import React from 'react';
import { Modal } from './Modal';

export function DisplayModal({ isOpen, onClose, blocks, osMemory }) {
  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title="Memory State Display"
      wide={true}
      footer={
        <button className="modal-btn confirm" onClick={onClose}>Close</button>
      }
    >
      <div className="memory-table-container">
        <table className="memory-table">
          <thead>
            <tr>
              <th>Block</th>
              <th>Start Address</th>
              <th>End Address</th>
              <th>Size</th>
              <th>Status</th>
              <th>Process</th>
            </tr>
          </thead>
          <tbody>
            <tr className="os">
              <td>OS</td>
              <td>0x0000</td>
              <td>0x{(osMemory - 1).toString(16).padStart(4, '0').toUpperCase()}</td>
              <td>{osMemory} KB</td>
              <td>Reserved</td>
              <td>Operating System</td>
            </tr>
            {blocks.map((block, index) => (
              <tr key={block.id} className={block.isAllocated ? 'allocated' : 'free'}>
                <td>{index + 1}</td>
                <td>0x{block.startAddress.toString(16).padStart(4, '0').toUpperCase()}</td>
                <td>0x{block.endAddress.toString(16).padStart(4, '0').toUpperCase()}</td>
                <td>{block.size} KB</td>
                <td>{block.isAllocated ? 'Allocated' : 'Free'}</td>
                <td>{block.processId || '-'}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </Modal>
  );
}
