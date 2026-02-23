/**
 * VisualizationArea Component
 * Center area containing either the educational city skyline
 * or the live process skyline, based on mode toggle
 */

import React from 'react';
import { CityViewport } from '../visualization/CityViewport';
import { AlgorithmInspector } from '../visualization/AlgorithmInspector';
import { LiveCityViewport } from '../LiveCityViewport';

export function VisualizationArea({
  // Educational mode props
  blocks,
  osMemory,
  totalMemory,
  zoomLevel,
  onZoomIn,
  onZoomOut,
  onResetView,
  selectedBlock,
  onBlockClick,
  // Live mode props
  isLiveMode = false,
  liveProcesses = [],
  liveLoading = false,
  liveError = null,
  livePermissionWarning = false,
  liveLastUpdated = null,
  livePressure = null,
}) {
  return (
    <main className={`visualization-area ${isLiveMode ? 'visualization-area--live' : ''}`}>
      <div className="viz-header">
        <h2 className="viz-title">
          {isLiveMode ? (
            <>
              <span className="neon-text cyan">Live</span> Process Skyline
            </>
          ) : (
            <>
              <span className="neon-text cyan">Memory</span> City Skyline
            </>
          )}
        </h2>
        <div className="viz-controls">
          <button className="viz-btn" title="Zoom In" onClick={onZoomIn}>+</button>
          <button className="viz-btn" title="Zoom Out" onClick={onZoomOut}>−</button>
          <button className="viz-btn" title="Reset View" onClick={onResetView}>⌂</button>
        </div>
      </div>

      {isLiveMode ? (
        <LiveCityViewport
          processes={liveProcesses}
          loading={liveLoading}
          error={liveError}
          permissionWarning={livePermissionWarning}
          lastUpdated={liveLastUpdated}
          zoomLevel={zoomLevel}
          pressure={livePressure}
        />
      ) : (
        <CityViewport
          blocks={blocks}
          osMemory={osMemory}
          totalMemory={totalMemory}
          zoomLevel={zoomLevel}
          onBlockClick={onBlockClick}
        />
      )}

      {!isLiveMode && <AlgorithmInspector selectedBlock={selectedBlock} />}
    </main>
  );
}
