// ライセンス: GPL2

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#define _DEBUG
#include "jddebug.h"

#include "aboutconfig.h"
#include "aboutconfigdiag.h"

#include "globalconf.h"
#include "configitems.h"
#include "defaultconf.h"

#include "skeleton/msgdiag.h"

#include "jdlib/miscutil.h"

#include "colorid.h"

enum
{
    CONFTYPE_STR = 0,
    CONFTYPE_INT,
    CONFTYPE_BOOL,
    CONFTYPE_COMMENT
};


using namespace CONFIG;


AboutConfig::AboutConfig( Gtk::Window* parent )
    : SKELETON::PrefDiag( parent, "", true )
{
    CONFIG::bkup_conf();

    pack_widgets();
}


//
// widgetのパック
//
void AboutConfig::pack_widgets()
{
    m_label.set_text( "動作保証外です。高度な設定を変更するとJDが誤作動する場合があります。" );

    m_liststore = Gtk::ListStore::create( m_columns );
    m_treeview.set_model( m_liststore );
    m_treeview.set_size_request( 640, 400 );
    m_treeview.signal_row_activated().connect( sigc::mem_fun( *this, &AboutConfig::slot_row_activated ) );

    Gtk::TreeViewColumn* column = Gtk::manage( new Gtk::TreeViewColumn( "設定名", m_columns.m_col_name ) );
    column->set_fixed_width( 480 );
    column->set_sizing( Gtk::TREE_VIEW_COLUMN_FIXED );
    column->set_resizable( true );
    m_treeview.append_column( *column );
    Gtk::CellRenderer *cell = column->get_first_cell_renderer();
    if( cell ) column->set_cell_data_func( *cell, sigc::mem_fun( *this, &AboutConfig::slot_cell_data ) );

    column = Gtk::manage( new Gtk::TreeViewColumn( "値", m_columns.m_col_value ) );
    m_treeview.append_column( *column );
    cell = column->get_first_cell_renderer();
    if( cell ) column->set_cell_data_func( *cell, sigc::mem_fun( *this, &AboutConfig::slot_cell_data ) );

    m_scrollwin.add( m_treeview );
    m_scrollwin.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS );

    get_vbox()->set_spacing( 8 );
    get_vbox()->pack_start( m_label, Gtk::PACK_SHRINK );
    get_vbox()->pack_start( m_scrollwin );

    set_title( "about:config　高度な設定" );
    show_all_children();

    append_rows();
}


//
// OK
//
void AboutConfig::slot_ok_clicked()
{
    SKELETON::MsgDiag mdiag( NULL, "一部の設定はJDを再起動しない限り有効になりません。JDを再起動してください。" );
    mdiag.run();
}


//
// キャンセルが押されたら設定を戻す
//
void AboutConfig::slot_cancel_clicked()
{
    CONFIG::restore_conf();
}


//
// 実際の描画の際に cellrendere のプロパティをセットするスロット関数
//
void AboutConfig::slot_cell_data( Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& it )
{
    Gtk::TreeModel::Row row = *it;

    if( row[ m_columns.m_col_drawbg ] ){
        cell->property_cell_background() = CONFIG::get_color( COLOR_BACK_HIGHLIGHT_TREE );
        cell->property_cell_background_set() = true;
    }
    else cell->property_cell_background_set() = false;
}


