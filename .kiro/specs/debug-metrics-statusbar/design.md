# Debug Metrics Status Bar Mode - Design Document

## Overview
Реализация режима отладки для pipGUI, который отображает метрики производительности (CPU и RAM) в статус-баре. Ключевое отличие от текущей реализации - использование time-based метода измерения CPU вместо FreeRTOS idle tasks, что обеспечивает независимость от RTOS.

## Architecture

### Component Structure
```
lib/pipSystem/pipGUI/core/
├── Debug.hpp          # Публичный API класса Debug
├── Debug.cpp          # Реализация измерения метрик
└── api/
    └── parts/
        └── StatusBar.inc  # Интеграция с статус-баром
```

### Class Diagram
```
┌─────────────────────────────────────┐
│           Debug (static)            │
├─────────────────────────────────────┤
│ - _metrics: DebugMetrics            │
│ - _enabled: bool                    │
│ - _metricsStatusBarEnabled: bool    │
│ - _lastUpdateTime: uint32_t         │
│ - _busyTimeAccum: uint32_t          │
│ - _renderStartTime: uint32_t        │
│ - _isRendering: bool                │
├─────────────────────────────────────┤
│ + init()                            │
│ + update()                          │
│ + beginRender()                     │
│ + endRender()                       │
│ + setMetricsStatusBarEnabled(bool)  │
│ + isMetricsStatusBarEnabled(): bool │
│ + formatStatusBar(char*, size_t)    │
│ + metrics(): DebugMetrics&          │
└─────────────────────────────────────┘
```

## Detailed Design

### 1. CPU Measurement Algorithm (Time-Based)

#### Принцип работы
Вместо измерения idle time через FreeRTOS, измеряем фактическое время работы библиотеки:

```
CPU% = (busy_time / total_time) * 100

где:
- busy_time = сумма времени всех render/flush операций за период
- total_time = время между обновлениями метрик (100ms)
```

#### Точки измерения
1. **beginRender()** - вызывается в начале render операции
2. **endRender()** - вызывается после завершения flush операции

#### Алгоритм
```cpp
// При начале render
void Debug::beginRender() {
    if (!_enabled) return;
    _renderStartTime = micros();
    _isRendering = true;
}

// После завершения flush
void Debug::endRender() {
    if (!_enabled || !_isRendering) return;
    uint32_t elapsed = micros() - _renderStartTime;
    _busyTimeAccum += elapsed;
    _isRendering = false;
}

// Каждые 100ms в update()
void Debug::update() {
    uint32_t now = micros();
    uint32_t totalElapsed = now - _lastUpdateTime;
    
    if (totalElapsed >= 100000) { // 100ms
        // Вычисляем CPU%
        _metrics.cpuPercent = (_busyTimeAccum * 100) / totalElapsed;
        
        // Сброс аккумулятора
        _busyTimeAccum = 0;
        _lastUpdateTime = now;
        
        // Обновление RAM метрик
        updateRamMetrics();
    }
}
```

### 2. RAM Metrics

#### Выбранные метрики
Для отслеживания утечек памяти и фрагментации используем несколько метрик:

1. **Total Free Heap** - общая свободная память (DRAM + IRAM + PSRAM если есть)
   ```cpp
   esp_get_free_heap_size()
   ```

2. **Free Internal DRAM** - свободная внутренняя память (самое важное)
   ```cpp
   heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL)
   ```

3. **Largest Free Block** - самый большой непрерывный блок (индикатор фрагментации)
   ```cpp
   heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
   ```

4. **Minimum Free Heap** - минимум за всё время работы (для выявления утечек)
   ```cpp
   esp_get_minimum_free_heap_size()
   ```

#### Отображение
В статус-баре показываем несколько метрик в компактном формате:
```
"CPU:45% T:156k D:128k L:64k M:120k"

где:
- CPU:xx% - загрузка процессора
- T:xxk - Total free heap (общая свободная память)
- D:xxk - DRAM free (внутренняя память)
- L:xxk - Largest block (самый большой блок)
- M:xxk - Min free (минимум за всё время)
```

