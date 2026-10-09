# rstack

A C23 library implementing **recursive stacks** — a stack data structure whose elements can be either 64-bit unsigned integers (`uint64_t`) or references to other stacks, enabling the construction of arbitrary nested and cyclic directed graphs.

Memory management uses **reference counting** combined with **cycle detection** (inspired by the trial deletion algorithm) to identify and reclaim isolated subgraphs without leaks.

## Features

- **Heterogeneous elements** — push numeric values or entire stacks onto a stack
- **Cycle-safe memory management** — reference counting with automatic detection and collection of isolated cyclic subgraphs
- **Value-oriented reachability** — `rstack_empty` and `rstack_front` traverse the graph depth-first, operating on reachable numeric values rather than raw node counts
- **File I/O** — serialize numeric values to a file and deserialize them back into a new stack
- **Robust error handling** — all functions report errors via return values and `errno`

## Build

The implementation requires a **C23-compliant compiler** (uses `nullptr` and standard `bool`):

```sh
cc -std=c23 -Wall -Wextra -Wpedantic -c rstack.c -o rstack.o
```

To build as a shared library:

```sh
cc -std=c23 -shared -fPIC -o librstack.so rstack.c    # Linux / macOS
```

## API Reference

### Types

```c
typedef struct rstack rstack_t;

typedef struct {
    bool     flag;   // true if value is valid
    uint64_t value;  // the actual result (only meaningful when flag == true)
} result_t;
```

### Functions

| Function | Signature | Description |
|---|---|---|
| `rstack_new` | `rstack_t* rstack_new()` | Creates a new empty stack (ref count = 1). Returns `nullptr` on allocation failure. |
| `rstack_delete` | `void rstack_delete(rstack_t *rs)` | Decrements the reference count. Frees the stack when it reaches 0, or triggers cycle detection to collect isolated subgraphs. |
| `rstack_push_value` | `int rstack_push_value(rstack_t *rs, uint64_t value)` | Pushes a numeric value onto the top of `rs`. Returns `0` on success, `-1` on error. |
| `rstack_push_rstack` | `int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2)` | Pushes a reference to `rs2` onto `rs1`, incrementing `rs2`'s ref count. Returns `0` on success, `-1` on error. |
| `rstack_pop` | `void rstack_pop(rstack_t *rs)` | Removes the top element. If it was a stack reference, `rstack_delete` is called on it. |
| `rstack_empty` | `bool rstack_empty(rstack_t *rs)` | Returns `true` if no numeric values are reachable (a stack containing only empty stacks or cycles of empty stacks is considered empty). |
| `rstack_front` | `result_t rstack_front(rstack_t *rs)` | Depth-first search from the top for the first reachable numeric value. Returns `{true, value}` on success, `{false, 0}` otherwise. |
| `rstack_read` | `rstack_t* rstack_read(char const *path)` | Reads whitespace-separated decimal integers from a file into a new stack. Rejects leading zeros and values exceeding `UINT64_MAX`. Returns `nullptr` on error. |
| `rstack_write` | `int rstack_write(char const *path, rstack_t *rs)` | Writes all reachable numeric values to a file (bottom-to-top order, one per line). Stops at cycles. Returns `0` on success, `-1` on error. |

## Usage Example

```c
#include "rstack.h"
#include <stdio.h>
#include <inttypes.h>

int main(void) {
    // Create two stacks
    rstack_t *a = rstack_new();
    rstack_t *b = rstack_new();

    // Push values onto stack b
    rstack_push_value(b, 10);
    rstack_push_value(b, 20);

    // Push stack b as an element of stack a, then push a value on top
    rstack_push_rstack(a, b);
    rstack_push_value(a, 42);

    // Front returns the first reachable numeric value (42)
    result_t r = rstack_front(a);
    if (r.flag) {
        printf("front = %" PRIu64 "\n", r.value);  // 42
    }

    // Serialize to file
    rstack_write("output.txt", a);

    // Clean up — cycle detection handles the mutual references
    rstack_delete(a);
    rstack_delete(b);

    // Read back from file
    rstack_t *loaded = rstack_read("output.txt");
    if (loaded) {
        while (!rstack_empty(loaded)) {
            result_t v = rstack_front(loaded);
            if (v.flag) printf("%" PRIu64 "\n", v.value);
            rstack_pop(loaded);
        }
        rstack_delete(loaded);
    }

    return 0;
}
```

## Key Concepts

### Reference Counting

- A newly created stack starts with a reference count of **1**.
- `rstack_push_rstack` **increments** the pushed stack's reference count.
- `rstack_pop` and `rstack_delete` **decrement** the reference count.
- When the count reaches **0**, the stack and all its nodes are freed.

### Cycle Detection (Trial Deletion)

When `rstack_delete` is called on a stack that still has references but contains stack-type elements, the library runs a three-phase algorithm:

1. **Mark Grey** — recursively decrement reference counts of all reachable stacks (trial deletion)
2. **Search for isolated cycles** — if a stack's count dropped to 0, it belongs to an isolated subgraph (mark white); otherwise, restore it (revert to black)
3. **Delete white stacks** — free all stacks marked white (the isolated cycle)

This ensures that cyclic graphs like `A → B → A` are properly collected when no external references remain.

### Value-Oriented Reachability

`rstack_front` and `rstack_empty` perform a depth-first search through nested stacks to find numeric values. Visited stacks are colored to prevent infinite loops in cyclic graphs:

- **Grey** — currently being visited (cycle detected, skip)
- **White** — already searched, contains no values (skip)
- **Black** — default state (search this stack)

## Project Structure

```
.
├── rstack.h      # Public API header
├── rstack.c      # Implementation (601 lines)
└── README.md     # This file
```

## Error Handling

All functions set `errno` on failure:

| Error | Condition |
|---|---|
| `EINVAL` | `nullptr` argument passed to a function |
| `ENOMEM` | Memory allocation failed |

Functions returning `int` use `0` for success and `-1` for failure.  
`rstack_new` and `rstack_read` return `nullptr` on failure.

## License

This project does not currently specify a license.

## Author

**Anna Koziara**  
ak479522@students.mimuw.edu.pl
