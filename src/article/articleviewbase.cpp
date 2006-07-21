// ライセンス: 最新のGPL

//#define _DEBUG
#include "jddebug.h"

#include "articleadmin.h"
#include "articleviewbase.h"
#include "articleview.h"
#include "drawareamain.h"
#include "toolbar.h"

#include "jdlib/miscutil.h"

#include "dbtree/articlebase.h"
#include "dbtree/interface.h"

#include "dbimg/imginterface.h"

#include "skeleton/popupwin.h"

#include "config/globalconf.h"

#include "global.h"
#include "httpcode.h"
#include "command.h"
#include "session.h"
#include "viewfactory.h"
#include "sharedbuffer.h"
#include "cache.h"
#include "prefdiagfactory.h"

#include <sstream>

#ifndef MAX
#define MAX( a, b ) ( a > b ? a : b )
#endif

#ifndef MIN
#define MIN( a, b ) ( a < b ? a : b )
#endif

using namespace ARTICLE;



ArticleViewBase::ArticleViewBase( const std::string& url )
    : SKELETON::View( url ),
      m_url_article( url ),
      m_toolbar( 0 ),
      m_popup_win( 0 ),
      m_popup_shown( 0 ),
      m_number_popup_shown( 0 ),
      m_enable_menuslot( true ),
      m_current_bm( 0 )
{
    clear();

    // マウスジェスチャ可能
    SKELETON::View::set_enable_mg( true );

    // コントロールモード設定
    SKELETON::View::get_control().set_mode( CONTROL::MODE_ARTICLE );
}



ArticleViewBase::~ArticleViewBase()
{
#ifdef _DEBUG    
    std::cout << "ArticleViewBase::~ArticleViewBase : " << get_url() << std::endl;
#endif

    clear();
}



//
// コピー用URL( readcgi型 )
//
// メインウィンドウのURLバーなどに表示する)
//
const std::string ArticleViewBase::url_for_copy()
{
    return DBTREE::url_readcgi( m_url_article, 0, 0 );
}


JDLIB::RefPtr_Lock< DBTREE::ArticleBase >& ArticleViewBase::get_article()
{
    assert( m_article );
    return  m_article;
}


DrawAreaBase* ArticleViewBase::drawarea()
{
    assert( m_drawarea );
    return m_drawarea;
}


DrawAreaBase* ArticleViewBase::create_drawarea()
{
    return Gtk::manage( new ARTICLE::DrawAreaMain( m_url_article ) );
}


//
// メンバ変数初期化
//
void ArticleViewBase::clear()
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::clear " << get_url() << std::endl;
#endif    

    hide_popup( true );
    delete_popup();

    m_popupmenu_shown = false;
}



//
// セットアップ
//
// 各派生ビューで初期設定が済んだ後に呼ばれる
// 
void ArticleViewBase::setup_view()
{
#ifdef _DEBUG    
    std::cout << "ArticleViewBase::setup_view " << get_url() << " url_article = " << m_url_article << std::endl;
#endif

    clear();
    
    m_article = DBTREE::get_article( m_url_article );
    m_drawarea = create_drawarea();
    assert( m_article );
    assert( m_drawarea );

    m_drawarea->sig_leave_notify().connect(  sigc::mem_fun( *this, &ArticleViewBase::slot_leave_drawarea ) );
    m_drawarea->sig_button_press().connect(  sigc::mem_fun( *this, &ArticleViewBase::slot_button_press_drawarea ));
    m_drawarea->sig_button_release().connect(  sigc::mem_fun( *this, &ArticleViewBase::slot_button_release_drawarea ));
    m_drawarea->sig_scroll_event().connect(  sigc::mem_fun( *this, &ArticleViewBase::slot_scroll_drawarea ));
    m_drawarea->sig_motion_notify().connect(  sigc::mem_fun( *this, &ArticleViewBase::slot_motion_notify_drawarea ) );
    m_drawarea->sig_key_press().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_key_press_drawarea ) );
    m_drawarea->sig_key_release().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_key_release_drawarea ) );    
    m_drawarea->sig_on_url().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_on_url ) );
    m_drawarea->sig_leave_url().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_leave_url ) );

    pack_widget();
    setup_action();

    show_all_children();    
}


//
// ツールバーやスクロールバーのパッキング
//
void ArticleViewBase::pack_widget()
{
    // ツールバーの設定
    m_toolbar = Gtk::manage( new ArticleToolBar() );
    m_toolbar->m_button_close.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::close_view ) );
    m_toolbar->m_button_reload.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::reload ) );
    m_toolbar->m_button_write.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_write ) );
    m_toolbar->m_button_delete.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_delete ) );        
    m_toolbar->m_button_board.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_open_board ) );
    m_toolbar->m_button_favorite.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_favorite ) );
    m_toolbar->m_button_stop.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::stop ) );
    m_toolbar->m_button_preferences.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_preferences ) );
    m_toolbar->m_button_open_search.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_open_search ) );

    // 検索バー
    m_toolbar->m_entry_search.signal_activate().connect( sigc::mem_fun( *this, &ArticleViewBase::slot_active_search ) );
    m_toolbar->m_entry_search.signal_operate().connect( sigc::mem_fun( *this, &ArticleViewBase::slot_entry_operate ) );
    m_toolbar->m_button_close_search.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_close_search ) );
    m_toolbar->m_button_up_search.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_up_search ) );
    m_toolbar->m_button_down_search.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_down_search ) );
    m_toolbar->m_button_drawout_and.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_drawout_and ) );
    m_toolbar->m_button_drawout_or.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_drawout_or ) );
    m_toolbar->m_button_clear_hl.signal_clicked().connect( sigc::mem_fun(*this, &ArticleViewBase::slot_push_claar_hl ) );

    pack_start( *m_toolbar, Gtk::PACK_SHRINK, 2 );
    pack_start( *m_drawarea, Gtk::PACK_EXPAND_WIDGET, 2 );
}




