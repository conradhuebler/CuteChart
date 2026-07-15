# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CuteChart is a Qt6/C++17-based enhanced chart visualization framework built on top of Qt Charts. It provides advanced charting capabilities with extensive customization options, interactive features, and professional data visualization components specifically designed for [SupraFit](https://github.com/conradhuebler/SupraFit).

## Build System

### Primary Build Commands
```bash
# Basic build
mkdir -p build && cd build
cmake .. && make -j4

# Run example application
./example
```

## General instructions

- Each source code dir has a CLAUDE.md with basic informations of the code, the corresponding knowledge and logic in the directory 
- If a file is not present or outdated, create or update it
- Task corresponding to code have to be placed in the correct CLAUDE.md file
- Each CLAUDE.md may contain a variable part, where short-term information, bugs etc things are stored. Obsolete information have to be removed
- Each CLAUDE.md has a preserved part, which should no be edited by CLAUDE, only created if not present
- Each CLAUDE.md may contain an **instructions block** filled by the operator/programmer and from CLAUDE if approved with future tasks and visions that must be considered during code development
- Each CLAUDE.md file content should be important for ALL subdirectories
- If new knowledge is obtained from Claude Code Conversation preserve it in the CLAUDE.md files
- Always give improvments to existing code

### CMake Configuration
- **Qt Version**: Qt6 (minimum 6.1) with Core, Widgets, and Charts modules
- **C++ Standard**: C++17 required
- **Build Targets**: `cutechart` (static library) and `example` (demo application)
- **Compiler Flags**: Extensive warning flags enabled for GCC builds

### Code Organization
- Variable sections updated regularly with short-term information
- Preserved sections contain permanent knowledge and patterns
- Instructions blocks contain operator-defined future tasks and visions

### Implementation Standards
- Mark new functions as "Claude Generated" for traceability
- Document new functions briefly (doxygen ready)
- Document existing undocumented functions if appearing regulary (briefly and doxygen ready)
- Remove TODO Hashtags and text if done and approved
- Use modern Qt6 patterns and avoid deprecated functions
- Implement comprehensive error handling and logging 
- Debugging output with qDebug() within #ifdef DEBUG_ON #endif
- Check if this is written correctly (CMakeLists.txt and include) 
- non-debugging console output is realised with fmt, port away from std::cout if appearing
- Maintain backward compatibility where possible
- **Always check and consider instructions blocks** in relevant CLAUDE.md files before implementing 
- reformulate and clarify task and vision entries if not alredy marked as CLAUDE formatted
- in case of compiler warning for deprecated suprafit functions, replace the old function call with the new one


## Architecture Overview

### Core Components

**ChartView** (`src/chartview.h/.cpp`)
- Main scrollable chart widget extending QScrollArea
- Manages chart configuration, zooming, selection strategies
- Handles axis scaling with multiple auto-scale strategies
- Provides export capabilities and font configuration
- Configuration managed through JSON objects with default settings

**ChartViewPrivate** (`src/chartviewprivate.h/.cpp`)
- Internal implementation handling chart interaction logic
- Mouse/keyboard event processing for zooming and selection
- Manages vertical tracking lines and selection boxes

**Enhanced Series Types** (`src/series.h/.cpp`)
- `LineSeries`: Extended QLineSeries with custom styling (dash-dot lines, width control)
- `ScatterSeries`: Enhanced QScatterSeries with legend control
- `BoxPlotSeries`: Statistical visualization series for BoxWhisker data
- State pattern implementation for series property management during operations

**ListChart** (`src/listchart.h/.cpp`)
- Composite widget combining ChartView with series management list
- Interactive series toggling, renaming, and color customization
- HTML-enabled list items via custom delegate (`HTMLListItem`)

**ChartTools Utilities** (`src/tools.h`)
- Mathematical utilities for axis scaling and "nice numbers" formatting
- Functions: `NiceScalingMin/Max`, `IdealInterval`, `CustomCeil/Floor`
- JSON object merging utilities for configuration management

