/**
 * BottomPanel Component
 * Timeline section and fragmentation dashboard
 */

import React from 'react';
import { Timeline } from '../timeline/Timeline';
import { FragmentationDashboard } from '../stats/FragmentationDashboard';

export function BottomPanel({ history, stats, onStepClick, onScrub }) {
  return (
    <footer className="bottom-panel">
      <Timeline
        history={history}
        onStepClick={onStepClick}
        onScrub={onScrub}
      />
      <FragmentationDashboard stats={stats} />
    </footer>
  );
}
