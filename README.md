# rstack

A C23 library implementing recursive stacks. Elements can be either 64-bit unsigned integers (`uint64_t`) or references to other stacks, allowing the construction of arbitrary nested and cyclic directed graphs.

Memory management uses reference counting combined with cycle detection to identify and reclaim isolated subgraphs without leaks.

## Key Concepts

- **Value-oriented reachability**: `rstack_empty` and `rstack_front` operate on reachable numeric values rather than raw node counts:
  - `rstack_empty` returns `true` if no numeric values are reachable (a stack containing only empty stacks or cycles of empty stacks is considered empty).
  - `rstack_front` searches depth-first from the top of the stack, descending into child stacks to find the first reachable number. It returns a `result_t { bool flag; uint64_t value; }` — `flag` must be checked before reading `value`.
- **Cycle-safe traversal**: Recursive searches and serialization track visited states to prevent infinite loops when cycles are present.
- **Serialization**:
  - `rstack_write`: Recursively serializes reachable numbers from the bottom of the stack toward the top, writing one decimal value per line.
  - `rstack_read`: Parses whitespace-separated decimal integers from a file into a newly allocated stack. Rejects malformed numbers (leading zeros, overflow beyond `UINT64_MAX`).

## Build

The implementation requires a C23-compliant compiler (uses `nullptr` and standard `bool`):

```sh
cc -std=c23 -Wall -Wextra -Wpedantic -c rstack.c -o rstack.o
