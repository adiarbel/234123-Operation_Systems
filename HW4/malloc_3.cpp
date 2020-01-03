#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <assert.h> // TODO: REMOVE

#define MIN_MALLOC_MEM_UNIT 128

class LogEntry;
void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* oldp, size_t size);
void free(void* p);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();
size_t potential_merge_mem(LogEntry* curr);
void merge_adj_log_entries(LogEntry* curr);
size_t align_mem(size_t size);
LogEntry* get_heap_head();

class LogEntry
{
public:
      LogEntry(size_t size,size_t data_size, bool is_free, void* mem_pointer)
    {
      m_size = size;
      m_data_size = data_size;
      m_is_free = is_free;
      m_next=NULL;
      m_prev=NULL;
      m_mem_pointer = mem_pointer;
    }
    LogEntry* get_next(){return m_next;}
    void set_next(LogEntry* new_next){m_next = new_next;}
    LogEntry* get_prev(){return m_prev;}
    void set_prev(LogEntry* new_prev){m_prev = new_prev;}
    bool is_free(){return m_is_free;}
    void set_free(bool new_status){m_is_free = new_status;}
    size_t get_size(){return m_size;}
    void set_size(size_t new_size){m_size = new_size;}
    size_t get_data_size(){return m_data_size;}
    void set_data_size(size_t new_size){m_data_size = new_size;}
    void* get_mem_pointer(){return m_mem_pointer;}
private:
    LogEntry* m_next;
    LogEntry* m_prev;
    bool m_is_free;
    size_t m_size; // block size
    size_t m_data_size; // data size in block
    void* m_mem_pointer;
};
//======== Log of memory entries =======
 LogEntry* log = NULL;

void* malloc(size_t size)
{
  /*
  1. LogEntry is the metadata class
  2. Is there any free available memory (which doesn't require brk)
    2.1 Checked by going over the list and search for pre-allocated areas
    2.2 If there is any, great but don't change its current size
    2.3 If there is no such area, allocate new one using sbrk
  */
  if(size==0 || size>100000000)
  {
    return NULL;
  }
  LogEntry* iter = get_heap_head();
  LogEntry* temp_entry;
  size_t aligned_size;
  void *iter_void ;
  char *iter_bytes;
  //===== realloc servicing - Part 3 problem 2 ======

  while (iter != NULL)
  {
    if(potential_merge_mem(iter) >= size)
    {
      merge_adj_log_entries(iter);
    }
    iter = iter->get_prev();
  }
  iter = get_heap_head();

  while (iter != NULL)
  {
    if(iter->is_free() && iter->get_size() >= size)
    {
      // Part3 - problem 1:
      if(iter->get_size() + sizeof(LogEntry) >= align_mem(sizeof(LogEntry)+size) + align_mem(MIN_MALLOC_MEM_UNIT + sizeof(LogEntry)))
      {
        size_t mem_residual_size = iter->get_size() - align_mem(sizeof(LogEntry) + size); //may be more than MIN_MALLOC_...
        iter_void = (void*) iter;
        iter_bytes = (char*) iter_void;
        temp_entry = (LogEntry*)(iter_bytes + align_mem(size+sizeof(LogEntry)));//temp_entry = (LogEntry*)(iter_bytes + align_mem(size+sizeof(LogEntry)));
        LogEntry meta2(mem_residual_size,mem_residual_size,true,temp_entry+1); // Pointers arithmetic
        std::memcpy(temp_entry, &meta2, sizeof(LogEntry));

        iter->set_size(align_mem(size + sizeof(LogEntry)) - sizeof(LogEntry));

        temp_entry->set_next(iter);
        temp_entry->set_prev(iter->get_prev());
        if(iter->get_prev()!=NULL)
        {
          iter->get_prev()->set_next(temp_entry);
        }
        iter->set_prev(temp_entry);
        if(iter==log)
        {
          log = temp_entry;
        }
      }
      iter->set_free(false);
      iter->set_data_size(size);
      return iter->get_mem_pointer();
    }
    iter = iter->get_prev();
  }

  //==== Problem 3 ====
  void* prog_brk;
  if(log!=NULL && log->is_free())
  {
    aligned_size = align_mem(size - log->get_size()); //Delta
    prog_brk = sbrk(aligned_size);
    if(prog_brk==(void*)(-1))
    {
      return NULL;
    }
    log->set_size(log->get_size()+aligned_size);
    log->set_free(false);
    log->set_data_size(size);
    return log->get_mem_pointer();
  }
  else
  {
    aligned_size = align_mem(size+sizeof(LogEntry));
    prog_brk = sbrk(aligned_size);//prog_brk = sbrk(sizeof(LogEntry)+size);
    if(prog_brk==(void*)(-1))
    {
      return NULL;
    }

    temp_entry = (LogEntry*)prog_brk;
    LogEntry log_entry(aligned_size-sizeof(LogEntry),size,false,temp_entry + 1);//LogEntry log_entry(aligned_size - sizeof(LogEntry),size,false,temp_entry + 1); // Pointers arithmetic : temp_entry/prog_brk + sizeof(LogEntry)
    std::memcpy(temp_entry, &log_entry, sizeof(LogEntry));
    temp_entry->set_next(log);
    if(log!=NULL)
    {
      log->set_prev(temp_entry);
    }
    log = temp_entry; // NOTE: The pointer is to the top entry, not the memory head
    return temp_entry->get_mem_pointer();
  }
}

