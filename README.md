# NeonHeap: Memory Allocation Visualizer

A full-stack Operating Systems project with a **C backend** and **React (Vite) frontend** that visualizes dynamic memory allocation algorithms and real OS process activity through an interactive cyberpunk-themed city skyline interface.

---

## 🎯 Overview

NeonHeap is a Memory Allocation Visualizer designed to make core OS memory management concepts tangible and interactive. It supports **two modes**:

| Mode | Purpose |
|------|---------|
| **Educational Mode** | Simulate First Fit, Best Fit, Worst Fit, compaction, and buddy system with a visual city skyline |
| **Live System Mode** | Visualize the top 10 real OS processes as buildings, inspect any process on click, and monitor system-wide memory pressure |

The C backend uses real OS system calls (`mmap`, `proc_pidinfo`, `sysctl`) to back the simulation with actual memory and process data. The React frontend renders everything as a cyberpunk neon city skyline.

---

## ✨ Features

### Educational Mode
- **Allocation Algorithms:** First Fit, Best Fit, Worst Fit
- **Deallocation** with automatic adjacent-hole merging
- **Memory Compaction** (defragmentation)
- **Buddy System** — convert/revert with visual buddy tree
- **Algorithm Comparison** — side-by-side under identical workloads
- **Fragmentation Analysis** — real-time external fragmentation percentage
- **Timeline & History** — step-by-step scrubbing through past operations
- **Preset Scenarios** — Small, Medium, Heavy, Random workloads

### Live System Mode
- **Live Process Skyline** — top 10 real OS processes as buildings (height ∝ RSS)
- **Process Detail on Click** — modal with threads, page faults, pageins, COW faults, executable path, start time
- **Memory Pressure Indicator** — system-wide RAM usage bar (LOW / MODERATE / HIGH / CRITICAL)
- **Auto-polling** — refreshes every 3 seconds
- **Permission Warning** — prompts to use `sudo` for full visibility

