/**
 * NeonHeap: Memory Allocation Visualizer
 * Main App Component
 */

import React, { useState, useCallback } from 'react';
import { useMemoryManager } from './hooks/useMemoryManager';
import { CONFIG } from './config/constants';

// Layout Components
import { TopBar } from './components/layout/TopBar';
import { ControlsPanel } from './components/layout/ControlsPanel';
import { VisualizationArea } from './components/layout/VisualizationArea';
import { RightPanel } from './components/layout/RightPanel';
import { BottomPanel } from './components/layout/BottomPanel';

// Visualization Components
import { Particles } from './components/visualization/Particles';

// Modal Components
import { AllocationModal } from './components/modals/AllocationModal';
import { DeallocateModal } from './components/modals/DeallocateModal';
import { AnalysisModal } from './components/modals/AnalysisModal';
import { DisplayModal } from './components/modals/DisplayModal';
import { ExitModal } from './components/modals/ExitModal';
import { ComparisonModal } from './components/modals/ComparisonModal';

import './index.css';

// Presets data
const PRESETS = {
  small: [
    { size: 100, name: 'A1', algo: 'firstFit' },
    { size: 64, name: 'A2', algo: 'firstFit' },
    { size: 96, name: 'A3', algo: 'bestFit' },
    { size: 48, name: 'A4', algo: 'worstFit' }
  ],
  medium: [
    { size: 64, name: 'M1', algo: 'firstFit' },
    { size: 128, name: 'M2', algo: 'bestFit' },
    { size: 32, name: 'M3', algo: 'worstFit' },
    { size: 200, name: 'M4', algo: 'firstFit' },
    { deallocate: 'M2' },
    { size: 48, name: 'M5', algo: 'bestFit' },
    { size: 96, name: 'M6', algo: 'worstFit' },
    { size: 64, name: 'M7', algo: 'firstFit' }
  ],
  heavy: [
    { size: 64, name: 'H1', algo: 'firstFit' },
    { size: 32, name: 'H2', algo: 'bestFit' },
    { size: 128, name: 'H3', algo: 'worstFit' },
    { size: 16, name: 'H4', algo: 'firstFit' },
    { size: 64, name: 'H5', algo: 'bestFit' },
    { size: 48, name: 'H6', algo: 'worstFit' },
    { deallocate: 'H2' },
    { deallocate: 'H4' },
    { deallocate: 'H6' },
    { size: 32, name: 'H7', algo: 'firstFit' },
    { size: 16, name: 'H8', algo: 'bestFit' },
    { size: 24, name: 'H9', algo: 'worstFit' },
    { size: 40, name: 'H10', algo: 'firstFit' }
  ]
};

function generateRandomPreset() {
  const operations = [];
  const names = [];
  const algorithms = ['firstFit', 'bestFit', 'worstFit'];

  for (let i = 0; i < 10; i++) {
    const name = `R${i + 1}`;
    names.push(name);
    operations.push({
      size: Math.floor(Math.random() * 128) + 16,
      name,
      algo: algorithms[Math.floor(Math.random() * algorithms.length)]
    });
  }

  for (let i = 0; i < 3; i++) {
    const idx = Math.floor(Math.random() * names.length);
    operations.push({ deallocate: names[idx] });
  }

  for (let i = 0; i < 4; i++) {
    operations.push({
      size: Math.floor(Math.random() * 64) + 16,
      name: `R${11 + i}`,
      algo: algorithms[Math.floor(Math.random() * algorithms.length)]
    });
  }

  return operations;
}