//
// アクション初期化
//
void ArticleViewBase::setup_action()
{
    // アクショングループを作ってUIマネージャに登録
    action_group() = Gtk::ActionGroup::create();
    action_group()->add( Gtk::Action::create( "BookMark", "ブックマーク設定/解除"), sigc::mem_fun( *this, &ArticleViewBase::slot_bookmark ) );
    action_group()->add( Gtk::Action::create( "OpenBrowser", "ブラウザで開く"), sigc::mem_fun( *this, &ArticleViewBase::slot_open_browser ) );
    action_group()->add( Gtk::Action::create( "Google", "googleで検索"), sigc::mem_fun( *this, &ArticleViewBase::slot_search_google ) );
    action_group()->add( Gtk::Action::create( "CopyURL", "URLをコピー"), sigc::mem_fun( *this, &ArticleViewBase::slot_copy_current_url ) );
    action_group()->add( Gtk::Action::create( "CopyID", "IDコピー"), sigc::mem_fun( *this, &ArticleViewBase::slot_copy_id ) );
    action_group()->add( Gtk::Action::create( "Copy", "Copy"), sigc::mem_fun( *this, &ArticleViewBase::slot_copy_selection_str ) );
    action_group()->add( Gtk::Action::create( "WriteRes", "レスする" ),sigc::mem_fun( *this, &ArticleViewBase::slot_write_res ) );
    action_group()->add( Gtk::Action::create( "QuoteRes", "参照レスする"),sigc::mem_fun( *this, &ArticleViewBase::slot_quote_res ) );
    action_group()->add( Gtk::Action::create( "CopyRes", "レスをコピー"),
                         sigc::bind< bool >( sigc::mem_fun( *this, &ArticleViewBase::slot_copy_res ), false ) );
    action_group()->add( Gtk::Action::create( "CopyResRef", "参照コピー"),
                         sigc::bind< bool >( sigc::mem_fun( *this, &ArticleViewBase::slot_copy_res ), true ) );
    action_group()->add( Gtk::Action::create( "Delete", "削除する"), sigc::mem_fun( *this, &ArticleViewBase::delete_view ) );
    action_group()->add( Gtk::Action::create( "Favorite", "お気に入りに登録する"), sigc::mem_fun( *this, &ArticleViewBase::slot_favorite ) );
    action_group()->add( Gtk::Action::create( "Preference", "プロパティ"), sigc::mem_fun( *this, &ArticleViewBase::slot_push_preferences ) );
    action_group()->add( Gtk::Action::create( "PreferenceImage", "画像のプロパティ"), sigc::mem_fun( *this, &ArticleViewBase::slot_preferences_image ) );


    // 抽出系
    action_group()->add( Gtk::Action::create( "Drawout_Menu", "抽出" ) );
    action_group()->add( Gtk::Action::create( "DrawoutWord", "キーワード抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_selection_str ) );
    action_group()->add( Gtk::Action::create( "DrawoutRes", "レス抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_res ) );
    action_group()->add( Gtk::Action::create( "DrawoutID", "ID抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_id ) );
    action_group()->add( Gtk::Action::create( "DrawoutBM", "ブックマーク抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_bm ) );
    action_group()->add( Gtk::Action::create( "DrawoutURL", "URL抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_url ) );
    action_group()->add( Gtk::Action::create( "DrawoutRefer", "参照抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_refer ) );
    action_group()->add( Gtk::Action::create( "DrawoutAround", "周辺抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_around ) );
    action_group()->add( Gtk::Action::create( "DrawoutTmp", "テンプレート抽出"), sigc::mem_fun( *this, &ArticleViewBase::slot_drawout_tmp ) );

    // あぼーん系
    action_group()->add( Gtk::Action::create( "AboneID", "NG IDに追加"), sigc::mem_fun( *this, &ArticleViewBase::slot_abone_id ) );
    action_group()->add( Gtk::Action::create( "AboneName", "NG 名前に追加"), sigc::mem_fun( *this, &ArticleViewBase::slot_abone_name ) );
    action_group()->add( Gtk::Action::create( "AboneWord", "NG ワードに追加"), sigc::mem_fun( *this, &ArticleViewBase::slot_abone_word ) );

    // 設定
    action_group()->add( Gtk::Action::create( "Setting_Menu", "設定" ) );
    action_group()->add( Gtk::ToggleAction::create( "TranspAbone", "透明あぼ〜ん", std::string(), false ),
                         sigc::mem_fun( *this, &ArticleViewBase::slot_toggle_abone_transparent ) );
    action_group()->add( Gtk::ToggleAction::create( "ChainAbone", "連鎖あぼ〜ん", std::string(), false ),
                         sigc::mem_fun( *this, &ArticleViewBase::slot_toggle_abone_chain ) );


    // 移動系
    action_group()->add( Gtk::Action::create( "Move_Menu", "移動" ) );
    action_group()->add( Gtk::Action::create( "Home", "Home"), sigc::mem_fun( *this, &ArticleViewBase::goto_top ) );
    action_group()->add( Gtk::Action::create( "GotoNew", "GotoNew"), sigc::mem_fun( *this, &ArticleViewBase::goto_new ) );
    action_group()->add( Gtk::Action::create( "End", "End"), sigc::mem_fun( *this, &ArticleViewBase::goto_bottom ) );
    action_group()->add( Gtk::Action::create( "PreBookMark", "PreBookMark"), sigc::mem_fun( *this, &ArticleViewBase::slot_pre_bm ) );
    action_group()->add( Gtk::Action::create( "NextBookMark", "NextBookMark"), sigc::mem_fun( *this, &ArticleViewBase::slot_next_bm ) );
    action_group()->add( Gtk::Action::create( "Jump", "ジャンプ"), sigc::mem_fun( *this, &ArticleViewBase::slot_jump ) );

    // 画像系
    action_group()->add( Gtk::Action::create( "Cancel_Mosaic", "モザイク解除"), sigc::mem_fun( *this, &ArticleViewBase::slot_cancel_mosaic ) );
    action_group()->add( Gtk::ToggleAction::create( "ProtectImage", "キャッシュを保護する", std::string(), false ),
                         sigc::mem_fun( *this, &ArticleViewBase::slot_toggle_protectimage ) );
    action_group()->add( Gtk::Action::create( "Delete_Menu", "削除" ) );    
    action_group()->add( Gtk::Action::create( "DeleteImage", "削除する"), sigc::mem_fun( *this, &ArticleViewBase::slot_deleteimage ) );
    action_group()->add( Gtk::Action::create( "SaveImage", "保存"), sigc::mem_fun( *this, &ArticleViewBase::slot_saveimage ) );


    ui_manager() = Gtk::UIManager::create();    
    ui_manager()->insert_action_group( action_group() );

    // レイアウト
    Glib::ustring str_ui =
    "<ui>"

    // 削除ボタン押したときのポップアップ
    "<popup name='popup_menu_delete'>"
    "<menuitem action='Delete'/>"
    "</popup>"

    // レス番号をクリックしたときのメニュー
    "<popup name='popup_menu_res'>"
    "<menuitem action='BookMark'/>"
    "<separator/>"
    "<menuitem action='DrawoutRefer'/>"
    "<menuitem action='DrawoutAround'/>"
    "<menuitem action='DrawoutRes'/>"
    "<separator/>"
    "<menuitem action='WriteRes'/>"
    "<menuitem action='QuoteRes'/>"
    "<separator/>"
    "<menuitem action='OpenBrowser'/>"
    "<separator/>"
    "<menuitem action='CopyURL'/>"
    "<menuitem action='CopyRes'/>"
    "<menuitem action='CopyResRef'/>"
    "<separator/>"
    "<menuitem action='AboneName'/>"
    "</popup>"

    // レスアンカーをクリックしたときのメニュー
    "<popup name='popup_menu_anc'>"
    "<menuitem action='Jump'/>"
    "<menuitem action='DrawoutAround'/>"
    "<menuitem action='DrawoutRes'/>"
    "</popup>"

    // IDをクリックしたときのメニュー
    "<popup name='popup_menu_id'>"
    "<menuitem action='DrawoutID'/>"
    "<menuitem action='CopyID'/>"
    "<separator/>"
    "<menuitem action='AboneID'/>"
    "</popup>"


    // 通常の右クリックメニュー
    "<popup name='popup_menu'>"

    "<menu action='Drawout_Menu'>"
    "<menuitem action='DrawoutWord'/>"
    "<menuitem action='DrawoutBM'/>"
    "<menuitem action='DrawoutURL'/>"
    "<menuitem action='DrawoutTmp'/>"
    "</menu>"

    "<menu action='Move_Menu'>"
    "<menuitem action='Home'/>"
    "<menuitem action='End'/>"
    "<menuitem action='GotoNew'/>"
    "<menuitem action='PreBookMark'/>"
    "<menuitem action='NextBookMark'/>"
    "</menu>"

    "<separator/>"
    "<menuitem action='OpenBrowser'/>"
    "<menuitem action='Google'/>"

    "<separator/>"
    "<menuitem action='CopyURL'/>"
    "<menuitem action='Copy'/>"

    "<separator/>"
    "<menuitem action='AboneWord'/>"

    "<separator/>"

    "<menu action='Setting_Menu'>"
    "<menuitem action='TranspAbone'/>"
    "<menuitem action='ChainAbone'/>"
    "</menu>"

    "<menuitem action='Preference'/>"

    "</popup>"

    // 画像メニュー
    "<popup name='popup_menu_img'>"
    "<menuitem action='Cancel_Mosaic'/>"
    "<separator/>"
    "<menuitem action='OpenBrowser'/>"
    "<separator/>"
    "<menuitem action='CopyURL'/>"
    "<separator/>"
    "<menuitem action='SaveImage'/>"
    "<separator/>"
    "<menuitem action='ProtectImage'/>"
    "<menu action='Delete_Menu'>"
    "<menuitem action='DeleteImage'/>"
    "</menu>"
    "<separator/>"
    "<menuitem action='PreferenceImage'/>"
    "</popup>"

    "</ui>";

    ui_manager()->add_ui_from_string( str_ui );

    // ポップアップメニューにショートカットキーやマウスジェスチャを表示
    Gtk::Menu* popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu" ) );
    CONTROL::set_menu_motion( popupmenu );
}




//
// drawarea 上にマウスポインタがあったら true
//
bool ArticleViewBase::is_mouse_on_drawarea()
{
    return m_drawarea->is_mouse_on_drawarea();
}


//
// クライアント領域幅
//
const int ArticleViewBase::width_client()
{
#ifdef _DEBUG
    if( m_drawarea ) std::cout << "ArticleViewBase::width_client : " << m_drawarea->width_client() << std::endl;
#endif

    if( m_drawarea ) return m_drawarea->width_client();

    return SKELETON::View::width_client();
}


//
// クライアント領高さ
//
const int ArticleViewBase::height_client()
{
#ifdef _DEBUG
    if( m_drawarea ) std::cout << "ArticleViewBase::height_client : " << m_drawarea->height_client() << std::endl;
#endif

    if( m_drawarea ) return m_drawarea->height_client();

    return SKELETON::View::height_client();
}


//
// コマンド
//
bool ArticleViewBase::set_command( const std::string& command, const std::string& arg )
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::set_command " << get_url() << std::endl
              << "command = " << command << std::endl;
#endif    

    if( command == "append_dat" ) append_dat( arg );
    else if( command == "goto_num" ) goto_num( atoi( arg.c_str() ) );
    else if( command == "delete_popup" ) delete_popup();

    return true;
}


//
// クロック入力
//
// クロックタイマーの本体はコアが持っていて、定期的にadminがアクティブなviewにクロック入力を渡す
//
void ArticleViewBase::clock_in()
{
    assert( m_drawarea );

    View::clock_in();

    // ポップアップが出てたらそっちにクロックを回す
    if( is_popup_shown() && m_popup_win->view() ){
        m_popup_win->view()->clock_in();
        return;
    }

    m_drawarea->clock_in();

    return;
}



//
// 再読み込み
//
void ArticleViewBase::reload()
{
    View::reset_autoreload_counter();

    // DAT落ちしてるとロードしないので状態をリセットしておく
    DBTREE::article_reset_status( m_url_article );
    CORE::core_set_command( "open_article", m_url_article , "true" );
}


//
// ロード停止
//
void ArticleViewBase::stop()
{
    assert( m_article );
    m_article->stop_load();
}


//
// 再描画
//
void ArticleViewBase::redraw_view()
{
    assert( m_drawarea );
    m_drawarea->redraw_view();
}

//
// フォーカスイン
//
void ArticleViewBase::focus_view()
{
    assert( m_drawarea );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::focus_view\n";
#endif

    m_drawarea->focus_view();
    m_drawarea->redraw_view();
}


//
// フォーカスアウト
//
void ArticleViewBase::focus_out()
{
    SKELETON::View::focus_out();

#ifdef _DEBUG
    std::cout << "ArticleViewBase::focus_out " << get_url() << std::endl;
#endif

    m_drawarea->focus_out();

    // フォーカスアウトした瞬間に、子ポップアップが表示されていて、かつ
    // ポインタがその上だったらポップアップは消さない
    if( is_mouse_on_popup() ) return;

    hide_popup();
}


//
// 閉じる
//
void ArticleViewBase::close_view()
{
    if( m_article->is_loading() ){
        Gtk::MessageDialog mdiag( "読み込み中です" );
        mdiag.run();
        return;
    }

    ARTICLE::get_admin()->set_command( "close_currentview" );
}


//
// 記事削除
//
void ArticleViewBase::delete_view()
{
    CORE::core_set_command( "delete_article", m_url_article );
}



//
// viewの操作
//
void ArticleViewBase::operate_view( const int& control )
{
    assert( m_drawarea );

    if( control == CONTROL::None ) return;

    // スクロール系操作
    if( m_drawarea->set_scroll( control ) ) return;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::operate_view control = " << control << std::endl;
#endif    

    // その他の処理
    switch( control ){
            
        // リロード
        case CONTROL::Reload:
            reload();
            break;

            // コピー
        case CONTROL::Copy:
            slot_copy_selection_str();
            break;
        
            // 検索
        case CONTROL::Search:
            open_searchbar( false );
            break;

        case CONTROL::SearchInvert:
            open_searchbar( true );
            break;

        case CONTROL::SearchNext:
            slot_push_down_search();
            break;

        case CONTROL::SearchPrev:
            slot_push_up_search();
            break;

            // 閉じる
        case CONTROL::Quit:
            ARTICLE::get_admin()->set_command( "close_currentview" );
            break;

            // 書き込み
        case CONTROL::WriteMessage:
            slot_push_write();
            break;

            // 削除
        case CONTROL::Delete:
        {
            Gtk::MessageDialog mdiag( "ログを削除しますか？", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL );
            if( mdiag.run() != Gtk::RESPONSE_OK ) return;
            delete_view();
            break;
        }

            // Board に切り替え
        case CONTROL::Left:
            CORE::core_set_command( "switch_board" );
            break;

        case CONTROL::ToggleArticle:
            CORE::core_set_command( "toggle_article" );
            break;

            // image に切り替え
        case CONTROL::Right:
            CORE::core_set_command( "switch_image" );
            break;

        case CONTROL::TabLeft:
            ARTICLE::get_admin()->set_command( "tab_left" );
            break;

        case CONTROL::TabRight:
            ARTICLE::get_admin()->set_command( "tab_right" );
            break;

        case CONTROL::PreBookMark:
            slot_pre_bm();
            break;

        case CONTROL::NextBookMark:
            slot_next_bm();
            break;

            // 親にhideを依頼する and ローディング停止
        case CONTROL::StopLoading:
            stop();
            sig_hide_popup().emit();
            break;
    }
}


//
// 一番上へ
//
void ArticleViewBase::goto_top()
{
    assert( m_drawarea );
    m_drawarea->goto_top();
}



//
// 一番下へ
//
void ArticleViewBase::goto_bottom()
{
    assert( m_drawarea );
    m_drawarea->goto_bottom();
}



//
// num番へジャンプ
//
void ArticleViewBase::goto_num( int num )
{
    assert( m_drawarea );
    m_drawarea->goto_num( num );
}



//
// 新着に移動
//
void ArticleViewBase::goto_new()
{
    assert( m_drawarea );
    m_drawarea->goto_new();
}



//
// 検索バーを開く
//
void ArticleViewBase::open_searchbar( bool invert )
{
    if( m_toolbar ){ 
        m_toolbar->show_searchbar(); 
        m_search_invert = invert;
        m_toolbar->m_entry_search.grab_focus(); 
    }
}



//
// 検索バーを開くボタンを押した
//
void ArticleViewBase::slot_push_open_search()
{
    if( ! m_toolbar->m_searchbar_shown ) open_searchbar( false );
    else slot_push_close_search();
}



//
// 検索バーを隠すボタンを押した
//
void ArticleViewBase::slot_push_close_search()
{
    if( m_toolbar ){
        m_toolbar->hide_searchbar();
        m_drawarea->focus_view();
    }
}


//
// 前を検索
//
void ArticleViewBase::slot_push_up_search()
{
    m_search_invert = true;
    slot_active_search();
    m_drawarea->redraw_view();
}



//
// 次を検索
//
void ArticleViewBase::slot_push_down_search()
{
    m_search_invert = false;
    slot_active_search();
    m_drawarea->redraw_view();
}



//
// 別のタブを開いてキーワード抽出 (AND)
//
void ArticleViewBase::slot_push_drawout_and()
{
    std::string query = m_toolbar->m_entry_search.get_text();
    if( query.empty() ) return;

    CORE::core_set_command( "open_article_keyword" ,m_url_article, query, "false" );
}


//
// 別のタブを開いてキーワード抽出 (OR)
//
void ArticleViewBase::slot_push_drawout_or()
{
    std::string query = m_toolbar->m_entry_search.get_text();
    if( query.empty() ) return;

    CORE::core_set_command( "open_article_keyword" ,m_url_article, query, "true" );
}


//
// ハイライト解除
//
void ArticleViewBase::slot_push_claar_hl()
{
    assert( m_drawarea );

    m_query = std::string();
    m_drawarea->clear_highlight();
}



//
// メッセージ書き込みボタン
//
void ArticleViewBase::slot_push_write()
{
    CORE::core_set_command( "open_message" ,m_url_article, std::string() );
}



//
// 削除ボタン
//
void ArticleViewBase::slot_push_delete()
{
    Gtk::Menu* popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_delete" ) );
    if( popupmenu ) { 
        popupmenu->signal_hide().connect( sigc::mem_fun( *this, &ArticleViewBase::slot_hide_popupmenu ) ); 
        popupmenu->popup( 0, gtk_get_current_event_time() ); 
        m_popupmenu_shown = true;
    }
}


//
// 板を開くボタン
//
void ArticleViewBase::slot_push_open_board()
{
    CORE::core_set_command( "open_board", DBTREE::url_subject( m_url_article ), "true" );
}





//
// 設定ボタン
//
void ArticleViewBase::slot_push_preferences()
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_push_preference\n";
#endif

    SKELETON::PrefDiag* pref= CORE::PrefDiagFactory( CORE::PREFDIAG_ARTICLE, m_url_article );
    pref->run();
    delete pref;
}



//
// 画像プロパティ表示
//
void ArticleViewBase::slot_preferences_image()
{
    if( m_url_tmp.empty() ) return;
    std::string url = m_url_tmp;

    SKELETON::PrefDiag* pref= CORE::PrefDiagFactory( CORE::PREFDIAG_IMAGE, url );
    pref->run();
    delete pref;
}




//
// 前のブックマークに移動
//
void ArticleViewBase::slot_pre_bm()
{
    assert( m_article );

    if( m_current_bm == 0 ) m_current_bm = m_drawarea->seen_current();

    for( int i = m_current_bm -1 ; i >= 1 ; --i ){
        if( m_article->is_bookmarked( i ) ){
            goto_num( i );
            m_current_bm = i;
            return;
        }
    }

    for( int i = m_article->get_number_load() ; i > m_current_bm ; --i ){
        if( m_article->is_bookmarked( i ) ){
            goto_num( i );
            m_current_bm = i;
            return;
        }
    }
}



//
// 後ろのブックマークに移動
//
void ArticleViewBase::slot_next_bm()
{
    assert( m_article );

    if( m_current_bm == 0 ) m_current_bm = m_drawarea->seen_current();

    for( int i = m_current_bm + 1; i <= m_article->get_number_load() ; ++i ){
        if( m_article->is_bookmarked( i ) ){
            goto_num( i );
            m_current_bm = i;
            return;
        }
    }

    for( int i = 1; i <= m_current_bm ; ++i ){
        if( m_article->is_bookmarked( i ) ){
            goto_num( i );
            m_current_bm = i;
            return;
        }
    }
}



//
// ジャンプ
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_jump()
{
    goto_num( atoi( m_str_num.c_str() ) );
}



//
// レスを抽出して表示
//
// num は "from-to"　の形式 (例) 3から10を抽出したいなら "3-10"
// show_title == trueの時は 板名、スレ名を表示
// show_abone == trueの時はあぼーんしたレスも表示する
// 
void ArticleViewBase::show_res( const std::string& num, bool show_title )
{
    assert( m_article );

    // 板名、スレ名
    if( show_title ){

        std::string html;
        std::string tmpstr = DBTREE::board_name( m_url_article );
        if( ! tmpstr.empty() ) html += "[ " + tmpstr + " ] ";

        tmpstr = DBTREE::article_subject( m_url_article );
        if( ! tmpstr.empty() ) html += tmpstr;

        if( ! html.empty() ) append_html( html );
    }

    std::list< int > list_resnum = m_article->get_res_str_num( num );

    if( !list_resnum.empty() ) append_res( list_resnum );
    else if( !show_title ) append_html( "未取得レス" );
}



//
// ID で抽出して表示
// 
void ArticleViewBase::show_id( const std::string& id_name )
{
    assert( m_article );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::show_id " << id_name << std::endl;
#endif
    
    std::list< int > list_resnum = m_article->get_res_id_name( id_name );       

    std::ostringstream comment;
    comment << "ID:" << id_name.substr( strlen( PROTO_ID ) ) << "  " << list_resnum.size() << " 件";
      
    append_html( comment.str() );
    if( ! list_resnum.empty() ) append_res( list_resnum );
}



//
// ブックマークを抽出して表示
//
void ArticleViewBase::show_bm()
{
    assert( m_article );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::show_bm " << std::endl;
#endif
    
    std::list< int > list_resnum = m_article->get_res_bm();

    if( ! list_resnum.empty() ) append_res( list_resnum );
    else append_html( "ブックマークはセットされていません" );
}




//
// URLを含むレスを抽出して表示
// 
void ArticleViewBase::show_res_with_url()
{
    assert( m_article );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::show_res_with_url\n";
#endif

    std::list< int > list_resnum = m_article->get_res_with_url();

    if( ! list_resnum.empty() ) append_res( list_resnum );
    else append_html( "リンクを含むスレはありません" );
}



//
// num 番のレスを参照してるレスを抽出して表示
// 
void ArticleViewBase::show_refer( int num )
{
    assert( m_article );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::show_refer " << num << std::endl;
#endif

    std::list< int > list_resnum = m_article->get_res_reference( num );

    // num 番は先頭に必ず表示
    list_resnum.push_front( num );

    append_res( list_resnum );
}




//
// キーワードで抽出して表示
// 
void ArticleViewBase::drawout_keywords( const std::string& query, bool mode_or )
{
    assert( m_article );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::drawout_keywords " << query << std::endl;
#endif

    std::list< int > list_resnum = m_article->get_res_query( query, mode_or );         

    std::ostringstream comment;
    comment << query << "  " << list_resnum.size() << " 件";

    append_html( comment.str() );
    append_res( list_resnum );

    // ハイライト表示
    std::list< std::string > list_query = MISC::split_line( query );
    m_drawarea->search( list_query, false );
}



//
// html をappend
//
void ArticleViewBase::append_html( const std::string& html )
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::append_html html = " << html << std::endl;
#endif

    assert( m_drawarea );
    m_drawarea->append_html( html );
    m_drawarea->redraw_view();
}



//
// dat をappend
//
void ArticleViewBase::append_dat( const std::string& dat, int num )
{
    assert( m_drawarea );
    m_drawarea->append_dat( dat, num );
    m_drawarea->redraw_view();
}



//
// リストで指定したレスの表示
//
void ArticleViewBase::append_res( std::list< int >& list_resnum )
{
    assert( m_drawarea );
    m_drawarea->append_res( list_resnum );
    m_drawarea->redraw_view();
}



//
// drawareaから出た
//
bool ArticleViewBase::slot_leave_drawarea( GdkEventCrossing* ev )
{
    // クリックしたときやホイールを回すと ev->mode に　GDK_CROSSING_GRAB
    // がセットされてイベントが発生する場合がある
    if( ev->mode == GDK_CROSSING_GRAB ) return false;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_leave_drawarea\n";
#endif

    focus_out();

    return false;
}



//
// drawarea のクリックイベント
//
bool ArticleViewBase::slot_button_press_drawarea( GdkEventButton* event )
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_button_press_drawarea url = " << get_url() << std::endl;
#endif

    // マウスジェスチャ
    SKELETON::View::get_control().MG_start( event );

    return true;
}



//
// drawarea でのマウスボタンのリリースイベント
//
bool ArticleViewBase::slot_button_release_drawarea( std::string url, int res_number, GdkEventButton* event )
{
    /// マウスジェスチャ
    int mg = SKELETON::View::get_control().MG_end( event );

#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_button_release_drawarea mg = " << mg << " url = " << get_url() << std::endl;
#endif

    if( event->type == GDK_BUTTON_RELEASE ){

        if( ! is_mouse_on_popup() ){

            // マウスジェスチャ
            if( mg != CONTROL::None && enable_mg() ) operate_view( mg );
                
            else if( ! click_url( url, res_number, event ) ){

                if( SKELETON::View::get_control().button_alloted( event, CONTROL::PopupmenuButton ) ) show_menu( url );
            }
        }
    }

    return true;
}



//
// drawarea でマウスが動いた
//
bool ArticleViewBase::slot_motion_notify_drawarea( GdkEventMotion* event )
{
    /// マウスジェスチャ
    SKELETON::View::get_control().MG_motion( event );

    return true;
}



//
// drawareaのキープレスイベント
//
bool ArticleViewBase::slot_key_press_drawarea( GdkEventKey* event )
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_key_press_drawarea\n";
#endif

    // ポップアップはキーフォーカスを取れないので親からキー入力を送ってやる
    ArticleViewBase* popup_article = NULL;
    if( is_popup_shown() ) popup_article = dynamic_cast< ArticleViewBase* >( m_popup_win->view() );
    if( popup_article ) return popup_article->slot_key_press_drawarea( event );
    
    operate_view( SKELETON::View::get_control().key_press( event ) );

    return true;
}



//
// drawareaのキーリリースイベント
//
bool ArticleViewBase::slot_key_release_drawarea( GdkEventKey* event )
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_key_release_drawarea\n";
#endif

    // ポップアップはキーフォーカスを取れないのでキー入力を送ってやる
    ArticleViewBase* popup_article = NULL;
    if( is_popup_shown() ) popup_article = dynamic_cast< ArticleViewBase* >( m_popup_win->view() );
    if( popup_article ) return popup_article->slot_key_release_drawarea( event );
   
    return true;
}



//
// drawareaのマウスホイールイベント
//
bool ArticleViewBase::slot_scroll_drawarea( GdkEventScroll* event )
{
    // ポップアップしているときはそちらにイベントを送ってやる
    ArticleViewBase* popup_article = NULL;
    if( is_popup_shown() ) popup_article = dynamic_cast< ArticleViewBase* >( m_popup_win->view() );
    if( popup_article ) return popup_article->slot_scroll_drawarea( event );

    m_drawarea->wheelscroll( event );
    return true;
}



//
// リンクの上にポインタが来た
//
// drawareaのsig_on_url()シグナルとつなぐ
//
void ArticleViewBase::slot_on_url( std::string url, int res_number )
{

#ifdef _DEBUG    
    std::cout << "ArticleViewBase::slot_on_url " << url << std::endl;
#endif

    CORE::VIEWFACTORY_ARGS args;
    SKELETON::View* view_popup = NULL;

    // 画像ポップアップ
    if( DBIMG::is_loadable( url )
        && ( DBIMG::is_loading( url ) || ( DBIMG::get_code( url ) != HTTP_ERR && DBIMG::get_code( url ) != HTTP_INIT ) ) ){

#ifdef _DEBUG
        std::cout << "image " << DBIMG::get_code( url ) << " " << DBIMG::is_loading( url ) << "\n";
#endif

        view_popup = CORE::ViewFactory( CORE::VIEW_IMAGEPOPUP,  url );
    }

    // レスポップアップ
    else if( url.find( PROTO_ANCHORE ) == 0 ){
      
        args.arg1 = url.substr( strlen( PROTO_ANCHORE) );
        args.arg2 = "false"; // 板名、スレ名非表示
        args.arg3 = "false"; // あぼーんレス非表示

#ifdef _DEBUG
        std::cout << "anchore = " << args.arg1 << std::endl;
#endif

        view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPRES, m_url_article, args );
    }

    // あぼーんされたレスをポップアップ表示
    else if( url.find( PROTO_RES ) == 0 ){

        args.arg1 = url.substr( strlen( PROTO_RES ) );
        args.arg2 = "false"; // 板名、スレ名非表示
        args.arg3 = "true"; // あぼーんレス表示

#ifdef _DEBUG
        std::cout << "res = " << args.arg1 << std::endl;
#endif

        if( m_article->get_abone( atoi( args.arg1.c_str() ) ) ){
            view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPRES, m_url_article, args );
        }
    }

    // ID:〜の範囲選択の上にポインタがあるときIDポップアップ
    else if( url.find( PROTO_ID ) == std::string::npos ){

        args.arg1 = PROTO_ID + url.substr( 3 );
        int num_id = m_article->get_num_id_name( args.arg1 );

#ifdef _DEBUG
        std::cout << "num_id = " << num_id << std::endl;
#endif

        if( num_id >= 1 ){
            view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPID, m_url_article, args );
        }
    }

    // その他のリンク
    if( !view_popup ){

        // dat 又は板の場合
        int num_from, num_to;
        std::string url_dat = DBTREE::url_dat( url, num_from, num_to );
        std::string url_subject = DBTREE::url_subject( url );

#ifdef _DEBUG
        std::cout << "url_dat = " << url_dat << std::endl;
        std::cout << "url_subject = " << url_subject << std::endl;
#endif

        // 他スレ
        if( ! url_dat.empty() ){

            num_from = MAX( 1, num_from ); // 最低でも1レス目は表示
            num_to = MAX( num_from, num_to );
            std::stringstream ss_tmp;
            ss_tmp << num_from << "-" << num_to;

            args.arg1 = ss_tmp.str();
            args.arg2 = "true"; // 板名、スレ名表示
            args.arg3 = "false"; // あぼーんレス非表示

            view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPRES, url_dat, args );
        }

        // 板
        else if( ! url_subject.empty() ){

            std::string tmpstr = DBTREE::board_name( url );
            args.arg1 = "[ " + tmpstr + " ] ";

            view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPHTML, m_url_article, args );
        }
    }

    if( view_popup ) show_popup( view_popup );
}



//
// リンクからマウスが出た
//
void ArticleViewBase::slot_leave_url()
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_leave_url\n";
#endif

    hide_popup();
}



//
// リンクをクリック
//
bool ArticleViewBase::click_url( std::string url, int res_number, GdkEventButton* event )
{
    assert( m_article );
    if( url.empty() ) return false;
    if( !res_number ) return false;

    CONTROL::Control& control = SKELETON::View::get_control();
  
#ifdef _DEBUG    
    std::cout << "ArticleViewBase::click_url " << url << std::endl;
#endif

    hide_popup();
      
    // ID クリック
    if( url.find( PROTO_ID ) == 0 ){

        int num_id = m_article->get_num_id_name( res_number );
        m_id_name = m_article->get_id_name( res_number );

        // ID ポップアップ
        if( num_id >= 1 && control.button_alloted( event, CONTROL::PopupIDButton ) ){
            CORE::VIEWFACTORY_ARGS args;
            args.arg1 = m_id_name;
            SKELETON::View* view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPID, m_url_article, args );
            show_popup( view_popup );
        }
        else if( control.button_alloted( event, CONTROL::DrawoutIDButton ) ) slot_drawout_id();
        else if( control.button_alloted( event, CONTROL::PopupmenuIDButton ) ) show_menu( url );
    }

    // BE クリック
    else if( url.find( PROTO_BE ) == 0 ){

        std::stringstream ssurl;
        ssurl << "http://be.2ch.net/test/p.php?i="
              << url.substr( strlen( PROTO_BE ) )
              << "&u=d:"
              << DBTREE::url_readcgi( m_url_article, res_number, 0 );
#ifdef _DEBUG    
        std::cout << "open  " << ssurl.str() << std::endl;
#endif
        if( control.button_alloted( event, CONTROL::OpenBeButton ) ) CORE::core_set_command( "open_url_browser", ssurl.str() );
        else if( control.button_alloted( event, CONTROL::PopupmenuBeButton ) ) show_menu( ssurl.str() );
    }

    // アンカーをクリック
    else if( url.find( PROTO_ANCHORE ) == 0 ){

        // ジャンプ先セット
        m_str_num = url.substr( strlen( PROTO_ANCHORE ) );

#ifdef _DEBUG
        std::cout << "anchor num = " << m_str_num << std::endl;
#endif
        if( control.button_alloted( event, CONTROL::PopupmenuAncButton ) ) show_menu( url );
        else if( control.button_alloted( event, CONTROL::DrawoutAncButton ) ) slot_drawout_around();
    }

    // レス番号クリック
    else if( url.find( PROTO_RES ) == 0 ){

        m_str_num = url.substr( strlen( PROTO_RES ) );
        m_name = m_article->get_name( atoi( m_str_num.c_str() ) );
        m_url_tmp = DBTREE::url_readcgi( m_url_article, atoi( m_str_num.c_str() ), 0 );

        if( control.button_alloted( event, CONTROL::PopupmenuResButton ) ) show_menu( url );

        // ブックマークセット
        else if( control.button_alloted( event, CONTROL::BmResButton ) ) slot_bookmark();

        // 参照ポップアップ表示
        else if( control.button_alloted( event, CONTROL::ReferResButton ) ){

            CORE::VIEWFACTORY_ARGS args;
            args.arg1 = m_str_num;
            SKELETON::View* view_popup = CORE::ViewFactory( CORE::VIEW_ARTICLEPOPUPREFER, m_url_article, args );
            show_popup( view_popup );
        }
    }
  
    // 画像クリック
    else if( DBIMG::is_loadable( url ) ){

        if( control.button_alloted( event, CONTROL::PopupmenuImageButton ) ) show_menu( url );

        else if( ! DBIMG::is_cached( url ) && ! SESSION::is_online() ){
            Gtk::MessageDialog mdiag( "オフラインです" );
            mdiag.run();
        }

        else{

            bool top = true;

            // バックで開く
            if( control.button_alloted( event, CONTROL::OpenBackImageButton ) ) top = false;
        
            // キャッシュに無かったらロード
            if( ! DBIMG::is_cached( url ) ){
                std::stringstream refurl;
                refurl << DBTREE::url_readcgi( m_url_article, res_number, 0 );
                DBIMG::set_refurl( url, refurl.str() );

                DBIMG::download_img( url );
                hide_popup();
                show_popup( CORE::ViewFactory( CORE::VIEW_IMAGEPOPUP,  url ) );
                top = false;
            }

            CORE::core_set_command( "open_image", url );
            if( top ) CORE::core_set_command( "switch_image" );

            m_drawarea->redraw_view();
        }
    }

    // ブラウザで開く
    else if( control.button_alloted( event, CONTROL::ClickButton ) ) CORE::core_set_command( "open_url", url );

    else return false;

    return true;
}



//
// ポップアップが表示されているか
//
const bool ArticleViewBase::is_popup_shown() const
{
    return ( m_popup_win && m_popup_shown );
}


//
// ポップアップが表示されていてかつマウスがその上にあるか
//
const bool ArticleViewBase::is_mouse_on_popup()
{
    if( ! is_popup_shown() ) return false;

    ArticleViewBase* popup_article = dynamic_cast< ArticleViewBase* >( m_popup_win->view() );
    if( ! popup_article ) return false;

    return popup_article->is_mouse_on_drawarea();
}



//
// ポップアップ表示
//
// view にあらかじめ内容をセットしてから呼ぶこと
// viewは SKELETON::PopupWin のデストラクタで削除される
//
void ArticleViewBase::show_popup( SKELETON::View* view )
{
    hide_popup();
    if( !view ) return;

    delete_popup();

    const int mrg = CONFIG::get_margin_popup();;
    
    m_popup_win = new SKELETON::PopupWin( this, view, mrg );
    m_popup_win->signal_leave_notify_event().connect( sigc::mem_fun( *this, &ArticleViewBase::slot_popup_leave_notify_event ) );
    m_popup_win->sig_hide_popup().connect( sigc::mem_fun( *this, &ArticleViewBase::slot_hide_popup ) );
    m_popup_shown = true;
}



//
// 子 popup windowの外にポインタが出た
//
bool ArticleViewBase::slot_popup_leave_notify_event( GdkEventCrossing* event )
{
    slot_hide_popup();
    return true;
}


//
// 子 popup windowからhide依頼が来た
//
void ArticleViewBase::slot_hide_popup()
{
    hide_popup();

    // ポインタがwidgetの外にあったら親に知らせて自分も閉じてもらう
    if( ! is_mouse_on_drawarea() ) sig_hide_popup().emit();
}



//
// popup のhide
//
// force = true ならチェック無しで強制 hide
//
void ArticleViewBase::hide_popup( bool force )
{
    if( ! is_popup_shown() ) return;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::hide_popup force = " << force << " " << get_url() << std::endl;
#endif

    if( ! force ){
        
        // ArticleView をポップアップ表示している場合
        ArticleViewBase* popup_article = NULL;
        popup_article = dynamic_cast< ArticleViewBase* >( m_popup_win->view() );

        if( popup_article ){

            // 孫のpopupが表示されてたらhideしない
            if( popup_article->is_popup_shown() ) return;

            // ポップアップメニューが表示されてたらhideしない
            // ( ポップアップメニューがhideしたときにhideする )
            if( popup_article->is_popupmenu_shown() ) return;

#ifdef _DEBUG
        std::cout << "target = " << popup_article->get_url() << std::endl;
#endif
        }
    }

    m_popup_win->hide();
    m_popup_shown = false;
    m_number_popup_shown = false;
}



//
// ポップアップの削除
//
void ArticleViewBase::delete_popup()
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::delete_popup " << get_url() << std::endl;
#endif

    if( m_popup_win ) delete m_popup_win;
    m_popup_win = NULL;
    m_popup_shown = false;
    m_number_popup_shown = false;
}




