/*
 *  mirror.c
 *  mmap4fun
 *
 *  Created by Jaeden Amero on 2023-07-15.
 *  Copyright 2023. SPDX-License-Identifier: Apache-2.0
 */

/* Map the same memory twice contiguously within a process's memory space. */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

/*
 * Goal: Map the same memory twice to different, contiguous virtual addresses
 * within the process's address space.
 *
 * One technique, which works on both Linux and FreeBSD, is to create a shared
 * memory object (referenced as fd) and mmap() the object twice at contiguous
 * addresses. One can use either shm_open() (POSIX) or memfd_create() (GNU) to
 * create the shared memory object. (FreeBSD provides memfd_create() since
 * FreeBSD 13.0.)
 */

int main(int argc, const char *argv[])
{
    const off_t offset = 0;
    size_t length = 8 * 4096; /* Assuming page length of 4 KiB */
    int prot = PROT_READ | PROT_WRITE;
    int mfd_flags = 0;

    int ret;
    int fd;
    char *base;
    char *buffer;
    char *mirror;
    const char *name = "/mmap4fun_mirror";

    /* Create a shared memory object which we can map twice within our virtual
     * address space. */
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd == -1)
    {
        err(EXIT_FAILURE, "%s: Failed to create shared memory object",
            __func__);
    }

    /* Set the size of the shared memory object. */
    ret = ftruncate(fd, length);
    if (ret < 0)
    {
        err(EXIT_FAILURE, "%s: Failed to set size of shared memory object",
            __func__);
    }

    /* Map the entire destination virtual address space, as without this, the
     * two mirror mappings can remove existing mappings causing memory
     * corruption. By pre-allocating the virtual address space we'll need for
     * the two mirror mappings (with inaccessible pages as we don't need to use
     * the mapping), we can safely make the two mirror mappings as they'll
     * remove the unneeded reservation instead of something critical. We are
     * also guaranteed to have contiguous virtual address space mappings. */
    base =
        mmap(NULL, length * 2, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, offset);
    if (base == MAP_FAILED)
    {
        err(EXIT_FAILURE, "%s: Failed to map virtual memory", __func__);
    }

    buffer = mmap(&base[0], length, prot, MAP_FIXED | MAP_SHARED, fd, offset);
    if (buffer != &base[0])
    {
        err(EXIT_FAILURE, "%s: Failed to map buffer", __func__);
    }

    mirror = mmap(&base[length], length, prot, MAP_FIXED | MAP_SHARED, fd,
                  offset);
    if (mirror != &base[length])
    {
        err(EXIT_FAILURE, "%s: Failed to map contiguous mirror", __func__);
    }

    /* We are done mapping memory, so we can remove the shared memory object.
     * The mappings will remain valid until the they are unmapped or the
     * process exits. */
    ret = shm_unlink(name);
    if (ret < 0)
    {
        err(EXIT_FAILURE, "%s: Failed to unlink shared memory object",
            __func__);
    }

    int i = 2;
    printf("base: %p buffer: %p mirror: %p\n", base, buffer, mirror);
    printf("length: %lu (0x%08lx)\n", length, length);
    printf(" .. initial state ..\n");
    printf("-buffer[%d]: %c (0x%02x)\n", i, buffer[i], buffer[i]);
    printf("-mirror[%d]: %c (0x%02x)\n", i, buffer[i], buffer[i]);
    printf(" .. write to buffer ..\n");
    buffer[i] = 'A';
    printf(" buffer[%d]: %c (0x%02x)\n", i, buffer[i], buffer[i]);
    printf(" mirror[%d]: %c (0x%02x)\n", i, mirror[i], mirror[i]);
    printf(" .. write to mirror ..\n");
    mirror[i] = 'M';
    printf(" buffer[%d]: %c (0x%02x)\n", i, buffer[i], buffer[i]);
    printf(" mirror[%d]: %c (0x%02x)\n", i, mirror[i], mirror[i]);

    /* With a single munmap(), we can unmap both the mirror and buffer by
     * unmapping their entire combined address space. */
    munmap(base, length * 2);
}