### Visual Design
- Cyberpunk neon dark theme with glassmorphism
- Ambient particles + scanline overlays
- Smooth building rise/pulse animations
- Responsive layout (desktop → mobile)

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────┐
│                React Frontend (Vite)                │
│  Components: CityViewport, LiveCityViewport,        │
│  ProcessDetailModal, MemoryPressureBar, etc.        │
│  Hooks: useMemoryManager, useLiveProcesses          │
├─────────────────────────────────────────────────────┤
│               HTTP JSON API (Port 8080)             │
├─────────────────────────────────────────────────────┤
│                   C Backend                         │
│  memory_manager.c  — allocation algorithms          │
│  memory_structures.c — linked list, buddy tree      │
│  os_memory.c — mmap/munmap, sysctl, sysconf         │
│  proc_reader.c — real process scanning              │
│  http_server.c — POSIX socket HTTP server           │
└─────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
Memory_Management_Visualizer/
├── include/
│   ├── memory_manager.h       # Allocation/deallocation API
│   ├── memory_structures.h    # MemoryBlock, BuddyNode structs
│   ├── os_memory.h            # mmap/munmap, sysctl wrappers
│   ├── proc_reader.h          # Process scanning API
│   └── http_server.h          # HTTP server API
├── src/
│   ├── main.c                 # Entry point (menu + server modes)
│   ├── memory_manager.c       # First/Best/Worst Fit, compaction, buddy
│   ├── memory_structures.c    # Linked list operations
│   ├── os_memory.c            # Real OS memory via mmap
│   ├── proc_reader.c          # Process list, detail, memory pressure
│   └── http_server.c          # POSIX socket HTTP + JSON API
├── UI for MAV/                # React + Vite frontend
│   ├── src/
│   │   ├── App.jsx
│   │   ├── index.css
│   │   ├── api/
│   │   │   ├── getProcesses.js
│   │   │   ├── getProcessDetail.js
│   │   │   └── getMemoryPressure.js
│   │   ├── hooks/
│   │   │   ├── useMemoryManager.js
│   │   │   └── useLiveProcesses.js
│   │   └── components/
│   │       ├── LiveCityViewport.jsx
│   │       ├── LiveProcessBlock.jsx
│   │       ├── ProcessDetailModal.jsx
│   │       ├── MemoryPressureBar.jsx
│   │       ├── layout/
│   │       ├── modals/
│   │       ├── visualization/
│   │       └── ...
│   └── package.json
├── build/
│   └── memory_visualizer      # Compiled backend binary
├── docs/
│   ├── README.md              # This file
│   └── project_report.pdf
└── presentation.html
```

---

## 📦 Installation & Usage

### Prerequisites
- **GCC** compiler (Xcode CLI tools on macOS, `build-essential` on Linux)
- **Node.js** ≥ 18 and **npm**

### 1. Compile the C Backend

```bash
cd Memory_Management_Visualizer
gcc src/*.c -I include -o build/memory_visualizer -framework CoreFoundation
```

> On Linux, omit `-framework CoreFoundation`:
> ```bash
> gcc src/*.c -I include -o build/memory_visualizer
> ```

### 2. Start the Backend Server

```bash
./build/memory_visualizer --server 8080
```

The server will display all available API endpoints and listen on `http://localhost:8080`.

### 3. Start the React Frontend

```bash
cd "UI for MAV"
npm install
npm run dev
```

Open `http://localhost:5173` in your browser.

### 4. Interactive Text Mode (Optional)

```bash
./build/memory_visualizer
```

Runs a terminal-based menu for direct interaction without the React frontend.

---

## 🔌 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/status` | Health check |
| `GET` | `/api/blocks` | All memory blocks (educational) |
| `GET` | `/api/stats` | Memory statistics |
| `GET` | `/api/sysinfo` | OS system info (page size, RAM) |
| `POST` | `/api/allocate` | Allocate memory `{size, algorithm}` |
| `POST` | `/api/deallocate` | Free process `{processId}` |
| `POST` | `/api/compact` | Run compaction |
| `POST` | `/api/autocompact` | Auto-compact `{threshold}` |
| `POST` | `/api/buddy/convert` | Enable buddy system |
| `POST` | `/api/buddy/revert` | Revert to normal |
| `POST` | `/api/reset` | Reset memory |
| `GET` | `/api/processes/top` | Top 10 real OS processes |
| `GET` | `/api/process/<pid>` | Extended detail for single PID |
| `GET` | `/api/memory/pressure` | System-wide memory pressure |

---

## 🧮 Algorithms Implemented

| Algorithm | Strategy | Time Complexity | Fragmentation |
|-----------|----------|----------------|---------------|
| **First Fit** | First hole that fits | O(n) | Moderate |
| **Best Fit** | Smallest sufficient hole | O(n) | Creates tiny holes |
| **Worst Fit** | Largest available hole | O(n) | Wastes large holes |
| **Compaction** | Slide all processes to one end | O(n) | Eliminates external frag. |
| **Buddy System** | Power-of-2 splits/merges | O(log n) | Internal fragmentation |

---

## 🛠️ Technical Stack

| Layer | Technology |
|-------|-----------|
| **Backend Language** | C (C11) |
| **OS APIs (macOS)** | `libproc` (`proc_pidinfo`, `proc_listallpids`), `sysctl`, `mmap`/`munmap` |
| **OS APIs (Linux)** | `/proc` filesystem, `sysconf`, `readlink` |
| **Networking** | POSIX sockets (single-threaded HTTP) |
| **Frontend** | React 18 + Vite |
| **Styling** | Vanilla CSS (cyberpunk neon theme) |
| **Platform** | macOS (primary), Linux (via `#ifdef`) |

---

## 📊 Key OS Concepts Demonstrated

- **Dynamic Partitioning** — variable-sized allocation from a contiguous pool
- **External Fragmentation** — scattered free holes; formula: `(TotalFree − LargestHole) / UserMemory × 100`
- **Hole Merging** — adjacent free blocks coalesce on deallocation
- **Memory Compaction** — relocate all processes to eliminate fragmentation
- **Buddy System** — power-of-2 splitting/coalescing for efficient allocation
- **Virtual Memory** — `mmap()`/`munmap()` for real OS-backed memory regions
- **Process Introspection** — reading live kernel process data via system APIs
- **Memory Pressure** — system-wide RAM utilization monitoring
---

## 📄 License

This project is developed for academic purposes as part of an Operating Systems course.