function App() {
  const {
    manager,
    blocks,
    history,
    stats,
    allocate,
    deallocate,
    reset,
    restoreState,
    getFreeBlocks,
    getAllocatedBlocks
  } = useMemoryManager();

  // UI State
  const [zoomLevel, setZoomLevel] = useState(1);
  const [selectedBlock, setSelectedBlock] = useState(null);
  const [isExited, setIsExited] = useState(false);

  // Modal State
  const [allocationModal, setAllocationModal] = useState({ open: false, algorithm: null });
  const [deallocateModal, setDeallocateModal] = useState(false);
  const [analysisModal, setAnalysisModal] = useState(false);
  const [displayModal, setDisplayModal] = useState(false);
  const [exitModal, setExitModal] = useState(false);
  const [comparisonModal, setComparisonModal] = useState(false);

  // Handlers
  const handleZoomIn = useCallback(() => {
    setZoomLevel(z => Math.min(2, z + 0.1));
  }, []);

  const handleZoomOut = useCallback(() => {
    setZoomLevel(z => Math.max(0.5, z - 0.1));
  }, []);

  const handleResetView = useCallback(() => {
    setZoomLevel(1);
  }, []);

  const handleBlockClick = useCallback((block) => {
    setSelectedBlock(block);
  }, []);

  const handlePreset = useCallback((presetId) => {
    reset();
    const operations = presetId === 'random' ? generateRandomPreset() : PRESETS[presetId];
    
    operations.forEach(op => {
      if (op.deallocate) {
        deallocate(op.deallocate);
      } else {
        allocate(op.size, op.name, op.algo);
      }
    });
  }, [reset, allocate, deallocate]);

  const handleReset = useCallback(() => {
    if (window.confirm('Are you sure you want to reset all memory? This will clear all allocations.')) {
      reset();
    }
  }, [reset]);

  const handleExit = useCallback(() => {
    setIsExited(true);
  }, []);

  const handleRestart = useCallback(() => {
    setIsExited(false);
    reset();
  }, [reset]);

  // Exit screen
  if (isExited) {
    return (
      <div className="exit-screen">
        <h1>SESSION TERMINATED</h1>
        <p>Thank you for using NeonHeap</p>
        <button onClick={handleRestart}>RESTART SIMULATION</button>
      </div>
    );
  }

  // Apply distortion on high fragmentation
  const fragmentation = parseFloat(stats?.fragmentation || 0);
  const distortionClass = fragmentation > CONFIG.CRITICAL_FRAG_THRESHOLD ? 'distortion-active' : '';

  return (
    <div className={distortionClass}>
      {/* Ambient Particles */}
      <Particles />

      {/* Scanline Overlay */}
      <div className="scanline-overlay" />

      {/* Main Container */}
      <div className="app-container">
        <TopBar stats={stats} />

        <div className="main-content">
          <ControlsPanel
            onFirstFit={() => setAllocationModal({ open: true, algorithm: 'firstFit' })}
            onBestFit={() => setAllocationModal({ open: true, algorithm: 'bestFit' })}
            onWorstFit={() => setAllocationModal({ open: true, algorithm: 'worstFit' })}
            onDeallocate={() => setDeallocateModal(true)}
            onDisplay={() => setDisplayModal(true)}
            onAnalysis={() => setAnalysisModal(true)}
            onCompare={() => setComparisonModal(true)}
            onReset={handleReset}
            onExit={() => setExitModal(true)}
            onPreset={handlePreset}
          />

          <VisualizationArea
            blocks={blocks}
            osMemory={manager.osMemory}
            totalMemory={manager.totalMemory}
            zoomLevel={zoomLevel}
            onZoomIn={handleZoomIn}
            onZoomOut={handleZoomOut}
            onResetView={handleResetView}
            selectedBlock={selectedBlock}
            onBlockClick={handleBlockClick}
          />

          <RightPanel stats={stats} />
        </div>

        <BottomPanel
          history={history}
          stats={stats}
          onStepClick={restoreState}
          onScrub={restoreState}
        />
      </div>

      {/* Modals */}
      <AllocationModal
        isOpen={allocationModal.open}
        onClose={() => setAllocationModal({ open: false, algorithm: null })}
        algorithm={allocationModal.algorithm}
        stats={stats}
        onAllocate={allocate}
        getFreeBlocks={getFreeBlocks}
      />

      <DeallocateModal
        isOpen={deallocateModal}
        onClose={() => setDeallocateModal(false)}
        allocatedBlocks={getAllocatedBlocks()}
        onDeallocate={deallocate}
      />

      <AnalysisModal
        isOpen={analysisModal}
        onClose={() => setAnalysisModal(false)}
        stats={stats}
      />

      <DisplayModal
        isOpen={displayModal}
        onClose={() => setDisplayModal(false)}
        blocks={blocks}
        osMemory={manager.osMemory}
      />

      <ExitModal
        isOpen={exitModal}
        onClose={() => setExitModal(false)}
        onConfirm={handleExit}
      />

      <ComparisonModal
        isOpen={comparisonModal}
        onClose={() => setComparisonModal(false)}
        manager={manager}
      />
    </div>
  );
}

export default App;
