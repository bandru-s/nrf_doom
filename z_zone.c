// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2023-2026 by Frenkel Smeijers
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "compiler.h"
#include "doomdef.h"
#include "i_system.h"
#include <stdint.h>
#include <stdlib.h>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC 1  // static entire execution time
#define PU_LEVEL 2   // static until level exited
#define PU_LEVSPEC 3 // a special thinker in a level
#define PU_CACHE 4

#define PU_PURGELEVEL PU_CACHE

//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
//

#if defined INSTRUMENTED
static int32_t running_count = 0;
#endif

#define ZONEID 0x1dea

typedef struct
{
#if SIZE_OF_SEGMENT_T == 2
  uint32_t size; // including the header and possibly tiny fragments
  uint16_t tag;  // purgelevel
#else
  uint32_t size : 24; // including the header and possibly tiny fragments
  uint32_t tag : 4;   // purgelevel
#endif
  void __far *__far *user; // NULL if a free block
  segment_t next;
  segment_t prev;
#if defined ZONEIDCHECK
  uint16_t id; // should be ZONEID
#endif
} memblock_t;

typedef char assertMemblockSize[sizeof(memblock_t) == 20 ? 1 : -1];

static memblock_t __far *mainzone_sentinal;
static segment_t mainzone_rover_segment;

static segment_t pointerToSegment(const memblock_t __far *ptr) {
#if defined RANGECHECK
  if ((((uint32_t)ptr) & 3) != 0)
    I_Error("pointerToSegment: pointer is not aligned: 0x%lx", ptr);
#endif

  return D_FP_SEG(ptr);
}

static memblock_t __far *segmentToPointer(segment_t seg) {
  return D_MK_FP(seg, 0);
}

boolean Z_EqualNames(const char __far *farName, const char *nearName) {
  return _fmemcmp(farName, nearName, 8) == 0;
}

//
// Z_Init
//
void Z_Init(void) {
  // allocate all available conventional memory.
  uint32_t heapSize;
  static uint8_t __far *mainzone;
  mainzone = I_ZoneBase(&heapSize);

  // align blocklist
  uint_fast8_t i = 0;
  static uint8_t __far mainzone_sentinal_buffer[PARAGRAPH_SIZE * 2];
  uint32_t b = (uint32_t)&mainzone_sentinal_buffer[i++];
  while ((b & (PARAGRAPH_SIZE - 1)) != 0)
    b = (uint32_t)&mainzone_sentinal_buffer[i++];
  mainzone_sentinal = (memblock_t __far *)b;

#if defined __WATCOMC__ && defined _M_I86
  // normalize pointer
  mainzone_sentinal = D_MK_FP(D_FP_SEG(mainzone_sentinal) + D_FP_OFF(mainzone_sentinal) / PARAGRAPH_SIZE, 0);
#endif

  // set the entire zone to one free block
  memblock_t __far *block = (memblock_t __far *)mainzone;
  mainzone_rover_segment = pointerToSegment(block);

  mainzone_sentinal->tag = PU_STATIC;
  mainzone_sentinal->user = (void __far *)mainzone;
  mainzone_sentinal->next = mainzone_rover_segment;
  mainzone_sentinal->prev = mainzone_rover_segment;

  block->size = heapSize;
  block->tag = 0;
  block->user = NULL; // NULL indicates a free block.
  block->prev = pointerToSegment(mainzone_sentinal);
  block->next = block->prev;
#if defined ZONEIDCHECK
  block->id = ZONEID;
#endif

  printf("%ld bytes allocated for zone\n", heapSize);
}

static void Z_FreeBlock(memblock_t __far *block) {
#if defined ZONEIDCHECK
  if (block->id != ZONEID)
    I_Error("Z_FreeBlock: block has id %x instead of ZONEID", block->id);
#endif

    if (block->user && (uint32_t)block->user != 1) {
    // clear the user's mark
    *block->user = NULL;
  }

  // mark as free
  block->user = NULL;
  block->tag = 0;

#if defined INSTRUMENTED
  running_count -= block->size;
  printf("Free: %ld\n", running_count);
#endif

  memblock_t __far *other = segmentToPointer(block->prev);

  if (!other->user) {
    // merge with previous free block
    other->size += block->size;
    other->next = block->next;
    segmentToPointer(other->next)->prev = block->prev; // == pointerToSegment(other);

    if (pointerToSegment(block) == mainzone_rover_segment)
      mainzone_rover_segment = block->prev; // == pointerToSegment(other);

    block = other;
  }

  other = segmentToPointer(block->next);
  if (!other->user) {
    // merge the next free block onto the end
    block->size += other->size;
    block->next = other->next;
    segmentToPointer(block->next)->prev = pointerToSegment(block);

    if (pointerToSegment(other) == mainzone_rover_segment)
      mainzone_rover_segment = pointerToSegment(block);
  }
}

