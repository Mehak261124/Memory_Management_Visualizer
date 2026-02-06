/**
 * ExitModal Component
 * Exit confirmation modal
 */

import React from 'react';
import { Modal } from './Modal';

export function ExitModal({ isOpen, onClose, onConfirm }) {
  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title="⚠ Exit Confirmation"
      headerClass="warning"
      footer={
        <>
          <button className="modal-btn cancel" onClick={onClose}>Cancel</button>
          <button className="modal-btn confirm danger" onClick={onConfirm}>Exit Session</button>
        </>
      }
    >
      <p className="warning-message">
        Are you sure you want to end the simulation?
      </p>
      <p className="warning-sub">All current memory state will be lost.</p>
    </Modal>
  );
}
