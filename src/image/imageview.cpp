// ライセンス: 最新のGPL

#define _DEBUG
#include "jddebug.h"

#include "imageadmin.h"
#include "imageview.h"
#include "imagearea.h"

#include "dbimg/img.h"

#include "command.h"
#include "httpcode.h"
#include "controlid.h"

#include <sstream>

#ifndef MAX
#define MAX( a, b ) ( a > b ? a : b )
#endif


#ifndef MIN
#define MIN( a, b ) ( a < b ? a : b )
#endif


using namespace IMAGE;


ImageViewMain::ImageViewMain( const std::string& url )
    : ImageViewBase( url ),
      m_scrwin( 0 ),
      m_length_prev( 0 ),
      m_show_status( false ),
      m_show_label( false )
{
#ifdef _DEBUG    
    std::cout << "ImageViewMain::ImageViewMain : " << get_url() << std::endl;
#endif

    // スクロールウィンドウを作ってEventBoxを貼る
    m_scrwin = Gtk::manage( new Gtk::ScrolledWindow() );
    assert( m_scrwin );

    m_scrwin->property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    m_scrwin->property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    m_scrwin->add( get_event() );
    pack_start( *m_scrwin );

    setup_common();
}


//
// クロック入力
//
void ImageViewMain::clock_in()
{
    View::clock_in();

    // viewがアクティブになった(クロック入力が来た)ときにステータス表示
    if( m_show_status ){
        m_show_status = false;
        IMAGE::get_admin()->set_command( "set_status", get_url(), get_status() );
    }

    // ロード中
    if( loading() ){

        // 読み込みサイズの表示更新
        if( get_img()->is_loading() ) show_status();

        // ロード完了
        // 次にclock_in()が呼ばれたら下のelseの中に入る
        else{

            set_loading( false ); 
            show_view();
            show_status();
        }
    }

    // ロード中でない
    else{

        // バックグラウンドで開いた時やロード直後に画像を表示すると重くなるので
        // ビューがアクティブになった(クロック入力が来た) 時点で画面を表示する
        if( get_imagearea() && ! get_imagearea()->is_ready() ) {

            get_imagearea()->show_image();

            // 読み込みエラーが起きたらimageareaを除いてラベルを貼る
            if( ! get_imagearea()->get_errmsg().empty() ){
                set_status( get_imagearea()->get_errmsg() );
                remove_imagearea();
                set_label();
            }

            show_status();
        }

    }
}



//
// ラベルを貼る
//
void ImageViewMain::set_label()
{
    if( !m_show_label ){
        m_label.set_text( get_status() );
        get_event().add( m_label );
        m_label.show();
        m_show_label = true;
    }
}



//
// ラベルをremove
//
void ImageViewMain::remove_label()
{
    if( m_show_label ){
        get_event().remove();
        m_show_label = false;
    }
}



//
// 表示
//
void ImageViewMain::show_view()
{
    if( loading() ) return;

#ifdef _DEBUG
    std::cout << "ImageViewMain::show_view url = " << get_url() << std::endl;
#endif    

    // 画像を既に表示している
    if( get_imagearea() ){

        // キャッシュされてるなら再描画
        if( get_img()->is_cached() ){
#ifdef _DEBUG
            std::cout << "redraw\n";
#endif    
            get_imagearea()->show_image();
            show_status();
            return;
        }

        remove_imagearea();
    }

    remove_label();

    // 読み込み中        
    if( get_img()->is_loading() ){

#ifdef _DEBUG
        std::cout << "now loading\n";
#endif    
        set_loading( true );
        m_length_prev = 0;
        set_status( "loading..." );
        m_show_status = true; // viewがアクティブになった時点でステータス表示

        set_label();
    }

    // キャッシュがあるなら画像を表示
    else if( get_img()->is_cached() ){

#ifdef _DEBUG
        std::cout << "set image\n";
#endif    
        // 表示はビューがアクティブになった時に clock_in()の中で行う
        set_imagearea( Gtk::manage( new ImageAreaMain( get_url() ) ) );
    }

    // エラー
    else{

        set_status( get_img()->get_str_code() );
        m_show_status = true; // viewがアクティブになった時点でステータス表示

        set_label();
    }

    show_all_children();
}



