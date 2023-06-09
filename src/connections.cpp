#include "connections.h"

connections::connections(int MaxConnections, int NumThreads)
{
    LOG_DEBUG << "connections Ctor - Creating " << NumThreads << " queues, " << MaxConnections << " connections.";
    nThreads = NumThreads;
    MaxConn = MaxConnections;

    current_connections = reinterpret_cast<connection *>(this+1);

    memset((void *)current_connections, 0, sizeof(connection)*MaxConn);

    first_free = 0;

    for(int i=0; i < MaxConn-1; i++) {
      current_connections[i].next_info = i+1;
    }
  
    current_connections[MaxConn-1].next_info = -1;

    initialized = true;
 }

void connections::mark_obsolete(int idx)
{
  int next,prev;
  int ind;

  this->current_connections[idx].status = st_obsolete;

}

void connections::delete_obsolete(int idx) 
{
  memset(&(current_connections[idx]), 0, sizeof(connection));
  this->current_connections[idx].next_info = this->first_free;
  this->first_free = idx;
}

int connections::ending_operation(int idx, Semaphore &sem, connection &cur_conn)
{
  if( idx < 0 || idx >= MaxConn)
    return -1;

  sem.Lock();  

  cur_conn = this->current_connections[idx];

  if( this->current_connections[idx].status != st_ready )
  {
    sem.Unlock();
    return -2;
  }

  time( &(this->current_connections[idx].last_op));
  this->current_connections[idx].num_ops++;

  cur_conn = this->current_connections[idx];

  sem.Unlock();

  return 0;
}

int connections::clean_repeated_ip(sockaddr_in *ppal, Semaphore &sem)
{
  int nThread = -1;

  for(int i=0; i < MaxConn-1; i++) 
  {
    switch(current_connections[i].status) 
    {
      case 2:   /* Sinchronized, lets see ... */
      case 1:   /* Not sinchronized, lets see ... */
	      if((ppal->sin_addr.s_addr==current_connections[i].sockaddr.sin_addr.s_addr) && 
	         (ppal->sin_port==current_connections[i].sockaddr.sin_port)) 
        {
          sem.Lock();
          
          nThread = current_connections[i].nthread;
		      mark_obsolete(i);
          
          sem.Unlock();      		
	      }
	      break;
      case 3: /* ya marcado, pasar de el */
      case 0: /* vacio, pasar de el */
      default:
	      break;
    }
  }
  return nThread;
}

int connections::register_new_conn(int nthread, int sd, sockaddr_in s_in, Semaphore &sem)
{
  int idx = -1;
  
  sem.Lock();

  if(first_free==-1) 
  {
    sem.Unlock();
    return idx;  
  }

  idx = first_free;
  first_free = current_connections[idx].next_info;
  
  memset(&current_connections[idx], 0, sizeof(connection));

  current_connections[idx].status = st_ready;
  current_connections[idx].next_info = -1;
  current_connections[idx].nthread = nthread;
  current_connections[idx].sd = sd;
  memcpy(&(current_connections[idx].sockaddr), &s_in, sizeof(struct sockaddr_in));
  time(&current_connections[idx].entry);

  sem.Unlock();
  return idx;
}

int connections::check_obsolete(int idx_con, Semaphore &sem)
{
  if( idx_con < 0 || idx_con >= MaxConn )
    return -1;

  if(current_connections[idx_con].status == st_obsolete ) 
  {
    sem.Lock();

    delete_obsolete(idx_con);
    
    sem.Unlock();

    return 1;
  }

  return 0;
}


int connections::unregister_conn(int idx, Semaphore &sem)
{
  sem.Lock();
    
  if(current_connections[idx].status == st_ready ) {
      mark_obsolete(idx);
  }

  delete_obsolete(idx);

  sem.Unlock();

  return 1;
}
