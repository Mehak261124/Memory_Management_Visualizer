/**
 * NeonButton Component
 * Reusable neon-styled button with color variants
 */

import React from 'react';

export function NeonButton({ 
  color = 'cyan', 
  icon, 
  text, 
  description, 
  onClick, 
  className = '',
  ...props 
}) {
  return (
    <button 
      className={`neon-btn ${color} ${className}`} 
      onClick={onClick}
      {...props}
    >
      {icon && <span className="btn-icon">{icon}</span>}
      <span className="btn-text">{text}</span>
      {description && <span className="btn-desc">{description}</span>}
    </button>
  );
}