**Configuration System** (`src/chartconfig.h/.cpp`)
- JSON-based chart configuration with dialog interface
- Font management, export settings, theme control
- Modular configuration with preset management

### Key Design Patterns

1. **Strategy Pattern**: Zoom and selection strategies (`ZoomStrategy`, `SelectStrategy`, `AutoScaleStrategy`)
2. **State Pattern**: Series state management for export operations
3. **Factory Pattern**: `SeriesStateFactory` for creating appropriate state objects
4. **Configuration Pattern**: JSON-driven settings with default fallbacks

### Chart Interaction Features

- **Zooming**: Multiple strategies (none, horizontal, vertical, rectangular)
- **Selection**: Configurable selection modes with visual feedback
- **Callouts**: `PeakCallOut` annotations for data point highlighting
- **Vertical Lines**: Static reference lines at specified X positions
- **Export**: PNG export with cropping and transparency options

## Development Workflow

### Adding New Series Types
1. Extend appropriate Qt Charts series class
2. Implement corresponding state class for property management
3. Add factory support in `SeriesStateFactory::createState()`
4. Update `updateSeriesColor()` utility if needed

### Configuration Extensions
1. Add new options to `DefaultConfig` in `chartview.h`
2. Extend `ChartConfigDialog` for UI controls
3. Implement configuration application in `updateChartConfig()`
4. Update JSON schema handling

### Scaling and Formatting
- Use `ChartTools` namespace utilities for consistent axis formatting
- Prefer "nice numbers" approaches for professional appearance
- Consider both linear and logarithmic scaling requirements

## Integration Notes

### SupraFit Integration
- Designed as external dependency in `/external/CuteChart/`
- Provides statistical visualization capabilities for supramolecular analysis
- Supports scientific data presentation with publication-quality output

### Qt Charts Compatibility
- Built against Qt Charts framework with custom enhancements
- Maintains compatibility with standard Qt Charts API
- Extends functionality without breaking Qt Charts conventions

## Common Customization Points

- **Themes**: Extend chart themes through Qt Charts theme system
- **Export Formats**: Add new export formats via export settings presets
- **Interaction**: Customize mouse/keyboard handling in `ChartViewPrivate`
- **Scaling**: Implement new auto-scale strategies following existing pattern
- **Series Appearance**: Extend series classes for new visual properties

## TODO: Code Restructuring & Refactoring Tasks

### 1. **ChartView Class Decomposition (Priority: High)**
- **Problem**: ChartView.cpp (1115 lines) - monolithic class with too many responsibilities
- **Solution**: Split into specialized classes:
  ```cpp
  ChartView (UI Management only)
  ChartConfiguration (Config handling) 
  ChartAxisManager (Axis operations)
  ChartExporter (Export functionality)
  ```
- **Benefits**: Better testability, clearer separation of concerns, easier maintenance

### 2. **Header Dependencies Reduction (Priority: High)**
- **Problem**: chartview.h includes too many headers, causing compilation bloat
- **Solution**: Implement PIMPL pattern and forward declarations
  ```cpp
  // chartview.h - only interface
  class ChartViewImpl;
  class ChartView {
      std::unique_ptr<ChartViewImpl> d;
  };
  ```
- **Benefits**: Faster compilation, reduced coupling, cleaner public interface

### 3. **Configuration System Simplification (Priority: Medium)**
- **Problem**: JSON configuration code scattered across multiple classes
- **Solution**: Centralized configuration management
  ```cpp
  class ChartSettings {
      static ChartSettings& instance();
      void load(const QJsonObject&);
      QJsonObject save() const;
      // Template-based type-safe accessors
  };
  ```
- **Benefits**: Single source of truth, type safety, easier testing

### 4. **Series Management Extraction (Priority: Medium)**
- **Problem**: Series management logic duplicated in ChartView and ListChart
- **Solution**: Dedicated series manager
  ```cpp
  class SeriesManager {
      void addSeries(QAbstractSeries*);
      void removeSeries(int index);
      void updateColors();
      void applyState(const SeriesState&);
  };
  ```
- **Benefits**: DRY principle, consistent behavior, easier extension