Все метрики также доступны через API `Debug::metrics()` для логирования или анализа.

### 3. Status Bar Integration

#### Режим Debug Metrics
Когда `_metricsStatusBarEnabled == true`:
- Статус-бар отображает только метрики
- Все остальные элементы (иконки, батарея, время) скрыты
- Остаётся только фон статус-бара и текст с метриками

#### Интеграция в StatusBar
```cpp
// В методе отрисовки статус-бара
void GUI::drawStatusBar() {
    // Рисуем фон статус-бара
    drawStatusBarBackground();
    
    // Проверяем режим debug metrics
    if (Debug::isMetricsStatusBarEnabled()) {
        char metricsText[32];
        Debug::formatStatusBar(metricsText, sizeof(metricsText));
        
        // Отрисовываем только метрики
        drawText(metricsText, ...);
        return; // Пропускаем обычное содержимое
    }
    
    // Обычное содержимое статус-бара
    drawStatusBarIcons();
    drawBattery();
    // ...
}
```

### 4. Data Structures

#### DebugMetrics (обновлённая)
```cpp
struct DebugMetrics
{
    // CPU metrics
    uint8_t cpuPercent;           // 0-100%
    
    // RAM metrics (in bytes)
    uint32_t freeHeapTotal;       // Общая свободная память
    uint32_t freeHeapInternal;    // Свободная DRAM
    uint32_t largestFreeBlock;    // Самый большой блок
    uint32_t minFreeHeap;         // Минимум за всё время
    
    DebugMetrics() : cpuPercent(0),
                     freeHeapTotal(0),
                     freeHeapInternal(0), 
                     largestFreeBlock(0), 
                     minFreeHeap(0) {}
};
```

#### Debug Class (обновлённые члены)
```cpp
class Debug
{
private:
    // Существующие
    static DebugMetrics _metrics;
    static bool _enabled;
    
    // Новые для time-based CPU measurement
    static uint32_t _lastUpdateTime;      // Время последнего update()
    static uint32_t _busyTimeAccum;       // Аккумулятор busy time (микросекунды)
    static uint32_t _renderStartTime;     // Время начала render
    static bool _isRendering;             // Флаг активного render
    
    // Новые для metrics status bar
    static bool _metricsStatusBarEnabled;
    
    // Удаляем (больше не нужны для FreeRTOS метода)
    // static uint32_t _lastRuntimeCounter;
    // static uint32_t _lastIdleTimeCore0;
    // static uint32_t _lastIdleTimeCore1;
    
    // Dirty rect (без изменений)
    static bool _dirtyRectEnabled;
    static uint16_t _dirtyRectColor;
    static uint16_t _dirtyRectActiveColor;
    static DirtyRect _rects[8];
    static uint8_t _rectCount;
};
```

### 5. API Changes

#### Новые методы
```cpp
// Включить/выключить режим metrics в статус-баре
static void setMetricsStatusBarEnabled(bool enabled);
static bool isMetricsStatusBarEnabled();

// Маркеры начала/конца render операций
static void beginRender();
static void endRender();
```

#### Изменённые методы
```cpp
// update() - теперь использует time-based алгоритм
static void update();

// formatStatusBar() - расширенный формат с несколькими метриками памяти
static void formatStatusBar(char* out, size_t len);
```

#### Удалённые методы
```cpp
// Больше не нужны
// static uint32_t getRuntimeCounter();
```

### 6. Integration Points

#### В GUI::render()
```cpp
void GUI::render() {
    Debug::beginRender();
    
    // Существующий код отрисовки
    renderScreen();
    flushDisplay();
    
    Debug::endRender();
}
```

#### В GUI::loop() или main loop
```cpp
void loop() {
    // Периодическое обновление метрик
    Debug::update();
    
    // Остальная логика
    gui.loop();
}
```

## Implementation Plan