//
// ポップアップメニュー表示
//
void ArticleViewBase::show_menu( const std::string& url )
{
    assert( m_article );

#ifdef _DEBUG    
    std::cout << "ArticleViewBase::show_menu " << get_url() << " url = " << url << std::endl;
#endif

    // toggle　アクションを activeにするとスロット関数が呼ばれるので処理しないようにする
    m_enable_menuslot = false;

    // 子ポップアップが表示されていて、かつポインタがその上だったら表示しない
    ArticleViewBase* popup_article = NULL;
    if( is_popup_shown() ) popup_article = dynamic_cast< ArticleViewBase* >( m_popup_win->view() );
    if( popup_article && popup_article->is_mouse_on_drawarea() ) return;
    hide_popup();
    Glib::RefPtr< Gtk::Action > act, act2;
    act = action_group()->get_action( "CopyURL" );
    act2 = action_group()->get_action( "OpenBrowser" );

    // url がセットされてない
    if( url.empty() ) {
        if( act ) act->set_sensitive( false );
        if( act2 ) act2->set_sensitive( false );
        m_url_tmp = std::string();
    }

    // url がセットされている
    else {

        if( act ) act->set_sensitive( true );
        if( act2 ) act2->set_sensitive( true );

        // レス番号クリックの場合
        if( url.find( PROTO_RES ) == 0 ){
            m_url_tmp = DBTREE::url_readcgi( m_url_article, atoi( url.substr( strlen( PROTO_RES ) ).c_str() ), 0 );
        }

        // アンカークリックの場合
        else if( url.find( PROTO_ANCHORE ) == 0 ){
            m_url_tmp = DBTREE::url_readcgi( m_url_article, atoi( url.substr( strlen( PROTO_ANCHORE ) ).c_str() ), 0 );
        }
        
        else m_url_tmp = url;
    }


    // 範囲選択されてない
    std::string str_select = m_drawarea->str_selection();
    act = action_group()->get_action( "Copy" );
    if( act ){
        if( str_select.empty() ) act->set_sensitive( false );
        else act->set_sensitive( true );
    }

    act = action_group()->get_action( "DrawoutWord" );
    if( act ){
        if( str_select.empty() ) act->set_sensitive( false );
        else act->set_sensitive( true );
    }

    act = action_group()->get_action( "Google" );
    if( act ){
        if( str_select.empty() ) act->set_sensitive( false );
        else act->set_sensitive( true );
    }

    act = action_group()->get_action( "AboneWord" );
    if( act ){
        if( str_select.empty() ) act->set_sensitive( false );
        else act->set_sensitive( true );
    }

    // ブックマークがセットされていない
    act = action_group()->get_action( "DrawoutBM" );
    if( act ){
        if( m_article->get_num_bookmark() ) act->set_sensitive( true );
        else act->set_sensitive( false );
    }

    // 透明あぼーん
    act = action_group()->get_action( "TranspAbone" );
    if( act ){

        Glib::RefPtr< Gtk::ToggleAction > tact = Glib::RefPtr< Gtk::ToggleAction >::cast_dynamic( act ); 
        if( m_article->get_abone_transparent() ) tact->set_active( true );
        else tact->set_active( false );
    }

    // 連鎖あぼーん
    act = action_group()->get_action( "ChainAbone" );
    if( act ){

        Glib::RefPtr< Gtk::ToggleAction > tact = Glib::RefPtr< Gtk::ToggleAction >::cast_dynamic( act ); 
        if( m_article->get_abone_chain() ) tact->set_active( true );
        else tact->set_active( false );
    }

    // 画像
    if( DBIMG::is_loadable( url ) ){ 

        // モザイク
        act = action_group()->get_action( "Cancel_Mosaic" );
        if( act ){
            if( DBIMG::is_cached( url ) && DBIMG::get_mosaic( url ) ) act->set_sensitive( true );
            else act->set_sensitive( false );
        }

        // 保護のトグル切替え
        act = action_group()->get_action( "ProtectImage" );
        if( act ){

            if( DBIMG::is_cached( url ) ){

                act->set_sensitive( true );

                Glib::RefPtr< Gtk::ToggleAction > tact = Glib::RefPtr< Gtk::ToggleAction >::cast_dynamic( act ); 
                if( DBIMG::is_protected( url ) ) tact->set_active( true );
                else tact->set_active( false );
            }
            else act->set_sensitive( false );
        }

        // 削除
        act = action_group()->get_action( "DeleteImage" );
        if( act ){

            if( DBIMG::is_cached( url ) && ! DBIMG::is_protected( url ) ) act->set_sensitive( true );
            else act->set_sensitive( false );
        }
    }

    m_enable_menuslot = true;

    // 表示
    Gtk::Menu* popupmenu;

    // レス番号ポップアップメニュー
    if( url.find( PROTO_RES ) == 0 ){
        popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_res" ) );
    }

    //　アンカーポップアップメニュー
    else if( url.find( PROTO_ANCHORE ) == 0 ){
        popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_anc" ) );
    }

    // IDポップアップメニュー
    else if( url.find( PROTO_ID ) == 0 ){
        popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_id" ) );
    }

    // 画像ポップアップメニュー
    else if( DBIMG::is_loadable( url ) ){ 
        popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_img" ) );
    }

    // 通常メニュー
    else popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu" ) );

    if( popupmenu ) {
        popupmenu->signal_hide().connect( sigc::mem_fun( *this, &ArticleViewBase::slot_hide_popupmenu ) ); 
        popupmenu->popup( 0, gtk_get_current_event_time() );
        m_popupmenu_shown = true;
    }

}


