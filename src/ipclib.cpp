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
        setok(false);
    }
    else {
        setid(semid);
        setok(true);
        setflags(_st.buf->sem_perm.mode);
        setkey(_st.buf->sem_perm.__key);
        _nsems = _st.buf->sem_nsems;
        _sem_val.clear();
        _sem_val.reserve(_nsems);
        int sem_val;
        for(int i=0; i < _nsems; i++) 
        {
            if ((sem_val = semctl(semid,i,GETVAL)) < 0) 
            {
                LOG_ERROR  << "Can't init queue semaphore";
                setok(false);
                _sem_val.clear();
                break;
            }
            else
                _sem_val.emplace_back(sem_val);
        }
    }
}

Semaphore::Semaphore(const key_t key, const int nsems, const int sem_val,  bool deleteOnExit):
        Ipc{key,deleteOnExit}
{
    int flags = getflags();
    int id = semget(key,nsems,flags);

    if (id < 0) 
    {
        PLOG_DEBUG_IF(loglevel) << "semget fail!";
        switch(errno) {
        case ENOENT:
            flags |= IPC_CREAT | IPC_EXCL | 0666;
            if ((id = semget(key,nsems,flags)) < 0) {
			    LOG_ERROR << "semget: " << strerror(errno);
            }
            else {
                for (int i = 0; i < nsems; i++) {
                    if (semctl(id,i,SETVAL,sem_val)) {
                        LOG_ERROR  << "Can't init queue semaphore";
                    }
                }
                setok(true);
                LOG_DEBUG_IF(loglevel) << "Semaphore created!";
            }
            break;

        case EINVAL: LOG_ERROR << "EINVAL: " << strerror(errno); break;
        case EACCES: LOG_ERROR << "EACCES: " << strerror(errno); break;
        case ENOSPC: LOG_ERROR << "ENOSPC: " << strerror(errno); break;
        case EEXIST: LOG_ERROR << "EEXIST: " << strerror(errno); break;
        }
    }   
    else {
        if (key == IPC_PRIVATE) 
        {
            for (int i = 0; i < nsems; i++) 
            {
	            if (semctl(id,i,SETVAL,sem_val)) 
                {
                    LOG_ERROR << "Can't init queue semaphore";
    	        }
            }
        }
        setok(true);
        LOG_DEBUG_IF(loglevel) << "Semaphore adquired: " << getid();
    }

    if(*this)
    {
        setid(id);
        setflags(flags);
        setkey(key);
        _sem_val.assign(nsems,sem_val);             // 7 ints with a value of 100
        _nsems = nsems;
    }
}

