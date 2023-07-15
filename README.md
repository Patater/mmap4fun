# mmap4fun

Fun things you can do with `mmap()`.


### Mirroring

With `mmap()`, you can map the same memory twice to two different locations in
your processes address space. If you don't care which addresses you use to
access the memory, the approach is trivial: just `mmap()` some shared memory
twice. However, it's non-obvious how one needs to map the access regions
contiguously.

First, we need to understand that using `MAP_FIXED` will evict whatever other
mapping was at the address before. This can be catastrophic and cause your
program to crash, depending on what gets evicted.

The preferred approach is to map a non-fixed region of memory of the size you
want, enough to cover both contiguous mirror regions, and then `mmap()` two
fixed contiguous regions within this space. This way, when we map to this
location again with `MAP_FIXED`, only the freshly mapped and unused region will
be evicted, without harm.

```
make mirror
./mirror
```