//
// ポップアップメニューがhideしたときに呼ばれるslot
//
void ArticleViewBase::slot_hide_popupmenu()
{
    if( ! m_popupmenu_shown ) return;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_hide_popupmenu " << get_url() << std::endl;
#endif

    m_popupmenu_shown = false;

    // もしメニューを消したときにマウスポインタが領域外に
    // あったら自分自身をhide
    if( ! is_mouse_on_drawarea() ) sig_hide_popup().emit();
}




//
// クリップボードに選択文字コピーのメニュー
//
void ArticleViewBase::slot_copy_selection_str()
{
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_copy_selection_str " << get_url() << std::endl;
#endif    

    if( m_drawarea->str_selection().empty() ) return;
    COPYCLIP( m_drawarea->str_selection() );
}



//
// 選択して抽出
//
void ArticleViewBase::slot_drawout_selection_str()
{
    if( m_drawarea->str_selection().empty() ) return;
    CORE::core_set_command( "open_article_keyword" ,m_url_article, m_drawarea->str_selection(), "false" );
}


//
// google検索
//
void ArticleViewBase::slot_search_google()
{
    if( m_drawarea->str_selection().empty() ) return;
    CORE::core_set_command( "search_google" ,"", m_drawarea->str_selection() );
}



//
// ブックマーク設定、解除
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_bookmark()
{
    if( m_str_num.empty() ) return;

    int number = atoi( m_str_num.c_str() );
    bool bookmark = ! DBTREE::is_bookmarked( m_url_article, number );
    DBTREE::set_bookmark( m_url_article, number, bookmark );
    if( bookmark ) m_current_bm = number;
    m_drawarea->redraw_view();
    ARTICLE::get_admin()->set_command( "redraw_views", m_url_article );
}



