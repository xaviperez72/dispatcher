#include "checker_pids.h"

checker_pids* checker_pids::_me{nullptr};
bool checker_pids::_forker{false};
std::shared_ptr<keep_running_flags> checker_pids::_p_run = std::make_shared<keep_running_flags>();

void checker_pids::StoppingChildren()
{
    LOG_DEBUG << "Sending signal to children!";
    for(auto &child : _pids)
    {
        kill(child._pid, SIGUSR1);
    }
}

int checker_pids::operator()()
{
    int ret=0;

    _forker = true;       // Important to propagate signal to children and itself

    if(_pids.size() == 0)
    {
        LOG_ERROR << "At least one functor is needed.!!";
        return 1;
    }
    
    // Important to manage signal handler to kill children
    _me = this;

    LOG_DEBUG << "Running checker_pids with " << _pids.size() << " processes.";
    LOG_DEBUG << "_keep_accepting " << _p_run->_keep_accepting << " : _keep_working " << _p_run->_keep_working << " : _forker " << _forker;

    using namespace std::placeholders;    // adds visibility of _1, _2, _3,...

    _previousInterruptHandler_int = signal(SIGINT, &checker_pids::sigterm_func);
    _previousInterruptHandler_usr1 = signal(SIGUSR1, &checker_pids::sigterm_func);
    _previousInterruptHandler_term = signal(SIGTERM, &checker_pids::sigterm_func);

    while(_p_run->_keep_accepting.load()) 
    {
        time(&_dead._last_fork);

        for(auto &process : _pids)
        {
            if( process._pid == _dead._pid ) 
	        {
                if( process._pid != 0 && _dead._last_fork <= process._last_fork + 5 ) 
                {
                    LOG_DEBUG << "_dead._last_fork <= process._last_fork + 5 " << _dead._last_fork  << " <= " << process._last_fork + 5;

                    LOG_DEBUG << "Process " << process._procname << " (pid " << process._pid << ") dead too quick. Stopping";
                    raise(SIGTERM);
                    StoppingChildren();
                    break;
                }

                process._last_fork = _dead._last_fork;
                switch( process._pid = fork() ) 
                {
                    case -1:
                        LOG_ERROR << "fork: " << strerror(errno);
                        raise(SIGTERM);
                        StoppingChildren();
                        break;
                    
                    case 0:                             // Child process
                        _forker = false;                // I am not the forker, I am a child
                        ret=process._caller();          // Call to functor - operator ()
                        return ret;
                        break;

                    default:                               // Parent process
                        LOG_DEBUG << process._procname << " launched with pid " << process._pid;
                        break;
                }
                if(process._pid == -1)
                    break;
            }
        }

        LOG_DEBUG << "waiting - keep_accepting " << _p_run->_keep_accepting;

        _dead._pid = wait(NULL);
        LOG_DEBUG << "wake-up - child dead " << _dead._pid << "... keep_accepting " << _p_run->_keep_accepting;
    }

    
    bool waiting = true;
    while (waiting)
    {
        auto it_pid_alive = std::find_if(_pids.begin(), _pids.end(), [](checker_struct &ch){ return kill(ch._pid,0)!=-1;});
        if(it_pid_alive == _pids.end()) {
            LOG_DEBUG << "All process dead!!";
            waiting = false;
        }
        else {
            LOG_DEBUG << "Still alive, waiting: " << it_pid_alive->_pid;
            _dead._pid = wait(NULL);
            LOG_DEBUG << "_dead._pid:" <<_dead._pid;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    (void)signal(SIGINT,  _previousInterruptHandler_int);
    (void)signal(SIGUSR1, _previousInterruptHandler_usr1);
    (void)signal(SIGTERM, _previousInterruptHandler_term);

    LOG_DEBUG << "Ending checker_pids::operator() - _forker " << _forker;
    return static_cast<int>(_forker);
}

void checker_pids::add(std::function<int()> _call, std::string procname)
{ 
    checker_struct checker{_call,0,0,procname};
    _pids.reserve(_pids.size()+1);
    _pids.emplace_back(checker);
}

void checker_pids::sigterm_func(int s) 
{
    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " _keep_accepting " << _p_run->_keep_accepting << " : _forker " << _forker;
    
    if(_p_run->_keep_accepting && _forker) 
    {
        _p_run->_keep_accepting = false;
        if(_me!=nullptr) {
            _me->StoppingChildren();

            _me = nullptr;
        }
    }

    _p_run->_keep_accepting = false;

    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " _keep_accepting " << _p_run->_keep_accepting << " : _forker " << _forker;
}
