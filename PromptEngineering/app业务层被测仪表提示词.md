以下是补全后的完整 Codex 提示词，已按照 ESP-IDF 官方代码规范整合：

---

````markdown
# Task: Refactor app_ate_console.c — instrument manager entry stub

## Platform
- ESP-IDF: v5.5.3
- Chip: ESP32-P4
- Language: C (C17)

---

## Project architecture constraints

This project uses a strict three-layer design:

| Layer  | Prefix     | Responsibility                        |
|--------|------------|---------------------------------------|
| Driver | `driver_`  | Peripheral capability, raw I/O        |
| System | `system_`  | Communication services, device services |
| App    | `app_`     | Business semantics only               |

App layer rules:
- Call only System layer APIs
- Never call `uart_driver_install()`, `uart_write_bytes()`, `i2c_master_transmit()`, or any ESP-IDF peripheral API directly
- Never register `esp_event` handlers directly
- Never create FreeRTOS tasks directly
- Face "business objects + management responsibility" in naming, not communication medium

---

## ESP-IDF C code style (mandatory, from official style guide)

Apply all of the following rules when generating or modifying C code:

### Naming
- File-local variables and functions must be declared `static`
- Static variables must use `s_` prefix: `static bool s_initialized`
- Public names (non-static) must use component or module prefix to avoid conflicts:
  `app_instrument_mgr_start()`, `system_router_init()`
- Avoid unnecessary abbreviations unless the name would be very long

### Indentation
- 4 spaces per indent level
- Never use tabs

### Vertical spacing
- One blank line between functions
- No blank line at the beginning or end of a function body

### Line length
- Maximum 120 characters per line

### Horizontal spacing
- One space after control-flow keywords: `if (`, `for (`, `while (`, `switch (`
- No space after function names: `foo(a, b)`
- One space around binary operators
- No space around `.` and `->`

### Braces
- Function definitions: opening brace on its own line
- `if` / `for` / `while` bodies inside functions: opening brace on same line as statement

```c
// Correct function definition
static esp_err_t app_instrument_mgr_start(void)
{
    return ESP_OK;
}

// Correct if/for inside function
if (condition) {
    do_something();
}
```

### Comments
- Use `//` for single-line comments
- Use `/* */` for multi-line doc comments above functions
- Do not use single-line comments to disable code
- Do not add author/date comments — use git for that
- Do not leave commented-out dead code without explanation

### Error handling
- Use `ESP_RETURN_ON_ERROR` / `ESP_RETURN_ON_FALSE` from `esp_check.h`
- All functions that can fail must return `esp_err_t`
- Use `ESP_LOGE` / `ESP_LOGW` / `ESP_LOGI` for logging

### Line endings
- LF (Unix style) only

---

## Task scope (current stage only)

**Do only the following — nothing more:**

1. Rename `app_uart_xx()` (or equivalent) to `app_instrument_mgr_start()`
2. Update all call sites to use the new name and update the error string
3. Add a well-structured multi-line comment above the function
4. Keep the function as `static esp_err_t`, returning `ESP_OK`
5. Do NOT implement any of the following yet:
   - Multi-port UART scan
   - Baud rate negotiation
   - Hot-plug detection
   - Protocol TX/RX
   - FreeRTOS task creation
   - `esp_event` registration or posting
   - System layer calls (`system_instrument_service_*`)
   - Driver layer calls (`driver_uart_port_*`)

---

## Naming rationale (enforce strictly)

`app_instrument_mgr_start()` is correct because:
- `app_` → belongs to App layer
- `instrument` → the managed object is the downstream instrument under test, not a transport medium
- `mgr` → this is a multi-instance manager, not a single link action
- `start` → lifecycle entry point

**Forbidden names (do not use, do not suggest):**

| Forbidden name            | Why forbidden                                      |
|---------------------------|----------------------------------------------------|
| `app_uart_start`          | `uart` is a transport detail, not a business object |
| `app_uart_comm_start`     | Same issue                                         |
| `app_meter_start`         | `meter` is semantically narrower and less stable   |
| `app_instrument_start`    | Missing `mgr`, implies single-instance             |

---

## Required output

### Function definition

```c
/*
 * @brief Start the instrument manager business module.
 *
 * App-layer entry point for managing downstream instruments connected
 * to ESP32-P4. Up to 4 instrument links may be managed simultaneously
 * via UART channels.
 *
 * Current stage: placeholder stub only.
 * The following capabilities will be added in later iterations:
 *   - Port assignment and link initialization (via system layer)
 *   - Instrument online detection and identification
 *   - Protocol session management
 *   - Fault detection and reporting
 *
 * @note Do not add peripheral-layer calls (UART/I2C) directly here.
 *       All communication must go through system_instrument_service_*.
 *
 * @return ESP_OK  Always returns OK at current stub stage.
 */
static esp_err_t app_instrument_mgr_start(void)
{
    return ESP_OK;
}
```

### Call site

```c
ESP_RETURN_ON_ERROR(app_instrument_mgr_start(), TAG, "instrument manager start failed");
```

---

## Output requirements

- Output only the modified `app_ate_console.c` fragment directly related to this function
- Code must compile cleanly under ESP-IDF v5.5.3
- Naming must be consistent throughout the file
- Comments must be professional, concise, and architecture-aware
- Do not add unrelated implementations
- Do not output lengthy explanations
- Do not expand system or driver layer implementations

---

## Style enforcement summary

| Rule | Required value |
|------|----------------|
| Indent | 4 spaces, no tabs |
| Function brace | Own line |
| if/for brace | Same line as keyword |
| Static var prefix | `s_` |
| Module prefix | `app_` for this layer |
| Max line length | 120 characters |
| Error return | `esp_err_t` + `ESP_RETURN_ON_ERROR` |
| Comment style | `/* */` for doc blocks, `//` for inline |
| Line endings | LF only |
````

---

关于注释风格，参考了 ESP-IDF 官方 Doxygen 文档规范：使用 `@brief`、`@note`、`@return` 结构，与 ESP-IDF 头文件的 API 文档风格保持一致。 [[API 文档规范](https://docs.espressif.com/projects/esp-docs/en/latest/writing-documentation/writing-api-documentation.html); [ESP-IDF 风格指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/contribute/style-guide.html)]