int Semaphore::set(const int op)
{
    if(*this)
    {
        _sop.sem_num = 0;
	    _sop.sem_op = op;
	    _sop.sem_flg = SEM_UNDO;

	    errno = 0;
	    while (semop (getid(), &_sop, 1) == -1)
	    {
		    if(EINTR == errno)
		    {
			    errno = 0;
		    }
		    else
		    {
                LOG_ERROR << "semop: " << strerror(errno) << ":" << getid();
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
    if(*this)
    {
	    if(getDeleteOnExit())
	    {
            LOG_DEBUG_IF(loglevel) << "Removing semaphore " << getid();
		    if (semctl (getid(), 0, IPC_RMID) == -1)
		    {
                LOG_ERROR << "semctl: " << strerror(errno) << ":" << getid();
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
    if (get_stat(getid()) < 0) {
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
        setok(false);
    }
    else {
        setok(true);
        if ((_shmaddr = shmat(getid(), NULL, 0)) == NULL) {
            LOG_ERROR << "shmat: " << strerror(errno) << ":" << getid();
            setok(false);
        }
        //PLOG_DEBUG_IF(loglevel) << "SharedMemory Ctor getid() " << getid() << " _flags " << _flags << " _size " << _size;
    }
    if(*this)
    {
        setid(shmid);
        setflags(_st.shm_perm.mode);
        setkey(_st.shm_perm.__key);
        _size = _st.shm_segsz;
    }
   
}


SharedMemory::SharedMemory(const key_t key, int size,bool deleteOnExit):
        Ipc{key,deleteOnExit}
{
    int flags = getflags();
    int id = shmget(key, size, flags);

   if ( id  < 0)
   {
        if (errno == ENOENT) 
        {
	        flags |= IPC_CREAT;
            id = shmget(key, size, flags);
	        if (id < 0) 
            {
	            LOG_ERROR << "shmget: " << strerror(errno) << ":" << getid();
            }
            else {
                setok(true);
            }
        }
    }
    else {
        setok(true);
    }

    if(*this)
    {
        setid(id);
        setflags(flags);
        setkey(key);

        if ((_shmaddr = shmat(getid(), NULL, 0)) == NULL) {
            LOG_ERROR << "shmat: " << strerror(errno) << ":" << getid();
            setok(false);
        }
        else
            LOG_DEBUG_IF(loglevel) << "SharedMemory Created & Attached: " << getid();
    }
}

SharedMemory::~SharedMemory()
{
    if(*this && _shmaddr) 
    {
        if(getDeleteOnExit())
	    {
            LOG_DEBUG_IF(loglevel) << "Detaching SharedMemory " << getid();

            if (shmdt(_shmaddr)==-1) 
            {
                LOG_ERROR << "shmdt: " << strerror(errno) << ":" << getid();
            }
            else 
            {
                LOG_DEBUG_IF(loglevel) << "Removing SharedMemory " << getid();
		        if (shmctl(getid(),IPC_RMID,0)) 
		        {
                    LOG_ERROR << "shmctl: " << strerror(errno) << ":" << getid();
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
        setok(false);
    }
    else {
        setok(true);
        setid(msgid);
        setflags(_st.msg_perm.mode);
        setkey(_st.msg_perm.__key);
        _size = _st.msg_qbytes;

        // PLOG_DEBUG_IF(loglevel) << "MessageQueue connected getid() " << getid() << " _flags " << _flags << " _size " << _size;
    }
}

int MessageQueue::send(protomsg::st_protomsg *p_protomsg, std::string &pdata)
{
    if(*this)
    {
        int nbytes = sizeof(protomsg::st_protomsg)+pdata.size();
        std::unique_ptr<protomsg::st_protomsg> p_qmsg2((protomsg::st_protomsg*) ::operator new (nbytes));

        memcpy(p_qmsg2.get(), p_protomsg, sizeof(protomsg::st_protomsg));
        strcpy(p_qmsg2->msg, pdata.c_str());

        if (msgsnd(getid(), reinterpret_cast<void *>(p_qmsg2.get()), nbytes - sizeof(long), 0) < 0) 
        {
            PLOG_ERROR << "msgsnd: " << strerror(errno) << ":" << getid();
            return 0;
        }
        else 
            return 1;
    }
    else 
    {
        PLOG_ERROR << "send: " << getid() << " NOT OK!!";
    }
    return 0;
}

int MessageQueue::rcv(protomsg::st_protomsg *p_protomsg, std::string &pdata)
{
    if(*this) 
    {
        int msgbytes;
        std::unique_ptr<protomsg::st_protomsg> p_qmsg2((protomsg::st_protomsg*) ::operator new (sizeof(protomsg::st_protomsg)+protomsg::MAX_MSG_SIZE));
        protomsg::st_protomsg *p= p_qmsg2.get();

        if ((msgbytes = msgrcv(getid(), (void *) p, protomsg::MAX_MSG_SIZE , 0, 0)) < 0) {
            PLOG_ERROR << "msgrcv: " << strerror(errno) << ":" << getid();
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
        PLOG_ERROR << "send: " << getid() << " NOT OK!!";
    }
    return 0;
}

MessageQueue::MessageQueue(const key_t key, bool deleteOnExit):
        Ipc{key,deleteOnExit}
{
    struct msqid_ds qstatus;
    int flags=getflags();
    int id = msgget(key,flags);

    if (id < 0) 
    {
        if (errno == ENOENT) 
        {
	        flags |= IPC_CREAT;
	        if ((id = msgget(key,flags)) < 0) {
	            LOG_ERROR << "msgget: " << strerror(errno);
	        }
            else {
        	    if (msgctl(id,IPC_STAT,&qstatus) < 0) {
	                LOG_ERROR << "msgctl IPC_STAT: " << strerror(errno) << ":" << getid();
	            }
                else {
                    LOG_DEBUG << "msg_qbytes:" << qstatus.msg_qbytes;
                    setok(true);
                }
            }
        }
        else
        {
            LOG_ERROR << "msgget: " << strerror(errno) << ":" << getid();
        }
    } 
    else {
        setok(true);
        LOG_DEBUG_IF(loglevel) << "MessageQueue Created: " << getid();
    }

    if(*this)
    {
        setid(id);
        setflags(flags);
        setkey(key);
    }
}

MessageQueue::~MessageQueue()
{
    if(*this)
    {
        if(getDeleteOnExit())
        {
            LOG_DEBUG_IF(loglevel) << "Removing MessageQueue " << getid();
            if (msgctl(getid(),0,IPC_RMID))
	        {
                LOG_ERROR << "msgctl: " << strerror(errno) << ":" << getid();
    	    }
   	    }
    }
}