void AboutConfig::append_rows()
{
    // ネットワーク
    append_row( "■ ネットワーク" );
    append_row(	"JD ホームページのアドレス", get_confitem()->url_jdhp, CONF_URL_JDHP );
    append_row(	"板一覧を取得するサーバ", get_confitem()->url_bbsmenu, CONF_URL_BBSMENU );
    append_row( "2chログイン認証サーバのアドレス", get_confitem()->url_login2ch, CONF_LOGIN2CH );
    append_row( "p2ログイン認証サーバのアドレス", get_confitem()->url_loginp2, CONF_LOGINP2 );
    append_row( "2chにアクセスするときのエージェント名", get_confitem()->agent_for2ch, CONF_AGENT_FOR2CH );
    append_row( "2ch以外のサーバにアクセスするときのエージェント名", get_confitem()->agent_for_data, CONF_AGENT_FOR_DATA );
    append_row( "2chログイン認証サーバにアクセスするときのエージェント名", get_confitem()->x_2ch_ua, CONF_X_2CH_UA );
    append_row( "スレの読み込み時のタイムアウト値(秒)", get_confitem()->loader_timeout, CONF_LOADER_TIMEOUT );
    append_row( "書き込み時のタイムアウト値(秒)", get_confitem()->loader_timeout_post, CONF_LOADER_TIMEOUT_POST );
    append_row( "画像等のデータのロード時のタイムアウト値(秒)", get_confitem()->loader_timeout_img, CONF_LOADER_TIMEOUT_IMG );
    append_row( "更新チェック時のタイムアウト値(秒)", get_confitem()->loader_timeout_checkupdate, CONF_LOADER_TIMEOUT_CHECKUPDATE );
    append_row( "一般データのダウンロード時のバッファサイズ(Kbyte)", get_confitem()->loader_bufsize, CONF_LOADER_BUFSIZE );
    append_row( "スレ一覧のダウンロード時のバッファサイズ(Kbyte)", get_confitem()->loader_bufsize_board, CONF_LOADER_BUFSIZE_BOARD );

    // ツリービュー
    append_row( "" );
    append_row( "■ ツリービュー(板一覧、スレ一覧)" );
    append_row( "ツリービューでマウスホイールを回したときのスクロール量(行数)", get_confitem()->tree_scroll_size, CONF_TREE_SCROLL_SIZE );
    append_row( "ツリービューの行間スペース", get_confitem()->tree_ypad, CONF_TREE_YPAD );
    append_row( "カテゴリを開いたときにスクロールする", get_confitem()->scroll_tree, CONF_SCROLL_TREE );

    // 板一覧、お気に入り
    append_row( "" );
    append_row( "■ 板一覧、お気に入り、履歴" );
    append_row( "板移転時に確認ダイアログを表示する", get_confitem()->show_movediag, CONF_SHOW_MOVEDIAG );
    append_row( "板一覧内にあるリンクを全て板とみなす", get_confitem()->use_link_as_board, CONF_USE_LINK_AS_BOARD );
    append_row( "板一覧でカテゴリを常にひとつだけ開く", get_confitem()->open_one_category, CONF_OPEN_ONE_CATEGORY );
    append_row( "お気に入りでカテゴリを常にひとつだけ開く", get_confitem()->open_one_favorite, CONF_OPEN_ONE_FAVORITE );
    append_row( "右ペーンが空の時にサイドバーを最大化する", get_confitem()->expand_sidebar, CONF_EXPAND_SIDEBAR );
    append_row( "スレをお気に入りに追加した時にしおりをセットする", get_confitem()->bookmark_drop, CONF_BOOKMARK_DROP );
    append_row( "更新チェック時に板の更新もチェックする", get_confitem()->check_update_board, CONF_CHECK_UPDATE_BOARD );
    append_row( "起動時にお気に入りを自動でチェックする", get_confitem()->check_update_boot, CONF_CHECK_UPDATE_BOOT );

    // スレ一覧
    append_row( "" );
    append_row( "■ スレ一覧" );
    append_row( "インクリメント検索をする", get_confitem()->inc_search_board, CONF_INC_SEARCH_BOARD );
    append_row( "deleteを押したときに確認ダイアログを表示", get_confitem()->show_deldiag, CONF_SHOW_DELDIAG );
    append_row( "指定した値(時間)よりも後に立てられたスレを新着とみなす", get_confitem()->newthread_hour, CONF_NEWTHREAD_HOUR );
    append_row( "3ペーン時にスレ一覧やスレビューを最大化する", get_confitem()->expand_rpane, CONF_EXPAND_RPANE );
    append_row( "スレ一覧をロードする前にキャッシュにある一覧を表示する", get_confitem()->show_cached_board, CONF_SHOW_CACHED_BOARD );
    append_row( "お知らせスレ(924)のアイコンを表示する", get_confitem()->show_924, CONF_SHOW_924 );
    append_row( "dat落ちしたスレをNGスレタイトルから除く( 0: ダイアログ表示 1: 除く 2: 除かない )", get_confitem()->remove_old_abone_thread, CONF_REMOVE_OLD_ABONE_THREAD );


    // 次スレ検索
    append_row( "" );
    append_row( "■ 次スレ検索" );
    append_row( "類似度判定のしきい値(小さいほど判定が甘くなる、最大10)", get_confitem()->threshold_next, CONF_THRESHOLD_NEXT );
    append_row( "移行時にお気に入りのアドレスと名前を自動更新する(0: しない, 1:する, 2:追加)", get_confitem()->replace_favorite_next, CONF_REPLACE_FAVORITE_NEXT );
    append_row( "お気に入り自動更新の確認ダイアログを表示する", get_confitem()->show_diag_replace_favorite, CONF_SHOW_DIAG_REPLACE_FAVORITE );

    // スレビュー
    append_row( "" );
    append_row( "■ スレビュー" );
    append_row( "マウスホイールを回したときのスクロール速度", get_confitem()->scroll_size, CONF_SCROLL_SIZE );
    append_row( "上下キーを押したときのスクロール速度", get_confitem()->key_scroll_size, CONF_KEY_SCROLL_SIZE );
    append_row( "pageup、pagedownキーを押したときのスクロール速度", get_confitem()->key_fastscroll_size, CONF_KEY_FASTSCROLL_SIZE );
    append_row( "リロード後に末尾に自動的に移動する", get_confitem()->jump_after_reload, CONF_JUMP_AFTER_RELOAD );
    append_row( "リロード後に新着レスに自動的に移動する", get_confitem()->jump_new_after_reload, CONF_JUMP_NEW_AFTER_RELOAD );
    append_row( "ポップアップとカーソルの間の間隔(ピクセル)", get_confitem()->margin_popup, CONF_MARGIN_POPUP );
    append_row( "画像ポップアップとカーソルの間の間隔(ピクセル)", get_confitem()->margin_imgpopup, CONF_MARGIN_IMGPOPUP );
    append_row( "多重ポップアップの説明を表示する", get_confitem()->instruct_popup, CONF_INSTRUCT_POPUP );    
    append_row( "レス番号の上にマウスオーバーしたときに参照ポップアップ表示する", get_confitem()->refpopup_by_mo, CONF_REFPOPUP_BY_MO );
    append_row( "「名前」の上にマウスオーバーしたときにポップアップ表示する", get_confitem()->namepopup_by_mo, CONF_NAMEPOPUP_BY_MO );
    append_row( "IDの上にマウスオーバーしたときにIDをポップアップ表示する", get_confitem()->idpopup_by_mo, CONF_IDPOPUP_BY_MO );
    append_row( "スレビューとスレ一覧の切り替え方法説明ダイアログを表示する", get_confitem()->instruct_tglart, CONF_INSTRUCT_TGLART );
    append_row( "画像ビューとスレビューの切り替え方法説明ダイアログを表示する", get_confitem()->instruct_tglimg, CONF_INSTRUCT_TGLIMG );
    append_row( "リンクの下線を表示する", get_confitem()->draw_underline, CONF_DRAW_UNDERLINE );
    append_row( "レスを引用コピーするときに前に付ける引用文字", get_confitem()->ref_prefix, CONF_REF_PREFIX );
    append_row( "引用文字の後のスペース数", get_confitem()->ref_prefix_space, CONF_REF_PREFIX_SPACE );
    append_row( "RFC規定外の文字(^など)もURL判定に用いる", get_confitem()->loose_url, CONF_LOOSE_URL );
    append_row( "再読み込みボタンを押したときに全タブを更新する", get_confitem()->reload_allthreads, CONF_RELOAD_ALLTHREAD );
    append_row( "発言(同一ID)数をカウントする", get_confitem()->check_id, CONF_CHECK_ID );
    append_row( "レス参照数で色を変える回数(高)", get_confitem()->num_reference_high, CONF_NUM_REFERENCE_HIGH );
    append_row( "レス参照数で色を変える回数(低)", get_confitem()->num_reference_low, CONF_NUM_REFERENCE_LOW );
    append_row( "発言数で色を変える回数(高)", get_confitem()->num_id_high, CONF_NUM_ID_HIGH );
    append_row( "発言数で色を変える回数(低)", get_confitem()->num_id_low, CONF_NUM_ID_LOW );
    append_row( "WEB検索用のメニュー項目名", get_confitem()->menu_search_web, CONF_MENU_SEARCH_WEB );
    append_row( "WEB検索用のアドレス", get_confitem()->url_search_web, CONF_URL_SEARCH_WEB );
    append_row( "デフォルトで透明あぼーん", get_confitem()->abone_transparent, CONF_ABONE_TRANSPARENT );
    append_row( "デフォルトで連鎖あぼーん", get_confitem()->abone_chain, CONF_ABONE_CHAIN );

    // 書き込みウィンドウ
    append_row( "" );
    append_row( "■ 書き込みビュー" );
    append_row( "書き込み時に確認ダイアログを表示しない", get_confitem()->always_write_ok, CONF_ALWAYS_WRITE_OK );
    append_row( "書き込み中ダイアログを表示しない", get_confitem()->hide_writing_dialog, CONF_HIDE_WRITING_DIALOG );
    append_row( "アスキーアートメニューの履歴の保持数", get_confitem()->aahistory_size, CONF_AAHISTORY );
    append_row( "書き込みログの最大サイズ(バイト)", get_confitem()->maxsize_postlog, CONF_MAXSIZE_POSTLOG );
    append_row( "ビューを閉じても書き込み欄の日本語のON/OFF状態を保つ", get_confitem()->keep_im_status, CONF_KEEP_IM_STATUS );

    // 画像
    append_row( "" );
    append_row( "■ 画像" );
    append_row( "画像ポップアップの幅(ピクセル)", get_confitem()->imgpopup_width, CONF_IMGPOPUP_WIDTH );
    append_row( "画像ポップアップの高さ(ピクセル)", get_confitem()->imgpopup_height, CONF_IMGPOPUP_HEIGHT );
    append_row( "画像ビューを開いたときにウィンドウサイズに合わせる", get_confitem()->zoom_to_fit, CONF_ZOOM_TO_FIT );
    append_row( "画像ビューのフォーカスが外れたら折りたたむ", get_confitem()->fold_image, CONF_FOLD_IMAGE );
    append_row( "指定したサイズ(M byte)より大きい画像を表示するときに警告する", get_confitem()->max_img_size, CONF_MAX_IMG_SIZE );
    append_row( "指定した画素数(M pixel)より大きい画像を表示するときに警告する", get_confitem()->max_img_pixel, CONF_MAX_IMG_PIXEL );
    append_row( "モザイクのレベル", get_confitem()->mosaic_size, CONF_MOSAIC_SIZE );
    append_row( "画像ビューのスムージングレベル(0-2, 大きい程高画質で低速)", get_confitem()->imgmain_interp, CONF_IMGMAIN_INTERP );
    append_row( "埋め込み画像のスムージングレベル(0-2, 大きい程高画質で低速)", get_confitem()->imgemb_interp, CONF_IMGEMB_INTERP );
    append_row( "ポップアップ画像のスムージングレベル(0-2, 大きい程高画質で低速)", get_confitem()->imgpopup_interp, CONF_IMGPOPUP_INTERP );
    append_row( "画像のメモリキャッシュ枚数", get_confitem()->imgcache_size, CONF_IMGCACHE_SIZE );

    // ウィンドウ
    append_row( "" );
    append_row( "■ ウィンドウ" );
    append_row( "タブにアイコンを表示する", get_confitem()->show_tab_icon, CONF_SHOW_TAB_ICON );
    append_row( "タブに表示する文字列の最小文字数", get_confitem()->tab_min_str, CONF_TAB_MIN_STR );
    append_row( "各ビューと枠との間の余白", get_confitem()->view_margin, CONF_VIEW_MARGIN );
    append_row( "自前でウィンドウ配置を管理する", get_confitem()->manage_winpos, CONF_MANAGE_WINPOS );
    append_row( "スレビューのスクロールバーを左に配置する", get_confitem()->left_scrbar, CONF_LEFT_SCRBAR );

    // その他
    append_row( "" );
    append_row( "■ その他" );
    append_row( "履歴メニューの表示数", get_confitem()->history_size, CONF_HISTORY_SIZE );
    append_row( "履歴ビューの表示数", get_confitem()->historyview_size, CONF_HISTORYVIEW_SIZE );
    append_row( "マウスジェスチャを有効にする", get_confitem()->enable_mg, CONF_ENABLE_MG );
    append_row( "マウスジェスチャの判定開始半径", get_confitem()->mouse_radius, CONF_MOUSE_RADIUS );
    append_row( "起動時に開いていたスレ一覧を復元する", get_confitem()->restore_board, CONF_RESTORE_BOARD );
    append_row( "起動時に開いていたスレを復元する", get_confitem()->restore_article, CONF_RESTORE_ARTICLE );
    append_row( "起動時に開いていた画像を復元する", get_confitem()->restore_image, CONF_RESTORE_IMAGE );
#ifdef HAVE_MIGEMO_H
    append_row( "migomoの辞書ファイルの場所(migemo使用時のみ)", get_confitem()->migemodict_path, CONF_MIGEMO_PATH );
#endif
    append_row( "スレタイ検索用のメニュー項目名", get_confitem()->menu_search_title, CONF_MENU_SEARCH_TITLE );
    append_row( "スレタイ検索用のアドレス", get_confitem()->url_search_title, CONF_URL_SEARCH_TITLE );
    append_row( "スレタイ検索時にアドレスとスレタイを取得する正規表現", get_confitem()->regex_search_title, CONF_REGEX_SEARCH_TITLE );
    append_row( "Ctrl+qでウィンドウを閉じない", get_confitem()->disable_close, CONF_DISABLE_CLOSE );
    append_row( "書き込み履歴のあるスレを削除する時にダイアログを表示", get_confitem()->show_del_written_thread_diag, CONF_SHOW_DEL_WRITTEN_THREAD_DIAG );
    append_row( "FIFOの作成などにエラーがあったらダイアログを表示する", get_confitem()->show_diag_fifo_error, CONF_SHOW_DIAG_FIFO_ERROR );
}


