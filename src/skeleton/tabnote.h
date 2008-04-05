// ライセンス: GPL2
//
// タブ用の Notebook
//

#ifndef _TABNOTE_H
#define _TABNOTE_H

#include <gtkmm.h>

namespace SKELETON
{
    class TabLabel;

    // タブのクリック
    typedef sigc::signal< bool, GdkEventButton* > SIG_BUTTON_PRESS;
    typedef sigc::signal< bool, GdkEventButton* > SIG_BUTTON_RELEASE;

    // マウス移動
    typedef sigc::signal< void > SIG_TAB_MOTION_EVENT;
    typedef sigc::signal< void > SIG_TAB_LEAVE_EVENT;

    // D&D
    typedef sigc::signal< void, const int, const int, const int, const int > SIG_TAB_DRAG_MOTION;

    // タブ用の Notebook
    class TabNotebook : public Gtk::Notebook
    {
        SIG_BUTTON_PRESS m_sig_button_press;
        SIG_BUTTON_RELEASE m_sig_button_release;

        SIG_TAB_MOTION_EVENT m_sig_tab_motion_event;
        SIG_TAB_LEAVE_EVENT m_sig_tab_leave_event;

        SIG_TAB_DRAG_MOTION m_sig_tab_drag_motion;

        // タブ幅計算用layout
        Glib::RefPtr< Pango::Layout > m_layout_tab;

        int m_pre_width;
        bool m_fixtab;
        int m_tab_mrg;
        int m_ythickness;

      public:

        SIG_BUTTON_PRESS sig_button_press(){ return m_sig_button_press; }
        SIG_BUTTON_RELEASE sig_button_release(){ return m_sig_button_release; }

        SIG_TAB_MOTION_EVENT sig_tab_motion_event(){ return  m_sig_tab_motion_event; }
        SIG_TAB_LEAVE_EVENT sig_tab_leave_event(){ return m_sig_tab_leave_event; }

        SIG_TAB_DRAG_MOTION sig_tab_drag_motion(){ return m_sig_tab_drag_motion; }

        TabNotebook();

        void clock_in();

        // タブの幅を固定するか(デフォルト false );
        void set_fixtab( bool fix ){ m_fixtab = fix; }

        int append_tab( Widget& tab );
        int insert_tab( Widget& tab, int page );
        void remove_tab( int page );
        void reorder_child( int page1, int page2 );

        // タブ取得
        SKELETON::TabLabel* get_tablabel( int page );

        // タブの文字列取得/セット
        const std::string get_tab_fulltext( int page );
        void set_tab_fulltext( const std::string& str, int page );

        // タブ幅調整
        bool adjust_tabwidth();

        // マウスの下にあるタブの番号を取得
        // タブ上では無いときは-1を返す
        // マウスがタブの右側にある場合はページ数の値を返す
        const int get_page_under_mouse();

      private:

        // 各タブのサイズと座標を取得
        void calc_tabsize();

      protected:

        virtual bool on_expose_event( GdkEventExpose* event );

        // signal_button_press_event と signal_button_release_event は emit されない
        // ときがあるので自前でemitする
        virtual bool on_button_press_event( GdkEventButton* event );
        virtual bool on_button_release_event( GdkEventButton* event );

        virtual bool on_motion_notify_event( GdkEventMotion* event );
        virtual bool on_leave_notify_event( GdkEventCrossing* event );

        virtual bool on_drag_motion( const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
    };


    ///////////////////////////////////////////////////////////////

    class ToolBarNotebook : public Gtk::Notebook
    {
        bool m_show_tab_notebook;
        int m_xthickness;
        int m_ythickness;

      public:
        ToolBarNotebook();
        void set_show_tab_notebook( const bool show );

      protected:
        virtual bool on_expose_event( GdkEventExpose* event );
    };

    ///////////////////////////////////////////////////////////////

    class ViewNotebook : public Gtk::Notebook
    {
        int m_xthickness;
        int m_ythickness;

      public:
        ViewNotebook();

      protected:
        virtual bool on_expose_event( GdkEventExpose* event );
    };
}

#endif
