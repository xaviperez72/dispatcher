#include "checker_pids.h"

checker_pids* checker_pids::_me{nullptr};
std::atomic<bool> checker_pids::_keep_accepting{true};
std::atomic<bool> checker_pids::_keep_working{true};
std::atomic<bool> checker_pids::_forker{true};
    
void checker_pids::StoppingChildren()
{
    LOG_DEBUG << "Sending signal to children!";
    for(auto child : _pids)
    {
        kill(child._pid, SIGUSR1);
    }
}

int checker_pids::operator()()
{
    checker_pids::_forker = true;       // Important to propagate signal to children and itself

    if(_pids.size() == 0)
    {
        LOG_ERROR << "At least one functor is needed.!!";
        return 1;
    }
    
    // Important to manage signal handler to kill children
    _me = this;

    LOG_DEBUG << "Running checker_pids with " << _pids.size() << " processes.";
    LOG_DEBUG << "_keep_accepting " << _keep_accepting << " : _keep_working " << _keep_working << " : _forker " << _forker;

    using namespace std::placeholders;    // adds visibility of _1, _2, _3,...

    _previousInterruptHandler_int = signal(SIGINT, &checker_pids::sigterm_func);
    _previousInterruptHandler_usr1 = signal(SIGUSR1, &checker_pids::sigterm_func);
    _previousInterruptHandler_term = signal(SIGTERM, &checker_pids::sigterm_func);

    while(_keep_accepting) 
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
                    
                    case 0:                                // Child process
                        checker_pids::_forker = false;     // I am not the forker, I am a child
                        sleep(1);
                        return process._caller();         // Call to functor - operator ()

                    default:                               // Parent process
                        LOG_DEBUG << process._procname << " launched with pid " << process._pid;
                        sleep(1);
                        break;
                }
                if(process._pid == -1)
                    break;
            }
        }

        LOG_DEBUG << "waiting - keep_accepting " << _keep_accepting;

        _dead._pid = wait(NULL);
        sleep(1);
        LOG_DEBUG << "wake-up - child dead " << _dead._pid << "... keep_accepting " << _keep_accepting;
    }

    (void)signal(SIGINT, _previousInterruptHandler_int);
    (void)signal(SIGUSR1, _previousInterruptHandler_usr1);
    (void)signal(SIGTERM, _previousInterruptHandler_term);

    LOG_DEBUG << "end checker_pids::operator() - keep_accepting " << _keep_accepting;
    return static_cast<int>(checker_pids::_forker);
}

void checker_pids::add(std::function<int()> _call, std::string procname)
{ 
    checker_struct checker{std::move(_call),0,0,procname};
    
    _pids.emplace_back(std::move(checker));
}

void checker_pids::sigterm_func(int s) 
{
    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " _keep_accepting " << checker_pids::_keep_accepting << " : _forker " << _forker;
    
    if( (checker_pids::_keep_accepting || checker_pids::_keep_working) && _forker) 
    {
        checker_pids::_keep_accepting = false;
        checker_pids::_keep_working = false;
        if(_me!=nullptr) {
            LOG_DEBUG << "Before calling _me->StoppingChildren(); (fingers crossed)";
            _me->StoppingChildren();
            LOG_DEBUG << "Afet calling _me->StoppingChildren(); (fingers crossed)";
            _me = nullptr;
        }
    }

    checker_pids::_keep_accepting = false;
    checker_pids::_keep_working = false;

    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " _keep_accepting " << checker_pids::_keep_accepting << " : _forker " << _forker;
}
