#pragma once

#include <hps.h>
#include <sprk.h>
#include <QtWidgets>

namespace ts3d {
    class HPSWidget : public QWidget {
        Q_OBJECT
    public:
        HPSWidget( QWidget *parent );
        ~HPSWidget( void );

        HPS::Canvas getCanvas( void );
        HPS::View getView( void );

    private:
        void setupSceneDefaults();
        //void AttachView( HPS::View const & in_view );

    protected:
        void paintEvent( QPaintEvent * e );
        void resizeEvent( QResizeEvent * e );

        void mousePressEvent( QMouseEvent * event );
        void mouseDoubleClickEvent( QMouseEvent * event );
        void mouseReleaseEvent( QMouseEvent * event );
        void mouseMoveEvent( QMouseEvent * event );
        void wheelEvent( QWheelEvent * event );

        void keyPressEvent( QKeyEvent * e );
        void keyReleaseEvent( QKeyEvent * e );

        void focusOutEvent( QFocusEvent * );

        QPaintEngine* paintEngine() const { return 0; }

    private:
        HPS::Canvas _canvas;
        HPS::View _view;

        HPS::MouseEvent buildMouseEvent( QMouseEvent * in_event, HPS::MouseEvent::Action action, size_t click_count );
        HPS::KeyboardEvent buildKeyboardEvent( QKeyEvent * in_event, HPS::KeyboardEvent::Action action );
        void getModifierKeys( HPS::InputEvent * event );

    };
}