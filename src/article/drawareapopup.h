// ライセンス: GPL2

// ポップアップの表示部

#ifndef _DRAWAREAPOPUP_H
#define _DRAWAREAPOPUP_H

#include "drawareabase.h"

namespace ARTICLE
{
    class DrawAreaPopup : public ARTICLE::DrawAreaBase
    {

      public:

        // show_abone == true ならあぼーんされたスレも表示
        DrawAreaPopup( const std::string& url, bool show_abone );

      protected:

        // レイアウト実行
        virtual bool exec_layout();

        // リサイズした
        virtual bool slot_configure_event( GdkEventConfigure* event );
    };
}

#endif

