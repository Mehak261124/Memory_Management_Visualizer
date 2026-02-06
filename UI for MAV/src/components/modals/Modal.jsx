/**
 * Modal Component
 * Base modal wrapper
 */

import React from 'react';

export function Modal({ 
  isOpen, 
  onClose, 
  title, 
  children, 
  footer,
  wide = false,
  headerClass = ''
}) {
  if (!isOpen) return null;

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div 
        className={`modal glass-panel ${wide ? 'wide' : ''}`} 
        onClick={e => e.stopPropagation()}
      >
        <div className={`modal-header ${headerClass}`}>
          <h3>{title}</h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>
        <div className="modal-body">
          {children}
        </div>
        {footer && (
          <div className="modal-footer">
            {footer}
          </div>
        )}
      </div>
    </div>
  );
}
