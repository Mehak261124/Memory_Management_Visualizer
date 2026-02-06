/**
 * MemoryBlockView Component
 * Individual memory block (building) visualization
 */

import React from 'react';
import { CONFIG } from '../../config/constants';

export function MemoryBlockView({ block, totalMemory, maxHeight = 280, minHeight = 60, onClick }) {
  const widthPercent = (block.size / totalMemory) * 100;
  const heightPercent = block.size / CONFIG.MAX_BLOCK_SIZE;
  const height = Math.min(maxHeight, Math.max(minHeight, heightPercent * maxHeight));

  const numWindows = Math.floor(block.size / 20);

  return (
    <div
      className={`memory-block ${block.isAllocated ? 'allocated' : 'free'}`}
      style={{
        width: `${widthPercent}%`,
        minWidth: '60px',
        height: `${height}px`
      }}
      onClick={() => onClick && onClick(block)}
      data-block-id={block.id}
    >
      {block.isAllocated && (
        <div className="block-windows">
          {Array.from({ length: Math.min(numWindows, 24) }).map((_, i) => (
            <div
              key={i}
              className={`window ${Math.random() > CONFIG.WINDOW_FLICKER_CHANCE ? '' : 'off'}`}
              style={{ '--window-index': i }}
            />
          ))}
        </div>
      )}
      <div className="block-info">
        <div className="block-process">{block.isAllocated ? block.processId : 'FREE'}</div>
        <div className="block-size">{block.size} KB</div>
        <div className="block-address">
          0x{block.startAddress.toString(16).padStart(4, '0')} - 0x{block.endAddress.toString(16).padStart(4, '0')}
        </div>
      </div>
    </div>
  );
}
