
#ifndef LIBIP_MEM_H
#define LIBIP_MEM_H 1

void  MemConfig(int threaded);
void *MemAlloc(unsigned int size);
void *MemRealloc(void *ptr, unsigned int size);
void  MemFree(void *ptr);
void  MemExceptionHandler(void (*callback)(unsigned int));

#endif
