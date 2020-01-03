#include "Semaphore.hpp"


/*********************************************************************
// Constructs a new semaphore with a counter of 0
*********************************************************************/
Semaphore::Semaphore()
{
  count = 0;
  pthread_cond_init(&cond_var, NULL);
  pthread_mutex_init(&m_lock, NULL);
}
/*********************************************************************
// Constructs a new semaphore with a counter of val
*********************************************************************/
Semaphore::Semaphore(unsigned val)
{
  count = val;
  pthread_cond_init(&cond_var, NULL);
  pthread_mutex_init(&m_lock, NULL);
}
/*********************************************************************
* Comment
*********************************************************************/
void Semaphore::up() // Mark: 1 Thread has left the critical section
{
  pthread_mutex_lock(&m_lock);
  count++;
  pthread_cond_signal(&cond_var);
  pthread_mutex_unlock(&m_lock);
}
/*********************************************************************
* Comment
*********************************************************************/
void Semaphore::down() // Block untill counter > 0, and mark - One thread has entered the critical section.
{
  pthread_mutex_lock(&m_lock);
  while (count == 0) {
     pthread_cond_wait(&cond_var, &m_lock);
  }
  count--;
  pthread_mutex_unlock(&m_lock);
}
/*********************************************************************
// Destructs the Semaphore mutex & condVar
*********************************************************************/
Semaphore::~Semaphore()
{
  pthread_cond_destroy(&cond_var);
  pthread_mutex_destroy(&m_lock);
}
