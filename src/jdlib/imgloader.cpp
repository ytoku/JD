// ライセンス: GPL2

//#define _DEBUG
#include "jddebug.h"

#include "imgloader.h"
#include "cache.h"

#include "config/globalconf.h"

using namespace JDLIB;

/**********************************************************/
/* constructions ******************************************/

ImgLoader::ImgLoader( const std::string& file )
    : m_file( file ),
      m_width( 0 ),
      m_height( 0 ),
      m_stop( false ),
      m_stopped( false ),
      m_y( 0 )
{}

// static
Glib::RefPtr< ImgLoader > ImgLoader::get_loader( const std::string& file )
{
    ImgProvider& provider = ImgProvider::get_provider();
    Glib::RefPtr< ImgLoader > loader = provider.get_loader( file );
    if( ! loader ) {
        loader = Glib::RefPtr< ImgLoader >( new ImgLoader( file ) );
        provider.set_loader( loader );
    }
    return loader;
}

/**********************************************************/
/* get data from PixbufLoader *****************************/

// 画像サイズ取得
bool ImgLoader::get_size( int& width, int& height )
{
    bool ret = load( true, true );
    width = m_width;
    height = m_height;
    return ret;
}

Glib::RefPtr< Gdk::Pixbuf > ImgLoader::get_pixbuf( const bool pixbufonly )
{
    if( ! load( pixbufonly ) ) {
        return Glib::RefPtr< Gdk::Pixbuf >();
    }
    return m_loader->get_pixbuf();
}

Glib::RefPtr< Gdk::PixbufAnimation > ImgLoader::get_animation()
{
    if( ! load( false ) ) {
        return Glib::RefPtr< Gdk::PixbufAnimation >();
    }
    return m_loader->get_animation();
}

/**********************************************************/
/* create PixbufLoader ************************************/

// 画像読み込み
// 動画でpixbufonly = true の時はアニメーションさせない
const bool ImgLoader::load( const bool pixbufonly )
{
    return load( pixbufonly, false );
}

// private, thread safe
// sizeonly = true の時はサイズの取得のみ、pixbufonly = true も指定すること 
const bool ImgLoader::load( const bool pixbufonly, const bool sizeonly )
{
    m_loader_lock.lock();
    if( sizeonly && m_width && m_height ) return true;
    if( m_loader ) {
        // 以前の読み込みが中断せずに完了した、またはpixbufonly
        if( ! m_stopped || pixbufonly ) {
#ifdef _DEBUG
    std::cout << "ImgLoader don't load" << std::endl;
#endif
            m_loader_lock.unlock();
            return true;
        }
        // リロード
        m_width = 0;
        m_height = 0;
        m_stop = false;
        m_stopped = false;
        m_y = 0;
    }

    m_pixbufonly = pixbufonly;

#ifdef _DEBUG
    std::cout << "ImgLoader sizeonly = " << sizeonly
              << " file = " << m_file << std::endl;
    size_t total = 0;
#endif

    bool ret = true;

    FILE* f = NULL;
    const size_t bufsize = 8192;
    size_t readsize = 0;
    guint8 data[ bufsize ];

    f = fopen( to_locale_cstr( m_file ), "rb" );
    if( ! f ){
        m_errmsg = "cannot file open";
        m_loader_lock.unlock();
        return false;
    }

    try {
        m_loader = Gdk::PixbufLoader::create();

#if GTKMMVER > 240
        if( sizeonly ) m_loader->signal_size_prepared().connect( sigc::mem_fun( *this, &ImgLoader::slot_size_prepared ) );
#endif
        m_loader->signal_area_updated().connect( sigc::mem_fun( *this, &ImgLoader::slot_area_updated ) );

        while( ! m_stop || ! ( m_stopped = true ) ){

            readsize = fread( data, 1, bufsize, f );
            if( readsize ) m_loader->write( data, readsize );

#ifdef _DEBUG
            total += readsize;
            std::cout << readsize << " / " << total << std::endl;
#endif
            if( feof( f ) ) break;

#if GTKMMVER <= 240 // gdkのバージョンが古い場合はpixbufを取得してサイズを得る

            if( sizeonly && m_loader->get_pixbuf() ){
                m_width = m_loader->get_pixbuf()->get_width();
                m_height = m_loader->get_pixbuf()->get_height();
                m_stop = true;
            }
#endif
        }

        m_loader->close();
    }
    catch( Glib::Error& err )
    {
#ifdef _DEBUG
        std::string stop_s = m_stop ? "true" : "false";
        std::cout << "ImgLoader (" << stop_s << ") : " << m_file << std::endl;
#endif
        if( ! m_stop ){
            m_errmsg = err.what();
            m_loader.clear();
            ret = false;
        }
    }

    fclose( f );

#ifdef _DEBUG
    std::cout << "ImgLoader::load read = " << total << "  w = " << m_width << " h = " << m_height << std::endl;
#endif

    m_loader_lock.unlock();
    return ret;
}