//
// ポップアップメニューでブラウザで開くを選択
//
void ArticleViewBase::slot_open_browser()
{
    if( m_url_tmp.empty() ) return;
    std::string url = m_url_tmp;

    // 画像、かつキャッシュにある場合
    if( DBIMG::is_loadable( url ) && DBIMG::is_cached( url ) ) url = "file://" + CACHE::path_img( url );

    CORE::core_set_command( "open_url_browser", url );
}



//
// レスをする
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_write_res()
{
    if( m_str_num.empty() ) return;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_write_res number = " << m_str_num << std::endl;
#endif    

    CORE::core_set_command( "open_message" ,m_url_article, ">>" + m_str_num + "\n" );
}


//
// 参照レスをする
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_quote_res()
{
    assert( m_article );
    if( m_str_num.empty() ) return;
    
#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_quote_res number = " << m_str_num << std::endl;
#endif    

    CORE::core_set_command( "open_message" ,m_url_article,
                            ">>" + m_str_num + "\n" + m_article->get_res_str( atoi( m_str_num.c_str() ), true ) + "\n" );
}




//
// リンクのURLをコピーのメニュー
//
void ArticleViewBase::slot_copy_current_url()
{
    if( m_url_tmp.empty() ) return;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::slot_copy_current_url url = " << m_url_tmp << std::endl;
#endif    

    COPYCLIP( m_url_tmp );
}


