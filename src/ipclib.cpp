#include "ipclib.h"

using namespace std;

int Semaphore::get_stat(int semid)
{
    return semctl(semid, 0, IPC_STAT, _st);
}

Semaphore::Semaphore(int semid):Ipc{IPC_PRIVATE,false}
{
    if (get_stat(semid) < 0) 
    {
        LOG_ERROR << "semctl: " << strerror(errno);
        _ok = false;
    }
    else {
        /*
        según X/OPEN tenemos que definirla nosostros mismos 
        union semun {
            int val;                    // valor para SETVAL 
            struct semid_ds *buf;       // buffer para IPC_STAT, IPC_SET 
            unsigned short int *array;  // array para GETALL, SETALL 
            struct seminfo *__buf;      // buffer para IPC_INFO 
        };
        One semid data structure for each set of semaphores in the system. 
        struct semid_ds {
                struct ipc_perm sem_perm;       // permissions .. see ipc.h 
                time_t          sem_otime;      // last semop time 
                time_t          sem_ctime;      // last change time 
                struct sem      *sem_base;      // ptr to first semaphore in array 
                struct wait_queue *eventn;
                struct wait_queue *eventz;
                struct sem_undo  *undo;         // undo requests on this array 
                ushort          sem_nsems;      // no. of semaphores in array 
        };
        */
        _id = semid;
        _ok = true;
        _flags = _st.buf->sem_perm.mode;
        _nsems = _st.buf->sem_nsems;
        // PLOG_DEBUG_IF(loglevel) << "Semaphore Ctor _id " << _id << " _flags " << _flags << " _nsems " << _nsems;
    }
}

Semaphore::Semaphore(const key_t key, const int nsems, const int sem_val,  bool deleteOnExit):
        Ipc{key,deleteOnExit}
{
    
    if ((_id = semget(key,nsems,_flags)) < 0) 
    {
        PLOG_DEBUG_IF(loglevel) << "semget fail!";
        switch(errno) {
        case ENOENT:
            _flags |= IPC_CREAT | IPC_EXCL | 0666;
            if ((_id = semget(key,nsems,_flags)) < 0) {
			    LOG_ERROR << "semget: " << strerror(errno);
            }
            else {
                for (int i = 0; i < nsems; i++) {
                    if (semctl(_id,i,SETVAL,sem_val)) {
                        LOG_ERROR  << "Can't init queue semaphore";
                    }
                }
                _key = key;
                _sem_val = sem_val;
                _nsems = nsems;
                _ok = true;
                LOG_DEBUG_IF(loglevel) << "Semaphore created!";
            }
            break;

        case EINVAL: LOG_ERROR << "EINVAL: " << strerror(errno); break;
        case EACCES: LOG_ERROR << "EACCES"; break;
        case ENOSPC: LOG_ERROR << "ENOSPC"; break;
        case EEXIST: LOG_ERROR << "EEXIST"; break;
        }
    }   
    else {
        if (key == IPC_PRIVATE) {
            for (int i = 0; i < nsems; i++) {
	            if (semctl(_id,i,SETVAL,sem_val)) {
                    LOG_ERROR << "Can't init queue semaphore";
    	        }
            }
            _key = key;
            _sem_val = sem_val;
            _nsems = nsems;
            _ok = true;
            LOG_DEBUG_IF(loglevel) << "Semaphore created: " << _id;
        }
   }
}

int Semaphore::set(const int op)
{
    if(_ok)
    {
        _sop.sem_num = 0;
	    _sop.sem_op = op;
	    _sop.sem_flg = SEM_UNDO;

	    errno = 0;
	    while (semop (_id, &_sop, 1) == -1)
	    {
		    if(EINTR == errno)
		    {
			    errno = 0;
		    }
		    else
		    {
                LOG_ERROR << "semop: " << strerror(errno) << ":" << _id;
    		    return -1;
		    }
	    }
	    return 1;
    }
    else
        return -1;
}

Semaphore::~Semaphore()
{
    if(_ok) 
    {
	    if(_deleteOnExit)
	    {
            LOG_DEBUG_IF(loglevel) << "Removing semaphore " << _id;
		    if (semctl (_id, 0, IPC_RMID) == -1)
		    {
                LOG_ERROR << "semctl: " << strerror(errno) << ":" << _id;
	    	}
    	}
    }
}

