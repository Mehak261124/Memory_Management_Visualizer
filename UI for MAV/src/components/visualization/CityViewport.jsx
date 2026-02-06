/**
 * CityViewport Component
 * Container for the city skyline visualization
 */

import React from 'react';
import { MemoryBlockView } from './MemoryBlockView';

export function CityViewport({ 
  blocks, 
  osMemory, 
  totalMemory, 
  zoomLevel = 1,
  onBlockClick 
}) {
  return (
    <div className="city-viewport">
      <div 
        className="city-container" 
        style={{ transform: `scale(${zoomLevel})` }}
      >
        {/* Street Layer */}
        <div className="street-layer">
          <div className="neon-road" />
        </div>

        {/* Buildings Layer (Memory Blocks) */}
        <div className="buildings-layer">
          {/* OS Block */}
          <div
            className="memory-block os-block"
            style={{
              width: `${(osMemory / totalMemory) * 100}%`,
              height: '150px'
            }}
          >
            <div className="block-info">
              <div className="block-process">OS</div>
              <div className="block-size">{osMemory} KB</div>
              <div className="block-address">
                0x0000 - 0x{(osMemory - 1).toString(16).padStart(4, '0')}
              </div>
            </div>
          </div>

          {/* User Memory Blocks */}
          {blocks.map(block => (
            <MemoryBlockView
              key={block.id}
              block={block}
              totalMemory={totalMemory}
              onClick={onBlockClick}
            />
          ))}
        </div>

        {/* Address Bar */}
        <div className="address-bar" />
      </div>
    </div>
  );
}