void AboutConfig::append_row( const std::string& name, std::string& value, const std::string& defaultval )
{
    Gtk::TreeModel::Row row;
    row = *( m_liststore->append() );

    row[ m_columns.m_col_name ]  = name;
    row[ m_columns.m_col_type ] = CONFTYPE_STR;
    row[ m_columns.m_col_drawbg ] = false;
    row[ m_columns.m_col_value_str ] = &value;
    row[ m_columns.m_col_default_str ] = defaultval;

    set_value( row, value );
}


void AboutConfig::append_row( const std::string& name, int& value, const int defaultval )
{
    Gtk::TreeModel::Row row;
    row = *( m_liststore->append() );

    row[ m_columns.m_col_name ]  = name;
    row[ m_columns.m_col_type ] = CONFTYPE_INT;
    row[ m_columns.m_col_drawbg ] = false;
    row[ m_columns.m_col_value_int ] = &value;
    row[ m_columns.m_col_default_int ] = defaultval;

    set_value( row, value );
}


void AboutConfig::append_row( const std::string& name, bool& value, const bool defaultval )
{
    Gtk::TreeModel::Row row;
    row = *( m_liststore->append() );

    row[ m_columns.m_col_name ]  = name;
    row[ m_columns.m_col_type ] = CONFTYPE_BOOL;
    row[ m_columns.m_col_drawbg ] = false;
    row[ m_columns.m_col_value_bool ] = &value;
    row[ m_columns.m_col_default_bool ] = defaultval;

    set_value( row, value );
}


