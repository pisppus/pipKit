# Debug Metrics Status Bar Mode - Implementation Tasks

## Task List

### 1. Refactor CPU Measurement to Time-Based Method
- [x] 1.1 Remove FreeRTOS-dependent code from Debug.cpp
  - Remove `_lastRuntimeCounter`, `_lastIdleTimeCore0`, `_lastIdleTimeCore1` static variables
  - Remove `getRuntimeCounter()` method
  - Remove FreeRTOS includes and idle task references
  - Remove CPU measurement code from `update()` that uses FreeRTOS tasks

- [x] 1.2 Add time-based measurement infrastructure
  - Add new static variables to Debug class:
    - `_lastUpdateTime` (uint32_t) - timestamp of last update
    - `_busyTimeAccum` (uint32_t) - accumulated busy time in microseconds
    - `_renderStartTime` (uint32_t) - timestamp when render started
    - `_isRendering` (bool) - flag indicating active render operation
  - Initialize these variables in `Debug::init()`

- [x] 1.3 Implement beginRender() and endRender() methods
  - Create `Debug::beginRender()`:
    - Check if debug is enabled
    - Record current time using `micros()`
    - Set `_isRendering` flag to true
  - Create `Debug::endRender()`:
    - Check if debug is enabled and `_isRendering` is true
    - Calculate elapsed time since `_renderStartTime`
    - Add elapsed time to `_busyTimeAccum`
    - Set `_isRendering` flag to false

- [x] 1.4 Update Debug::update() with new CPU calculation algorithm
  - Get current time using `micros()`
  - Calculate total elapsed time since `_lastUpdateTime`
  - If elapsed >= 100ms (100000 microseconds):
    - Calculate CPU% = (`_busyTimeAccum` * 100) / total_elapsed
    - Store result in `_metrics.cpuPercent`
    - Reset `_busyTimeAccum` to 0
    - Update `_lastUpdateTime` to current time
    - Update RAM metrics:
      - `_metrics.freeHeapTotal = esp_get_free_heap_size()`
      - `_metrics.freeHeapInternal = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL)`
      - `_metrics.largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)`
      - `_metrics.minFreeHeap = esp_get_minimum_free_heap_size()`

- [x] 1.5 Update DebugMetrics structure
  - Remove `cpuCore0Percent` and `cpuCore1Percent` fields
  - Add `cpuPercent` field (uint8_t, 0-100)
  - Add `freeHeapTotal` field (uint32_t) for total free heap
  - Keep existing `freeHeapInternal`, `largestFreeBlock`, `minFreeHeap` fields
  - Update constructor to initialize all fields to 0

- [x] 1.6 Update formatStatusBar() for extended format with multiple RAM metrics
  - Remove dual-core vs single-core conditional logic
  - Use extended format: `"CPU:xx% T:xxk D:xxk L:xxk M:xxk"`
  - Format CPU from `_metrics.cpuPercent`
  - Format Total from `_metrics.freeHeapTotal / 1024`
  - Format DRAM from `_metrics.freeHeapInternal / 1024`
  - Format Largest from `_metrics.largestFreeBlock / 1024`
  - Format Min from `_metrics.minFreeHeap / 1024`

### 2. Add Metrics Status Bar Mode
- [x] 2.1 Add metrics status bar mode flag
  - Add `_metricsStatusBarEnabled` static bool to Debug class
  - Initialize to false in Debug.cpp

- [x] 2.2 Implement enable/disable API
  - Create `Debug::setMetricsStatusBarEnabled(bool enabled)` method
  - Create `Debug::isMetricsStatusBarEnabled()` method returning bool
  - Add public declarations to Debug.hpp

- [x] 2.3 Update Debug.hpp header file
  - Add new method declarations
  - Update comments to describe time-based CPU measurement
  - Add usage examples in comments

### 3. Integrate with Status Bar Component
- [x] 3.1 Locate status bar rendering code
  - Find the file/method responsible for drawing status bar
  - Identify where status bar content is rendered
  - Document the integration points

- [x] 3.2 Add metrics mode check in status bar rendering
  - At the beginning of status bar content rendering
  - Add check: `if (Debug::isMetricsStatusBarEnabled())`
  - If true: render only metrics and return early
  - If false: continue with normal status bar content

- [x] 3.3 Implement metrics rendering in status bar
  - Create buffer for formatted metrics string (32 bytes)
  - Call `Debug::formatStatusBar(buffer, sizeof(buffer))`
  - Render the metrics text in status bar area
  - Use appropriate font, color, and alignment
  - Ensure text fits within status bar bounds

### 4. Add Render Timing Markers
- [x] 4.1 Find GUI render entry point
  - Locate the main render method in GUI class
  - Identify where rendering starts (before any draw operations)
  - Document the location

- [x] 4.2 Add beginRender() call
  - Add `Debug::beginRender()` at the start of render cycle
  - Place before any drawing operations
  - Ensure it's called every frame

- [x] 4.3 Find display flush completion point
  - Locate where display flush completes
  - Identify the point after all pixels are sent to display
  - Document the location

- [x] 4.4 Add endRender() call
  - Add `Debug::endRender()` after flush completes
  - Place after all display operations are done
  - Ensure it's called every frame

### 5. Testing and Validation
- [ ] 5.1 Test CPU measurement accuracy
  - Create test scenario with known render time
  - Verify CPU% calculation is correct
  - Test with different load levels (idle, medium, heavy)
  - Verify measurements are stable over time

- [ ] 5.2 Test status bar metrics display
  - Enable metrics status bar mode
  - Verify metrics are displayed correctly
  - Verify normal status bar content is hidden
  - Test with different screen sizes/orientations

- [ ] 5.3 Test RAM metrics
  - Verify free heap is displayed correctly
  - Create memory allocations and verify metrics update
  - Check for memory leaks in debug code itself

- [ ] 5.4 Test mode switching
  - Toggle metrics status bar mode on/off
  - Verify smooth transition between modes
  - Verify no visual glitches

- [ ] 5.5 Test compatibility with dirty rect overlay
  - Enable both debug modes simultaneously
  - Verify both work independently
  - Verify no conflicts or crashes

- [ ] 5.6 Performance testing
  - Measure overhead of beginRender/endRender calls
  - Verify FPS is not significantly affected
  - Check memory usage hasn't increased significantly
  - Test on different ESP32 variants (ESP32, ESP32-S3, etc.)

### 6. Documentation
- [x] 6.1 Update Debug.hpp comments
  - Document new time-based CPU measurement method
  - Add usage examples for new API methods
  - Document integration requirements

- [x] 6.2 Create usage example
  - Create example sketch showing how to enable metrics mode
  - Show how to integrate beginRender/endRender calls
  - Demonstrate interpreting the metrics

## Notes

- All tasks should maintain backward compatibility where possible
- Dirty rect overlay functionality must remain unchanged
- Code should compile without warnings
- Follow existing code style and conventions
- Test on real hardware (ESP32) before marking tasks complete

## Dependencies

- Task 2 depends on Task 1 (need CPU measurement working first)
- Task 3 depends on Task 2 (need API before integration)
- Task 4 depends on Task 1 (need beginRender/endRender implemented)
- Task 5 depends on Tasks 1-4 (need everything implemented before testing)
- Task 6 can be done in parallel with implementation

## Estimated Effort

- Task 1: 2-3 hours (core refactoring)
- Task 2: 1 hour (simple API addition)
- Task 3: 1-2 hours (integration work)
- Task 4: 1 hour (adding markers)
- Task 5: 2-3 hours (thorough testing)
- Task 6: 1 hour (documentation)

**Total: 8-11 hours**
