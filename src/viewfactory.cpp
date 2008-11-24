// ライセンス: GPL2

#include "viewfactory.h"

#include "bbslist/bbslistview.h"
#include "bbslist/favoriteview.h"
#include "bbslist/selectlistview.h"

#include "board/boardview.h"

#include "article/articleview.h"
#include "article/articleviewsearch.h"
#include "article/articleviewpreview.h"
#include "article/articleviewinfo.h"
#include "article/articleviewpopup.h"
#include "article/articleviewetc.h"

#include "image/imageview.h"
#include "image/imageviewicon.h"
#include "image/imageviewpopup.h"

#include "message/messageview.h"

SKELETON::View* CORE::ViewFactory( int type, const std::string& url, VIEWFACTORY_ARGS view_args )
{
    switch( type )
    {
        case VIEW_BBSLISTVIEW:
            return new BBSLIST::BBSListViewMain( url, view_args.arg1, view_args.arg2 );

        case VIEW_FAVORITELIST:
            return new BBSLIST::FavoriteListView( url, view_args.arg1, view_args.arg2 );

        case VIEW_SELECTLIST:
            return new BBSLIST::SelectListView( url, view_args.arg1, view_args.arg2 );

            //////////////////

        case VIEW_BOARDVIEW:
            return new BOARD::BoardView( url );

        case VIEW_BOARDNEXT:
            return new BOARD::BoardViewNext( url, view_args.arg1 );

            /////////////////

        case VIEW_ARTICLEVIEW:
            return new ARTICLE::ArticleViewMain( url );

        case VIEW_ARTICLERES:
            return new ARTICLE::ArticleViewRes( url,  view_args.arg1, ( view_args.arg2 == "true" ), view_args.arg3 );

        case VIEW_ARTICLENAME:
            return new ARTICLE::ArticleViewName( url, view_args.arg1 );

        case VIEW_ARTICLEID:
            return new ARTICLE::ArticleViewID( url, view_args.arg1 );

        case VIEW_ARTICLEBM:
            return new ARTICLE::ArticleViewBM( url );

        case VIEW_ARTICLEPOST:
            return new ARTICLE::ArticleViewPost( url );

        case VIEW_ARTICLEURL:
            return new ARTICLE::ArticleViewURL( url );

        case VIEW_ARTICLEREFER:
            return new ARTICLE::ArticleViewRefer( url, view_args.arg1 );

        case VIEW_ARTICLEDRAWOUT:
            return new ARTICLE::ArticleViewDrawout( url, view_args.arg1, ( view_args.arg2 == "OR" ) );

        case VIEW_ARTICLEPREVIEW:
            return new ARTICLE::ArticleViewPreview( url );

        case VIEW_ARTICLEINFO:
            return new ARTICLE::ArticleViewInfo( url );

        case VIEW_ARTICLESEARCHLOG:
            return new ARTICLE::ArticleViewSearch( url, view_args.arg1, ARTICLE::SEARCHMODE_LOG , ( view_args.arg2 == "exec" ), ( view_args.arg3 == "OR" ) );

        case VIEW_ARTICLESEARCHALLLOG:
            return new ARTICLE::ArticleViewSearch( url, view_args.arg1, ARTICLE::SEARCHMODE_ALLLOG, ( view_args.arg2 == "exec" ), (view_args.arg3 == "OR" ) );

        case VIEW_ARTICLESEARCHTITLE:
            return new ARTICLE::ArticleViewSearch( url, view_args.arg1, ARTICLE::SEARCHMODE_TITLE, ( view_args.arg2 == "exec" ), (view_args.arg3 == "OR" ) );

        case VIEW_ARTICLEPOSTLOG:
            return new ARTICLE::ArticleViewPostlog( url, atoi( view_args.arg1.c_str() ) );

            /////////////////

        case VIEW_ARTICLEPOPUPHTML:
            return new ARTICLE::ArticleViewPopupHTML( url, view_args.arg1 );

        case VIEW_ARTICLEPOPUPRES:
            return new ARTICLE::ArticleViewPopupRes( url, view_args.arg1, ( view_args.arg2 == "true" ), ( view_args.arg3 == "true" ) );

        case VIEW_ARTICLEPOPUPNAME:
            return new ARTICLE::ArticleViewPopupName( url, view_args.arg1 );

        case VIEW_ARTICLEPOPUPID:
            return new ARTICLE::ArticleViewPopupID( url, view_args.arg1 );

        case VIEW_ARTICLEPOPUPREFER:
            return new ARTICLE::ArticleViewPopupRefer( url, view_args.arg1 );

        case VIEW_ARTICLEPOPUPDRAWOUT:
            return new ARTICLE::ArticleViewPopupDrawout( url, view_args.arg1, ( view_args.arg2 == "OR" ) );

            /////////////////

        case VIEW_IMAGEVIEW:
            return new IMAGE::ImageViewMain( url );

        case VIEW_IMAGEICON:
            return new IMAGE::ImageViewIcon( url );

        case VIEW_IMAGEPOPUP:
            return new IMAGE::ImageViewPopup( url );

            /////////////////

        case VIEW_MESSAGE:
            return new MESSAGE::MessageViewMain( url, view_args.arg1 );

        case VIEW_NEWTHREAD:
            return new MESSAGE::MessageViewNew( url, view_args.arg1 );

        default:
            return NULL;
    }
}

