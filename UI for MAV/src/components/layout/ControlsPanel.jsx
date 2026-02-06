/**
 * ControlsPanel Component
 * Left sidebar with algorithm buttons, operation buttons, and presets
 */

import React from 'react';
import { NeonButton } from '../ui/NeonButton';

const PRESETS = [
  { id: 'small', label: 'Small' },
  { id: 'medium', label: 'Medium' },
  { id: 'heavy', label: 'Heavy' },
  { id: 'random', label: 'Random' }
];

export function ControlsPanel({
  onFirstFit,
  onBestFit,
  onWorstFit,
  onDeallocate,
  onDisplay,
  onAnalysis,
  onCompare,
  onReset,
  onExit,
  onPreset
}) {
  return (
    <aside className="controls-panel">
      <div className="panel-header">
        <span className="panel-icon">◆</span>
        <h2>Control Hub</h2>
      </div>

      <div className="controls-group">
        <h3 className="group-title">Allocation Algorithms</h3>
        
        <NeonButton
          color="cyan"
          icon="▸"
          text="First Fit"
          description="Uses first suitable hole"
          onClick={onFirstFit}
        />
        
        <NeonButton
          color="magenta"
          icon="◈"
          text="Best Fit"
          description="Uses smallest suitable hole"
          onClick={onBestFit}
        />
        
        <NeonButton
          color="purple"
          icon="◇"
          text="Worst Fit"
          description="Uses largest hole"
          onClick={onWorstFit}
        />
      </div>

      <div className="controls-group">
        <h3 className="group-title">Operations</h3>
        
        <NeonButton
          color="orange"
          icon="✕"
          text="Deallocate Process"
          description="Frees memory & merges holes"
          onClick={onDeallocate}
        />
        
        <NeonButton
          color="blue"
          icon="⊞"
          text="Display Memory"
          description="Shows current layout"
          onClick={onDisplay}
        />
        
        <NeonButton
          color="yellow"
          icon="◎"
          text="Fragmentation Analysis"
          description="Detailed breakdown"
          onClick={onAnalysis}
        />
      </div>

      <div className="controls-group">
        <h3 className="group-title">Tools</h3>
        
        <NeonButton
          color="green"
          icon="⧉"
          text="Compare Algorithms"
          description="Side-by-side results"
          onClick={onCompare}
        />
        
        <NeonButton
          color="red"
          icon="↺"
          text="Reset Memory"
          description="Clear all processes"
          onClick={onReset}
        />
        
        <NeonButton
          color="dark"
          icon="⏻"
          text="Exit Session"
          description="End simulation"
          onClick={onExit}
        />
      </div>

      <div className="controls-group presets">
        <h3 className="group-title">Test Presets</h3>
        <div className="preset-buttons">
          {PRESETS.map(preset => (
            <button
              key={preset.id}
              className="preset-btn"
              onClick={() => onPreset(preset.id)}
            >
              {preset.label}
            </button>
          ))}
        </div>
      </div>
    </aside>
  );
}