### Phase 1: Refactor CPU Measurement
1. Удалить FreeRTOS-зависимый код из Debug.cpp
2. Добавить новые статические переменные для time-based метода
3. Реализовать `beginRender()` и `endRender()`
4. Обновить `update()` с новым алгоритмом
5. Обновить `formatStatusBar()` для упрощённого формата

### Phase 2: Add Metrics Status Bar Mode
1. Добавить `_metricsStatusBarEnabled` флаг
2. Реализовать `setMetricsStatusBarEnabled()` и `isMetricsStatusBarEnabled()`
3. Обновить структуру `DebugMetrics` (убрать cpuCore0/Core1, добавить cpuPercent)

### Phase 3: Integrate with Status Bar
1. Найти код отрисовки статус-бара в GUI
2. Добавить проверку `Debug::isMetricsStatusBarEnabled()`
3. Реализовать условную отрисовку (метрики vs обычное содержимое)

### Phase 4: Add Render Markers
1. Найти точки начала render в GUI
2. Добавить вызовы `Debug::beginRender()`
3. Найти точки окончания flush
4. Добавить вызовы `Debug::endRender()`

### Phase 5: Testing & Validation
1. Проверить корректность измерения CPU%
2. Проверить отображение в статус-баре
3. Проверить совместимость с dirty rect overlay
4. Проверить производительность

## Performance Considerations

### CPU Overhead
- `beginRender()` / `endRender()`: ~2-3 микросекунды (вызов micros() + арифметика)
- `update()`: ~50-100 микросекунд каждые 100ms (heap queries + форматирование)
- Общий overhead: < 0.1% CPU

### Memory Overhead
- Новые статические переменные: ~12 bytes
- Без динамической аллокации
- Минимальное влияние на heap

### Accuracy
- Точность измерения времени: 1 микросекунда (micros())
- Период обновления: 100ms
- Погрешность CPU%: ±1%

## Compatibility

### Platform Support
- ✅ ESP32 (Arduino framework)
- ✅ ESP32-S2, ESP32-S3, ESP32-C3
- ✅ Любой микроконтроллер с функцией micros() или аналогом
- ✅ Не зависит от FreeRTOS

### Backward Compatibility
- Dirty rect overlay продолжает работать независимо
- Существующий API `Debug::metrics()` сохраняется
- Изменения в структуре `DebugMetrics` могут потребовать обновления кода, использующего cpuCore0Percent/cpuCore1Percent

## Testing Strategy

### Unit Tests
1. Проверка корректности вычисления CPU%
   - Симуляция render операций известной длительности
   - Проверка результата вычисления

2. Проверка форматирования строки
   - Различные значения метрик
   - Граничные случаи (0%, 100%, большие значения RAM)

### Integration Tests
1. Проверка интеграции с статус-баром
   - Включение/выключение режима
   - Корректное отображение метрик
   - Скрытие обычного содержимого

2. Проверка совместимости с dirty rect
   - Оба режима включены одновременно
   - Независимое включение/выключение

### Performance Tests
1. Измерение overhead от вызовов beginRender/endRender
2. Проверка влияния на FPS
3. Проверка использования памяти

## Documentation

### API Documentation
Обновить комментарии в Debug.hpp:
- Описание time-based метода измерения CPU
- Примеры использования новых методов
- Рекомендации по интеграции

### User Guide
Добавить в документацию pipGUI:
- Как включить режим debug metrics
- Интерпретация метрик
- Советы по оптимизации на основе метрик

## Future Enhancements (Out of Scope)

1. Графическое отображение метрик (графики CPU/RAM)
2. Логирование метрик в файл или по сети
3. Настраиваемый формат отображения
4. Дополнительные метрики (FPS, render time, flush time)
5. Алерты при превышении порогов (CPU > 90%, RAM < 10k)

## References

- [LVGL System Monitor Documentation](https://docs.lvgl.io/master/debugging/sysmon.html) - Inspiration for time-based CPU measurement
- ESP-IDF Heap Memory Debugging - Documentation for heap_caps functions
- Arduino micros() function - High-resolution timer reference

---

**Content was rephrased for compliance with licensing restrictions**