//
// IDをコピー
//
// 呼び出す前に m_id_name にIDをセットしておくこと
//
void ArticleViewBase::slot_copy_id()
{
    std::string id = "ID:" + m_id_name.substr( strlen( PROTO_ID ) );
    COPYCLIP( id );
}



//
// レス番号クリック時のレスのコピーのメニュー
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_copy_res( bool ref )
{
    assert( m_article );
    if( m_str_num.empty() ) return;

#ifdef _DEBUG
    std::cout << "ArticleViewBase::copy_res number = " << m_str_num << std::endl;
#endif    

    COPYCLIP( m_article->get_res_str( atoi( m_str_num.c_str() ), ref ) );
}




//
// お気に入り登録
//
void ArticleViewBase::slot_favorite()
{
    CORE::DATA_INFO info;
    info.type = TYPE_THREAD;
    info.url = m_url_article;;
    info.name = DBTREE::article_subject( m_url_article );

    CORE::SBUF_clear_info();
    CORE::SBUF_append( info );

    CORE::core_set_command( "append_favorite", URL_FAVORITEVIEW );
}



//
// 別のタブを開いてレス抽出
//
// 呼び出す前に m_str_num に抽出するレスをセットしておくこと
//
void ArticleViewBase::slot_drawout_res()
{
    CORE::core_set_command( "open_article_res" ,m_url_article, m_str_num );
}