//
// Z_Free
//
void Z_Free(const void __far *ptr) {
#if defined RANGECHECK
  if ((((uint32_t)ptr) & 3) != 0)
    I_Error("pointerToSegment: pointer is not aligned: 0x%lx", ptr);
#endif

#if defined _M_I86
  memblock_t __far *block = (memblock_t __far *)(((uint32_t)ptr) - 0x00010000);
#else
  memblock_t __far *block = (memblock_t __far *)(((uint32_t)ptr) - sizeof(memblock_t));
#endif

  Z_FreeBlock(block);
}

static uint32_t Z_GetLargestFreeBlockSize(void) {
  uint32_t largestFreeBlockSize = 0;

  segment_t mainzone_sentinal_segment = pointerToSegment(mainzone_sentinal);

  for (memblock_t __far *block = segmentToPointer(mainzone_sentinal->next); pointerToSegment(block) != mainzone_sentinal_segment; block = segmentToPointer(block->next))
    if (!block->user && block->size > largestFreeBlockSize)
      largestFreeBlockSize = block->size;

  return largestFreeBlockSize;
}

static uint32_t Z_GetTotalFreeMemory(void) {
  uint32_t totalFreeMemory = 0;

  segment_t mainzone_sentinal_segment = pointerToSegment(mainzone_sentinal);

  for (memblock_t __far *block = segmentToPointer(mainzone_sentinal->next); pointerToSegment(block) != mainzone_sentinal_segment; block = segmentToPointer(block->next))
    if (!block->user)
      totalFreeMemory += block->size;

  return totalFreeMemory;
}

//
// Z_TryMalloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
// Because Z_TryMalloc is static, we can control the input and we can make sure tag is always < PU_PURGELEVEL.
//
#define MINFRAGMENT 64

static void __far *Z_TryMalloc(uint16_t size, int8_t tag, void __far *__far *user) {
    uint32_t rounded_size = (size + 3) & ~3;          // round to 4 bytes
    rounded_size += sizeof(memblock_t);                // add header

    memblock_t __far *sentinal = mainzone_sentinal;
    memblock_t __far *block = segmentToPointer(sentinal->next);

    /* Walk the ring, find first free block big enough */
    while (block != sentinal) {
        if (!block->user && block->size >= rounded_size) {
            /* Found one. Split if fragment is big enough. */
            if (block->size - rounded_size > MINFRAGMENT) {
                uint32_t block_addr  = (uint32_t)block;
                uint32_t newblock_addr = block_addr + rounded_size;

                memblock_t __far *newblock = (memblock_t __far *)newblock_addr;
                newblock->size = block->size - rounded_size;
                newblock->tag  = 0;
                newblock->user = NULL;
                newblock->next = block->next;
                newblock->prev = (uint32_t)block;
#if defined ZONEIDCHECK
                newblock->id   = ZONEID;
#endif
                segmentToPointer(block->next)->prev = newblock_addr;

                block->size = rounded_size;
                block->next = newblock_addr;
            }

            block->tag = tag;
            if (user)
                block->user = user;
            else
                block->user = (void __far *__far *)1;
#if defined ZONEIDCHECK
            block->id = ZONEID;
#endif

            mainzone_rover_segment = pointerToSegment(block);

            return (void __far *)(((uint32_t)block) + sizeof(memblock_t));
        }
        block = segmentToPointer(block->next);
    }

    return NULL;
}
static void __far *Z_Malloc(uint16_t size, int8_t tag, void __far *__far *user) {
  void __far *ptr = Z_TryMalloc(size, tag, user);
  if (!ptr) {
    uint32_t want = size;
    uint32_t have = Z_GetLargestFreeBlockSize();
    uint32_t total = Z_GetTotalFreeMemory();

//    semihost_write0("Z_Malloc FAIL: want=");
    {
      char b[6];
      b[0] = '0' + (want / 10000) % 10;
      b[1] = '0' + (want / 1000) % 10;
      b[2] = '0' + (want / 100) % 10;
      b[3] = '0' + (want / 10) % 10;
      b[4] = '0' + want % 10;
      b[5] = 0;
//      semihost_write0(b);
    }
//    semihost_write0(" have=");
    {
      char b[6];
      b[0] = '0' + (have / 10000) % 10;
      b[1] = '0' + (have / 1000) % 10;
      b[2] = '0' + (have / 100) % 10;
      b[3] = '0' + (have / 10) % 10;
      b[4] = '0' + have % 10;
      b[5] = 0;
//      semihost_write0(b);
    }
//    semihost_write0(" total=");
    {
      char b[6];
      b[0] = '0' + (total / 10000) % 10;
      b[1] = '0' + (total / 1000) % 10;
      b[2] = '0' + (total / 100) % 10;
      b[3] = '0' + (total / 10) % 10;
      b[4] = '0' + total % 10;
      b[5] = 0;
//      semihost_write0(b);
    }
//    semihost_write0("\n");

    I_Error("Z_Malloc: failed");
  }
  return ptr;
}

