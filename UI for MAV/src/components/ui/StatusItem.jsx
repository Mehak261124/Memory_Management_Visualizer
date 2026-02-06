/**
 * StatusItem Component
 * Status indicator for header
 */

import React from 'react';

export function StatusItem({ label, value, variant = '' }) {
  return (
    <div className="status-item">
      <span className="status-label">{label}</span>
      <span className={`status-value ${variant}`}>{value}</span>
    </div>
  );
}
