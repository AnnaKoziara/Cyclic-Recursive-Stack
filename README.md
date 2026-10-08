# Cyclic Recursive Stack

A C library implementing recursive stacks. Each stack can contain `uint64_t`
values and references to other stacks, so stacks can form nested or cyclic
graphs.

## Features

- Create and delete stacks; push and pop values or references to other stacks.
- Find the first reachable numeric value, searching from the top and traversing
  nested stacks (`rstack_front`).
- Check whether any numeric value is reachable (`rstack_empty`).
- Read and write stacks as decimal `uint64_t` values, one per line.
- Use reference counting and cycle-aware cleanup to reclaim unreachable cyclic
  graphs.

`rstack_empty` describes whether a numeric value is reachable, not whether the
stack has no direct nodes. `rstack_front` returns a `result_t`: check `flag`
before reading `value`.

## Build

The implementation uses `nullptr` and the `bool` type in `rstack.h`, so it
requires a C23 compiler:

```sh
cc -std=c23 -Wall -Wextra -Wpedantic -c rstack.c -o rstack.o
```

Include `rstack.h` in programs that use the library and link them with
`rstack.o`.

## API notes

- A newly created stack has one reference. Release owned references with
  `rstack_delete`.
- `rstack_push_rstack(parent, child)` adds a reference to `child`; popping that
  element releases it.
- Push functions return `0` on success and `-1` on failure.
- `rstack_new` and `rstack_read` return `NULL` on failure. Functions report
  error details through `errno` where applicable.
- `rstack_read` accepts unsigned decimal values separated by whitespace.
  `rstack_write` serializes reachable numeric values from the bottom of the
  stack toward the top.
