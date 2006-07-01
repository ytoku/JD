// ライセンス: 最新のGPL

#ifndef _VIEW_H
#define _VIEW_H

#include <gtkmm.h>
#include <string>

#include "tablabel.h"

#include "control.h"

namespace SKELETON
{
    // 自分がポップアップviewの時に(ポップアップウィンドウ経由で)親widgetにhideを依頼するシグナル
    typedef sigc::signal< void > SIG_HIDE_POPUP;    

    // 自分がポップアップviewの時にポップアップウィンドウにリサイズを依頼するシグナル
    typedef sigc::signal< void > SIG_RESIZE_POPUP;
    
    class View : public Gtk::VBox
    {
        SIG_HIDE_POPUP m_sig_hide_popup;
        SIG_RESIZE_POPUP m_sig_resize_popup;

        std::string m_url;

        // notebookのタブに張り付けるラベル
        SKELETON::TabLabel  m_label;

        // クライアント領域の幅、高さ
        int m_width_client;
        int m_height_client;

        // タブに表示する文字列
        // m_str_label にラベルの文字列を入れた後に Adminに set_tablabel コマンドを送ると表示される
        std::string m_str_label;

        // 入力コントローラ
        CONTROL::Control m_control;

        // ポップアップメニュー
        Glib::RefPtr< Gtk::ActionGroup > m_action_group;
        Glib::RefPtr< Gtk::UIManager > m_ui_manager;

        // メインウィンドウのステータスバーに表示する文字
        std::string m_status;

        // true ならマウスジェスチャ使用
        bool m_enable_mg; 

        // オートリロード
        int m_autoreload_mode; // モード
        int m_autoreload_sec; // 何秒おきにリロードするか
        int m_autoreload_counter; // オートリロード用のカウンタ

      protected:

        // UI
        Glib::RefPtr< Gtk::ActionGroup >& action_group(){ return m_action_group; }
        Glib::RefPtr< Gtk::UIManager >& ui_manager(){ return m_ui_manager; }

        // コントローラ
        CONTROL::Control& get_control(){ return m_control; }

        // ステータス
        void set_status( const std::string& status ){ m_status = status; }

        // マウスジェスチャ
        void set_enable_mg( bool mg ){ m_enable_mg = mg; }
        const bool enable_mg() const { return m_enable_mg; }

        // オートリロードのカウンタをインクリメント
        // 指定秒数を越えたら true を返す
        bool inc_autoreload_counter();

        // オートリロードのカウンタをリセット
        void reset_autoreload_counter(); 

    public:

        SIG_HIDE_POPUP sig_hide_popup(){ return m_sig_hide_popup; }
        SIG_RESIZE_POPUP sig_resize_popup(){ return m_sig_resize_popup; }
        
        View( const std::string& url, const std::string& arg1 = std::string(), const std::string& arg2 = std::string() );
        virtual ~View(){}

        virtual const std::string& get_url(){ return m_url; }
        void set_url( const std::string& url ){ m_url = url; }

        // 各view個別のコマンド
        virtual bool set_command( const std::string& command, const std::string& arg = std::string() ){ return true; }

        // コピー用のURL
        virtual const std::string url_for_copy(){ return m_url; }

        // メインウィンドウのステータスバーに表示する文字列
        virtual const std::string& get_status(){ return m_status; }

        // クライアント領域の幅、高さ
        virtual const int width_client(){ return m_width_client; }
        virtual const int height_client(){ return m_height_client; }
        void set_width_client( int val ){ m_width_client = val; }
        void set_height_client( int val ){ m_height_client = val; }

        // notebookのタブに張り付けるラベル
        SKELETON::TabLabel& get_tab_label() { return m_label; }

        // shutdown( SIGHUP )用
        virtual void shutdown(){}

        // クロック入力
        // clock_in()はビューがアクティブのときに呼び出される
        // clock_in_always()は常に呼び出されるので重い処理を含めてはいけない
        virtual void clock_in(){};
        virtual void clock_in_always();

        virtual void reload(){}
        virtual void set_autoreload_mode( int mode, int sec );
        virtual void stop(){}
        virtual void show_view(){}
        virtual void redraw_view(){}
        virtual void relayout(){}
        virtual void update_view(){}
        virtual void update_finish(){}        
        virtual void focus_view(){}
        virtual void focus_out(){ m_control.MG_reset(); }
        virtual void close_view(){}
        virtual void delete_view(){}
        virtual void update_item( const std::string& ){}
        virtual void operate_view( const int& ){}
        virtual void goto_top(){}
        virtual void goto_bottom(){}
    };
}

#endif