//
// 別のタブを開いて周辺のレスを抽出
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_drawout_around()
{
    const int range = 10;
    int center = atoi( m_str_num.c_str() );
    int from = MAX( 0, center - range );
    int to = center + range;
    std::stringstream ss;
    ss << from << "-" << to;
    CORE::core_set_command( "open_article_res" ,m_url_article, ss.str(), m_str_num );
}


//
// 別のタブを開いてテンプレート表示
//
void ArticleViewBase::slot_drawout_tmp()
{
    const int to = 20;
    std::stringstream ss;
    ss << "1-" << to;
    CORE::core_set_command( "open_article_res" ,m_url_article, ss.str() );
}




//
// 別のタブを開いてID抽出
//
// 呼び出す前に m_id_name にIDをセットしておくこと
//
void ArticleViewBase::slot_drawout_id()
{
    CORE::core_set_command( "open_article_id" ,m_url_article, m_id_name );
}



//
// 別のタブを開いてブックマーク抽出
//
void ArticleViewBase::slot_drawout_bm()
{
    CORE::core_set_command( "open_article_bm" ,m_url_article );
}



//
// 別のタブを開いて参照抽出
//
// 呼び出す前に m_str_num に対象のレス番号を入れておくこと
//
void ArticleViewBase::slot_drawout_refer()
{
    CORE::core_set_command( "open_article_refer" ,m_url_article, m_str_num );
}



