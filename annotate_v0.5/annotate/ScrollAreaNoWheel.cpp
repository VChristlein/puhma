#include "ScrollAreaNoWheel.h"
#include <iostream>
#include "MainWindow.h"
ScrollAreaNoWheel::ScrollAreaNoWheel(QWidget *parent)
	: QScrollArea( parent )
{
	setFocusPolicy(Qt::NoFocus);	
}
class MainWindow;
void ScrollAreaNoWheel::wheelEvent(QWheelEvent *event)
{

    // send event further
    if ( ((MainWindow*) parentWidget())->zoomAllowed() ){
        event->ignore();
        emit wheelTurned(event);
    } else {
        QScrollArea::wheelEvent(event);
    }
	
//// QWidget *parent = parentWidget();
//// 	if (parent != NULL)
//// 		parent->wheelEvent(event);
}

//void ScrollAreaNoWheel :: keyPressEvent(QKeyEvent *event)
//{
//	event->ignore();
//	emit keyPressEvent(event);
//}