### 5. **Event Handling Centralization (Priority: Low)**
- **Problem**: Mouse/keyboard event handling scattered across classes
- **Solution**: Centralized interaction handler
  ```cpp
  class ChartInteractionHandler {
      void handleZoom(const ZoomEvent&);
      void handleSelection(const SelectEvent&);
      void registerStrategy(std::unique_ptr<InteractionStrategy>);
  };
  ```
- **Benefits**: Cleaner event flow, easier to add new interaction modes

### 6. **Unit Testing Infrastructure (Priority: Medium)**
- **Current State**: No visible test infrastructure
- **Needed**: 
  - CMake test target
  - Mock Qt Charts components
  - Test fixtures for complex scenarios
  - Integration tests for configuration persistence

### 7. **API Modernization (Priority: Low)**
- **Replace**: Raw pointers with smart pointers where appropriate
- **Add**: Move semantics for heavy operations
- **Consider**: std::optional for optional parameters
- **Modernize**: C++17 features (structured bindings, if constexpr)

## Refactoring Status (2025 — actual state)

The 2023 decomposition created three components but never wired them into the live
`ChartView`; the monolithic `chartview.cpp` remained the only real code path, and two
parallel dead files (`chartview_pimpl.*`, `chartview_refactored.h`) were left behind
(never in the build). The 2025 pass corrected this.

### ✅ Done
- **Axis-rescale "freeze" fixed (2026-07-14)** — `qtNiceNumbersScale()` never cleared `m_pending`
  (only `spaceScale()` did), so after any QtNiceNumbers pass the guarded `formatAxis()` latched into
  a permanent no-op and plots stopped rescaling until a new series forced `forceFormatAxis()`.
  `m_pending` is now cleared once at the end of `forceFormatAxis()` (strategy-independent).
- **Visibility API unified** — `LineSeries`/`ScatterSeries::showLine()` now both emit
  `visibilityChangeRequested(bool)`; the owning `ChartWrapper` (SupraFit) is the single
  authority that applies visibility. `ScatterSeries::visibilityChanged(int)` is retained
  only for ChartWidget's scatter mirroring (deprecation candidate).
- **ChartExporter integrated** — `ChartView::exportPNG()` delegates the high-resolution
  render (grid removal, axis emphasis, transparency, per-series export appearance, crop)
  to `ChartExporter::createPixmap()`, a faithful 1:1 port of the old inline logic.
  ChartView keeps only the widget orchestration (resize, callouts, vertical line, cursor).
- **ChartAxisManager integrated** — `ChartView::spaceScale()` / `qtNiceNumbersScale()`
  delegate to the manager, which adopts ChartView's default axes via `setAxes()`. Its
  nice-numbers path was aligned to the monolith (Qt `applyNiceNumbers()`), not `ChartTools`.
- **ChartConfiguration integrated** — now owns the config state (current/pending/last JSON
  via raw accessors, plus the export-preset store); ChartView delegates storage and keeps
  the dialog-driven Apply/Ignore/Revert application logic. Pure state relocation, no
  behaviour change.
- **Dead code removed** — `chartview_pimpl.{h,cpp}` and `chartview_refactored.h` deleted
  (the latter declared a second, conflicting `ChartView`).
- **Public API preserved** — no external break: `chart()`, `privateView()`,
  `getChartConfig()`, enums, series ctors/setters all unchanged.

### ⏳ Remaining
- **No unit tests / CMake test target** (see TODO #6 above).
- ChartConfiguration also exposes validate/merge/font/QSettings helpers that ChartView does
  not yet use (config *application* still lives in `chartview.cpp`); a future step could move
  the rich-axis-schema apply/revert protocol into the component too.

### Architecture (current)
`ChartView` = UI orchestration + config application, delegating to:
`ChartConfiguration` (config + export-preset state), `ChartAxisManager` (axis scaling),
`ChartExporter` (render/export), `ChartViewPrivate` (mouse/zoom/selection). Components are
held as `std::unique_ptr` members and adopt ChartView's chart/axes rather than creating
their own.