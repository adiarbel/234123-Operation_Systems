#include <cstdlib>
#include <unistd.h>


void* malloc(size_t size)
{
  if(size==0 || size>100000000)
  {
    return NULL;
  }
  void* ptr = sbrk(size);
  if(ptr==(void*)(-1))
  {
    return NULL;
  }
  return ptr;
}
