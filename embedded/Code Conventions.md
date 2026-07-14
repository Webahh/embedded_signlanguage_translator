# Code Conventions - STM32N6570DK FSBL

## HEADER (`.h`)

### File Header Block

```c
/**
 * @file    module.h
 * @author  Name
 * @date    2026-06-23
 * @brief   One-line module description
 *
 * Usage
 * -----
 * 1. Module_Init()          – initialise hardware
 * 2. Module_Config(...)     – apply configuration
 * 3. Module_Start()         – begin operation
 */
```

- `@author`, `@date`, and `@brief` are mandatory.
- The **Usage** section is mandatory and must list the functions that must be called before others, in the required order

### Guard

```c
#ifndef MODULE_H
#define MODULE_H
...
#endif /* MODULE_H */
```

### Includes

```c
#include <stdint.h> 
#include <stdlib.h>
#include <stddef.h>

#include "stm32n657xx.h"
#include "stm32n657xx.h"

#include "simple_gpio.h"
```

- First Standard library
- Then MCU header
- Lastly Own project first

### Defines / Macros

```c
#define PREFIX_NAME   value
```

- `UPPER_SNAKE_CASE`, prefixed by module: `LCD_BG_WIDTH`, `GPIO_MODE_OUTPUT`, `CAM_OK`
- Public defines used in other Modules

### Typedefs

- **Hardware/peripheral config structs:** suffix `_TypeDef`
  `typedef struct { ... } GPIO_cfg_TypeDef;`
  `typedef struct { ... } CAM_handle_TypeDef;`
  `typedef struct { ... } LCD_Layer_cfg_TypeDef;`
- **Enums:** prefixed with module, `UPPER_SNAKE_CASE`
  `typedef enum { CAM_OK = 0, CAM_ERROR = 1 } CAM_Status;`

### Functions

```c
/**
 * @brief Brief method description
 *
 * Elaborate explanation of method
 *
 * @param [in]  a | description [...]
 * @param [out] b | description [...]
 *
 * @retval Return description
 *
 * @note Essential things to note about the method
 */
ReturnType func(param a, [...], param b);
```

- Minimum documentation must contain `@brief` and `@param [in]` / `@param [out]`
- `@note` and elaborate explanation are optional
- No anonymous parameter types
- `@return` for non-void methods
- `@return` The return type of all module functions (except `void`) is the module's status `_TypeDef` enum (e.g. `SCHEDULER_Status_TypeDef`) Values are returned via pointer parameters - never via the return value
- Function names follow the pattern `Module_SubPascalCase_action`:
  `RCC_Timer_enable()`, `SCHEDULER_Task_add()`, `GPIO_Pin_set()`.
  Top-level module in `UPPER_CASE`, any submodule / sub-part in
  `PascalCase`, and the action in `lowercase`.  Interrupt Service Routines
  (e.g. `TIM7_IRQHandler`) are exempted.

### Extern Variables

Group by category, one per line:

```c
// -- Read-only config (defined in .c with designated initializers) --
extern const GPIO_cfg_TypeDef   GPIO_default_cfg;
extern const RCC_PLL_cfg_TypeDef BOARD_PLL_cfg[4];

// -- Mutable runtime handles / state --
extern CAM_handle_TypeDef    CAM_handle_cfg;
extern LCD_Layer_TypeDef     LCD_Layer1_cfg;
```

- **Config data** = `extern const` - defined in `.c` with `{ .member = val }`
- **Runtime state** = `extern` (no `const`) - defined in `.c` without `const`, modified in place

---

## SOURCE (`.c`)

### Includes (order)

- First - Standard library: `#include <string.h>`
- Second - Own header first: `#include "module.h"`
- Third - Project drivers: `#include "simple_gpio.h"`

### Defines / Macros

```c
#define PREFIX_NAME   value
```

- `UPPER_SNAKE_CASE`, prefixed by module: `LCD_BG_WIDTH`, `GPIO_MODE_OUTPUT`, `CAM_OK`
- Private defines used in own Module start with `_` and omit the module prefix: `_TIMEOUT`, `_FIFO_THRESHOLD`

### Static / File-scope

```c
static volatile int     _some_counter;
static const uint8_t    _lookup[3] = { 10, 20, 30 };
```

- `snake_case` for variable names
- Static variables have the prefix `_`
- Documentation style identical to HEADER without Special return type type

### Functions

```c
MODULE_return_TypeDef Module_Action(param_t param){
    [...]
    // body (tab-indented)
}
```

- Opening brace on same line
- One blank line between methods
- The return type of all module functions (except `void`) is the module's
  status `_TypeDef` enum (e.g. `SCHEDULER_Status_TypeDef`)
  Values are returned via pointer parameters - never via the return value
- Function names follow the pattern `Module_SubPascalCase_action`:
  `RCC_Timer_enable()`, `SCHEDULER_Task_add()`, `GPIO_Pin_set()`.
  Top-level module in `UPPER_CASE`, any submodule / sub-part in
  `PascalCase`, and the action in `lowercase`.  Interrupt Service Routines
  (e.g. `TIM7_IRQHandler`) are exempted.

### Config Initializers

Use designated initializers, one member per line, tab-indented:

```c
const GPIO_cfg_TypeDef GPIO_LTDC_cfg = {
    .mode   = GPIO_MODE_AF,
    .otyp   = GPIO_OTYPE_PP,
    .pupdr  = GPIO_PUPD_NONE,
    .af     = GPIO_AF_LTDC,
    .speed  = GPIO_SPEED_VERY_HIGH
};
```

- Align `.param` and multiple assignments

### Comments

- `//` for single-line / inline
- `/* */` or `/** */` for block / Doxygen
- Use `//` inline comments within complex methods to explain non-obvious steps, loop structure, or tricky logic
- Section functions:

```c
// -------------------------------------------------------------------------
// Sectionname
// -------------------------------------------------------------------------
```

### Indentation

- **Tabs** (no spaces) - align struct members and designated init values with tabs