//
// 別のタブを開いてURL抽出
//
void ArticleViewBase::slot_drawout_url()
{
    CORE::core_set_command( "open_article_url" ,m_url_article );
}



//
// IDであぼ〜ん
//
// 呼び出す前に m_id_name にIDをセットしておくこと
//
void ArticleViewBase::slot_abone_id()
{
    DBTREE::add_abone_id( m_url_article, m_id_name );

    // 再レイアウト
    ARTICLE::get_admin()->set_command( "relayout_views", m_url_article );
}



//
// 名前であぼ〜ん
//
// 呼び出す前に m_name に名前をセットしておくこと
//
void ArticleViewBase::slot_abone_name()
{
    DBTREE::add_abone_name( m_url_article, m_name );

    // 再レイアウト
    ARTICLE::get_admin()->set_command( "relayout_views", m_url_article );
}


//
// 範囲選択した文字列でであぼ〜ん
//
void ArticleViewBase::slot_abone_word()
{
    DBTREE::add_abone_word( m_url_article, m_drawarea->str_selection() );

    // 再レイアウト
    ARTICLE::get_admin()->set_command( "relayout_views", m_url_article );
}


//
// 透明あぼーん
//
void ArticleViewBase::slot_toggle_abone_transparent()
{
    if( ! m_enable_menuslot ) return;

    assert( m_article );
    m_article->set_abone_transparent( ! m_article->get_abone_transparent() );

    // 再レイアウト
    ARTICLE::get_admin()->set_command( "relayout_views", m_url_article );
}

//
// 連鎖あぼーん
//
void ArticleViewBase::slot_toggle_abone_chain()
{
    if( ! m_enable_menuslot ) return;

    assert( m_article );
    m_article->set_abone_chain( ! m_article->get_abone_chain() );

    // 再レイアウト
    ARTICLE::get_admin()->set_command( "relayout_views", m_url_article );
}


//
// 画像のモザイク解除
//
void ArticleViewBase::slot_cancel_mosaic()
{
    if( ! DBIMG::is_cached( m_url_tmp ) ) return;
    DBIMG::set_mosaic( m_url_tmp, false );
    CORE::core_set_command( "redraw", m_url_tmp );
}



//
// 画像削除
//
void ArticleViewBase::slot_deleteimage()
{
    if( ! m_url_tmp.empty() ) CORE::core_set_command( "delete_image", m_url_tmp );
}


//
// 画像保存
//
void ArticleViewBase::slot_saveimage()
{
    DBIMG::save( m_url_tmp, std::string() );
}


//
// 画像保護
//
void ArticleViewBase::slot_toggle_protectimage()
{
    if( ! m_enable_menuslot ) return;

    DBIMG::set_protect( m_url_tmp , ! DBIMG::is_protected( m_url_tmp ) );
}



//
// 検索entryでenterを押した
//
void ArticleViewBase::slot_active_search()
{
    focus_view();
    std::string query = m_toolbar->m_entry_search.get_text();
    if( query.empty() ) return;

    std::list< std::string > list_query;
    list_query = MISC::split_line( query );

    if( m_query == query ) m_drawarea->search_move( m_search_invert );
    
    else{
        m_query = query;

        if( m_drawarea->search( list_query, m_search_invert ) ) m_drawarea->search_move( m_search_invert );
    }
}



//
// 検索entryの操作
//
void ArticleViewBase::slot_entry_operate( int controlid )
{
    if( controlid == CONTROL::Cancel ) focus_view();
    else if( controlid == CONTROL::DrawOutAnd ) slot_push_drawout_and();
}

