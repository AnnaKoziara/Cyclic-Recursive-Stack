# Cyclic Recursive Stack

A C implementation of recursive stacks that can contain either `uint64_t`
values or references to other stacks. This allows stacks to form cyclic
graphs.

The implementation uses reference counting: creating a stack starts its
reference count at one, pushing a stack reference increments its count, and
popping a reference decrements it. Unreachable cyclic stack graphs are handled
during cleanup to prevent memory leaks.

The source also provides operations for creating stacks, pushing and popping
elements, checking whether a stack is empty, accessing and searching values,
counting cycles recursively, and reading or writing stack values to files.