void* calloc(size_t num, size_t size)
{
  void* new_mem = malloc(num * size);
  if(new_mem==NULL)
    return NULL;
  std::memset(new_mem, 0, num*size);
  return new_mem;
}

// TODO: Check if needs to prevent allocations at any price
void* realloc(void* oldp, size_t size)
{
  LogEntry* iter = get_heap_head();
  void* iter_void;
  char* iter_bytes;
  LogEntry* temp_entry;
  void* malloc_mem;
  size_t old_size=0;
  // find old memory and free it. so later can use it for the new mem size.
  if(size==0 || size>100000000)
  {
    return NULL;
  }

  if(oldp!=NULL)
  {
    while (iter != NULL)
    {
      if(iter->get_mem_pointer() == oldp)
      {
        if(size<=iter->get_size())
        {
          if(iter->get_size() + sizeof(LogEntry) >= align_mem(sizeof(LogEntry)+size) + align_mem(MIN_MALLOC_MEM_UNIT + sizeof(LogEntry)))
          {
            size_t mem_residual_size = iter->get_size() - align_mem(sizeof(LogEntry) + size); //may be more than MIN_MALLOC_...
            iter_void = (void*) iter;
            iter_bytes = (char*) iter_void;
            temp_entry = (LogEntry*)(iter_bytes + align_mem(size+sizeof(LogEntry)));//temp_entry = (LogEntry*)(iter_bytes + align_mem(size+sizeof(LogEntry)));
            LogEntry meta2(mem_residual_size,mem_residual_size,true,temp_entry+1); // Pointers arithmetic
            std::memcpy(temp_entry, &meta2, sizeof(LogEntry));

            iter->set_size(align_mem(size + sizeof(LogEntry)) - sizeof(LogEntry));

            temp_entry->set_next(iter);
            temp_entry->set_prev(iter->get_prev());
            if(iter->get_prev()!=NULL)
            {
              iter->get_prev()->set_next(temp_entry);
            }
            iter->set_prev(temp_entry);
            if(iter==log)
            {
              log = temp_entry;
            }
            merge_adj_log_entries(temp_entry);
          }
          return iter->get_mem_pointer();
        }
        iter->set_free(true);
        break;
      }
      else
        iter = iter->get_prev();
    }
    old_size = iter->get_data_size(); // old_size = size in iter which did "break"
  }
  malloc_mem = malloc(size); // NOTE: toggles the is_free and updates data_size

  if(malloc_mem == NULL)
  {
    if(oldp!=NULL)
    {
      iter->set_free(false);
    }
    return NULL;
  }

  if(oldp!=NULL)
  {
    if(malloc_mem != iter->get_mem_pointer())
    {
      std::memcpy(malloc_mem,oldp,old_size);
    }
  }
  return malloc_mem;
}