int SharedMemory::get_stat(int shmid)
{
    return shmctl(shmid, IPC_STAT, &_st);
}

int SharedMemory::get_nattach()
{
    if (get_stat(_id) < 0) {
        LOG_ERROR << "shmctl: " << strerror(errno);
        return -1;
    }
    else
        return static_cast<int>(_st.shm_nattch);
}

SharedMemory::SharedMemory(int shmid):Ipc{IPC_PRIVATE,false}
{
    if (get_stat(shmid) < 0) 
    {
        LOG_ERROR << "shmctl: " << strerror(errno);
        _ok = false;
    }
    else {
        /*
            struct shmid_ds {
               struct ipc_perm shm_perm;  // permisos de operación 
               int shm_segsz;             // tamaño del segmento (bytes) 
               time_t shm_atime;          // tiempo de la última unión 
               time_t shm_dtime;          // tiempo de la última separación 
               time_t shm_ctime;          // tiempo de la última modificación 
               unsigned short shm_cpid;   // pid del creador 
               unsigned short shm_lpid;   // pid of último operador 
               short shm_nattch;          // nº. de uniones actuales 
            };
        */
        _id = shmid;
        _ok = true;
        _flags = _st.shm_perm.mode;
        _size = _st.shm_segsz;
        
        if ((_shmaddr = shmat(_id, NULL, 0)) == NULL) {
            LOG_ERROR << "shmat: " << strerror(errno) << ":" << _id;
            _ok = false;
        }
        //PLOG_DEBUG_IF(loglevel) << "SharedMemory Ctor _id " << _id << " _flags " << _flags << " _size " << _size;
    }
   
}


SharedMemory::SharedMemory(const key_t key, int size,bool deleteOnExit):
        Ipc{key,deleteOnExit}
{

   if ((_id = shmget(key, size, _flags)) < 0)
   {
        if (errno == ENOENT) 
        {
	        _flags |= IPC_CREAT;
	        if ((_id = shmget(key, size, _flags)) < 0) 
            {
	            LOG_ERROR << "shmget: " << strerror(errno) << ":" << _id;
            }
            else
                _ok = true;
        }
    }
    else 
        _ok = true;

    if(_ok == true)
    {
        if ((_shmaddr = shmat(_id, NULL, 0)) == NULL) {
            LOG_ERROR << "shmat: " << strerror(errno) << ":" << _id;
            _ok = false;
        }
        else
            LOG_DEBUG_IF(loglevel) << "SharedMemory Created & Attached: " << _id;
    }
}

SharedMemory::~SharedMemory()
{
    if(_ok && _shmaddr) 
    {
        if(_deleteOnExit)
	    {
            LOG_DEBUG_IF(loglevel) << "Detaching SharedMemory " << _id;

            if (shmdt(_shmaddr)==-1) 
            {
                LOG_ERROR << "shmdt: " << strerror(errno) << ":" << _id;
            }
            else 
            {
                LOG_DEBUG_IF(loglevel) << "Removing SharedMemory " << _id;
		        if (shmctl(_id,IPC_RMID,0)) 
		        {
                    LOG_ERROR << "shmctl: " << strerror(errno) << ":" << _id;
	    	    }
            }
        }
   }

}

int MessageQueue::get_stat(int msgid)
{
    return msgctl(msgid, IPC_STAT, &_st);
}

MessageQueue::MessageQueue(int msgid):Ipc{IPC_PRIVATE,false}
{
    if ( get_stat(msgid) < 0) 
    {
        LOG_ERROR << "msgctl: " << strerror(errno);
        _ok = false;
    }
    else {
        /*
        struct msqid_ds
        {
          struct ipc_perm msg_perm;	        // structure describing operation permission 
          __MSQ_PAD_TIME (msg_stime, 1);	// time of last msgsnd command 
          __MSQ_PAD_TIME (msg_rtime, 2);	// time of last msgrcv command 
          __MSQ_PAD_TIME (msg_ctime, 3);	// time of last change 
          __syscall_ulong_t __msg_cbytes;   // current number of bytes on queue 
          msgqnum_t msg_qnum;		        // number of messages currently on queue 
          msglen_t msg_qbytes;		        // max number of bytes allowed on queue 
          __pid_t msg_lspid;		        // pid of last msgsnd() 
          __pid_t msg_lrpid;		        // pid of last msgrcv() 
          __syscall_ulong_t __glibc_reserved4;
          __syscall_ulong_t __glibc_reserved5;
        };
        */
        _id = msgid;
        _ok = true;
        _flags = _st.msg_perm.mode;
        _size = _st.msg_qbytes;

        // PLOG_DEBUG_IF(loglevel) << "MessageQueue connected _id " << _id << " _flags " << _flags << " _size " << _size;
    }
}

