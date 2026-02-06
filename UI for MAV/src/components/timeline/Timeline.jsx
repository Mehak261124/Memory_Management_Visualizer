/**
 * Timeline Component
 * History timeline with step markers
 */

import React from 'react';

export function Timeline({ history, currentIndex, onStepClick, onScrub }) {
  return (
    <div className="timeline-section">
      <div className="timeline-header">
        <span className="timeline-icon">◷</span>
        <span className="timeline-title">Memory Timeline</span>
        <span className="timeline-count">{history.length} actions</span>
      </div>
      <div className="timeline-track">
        <div className="timeline-marker current" />
        {history.map((state, index) => (
          <div
            key={index}
            className={`timeline-step ${state.action.includes('Allocated') ? 'allocate' : 'deallocate'} ${index === history.length - 1 ? 'active' : ''}`}
            title={state.action}
            onClick={() => onStepClick(index)}
          >
            {index + 1}
          </div>
        ))}
      </div>
      <div className="timeline-scrubber">
        <input
          type="range"
          min="0"
          max={Math.max(0, history.length - 1)}
          value={currentIndex ?? history.length - 1}
          onChange={(e) => onScrub(parseInt(e.target.value))}
        />
      </div>
    </div>
  );
}
