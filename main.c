// Copyright C 2024 Marcus Lian Hanestad <marlhan@proton.me> ALL RIGHTS RESERVED

// Simple heap allocator
//
// Flaws:
//   - Assumes exclusive access to program break
//   - Does not actually free memory by giving it back to kernel
//   - Not thread safe

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

static char *alloc124 = NULL;

static void write_size_t(char *to, size_t n) {
  *(size_t *)to= n;
}

static size_t read_size_t(char *from) {
  return *(size_t *)from;
}

static void *malloc124(size_t size) {
  if (alloc124 == NULL) {
    alloc124 = sbrk((intptr_t)size + 8 + 8 + 1);
    if (alloc124== (void *)-1) {
        perror("sbrk()");
        return alloc124 = NULL;
    }
    write_size_t(alloc124, 8 + 8 + 1 + size);
    alloc124[8] = 0;
    write_size_t(alloc124 + 1 + 8, size);
  }

  char *program_break = sbrk(0);
  if ((void *)program_break == (void *)-1) {
    perror("sbrk()");
    return NULL;
  }

  size_t total_size = read_size_t(alloc124);

  size_t offset = 8;
  // Search through all allocated regions to find any free space
  while (offset < total_size) {
    size_t region_size = read_size_t(alloc124 + offset + 1);
    // Check if region is in use or have enough space for this alloc
    if (alloc124[offset] || region_size < size) {
      printf("In use region, size: %ld\n", region_size);
      offset += 1 + 8 + region_size;
      continue;
    }

    // We have found a free region that fits the required amount of bytes for this allocation
    printf("Found free region for size: %ld actually: %ld\n", size, region_size);
    alloc124[offset] = 1;
    memset(alloc124 + offset + 1 + 8, 0, region_size);
    return alloc124 + offset + 1 + 8;
  }

  // We need to allocate more heat as we could not find free space
  if (sbrk(1 + 8 + size) == (void *)-1) {
    perror("sbrk()");
    return NULL;
  }

  printf("Allocating new region of size: %ld\n", size);
  *(alloc124 + total_size) = 1;
  write_size_t(alloc124 + total_size + 1, size);
  memset(alloc124 + total_size + 1 + 8, 0, size);
  write_size_t(alloc124, total_size + size + 1 + 8);

  return (void *)(alloc124 + total_size + 8 + 1);
}

static void free124(void *ptr) {
  char *cptr = ptr;
  *(cptr - 9) = 0;
  printf("Freed region of size: %ld\n", read_size_t(cptr - 8));
}

int main() {
  assert(sizeof(size_t) == 8 && "Only 64 bit systems supported");

  char *mystr = malloc124(5);
  printf("total size: %ld. should be: %d\n", read_size_t(alloc124), 8 + 1 + 8 + 5);
  free124(mystr);

  char *mystr2 = malloc124(5);
  char *mystr3 = malloc124(5);
  printf("total size: %ld. should be: %d\n", read_size_t(alloc124), 8 + 1 + 8 + 5 + 8 + 1 + 5);

  char *mystr4 = malloc124(5);
  char *mystr5 = malloc124(1024);
  free124(mystr5);
  char *mystr6 = malloc124(1024);

  free124(mystr2);
  free124(mystr3);
  free124(mystr4);
  free124(mystr6);

  return 0;
}