int MessageQueue::send(protomsg::st_protomsg *p_protomsg, std::string &pdata)
{
    if(_ok) 
    {
        int nbytes = sizeof(protomsg::st_protomsg)+pdata.size();
        std::unique_ptr<protomsg::st_protomsg> p_qmsg2((protomsg::st_protomsg*) ::operator new (nbytes));

        memcpy(p_qmsg2.get(), p_protomsg, sizeof(protomsg::st_protomsg));
        strcpy(p_qmsg2->msg, pdata.c_str());

        if (msgsnd(_id, reinterpret_cast<void *>(p_qmsg2.get()), nbytes - sizeof(long), 0) < 0) 
        {
            PLOG_ERROR << "msgsnd: " << strerror(errno) << ":" << _id;
            return 0;
        }
        else 
            return 1;
    }
    else 
    {
        PLOG_ERROR << "send: " << _id << " NOT OK!!";
    }
    return 0;
}

int MessageQueue::rcv(protomsg::st_protomsg *p_protomsg, std::string &pdata)
{
    if(_ok) 
    {
        int msgbytes;
        std::unique_ptr<protomsg::st_protomsg> p_qmsg2((protomsg::st_protomsg*) ::operator new (sizeof(protomsg::st_protomsg)+protomsg::MAX_MSG_SIZE));
        protomsg::st_protomsg *p= p_qmsg2.get();

        if ((msgbytes = msgrcv(_id, (void *) p, protomsg::MAX_MSG_SIZE , 0, 0)) < 0) {
            PLOG_ERROR << "msgrcv: " << strerror(errno) << ":" << _id;
            return 0;
        }
        else {
            PLOG_DEBUG_IF(loglevel) << "msg(" << msgbytes << "):" << p_qmsg2->msg << "";
            memcpy(p_protomsg, p, sizeof(protomsg::st_protomsg)-1);
            pdata = std::string(p->msg);
            return 1;
        }
    }
    else 
    {
        PLOG_ERROR << "send: " << _id << " NOT OK!!";
    }
    return 0;
}

MessageQueue::MessageQueue(const key_t key, bool deleteOnExit):
        Ipc{key,deleteOnExit}
{
    struct msqid_ds qstatus;

    if ((_id = msgget(key,_flags)) < 0) 
    {
        if (errno == ENOENT) 
        {
	        _flags |= IPC_CREAT;
	        if ((_id = msgget(key,_flags)) < 0) {
	            LOG_ERROR << "msgget: " << strerror(errno) << ":" << _id;
	        }
            else {
        	    if (msgctl(_id,IPC_STAT,&qstatus) < 0) {
	                LOG_ERROR << "msgctl IPC_STAT: " << strerror(errno) << ":" << _id;
	            }
                else {
                    LOG_DEBUG << "msg_qbytes:" << qstatus.msg_qbytes;
                    _ok = true;
                        
                }
            }
        }
        else
        {
            LOG_ERROR << "msgget: " << strerror(errno) << ":" << _id;
        }
    } 
    else {
        _ok = true;
        LOG_DEBUG_IF(loglevel) << "MessageQueue Created: " << _id;
    }
}

MessageQueue::~MessageQueue()
{
    if(_ok) 
    {
        if(_deleteOnExit)
        {
            LOG_DEBUG_IF(loglevel) << "Removing MessageQueue " << _id;
            if (msgctl(_id,0,IPC_RMID))
	        {
                LOG_ERROR << "msgctl: " << strerror(errno) << ":" << _id;
    	    }
   	    }
    }

}
