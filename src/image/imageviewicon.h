// ライセンス: GPL2

//
// 画像アイコンクラス
//

#ifndef _IMAGEVIEWICON_H
#define _IMAGEVIEWICON_H

#include "imageviewbase.h"

namespace IMAGE
{
    class ImageViewIcon : public ImageViewBase
    {
        Gtk::EventBox* m_event_frame;

      public:
        ImageViewIcon( const std::string& url );
        virtual ~ImageViewIcon();

        virtual void clock_in();
        virtual void focus_view();
        virtual void focus_out();
        virtual void show_view();

      protected:

        virtual Gtk::Menu* get_popupmenu( const std::string& url );
        virtual bool slot_scroll_event( GdkEventScroll* event );

      private:

        virtual void switch_icon();

        void slot_drag_begin( const Glib::RefPtr<Gdk::DragContext>& context );
        void slot_drag_data_get( const Glib::RefPtr<Gdk::DragContext>& context,
                                 Gtk::SelectionData& selection_data, guint info, guint time );
        void slot_drag_data_received( const Glib::RefPtr<Gdk::DragContext>& context, int x, int y,
                                      const Gtk::SelectionData& selection_data, guint info, guint time );
        void slot_drag_end( const Glib::RefPtr< Gdk::DragContext >& context );
    };
}

#endif