void AboutConfig::append_row( const std::string& comment )
{
    Gtk::TreeModel::Row row;
    row = *( m_liststore->append() );

    row[ m_columns.m_col_name ]  = comment;
    row[ m_columns.m_col_type ] = CONFTYPE_COMMENT;
}


void AboutConfig::set_value( Gtk::TreeModel::Row& row, const std::string& value )
{
    row[ m_columns.m_col_value ] = value;

    const std::string defaultval = row[ m_columns.m_col_default_str ];
    if( value != defaultval ) row[ m_columns.m_col_drawbg ] = true;
    else row[ m_columns.m_col_drawbg ] = false;
}


void AboutConfig::set_value( Gtk::TreeModel::Row& row, const int& value )
{
    row[ m_columns.m_col_value ] = MISC::itostr( value );

    const int  defaultval = row[ m_columns.m_col_default_int ];
    if( value != defaultval ) row[ m_columns.m_col_drawbg ] = true;
    else row[ m_columns.m_col_drawbg ] = false;
}


void AboutConfig::set_value( Gtk::TreeModel::Row& row, const bool& value )
{
    if( value ) row[ m_columns.m_col_value ] = "はい";
    else row[ m_columns.m_col_value ] = "いいえ";

    const bool defaultval = row[ m_columns.m_col_default_bool ];
    if( value != defaultval ) row[ m_columns.m_col_drawbg ] = true;
    else row[ m_columns.m_col_drawbg ] = false;
}


