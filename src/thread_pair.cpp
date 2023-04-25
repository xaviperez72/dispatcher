#include "thread_pair.h"

using namespace std;

thread_pair::thread_pair(int write_queue_id, MessageQueue common_queue, int idx, 
        std::shared_ptr<checker_pids> shpt_pids, std::shared_ptr<connections> shpt_conn, 
        std::shared_ptr<signal_synch> shpt_sigsyn)
{
    _shpt_sigsyn = shpt_sigsyn;
    _write_queue = MessageQueue(write_queue_id);

    assert(_write_queue && "write_queue not operative.");
    assert(common_queue && "read_queue (common queue) not operative.");

    // _accept_mutex = std::make_shared<mutex>();

    _common_queue = common_queue;
    _idx = idx;
    _sharedptr_pids = shpt_pids;
    _p_cur_connections = shpt_conn;

    if (pipe(_pipe) < 0 ) {
        assert(false && "Cannot create a pipe");
	}

    std::thread(&thread_pair::reader_thread, this, idx).detach();
    std::thread(&thread_pair::writer_thread, this, idx).detach();
}


void thread_pair::reader_thread(int idx)
{
    LOG_DEBUG << "reader_thread " << idx;

    while(_sharedptr_pids->_keep_accepting.load()==true) 
    {
        std::unique_lock<std::mutex> lck(_shpt_sigsyn->_cv_mutex);

        auto shpt_pids = _sharedptr_pids;
        // wait for up to 10 seconds
        _shpt_sigsyn->_cv.wait_for(lck, std::chrono::seconds(10),
              [&shpt_pids]() { return !shpt_pids->_keep_accepting.load(); });
        
        // STEP 1 - 
        
    }

    LOG_DEBUG << "Ending reader_thread " << idx;
}

void thread_pair::writer_thread(int idx)
{
    LOG_DEBUG << "writer_thread " << idx;

    while(_sharedptr_pids->_keep_working.load()==true) 
    {
        std::unique_lock<std::mutex> lck(_shpt_sigsyn->_cv_mutex);

        auto shpt_pids = _sharedptr_pids;
        // wait for up to 10 seconds
        _shpt_sigsyn->_cv.wait_for(lck, std::chrono::seconds(10),
              [&shpt_pids]() { return !shpt_pids->_keep_working.load(); });

        // STEP 1 - ReadFromQueue  (idx_con on message)

        // STEP 2 - 
        
    }

    LOG_DEBUG << "Ending writer_thread " << idx;
}

int thread_pair::add_sockdata(socket_data_t sdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    _sockdata.emplace_back(sdt);
    
    return 0;
}

int thread_pair::remove_sockdata(const socket_data_t &sdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    std::list<socket_data_t>::iterator findIter = std::find(_sockdata.begin(), _sockdata.end(), sdt);

    if(findIter == _sockdata.end())
        return -1;
    else
        return 0;
}

int thread_pair::get_sockdata_list(list<socket_data_t> &lsdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    lsdt = _sockdata;
    
    return 0;
}
