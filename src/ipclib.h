#ifndef IPCLIB_H_
#define IPCLIB_H_

#include "common.h"
#include "protocol_msg.h"
#include <string>
#include <memory>

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


/**
 * Common IPC object (ipclib.h)
 */
class Ipc 
{
    key_t _key{IPC_PRIVATE};        // Key used to connect/create.
	int _id{-1};                    // ID
	bool _deleteOnExit{false};      // Flag to delete on exit
    int _flags{0666};               // Flags on connect/create
    bool _ok{false};                // Flag true=operative Ipc, false=no operative Ipc

public:
    Ipc() = default;
   /**
    * Ipc Constructor
    *
    * @param key (default key=IPC_PRIVATE) 
    * @param deleteOnExit = ( false = don't remove semaphore on destruction. true = remove it)
    */
    explicit Ipc(key_t key, bool deleteOnExit):_key{key},_deleteOnExit{deleteOnExit}{}
    Ipc(Ipc const &) = default;
    Ipc(Ipc &&) = default;
    Ipc& operator=(Ipc const &) = default;
    Ipc& operator=(Ipc &&) = default;
    virtual ~Ipc() = default;

    // Enable delete IPC on exit.
    void EnableDelete() { 	_deleteOnExit = true; }     
    
    // Disable delete IPC on exit.
    void DisableDelete() { 	_deleteOnExit = false; }

    // Get key id
    key_t getkey() const { return _key; }

    // Get id
    int getid() const { return _id; }

    // Get flgas
    int getflags() const { return _flags; }

    // Get deleteOnExit
    key_t getDeleteOnExit() const { return _deleteOnExit; }

    // Set key id
    void setkey(key_t key) { _key=key; }

    // Set id
    void setid(int id) { _id=id; }

    // Set ok
    void setok(bool ok) { _ok=ok; }

    // Set flags
    void setflags(int flags) { _flags=flags; }

    // Operator bool for IPC's
    virtual operator bool() const { return _ok == true; }
};



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
 * Semaphore Object IPC (ipclib.h)
 */
class Semaphore final : public Ipc 
{
    int _nsems;                         // Number of semaphores
    std::vector<int> _sem_val;          // Semaphore value vector
    struct sembuf _sop;                 // To operate on semaphore
    semun _st;                          
	static constexpr int LOCK = -1;
    static constexpr int UNLOCK = 1;
    // SetS a LOCK or UNLOCK.
    int set(const int op);              
    // Run semctl to get all info.
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
    /**
    * Semaphore Constructor
    *
    * @param semid Id for an existing Semaphore
    */
    Semaphore(int semid);

    Semaphore() = default;
    Semaphore(Semaphore const &) = default;
    Semaphore(Semaphore &&) = default;
    Semaphore& operator=(Semaphore const &) = default;
    Semaphore& operator=(Semaphore &&) = default;
    ~Semaphore();
    
    // Lock semaphore
    int Lock(){ int ret=set(LOCK); PLOG_DEBUG_IF(loglevel) << "Semaphore Locked!"; return ret; }

    // Unlock semaphore
    int Unlock(){ int ret=set(UNLOCK); PLOG_DEBUG_IF(loglevel) << "Semaphore Unlocked!"; return ret; }
};


/**
 * Shared Memory Object IPC (ipclib.h)
 */
class SharedMemory final : public Ipc 
{
	void* _shmaddr{nullptr};                // Shared memory address. 
	int _size{4};                           // Size of shared memory
    shmid_ds _st;                           // Info of the shared memory
    // Run semctl to get all info.
    int get_stat(int shmid);                

public: 
   /**
    * SharedMemory Constructor
    *
    * @param key (default IPC_PRIVATE) 
    * @param size Memory size 
    * @param deleteOnExit = false (don't remove semaphore on destruction. true = remove it)
    */
	SharedMemory(const key_t key, int size, bool deleteOnExit);

    /**
    * SharedMemory Constructor
    *
    * @param shmid Id for an existing SharedMemory
    */
    SharedMemory(int shmid);

    SharedMemory() = default;
    SharedMemory(SharedMemory const &) = default;
    SharedMemory(SharedMemory &&) = default;
    SharedMemory& operator=(SharedMemory const &) = default;
    SharedMemory& operator=(SharedMemory &&) = default;
    ~SharedMemory();

    // Get shared memory address
    void *getaddr() const { return _shmaddr; }
    // Get number of processes attached to shared memory
    int get_nattach();
};


/**
 * Message Queue IPC (ipclib.h)
 */
class MessageQueue final : public Ipc 
{
    msqid_ds _st;                   // Message queue info
    int _size;                      // Size of message queue
    // Run msgctl to get all info.
    int get_stat(int msgid);
public:
   
   /**
    * MessageQueue Constructor
    *
    * @param key (default IPC_PRIVATE) 
    * @param deleteOnExit = false (don't remove semaphore on destruction. true = remove it)
    */
    MessageQueue(const key_t key, bool deleteOnExit);
    
    /**
    * MessageQueue Constructor
    *
    * @param msgid Id for an existing MessageQueue
    */
    MessageQueue(int msgid);
    
    MessageQueue() = default;
    MessageQueue(MessageQueue const &) = default;
    MessageQueue(MessageQueue &&) = default;
    MessageQueue& operator=(MessageQueue const &) = default;
    MessageQueue& operator=(MessageQueue &&) = default;
    ~MessageQueue();

    // Sends a protomsg::st_protomsg to the message queue (IPC)
    int send(protomsg::st_protomsg *p_protomsg, std::string &pdata);
    // Receives a protomsg::st_protomsg of the message queue (IPC)
    int rcv(protomsg::st_protomsg *p_protomsg, std::string &pdata);

};

#endif