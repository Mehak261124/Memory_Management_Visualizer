/**
 * VisualizationArea Component
 * Center area containing the city skyline memory visualization
 */

import React from 'react';
import { CityViewport } from '../visualization/CityViewport';
import { AlgorithmInspector } from '../visualization/AlgorithmInspector';

export function VisualizationArea({
  blocks,
  osMemory,
  totalMemory,
  zoomLevel,
  onZoomIn,
  onZoomOut,
  onResetView,
  selectedBlock,
  onBlockClick
}) {
  return (
    <main className="visualization-area">
      <div className="viz-header">
        <h2 className="viz-title">
          <span className="neon-text cyan">Memory</span> City Skyline
        </h2>
        <div className="viz-controls">
          <button className="viz-btn" title="Zoom In" onClick={onZoomIn}>+</button>
          <button className="viz-btn" title="Zoom Out" onClick={onZoomOut}>−</button>
          <button className="viz-btn" title="Reset View" onClick={onResetView}>⌂</button>
        </div>
      </div>

      <CityViewport
        blocks={blocks}
        osMemory={osMemory}
        totalMemory={totalMemory}
        zoomLevel={zoomLevel}
        onBlockClick={onBlockClick}
      />

      <AlgorithmInspector selectedBlock={selectedBlock} />
    </main>
  );
}