void __far *Z_TryMallocStatic(uint16_t size) {
  return Z_TryMalloc(size, PU_STATIC, NULL);
}

void __far *Z_MallocStatic(uint16_t size) {
  return Z_Malloc(size, PU_STATIC, NULL);
}

void __far *Z_MallocLevel(uint16_t size, void __far *__far *user) {
  return Z_Malloc(size, PU_LEVEL, user);
}

void __far *Z_CallocLevel(uint16_t size) {
  void __far *ptr = Z_Malloc(size, PU_LEVEL, NULL);
  _fmemset(ptr, 0, size);
  return ptr;
}

void __far *Z_CallocLevSpec(uint16_t size) {
  void __far *ptr = Z_Malloc(size, PU_LEVSPEC, NULL);
  _fmemset(ptr, 0, size);
  return ptr;
}

//
// Z_FreeTags
//
void Z_FreeTags(void) {
  memblock_t __far *next;

  segment_t mainzone_sentinal_segment = pointerToSegment(mainzone_sentinal);
  int safety = 10000;   // max iterations

  for (memblock_t __far *block = segmentToPointer(mainzone_sentinal->next); pointerToSegment(block) != mainzone_sentinal_segment; block = next) {
    if (--safety <= 0) {
//      //semihost_write0("Z_FreeTags: iteration limit hit\n");
      return;
    }

    next = segmentToPointer(block->next);

    // Loop detection: next must differ from current
    if (pointerToSegment(next) == pointerToSegment(block)) {
//      //semihost_write0("Z_FreeTags: self-loop detected\n");
      return;
    }

    // already a free block?
    if (!block->user)
      continue;

    if (PU_LEVEL <= block->tag && block->tag <= (PU_PURGELEVEL - 1))
      Z_FreeBlock(block);
  }
//  //semihost_write0("Z_FreeTags: done\n");
}

//
// Z_CheckHeap
//
void Z_CheckHeap(void) {
  segment_t mainzone_sentinal_segment = pointerToSegment(mainzone_sentinal);

  for (memblock_t __far *block = segmentToPointer(mainzone_sentinal->next);; block = segmentToPointer(block->next)) {
    if (block->next == mainzone_sentinal_segment) {
      // all blocks have been hit
      break;
    }

#if defined ZONEIDCHECK
    if (block->id != ZONEID)
      I_Error("Z_CheckHeap: block has id %x instead of ZONEID", block->id);
#endif

    if (pointerToSegment(block) + block->size != block->next)
      I_Error("Z_CheckHeap: block size does not touch the next block\n");
    if (segmentToPointer(block->next)->prev != pointerToSegment(block))
      I_Error("Z_CheckHeap: next block doesn't have proper back link\n");

    if (!block->user && !segmentToPointer(block->next)->user)
      I_Error("Z_CheckHeap: two consecutive free blocks\n");
  }
}