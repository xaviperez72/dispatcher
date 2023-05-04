#ifndef PROTOCOL_MSG_H
#define PROTOCOL_MSG_H

namespace protopipe
{
    static constexpr char WEAKUP_PIPE[1]={'1'};
    static constexpr char ENDING_PIPE[1]={'2'};
    static constexpr int LEN_PIPEMSG=1;
    static char GETPIPEMSG[1];
}

namespace protomsg
{
    static constexpr long TYPE_WEAKUP_MSG=1;
    static constexpr long TYPE_NORMAL_MSG=2;
    static constexpr long TYPE_ENDING_MSG=3;
    static constexpr long MAX_MSG_SIZE=10000;
    struct st_protomsg
    {
	    long 		mtype{0};
        int         q_write{0};
        int         terf{0};
        int         terl{0};
        int         idx{0};
        char        guid[10]{0};
        char        pid[10]{0};
        char        aid[8]{0};
        char        cabx[8]{0};
        char        msg[1]{0};

        friend std::ostream& operator<<( std::ostream& os, const st_protomsg& v )
        {
	        os << v.mtype << ":" << v.q_write << ":" << v.terf << ":" << v.terl << ":" << v.idx << ":" << \
                v.guid << ":" << v.pid << ":" << v.aid << ":" << v.cabx;
	        return os;
        }
    };
}

#endif