void AboutConfig::slot_row_activated( const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column )
{
#ifdef _DEBUG
    std::cout << "AboutConfig::slot_row_activated path = " << path.to_string() << std::endl;
#endif

    Gtk::TreeModel::Row row = *( m_liststore->get_iter( path ) );
    if( ! row ) return;

    const int type = row[ m_columns.m_col_type ];
    if( type == CONFTYPE_COMMENT ) return;

    SKELETON::PrefDiag* pref = NULL;

    switch( type ){

        case CONFTYPE_STR:
            pref = new AboutConfigDiagStr( this, row[ m_columns.m_col_value_str ], row[ m_columns.m_col_default_str ] );
            pref->run();
            set_value( row, *row[ m_columns.m_col_value_str ] );
            break;

        case CONFTYPE_INT:
            pref = new AboutConfigDiagInt( this, row[ m_columns.m_col_value_int ], row[ m_columns.m_col_default_int ] );
            pref->run();
            set_value( row, *row[ m_columns.m_col_value_int ] );
            break;

        case CONFTYPE_BOOL:
            pref = new AboutConfigDiagBool( this, row[ m_columns.m_col_value_bool ], row[ m_columns.m_col_default_bool ] );
            pref->run();
            set_value( row, *row[ m_columns.m_col_value_bool ] );
            break;
    }

    if( pref ) delete pref;
}
