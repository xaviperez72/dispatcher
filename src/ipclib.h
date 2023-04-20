#ifndef IPCLIB_H_
#define IPCLIB_H_

#include "common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>

#ifdef  __cplusplus
}
#endif

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
    /* la union semun se define al incluir <sys/sem.h> */
#else
    /* según X/OPEN tenemos que definirla nosostros mismos */
    union semun {
        int val;                    /* valor para SETVAL */
        struct semid_ds *buf;       /* buffer para IPC_STAT, IPC_SET */
        unsigned short int *array;  /* array para GETALL, SETALL */
        struct seminfo *__buf;      /* buffer para IPC_INFO */
    };
#endif

/**
 * IPC Common IPC objects
 */
class Ipc {
public:
    Ipc() = default;
    Ipc(key_t key, bool deleteOnExit):_key{key},_deleteOnExit{deleteOnExit}{}
    key_t _key{IPC_PRIVATE};     // Key used to connect/create.
	int _id{-1};                 // ID
	bool _deleteOnExit{false};   // Flag to delete on exit
    int _flags{0666};            // Flags on connect/create
    bool _ok{false};             // Flag true=operative Ipc, false=no operative Ipc

    void EnableDelete() { 	_deleteOnExit = true; }
    void DisableDelete() { 	_deleteOnExit = false; }
    key_t getkey() const { return _key; }
    int getid() const { return _id; }
    virtual operator bool() const { return _ok == true; }
};

/**
 * IPC Semaphore Object
 */
class Semaphore : public Ipc {
    int _nsems;                 // Number of semaphores
    int _sem_val;               // Initial sem_val
    struct sembuf _sop;         // To operate on semaphore
	static constexpr int LOCK = -1;
    static constexpr int UNLOCK = 1;
    int set(const int op);
    semun _st;
    int get_stat(int semid);

public:
   /**
    * Semaphore Constructor
    *
    * @param key (default IPC_PRIVATE) 
    * @param nsems =1 Number of semaphores 
    * @param sem_val =1 Sempahore value
    * @param deleteOnExit = false (don't remove semaphore on destruction. true = remove it)
    */
    Semaphore(const key_t key, const int nsems, const int sem_val,  bool deleteOnExit);

    Semaphore(int semid);

    Semaphore() = default;

    /**
    * Lock semaphore
    */
    int Lock(){ int ret=set(LOCK); PLOG_DEBUG_IF(loglevel) << "Semaphore Locked!"; return ret; }
    /**
    * Unlock semaphore
    */
    int Unlock(){ int ret=set(UNLOCK); PLOG_DEBUG_IF(loglevel) << "Semaphore Unlocked!"; return ret; }

    virtual ~Semaphore();
};

/**
 * IPC Shared Memory Object
 */
class SharedMemory : public Ipc {
	void* _shmaddr{nullptr};
	int _size{4};
    shmid_ds _st;
    int get_stat(int shmid);
public: 
	SharedMemory(const key_t key, int size, bool deleteOnExit);
    SharedMemory(int shmid);
    SharedMemory() = default;
	virtual ~SharedMemory();
    void *getaddr() const { return _shmaddr; }
    int get_nattach();
};

template<typename T>
    struct qmsg {
        long   mtype;       /* Message type. */
        T      data;
    };

class MessageQueue : public Ipc {
    msqid_ds _st;
    int _size;
    int get_stat(int msgid);
public:
    MessageQueue(int msgid);
    MessageQueue(const key_t key, bool deleteOnExit);
    MessageQueue() = default;
    virtual ~MessageQueue();
    template<typename T>
        int send(long type, T& pdata)
        {
           if(_ok) 
           {
               qmsg<T> mymsg{};
               mymsg.mtype = type;
               mymsg.data = pdata;

               if (msgsnd(_id, &mymsg, sizeof(T), 0) < 0) {
                   PLOG_ERROR << "msgsnd: " << strerror(errno) << ":" << _id;
                   return 0;
               }
               else 
                   return 1;
           }
           return 0;
        }
    template<typename T>
        int rcv(long &type, T& pdata)
        {
            if(_ok) 
            {
                qmsg<T> mymsg;
                int msgbytes;

                if ((msgbytes = msgrcv(_id,&mymsg,sizeof(T),0,0)) < 0) {
                    PLOG_ERROR << "msgrcv: " << strerror(errno) << ":" << _id;
                    return 0;
                }
                else {
                    PLOG_DEBUG_IF(loglevel) << "msgrcv " << msgbytes << " bytes.";
                    PLOG_DEBUG_IF(loglevel) << "sizeof(T) " << sizeof(T) << " bytes.";
                    type = mymsg.mtype;
                    pdata = mymsg.data;
                    return 1;
                }

            }
            return 0;
        }
};

#endif