// PixbufLoaderが生成する画像の大きさを計算するのに必要な量のデータを受け取った時のシグナルハンドラ
void ImgLoader::slot_size_prepared( int w, int h )
{
#ifdef _DEBUG
    std::cout << "ImgLoader::slot_size_prepared w = " << w << " h = " << h << std::endl;
#endif

    m_width = w;
    m_height = h;
    request_stop();
}


// 画像の一部分が更新された時のシグナルハンドラ
void ImgLoader::slot_area_updated(int x, int y, int w, int h )
{
    if( m_pixbufonly ){

#ifdef _DEBUG
        std::cout << "ImgLoader::slot_area_updated x = " << x << " y = " << y << " w = " << w << " h = " << h << std::endl;
#endif

        // アニメーション画像を表示する際、幅や高さが元の値と異なる時に、全ての画像データを
        // 読み込まなくても pixbuf だけ取り出せれば良いので、pixbufを取り出せるようになった時点で
        // 画像データの読み込みを途中で止めて表示待ち時間を短縮する
        if( y < m_y ) request_stop();
        m_y = y;
    }
}

/**********************************************************/
/* interface inner behavior *******************************/

// 読み込み中断のリクエスト
inline void ImgLoader::request_stop()
{
#ifndef _WIN32
    // 中断をリクエストされても実際には読み込みが完了していることがある
    // windowsでは読み込みを中断すると、通常の画像でpixbufが壊れてしまうことがある
    m_stop = true;
#endif
}

inline bool ImgLoader::equals( const std::string& file ) const
{
    return m_file == file;
}

/**********************************************************/
/* ImgProvider has relational <use> from ImgLoader ********/

ImgProvider::ImgProvider()
{
}

// static, NOT thread safe
ImgProvider& ImgProvider::get_provider()
{
    // singleton provider
    static ImgProvider instance;
    return instance;
}

Glib::RefPtr< ImgLoader > ImgProvider::get_loader( const std::string& file )
{
    m_cache_lock.lock();
    // ImgLoaderキャッシュをサーチ
    for( std::list< Glib::RefPtr< ImgLoader > >::iterator it = m_cache.begin();
            it != m_cache.end(); it++ ) {
        if( (*it)->equals( file ) ) {
            Glib::RefPtr< ImgLoader > ret = *it;
            // LRU: キャッシュをlistの先頭に移動
            if( it != m_cache.begin() ) {
                m_cache.erase( it );
                m_cache.push_front( ret );
            }
            m_cache_lock.unlock();
            return ret;
        }
    }
    m_cache_lock.unlock();
    return Glib::RefPtr< ImgLoader >(); //null
}

void ImgProvider::set_loader( Glib::RefPtr< ImgLoader > loader )
{
    int size = CONFIG::get_imgcache_size();
    if( size ) {
        m_cache_lock.lock();
        if( m_cache.size() >= (size_t)size ) {
            m_cache.pop_back();
        }
        m_cache.push_front( loader );
        m_cache_lock.unlock();
    }
}