//
// ステータス表示
//
void ImageViewMain::show_status()
{
    if( ! loading() ){

        // 画像が表示されていたら画像情報
        if( get_imagearea() ){

            std::stringstream ss;
            ss << get_imagearea()->get_width_org() << " x " << get_imagearea()->get_height_org();
            if( get_imagearea()->get_width_org() )
                ss << " (" << get_img()->get_size() << " %)";
            ss << " " << get_img()->total_length()/1024 << " kb ";
            if( get_img()->is_protected() ) ss << " キャッシュ保護されています";

            set_status( ss.str() );
        }

        // エラー(ネットワーク系)
        else if( get_img()->get_code() != HTTP_OK ) set_status( get_img()->get_str_code() );

        // ステータス標示
        IMAGE::get_admin()->set_command( "set_status", get_url(), get_status() );
        if( m_show_label ) m_label.set_text( get_status() );

#ifdef _DEBUG
            std::cout << "ImageViewMain::show_status : " << get_status() << std::endl;;
#endif
    }

    // ロード中
    else{

        // 読み込みサイズが更新した場合
        if( m_length_prev != get_img()->current_length() ){

            m_length_prev = get_img()->current_length();

            char tmpstr[ 256 ];
            snprintf( tmpstr, 256, "%zd k / %zd k", m_length_prev/1024, get_img()->total_length()/1024 );
            set_status( tmpstr );

            // ステータス標示
            IMAGE::get_admin()->set_command( "set_status", get_url(), get_status() );
            if( m_show_label ) m_label.set_text( get_status() );

#ifdef _DEBUG
            std::cout << "ImageViewMain::show_status : " << get_status() << std::endl;;
#endif
        }
    }
}


//
// ポップアップメニュー取得
//
// SKELETON::View::show_popupmenu() を参照すること
//
Gtk::Menu* ImageViewMain::get_popupmenu( const std::string& url )
{
    Gtk::Menu* popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu" ) );
    return popupmenu;
}



//
// ボタンクリック
//
bool ImageViewMain::slot_button_press( GdkEventButton* event )
{
    ImageViewBase::slot_button_press( event );

    // ドラッグして画像移動するときの起点
    m_x_motion = event->x_root;
    m_y_motion = event->y_root;

    return true;
}




//
// マウスモーション
//
bool ImageViewMain::slot_motion_notify( GdkEventMotion* event )
{
    ImageViewBase::slot_motion_notify( event );

    // スクロールバー移動
    if( m_scrwin ){

        GdkEventButton event_button;
        get_control().get_eventbutton( CONTROL::ClickButton, event_button );

#ifdef _DEBUG
        std::cout << "state = " << event->state << " / " << GDK_BUTTON1_MASK << " button = " << event_button.button << std::endl;
#endif

        if( ( event->state == GDK_BUTTON1_MASK && event_button.button == 1 )
            || ( event->state == 272 && event_button.button == 1 )
            || ( event->state == GDK_BUTTON2_MASK && event_button.button == 2 )
            || ( event->state == GDK_BUTTON3_MASK && event_button.button == 3 )
            ){

            Gtk::Adjustment* hadj = m_scrwin->get_hadjustment();
            Gtk::Adjustment* vadj = m_scrwin->get_vadjustment();

            gdouble dx = event->x_root - m_x_motion;
            gdouble dy = event->y_root - m_y_motion;

#ifdef _DEBUG
            std::cout << "dx = " << dx << " dy = " << dy << std::endl;
#endif

            m_x_motion = event->x_root;
            m_y_motion = event->y_root;

            if( hadj ) hadj->set_value(
                MAX( hadj->get_lower(), MIN( hadj->get_upper() - hadj->get_page_size(), hadj->get_value() - dx ) ) );

            if( vadj ) vadj->set_value(
                MAX( vadj->get_lower(), MIN( vadj->get_upper() - vadj->get_page_size(), vadj->get_value() - dy ) ) );
        }
    }

    return true;
}
