#ifndef PROTOCOL_MSG_H
#define PROTOCOL_MSG_H

namespace protopipe
{
    static constexpr char WEAKUP[1]={'1'};
    static constexpr int LEN_WEAKUP=sizeof(WEAKUP);
    static constexpr long TYPE_WEAKUP=1;
    static constexpr long MAX_MSG_SIZE=10000;
    static char GETWEAKUP[1];
}

namespace protomsg
{
    static constexpr long TYPE_NORMAL_MSG=2;
    typedef struct
    {
	    long 		mtype;
        int         q_write;
        int         terf;
        int         terl;
        int         idx;
        char        guid[10];
        char        pid[10];
        char        aid[8];
        char        cabx[8];
        char        msg[1];
    } st_protomsg;
}

#endif