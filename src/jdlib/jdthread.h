// ライセンス: GPL2

// スレッドクラス

#ifndef _JDTHREAD_H
#define _JDTHREAD_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_GTHREAD
#include <gtkmm.h>
#define JDTH_TYPE Glib::Thread*
#define JDTH_ISRUNNING( pth ) ( ( pth ) != NULL )
#define JDTH_CLEAR( pth ) ( ( pth ) = NULL )
#else
#include <pthread.h>
#define JDTH_TYPE pthread_t
#define JDTH_ISRUNNING( pth ) ( pth )
#define JDTH_CLEAR( pth ) ( pth = 0 )
#endif

typedef void* ( *STARTFUNC )( void * );

enum
{
    DEFAULT_STACKSIZE = 64
};

namespace JDLIB
{
    enum
    {
        DETACH = true,
        NODETACH = false
    };

    class Thread
    {
        JDTH_TYPE m_thread;

#ifdef USE_GTHREAD
        static void slot_wrapper( STARTFUNC func, void* arg );
#endif

    public:

        Thread();
        virtual ~Thread();

        const bool is_running() const { return JDTH_ISRUNNING( m_thread ); }

        // スレッド作成
        const bool create( STARTFUNC func , void * arg, const bool detach, const int stack_kbyte = DEFAULT_STACKSIZE );

        const bool join();
    };
}

#endif