size_t potential_merge_mem(LogEntry* curr)
{
  LogEntry* next = curr->get_next();
  LogEntry* prev = curr->get_prev();
  size_t mem_profit = curr->get_size();
  if(curr==NULL || !(curr->is_free()))
  {
    return 0;
  }
  if(prev != NULL && prev->is_free())
  {
    mem_profit += prev->get_size()+sizeof(LogEntry);
  }
  else if(next != NULL && next->is_free())
  {
    mem_profit += next->get_size()+sizeof(LogEntry);
  }
  return mem_profit;
}

void merge_adj_log_entries(LogEntry* curr)
{
  LogEntry* next = curr->get_next();
  LogEntry* prev = curr->get_prev();
  if( curr==NULL || !(curr->is_free()) )
  {
    return;
  }
  if(next != NULL && next->is_free())
  {
    // union of 2 - curr and next - removing curr
    // unlink curr meta-data
    next->set_prev(prev);
    if(prev!=NULL)
    {
      prev->set_next(next);
    }
    else
    {
      log = next;
    }
    // update next's fields
    next->set_size(next->get_size()+curr->get_size()+sizeof(LogEntry));
    curr = next;
  }

  if(prev != NULL && prev->is_free())
  {
    // union of 2 - curr and prev - removing prev
    // unlink prev meta-data

    curr->set_prev(prev->get_prev());
    if(prev->get_prev()!=NULL)
    {
      prev->get_prev()->set_next(curr);
    }
    else
    {
      log=curr;
    }
    // update curr's fields
    curr->set_size(curr->get_size()+prev->get_size()+sizeof(LogEntry));
  }
}
// TODO: check if needed logic to determine if to fold back brk() or not
void free(void* p)
{
  LogEntry* iter = get_heap_head();
  while (iter != NULL)
  {
    // Part3 - problem 2:
    if(iter->get_mem_pointer() == p)
    {
      iter->set_free(true);
      merge_adj_log_entries(iter);
      break;
    }
    iter = iter->get_prev();
  }
}

size_t _num_free_blocks()
{
  LogEntry* iter = log;
  size_t cnt = 0;
  while (iter != NULL)
  {
    if(iter->is_free())
    {
      cnt++;
    }
    iter = iter->get_next();
  }
  return cnt;
}

size_t _num_free_bytes()
{
  LogEntry* iter = log;
  size_t sum=0;
  while (iter != NULL)
  {
    if(iter->is_free())
      sum+=iter->get_size();
    iter = iter->get_next();
  }
  return sum;
}

size_t _num_allocated_blocks()
{
  LogEntry* iter = log;
  size_t count=0;
  while (iter != NULL)
  {
    count++;
    iter = iter->get_next();
  }
  return count;
}

size_t _num_allocated_bytes()
{

  LogEntry* iter = log;
  size_t sum=0;
  while (iter != NULL)
  {
    sum+=iter->get_size();
    iter = iter->get_next();
  }
  return sum;
}

size_t _num_meta_data_bytes()
{
  LogEntry* iter = log;
  size_t count=0;
  while (iter != NULL)
  {
    count++;
    iter = iter->get_next();
  }
  return count*_size_meta_data();
}

size_t _size_meta_data()
{
  return sizeof(LogEntry);
}

size_t align_mem(size_t size)
{
  if (size%4 == 0)
    return size;
  return (size)+4-(size)%4;
}

LogEntry* get_heap_head()
{
  LogEntry* iter = log;
  if(log==NULL)
  {
    return NULL;
  }
  while(iter!=NULL)
  {
    if(iter->get_next()==NULL)
      return iter;
    iter = iter->get_next();
  }
}
