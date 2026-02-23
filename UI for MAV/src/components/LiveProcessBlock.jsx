/**
 * LiveProcessBlock Component
 * Individual building visualization for one real OS process
 * Matches the cyberpunk/neon aesthetic of the existing MemoryBlockView
 */

import React, { useState, useCallback } from 'react';

const MAX_HEIGHT_PX = 300;
const MIN_HEIGHT_PX = 40;

export function LiveProcessBlock({ process, maxRSS, index, onClick }) {
  const [clicked, setClicked] = useState(false);

  // Height proportional to RSS relative to the largest process
  const ratio = maxRSS > 0 ? process.vmRSS_KB / maxRSS : 0;
  const height = Math.max(MIN_HEIGHT_PX, Math.floor(ratio * MAX_HEIGHT_PX));

  // Neon cyan for large processes (top 3), magenta for the rest
  const isLarge = index < 3;
  const accentColor = isLarge ? '#00ffff' : '#ff00ff';

  // Convert KB to MB for display
  const rssMB = (process.vmRSS_KB / 1024).toFixed(1);
  const vSizeMB = (process.vmSize_KB / 1024).toFixed(0);

  // Generate window count based on RSS
  const numWindows = Math.min(Math.floor(process.vmRSS_KB / 5000) + 2, 20);

  // Click handler with brief pulse animation
  const handleClick = useCallback(() => {
    setClicked(true);
    setTimeout(() => setClicked(false), 200);
    if (onClick) onClick(process);
  }, [onClick, process]);

  return (
    <div
      className={`live-building ${isLarge ? 'live-building--large' : 'live-building--small'} ${clicked ? 'live-building--clicked' : ''}`}
      style={{
        height: `${height}px`,
        '--accent-color': accentColor,
        '--building-index': index,
        cursor: 'pointer',
      }}
      title={`PID: ${process.pid}\nName: ${process.name}\nRSS: ${rssMB} MB\nVirtual: ${vSizeMB} MB`}
      onClick={handleClick}
    >
      {/* Windows */}
      <div className="live-building__windows">
        {Array.from({ length: numWindows }).map((_, i) => {
          // Deterministic: use PID and index as seed so state
          // never changes on re-render unless process actually changes
          const isOn = ((process.pid * 31 + i * 7) % 10) > 2;
          return (
            <div
              key={i}
              className={`live-building__window ${isOn ? '' : 'off'}`}
              style={{ '--window-index': i }}
            />
          );
        })}
      </div>

      {/* Info Label */}
      <div className="live-building__info">
        <div className="live-building__name" title={process.name}>
          {process.name.length > 12 ? process.name.substring(0, 12) + '…' : process.name}
        </div>
        <div className="live-building__pid">PID {process.pid}</div>
        <div className="live-building__rss">{rssMB} MB</div>
      </div>
    </div>
  );
}

