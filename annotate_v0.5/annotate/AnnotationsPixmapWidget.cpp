#include "AnnotationsPixmapWidget.h"
#include "ImgAnnotations.hpp"

#include <QtDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QPointF>
#include <QMessageBox>

#include <math.h>
#include <boost/foreach.hpp>

#include "MainWindow.h"
#include "functions.h"

using namespace IA;

AnnotationsPixmapWidget::AnnotationsPixmapWidget(ImgAnnotations *annotations,
                                                 QAbstractScrollArea* parentScrollArea,
                                                 QWidget *parent)
    : PixmapWidget(parentScrollArea, parent)
{


    mousePos = Somewhere;
    mouseMode = Browsing;

    activeObjID = -1;
    activeFixPoint  = -1;
    this->annotations = annotations;

    // should be NoFocus by default
    //setFocusPolicy(Qt::NoFocus);

    // needed to get mouse-move-events
    setMouseTracking(true);
    // right click menu
    setContextMenuPolicy( Qt::CustomContextMenu );
}

AnnotationsPixmapWidget::MouseMode AnnotationsPixmapWidget::getMouseMode()
{
    return mouseMode;
}

bool AnnotationsPixmapWidget :: haveActiveObject()
{
    return (activeObjID != -1);
}

void AnnotationsPixmapWidget::onVisibleObjectsChanged(const IDList& ids)
{    
    visibleObjIDs = ids;
//    qDebug() << " onVisibleObjectsChanged, new size: " << visibleObjIDs.size() << " elements: ";
//    BOOST_FOREACH(IA::ID id, visibleObjIDs)
//            qDebug() << id;
    this->activeObjID = activeObjID;
    activeFixPoint = -1;

    repaint();
}

void AnnotationsPixmapWidget::setVisibleObjects(const IDList& ids)
{
    onVisibleObjectsChanged(ids);

    // emit the signal that the active bounding box changed
    emit activeObjectChanged(activeObjID);
}

void AnnotationsPixmapWidget::onActiveObjectChanged(ID newActiveObjID)
{
    if (newActiveObjID == activeObjID)
        return;

    activeObjID = newActiveObjID;
    activeFixPoint = -1;

    repaint();
}

//void AnnotationsPixmapWidget::
//setFixpoints(Object* objs, QList<QPointF> fixpoints)
//{
//    obj->set("fixpoints", points2str(fix_points).toStdString());
//}

void AnnotationsPixmapWidget :: onPasteObj( ID paste_obj )
{    
    if ( !annotations->existsObject(paste_obj) )
        return;
    Object * obj = annotations->getObject(paste_obj);
    QRectF box = str2rect( QString::fromStdString(obj->get("bbox")) );
//    qDebug() << "old box posi: " << box;
    QPointF mouse_in_img = getMatrixInv().map( mapFromGlobal(QCursor::pos()) );
    box.moveTo( mouse_in_img );
    obj->set("bbox", rect2str(box).toStdString());

    // debug
//    QRectF box2 = str2rect( QString::fromStdString(obj->get("bbox")) );
//    qDebug() << "new box posi: " << box2;

    QList<QPointF> fix_points = str2points( QString::fromStdString(obj->get("fixpoints")) );
    if ( fix_points.empty() )
        return;
    // take first point as reference point
    QPointF delta =  mouse_in_img - fix_points.at(0) ;
    for (int i = 0; i < fix_points.count(); i++) {
        fix_points[i].rx() += delta.x();
        fix_points[i].ry() += delta.y();
    }
    obj->set("fixpoints", points2str(fix_points).toStdString());

    emit sthChanged();
}

void AnnotationsPixmapWidget::setActiveObject(ID newActiveObjID)
{
    if (newActiveObjID == activeObjID)
        return;

    onActiveObjectChanged(newActiveObjID);

    // emit the signal that the active bounding box changed
    emit activeObjectChanged(activeObjID);
}

void AnnotationsPixmapWidget::setMouseMode(MouseMode newMode)
{
    // don't accept the mode Dragging from outside, it's handled internally
    if (newMode == Dragging){
//        std::cerr << "cannot set mousemode to DRAGGING\n";
        return;
    }

    // if we are dragging an object, we ignore the modes given from outside
    if (mouseMode == Dragging){
//        std::cerr << "cannot set mousemode, we are dragging\n";
        return;
    }

//    std::cerr << "set mousemode to " << newMode << std::endl;
    mouseMode = newMode;

    // update the mouse cursor
    updateMouseCursor();
}

void AnnotationsPixmapWidget::paintEvent( QPaintEvent *event )
{
    // call the super class paintEvent method
    PixmapWidget::paintEvent(event);

    //
    // draw image and bounding boxes
    //

    QPainter p(this);

    // create pens and brushes
//    QBrush brush(QColor(0, 0, 150, 50));
    QBrush activeBrush(QColor(150, 0, 0, 50));
    QPen pen(QColor(0, 0, 220, 200));
    QPen activePen(QColor(220, 0, 0, 200));

    // adjust the coordinate system
    p.setMatrix( getMatrix() );

    // draw all inactive bounding boxes
    BOOST_FOREACH(ID objID, visibleObjIDs) {
        if ( !annotations->existsObject(objID) )
            continue;

        Object *obj = annotations->getObject(objID);
        Q_CHECK_PTR(obj);

        // get the current bounding box
        QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));
        // draw the rect
        if (objID == activeObjID) {
            p.setPen(activePen);
            p.fillRect(box, activeBrush);
            p.drawRect(box);
            p.drawEllipse(box.bottomLeft(), MARGIN, MARGIN);
            p.drawEllipse(box.bottomRight(), MARGIN, MARGIN);
            p.drawEllipse(box.topLeft(), MARGIN, MARGIN);
            p.drawEllipse(box.topRight(), MARGIN, MARGIN);
        }
        else {
            p.setPen(pen);
            //p.fillRect(box, brush);
            p.drawRect(box);
        }

        // draw the fix points (the pen has already set)
        QList<QPointF> fix_points = str2points( QString::fromStdString(obj->get("fixpoints")) );
        //        qSort(fix_points.begin(), fix_points.end(), lessP);
        QPolygonF poly( fix_points.toVector() );
        p.drawPolygon( poly );
        if (objID == activeObjID) {
            // need a painterpath to fill polygon
            QPainterPath pp;
            pp.addPolygon(poly);
            p.fillPath(pp, activeBrush);
        }

        QPointF from, to;
        for (int j = 0; j < fix_points.count(); j++) {
            // draw a cross
            // left
            from.rx() = fix_points[j].x() - (MARGIN + 3) / getZoomFactor();
            to.rx() = from.rx() + MARGIN / getZoomFactor();
            from.ry() = fix_points[j].y();
            to.ry() = fix_points[j].y();
            p.drawLine(from, to);
            // right
            from.rx() = fix_points[j].x() + (MARGIN + 3) / getZoomFactor();
            to.rx() = from.rx() - MARGIN / getZoomFactor();
            from.ry() = fix_points[j].y();
            to.ry() = fix_points[j].y();
            p.drawLine(from, to);
            // upper
            from.rx() = fix_points[j].x();
            to.rx() = fix_points[j].x();
            from.ry() = fix_points[j].y() - (MARGIN + 3) / getZoomFactor();
            to.ry() = from.ry() + MARGIN / getZoomFactor();
            p.drawLine(from, to);
            // lower
            from.rx() = fix_points[j].x();
            to.rx() = fix_points[j].x();
            from.ry() = fix_points[j].y() + (MARGIN + 3) / getZoomFactor();
            to.ry() = from.ry() - MARGIN / getZoomFactor();
            p.drawLine(from, to);

//            if (objID == activeObjID) {
//                // draw the number of the fix point in the upper left quadrant of the cross
//                QRectF stringBox(fix_points[j].x() - (MARGIN + 5), fix_points[j].y() - (MARGIN + 5), (MARGIN + 5), (MARGIN + 5));
//                QString str;
//                str.setNum(j);
//                p.drawText(stringBox, Qt::AlignHCenter | Qt::AlignVCenter, str);
//            }
            // draw circle around active fixpoint
            if ( objID == activeObjID && j == activeFixPoint && objID != -1) {
                QRectF rect( fix_points[j].x() - MARGIN / getZoomFactor(),
                             fix_points[j].y() - MARGIN / getZoomFactor(),
                             MARGIN / getZoomFactor() * 2, MARGIN / getZoomFactor() * 2 );
                p.drawEllipse(rect);
            }
        }
    } // end foreach id

    // if a score value is set .. draw the score value
    //		if (obj->isSet("score")) {
    //			// make sure that the score is not written outside the visible area
    //			QMatrix mInv = p.matrix().inverted();
    //			QRectF stringBox = p.matrix().mapRect(box);
    //			stringBox.setLeft(std::max(0.0, stringBox.left()));
    //			stringBox.setTop(std::max(0.0, stringBox.top()));
    //
    //			// transform the box back in the coordinate system of the painter
    //			stringBox = mInv.mapRect(stringBox);
    //
    //			// draw the score value on the screen
    //			p.drawText(stringBox, Qt::AlignLeft | Qt::AlignTop, QString::fromStdString(obj->get("score")));
    //		}

}

QList<QPointF> AnnotationsPixmapWidget :: getFixPoints( Object * obj )
{
    return str2points(QString::fromStdString(obj->get("fixpoints")));
}

void AnnotationsPixmapWidget :: checkClosestFixPoint( QPointF & xyMouse,
                                                IA::ID activte_obj_id )
{
    Object * obj = annotations->getObject(activte_obj_id);
    QList<QPointF> fix_points = str2points(QString::fromStdString(obj->get("fixpoints")));

    // check the distance to the fix points
    int closestPoint = -1;
    float closestDistance = -1;
    float tmpDistance;
    for (int i = 0; i < fix_points.count(); i++) {
        tmpDistance = sqrt(powf(fix_points[i].x() - xyMouse.x(), 2) + powf(fix_points[i].y() - xyMouse.y(), 2));
        if (closestPoint < 0 || tmpDistance < closestDistance) {
            closestDistance = tmpDistance;
            closestPoint = i;
        }
    }
    if (closestPoint >= 0 && closestDistance <= MARGIN / getZoomFactor()) {
        mousePos = CloseToFixPoint;
    }
}

void AnnotationsPixmapWidget :: mouseMoveEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::RightButton )
        return;
    if ( !annotations->existsObject(activeObjID) )
        setCursor(Qt::ArrowCursor);

    // get the mouse coordinate in the zoomed image
    QPointF xyMouse(event->x(), event->y());
    xyMouse = getMatrixInv().map(xyMouse);

    // treat mouse-modes
    if ( mouseMode == Browsing ) {
        if ( !annotations->existsObject(activeObjID) ){
            // TODO
            return;
        }
        else {
            setRectMousePos( xyMouse, activeObjID, true );
            // search further
            if (mousePos == Somewhere) {
                checkClosestFixPoint( xyMouse, activeObjID );
            }
            if (mousePos == Somewhere) {
                QPolygonF poly( getFixPoints(annotations->getObject(activeObjID) ).toVector() );
                // in poly?
                if ( poly.containsPoint(xyMouse, Qt::OddEvenFill) ) {
                    mousePos = InPolygon;
                }
            }
        }
    }
    else {
        if ( !annotations->existsObject(activeObjID) )
            return;

        if (mouseMode == Dragging) {
            // mouse is changing the box parameters .. adapt the bounding box
            // parameters according to the mouse position
            Object *obj = annotations->getObject(activeObjID);
            Q_CHECK_PTR(obj);
            QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));
            QList<QPointF> fixPoints = str2points(QString::fromStdString(obj->get("fixpoints")));
            if (mousePos == CloseToLeft || mousePos == CloseToTopLeft || mousePos == CloseToBottomLeft)
                // change the left
                box.setLeft(xyMouse.x());
            if (mousePos == CloseToRight || mousePos == CloseToTopRight || mousePos == CloseToBottomRight)
                // change the right
                box.setRight(xyMouse.x());
            if (mousePos == CloseToTop || mousePos == CloseToTopRight || mousePos == CloseToTopLeft)
                // change the top
                box.setTop(xyMouse.y());
            if (mousePos == CloseToBottom || mousePos == CloseToBottomRight || mousePos == CloseToBottomLeft)
                // change the bottom
                box.setBottom(xyMouse.y());
            if (mousePos == InBox) {
                // move the whole bounding box
                QPointF d = xyMouse - clicked_pos;
                box.translate(d);
                clicked_pos = xyMouse;
            }
            if ( mousePos == InPolygon ) {
                QPointF d = xyMouse - clicked_pos;
                // update the position of all fix points
                for (int i = 0; i < fixPoints.count(); i++) {
                    fixPoints[i] += d;
                }
                clicked_pos = xyMouse;
            }
            if (mousePos == CloseToFixPoint && activeFixPoint >= 0 && activeFixPoint < fixPoints.count()) {
                // change the active fix point
                fixPoints[activeFixPoint] = xyMouse;
            }

            // save changes
            if (box.isEmpty() && 0 == box.left() && 0 == box.right() && 0 == box.top() && 0 == box.bottom())
                obj->clear("bbox");
            else
                obj->set("bbox", rect2str(box).toStdString());
            if (0 >= fixPoints.size())
                obj->clear("fixpoints");
            else
                obj->set("fixpoints", points2str(fixPoints).toStdString());

            // tell that sth has been changed
            emit sthChanged();

            // request a repaint
            repaint();
        }
        else if ( mouseMode == CreateFixPoint ) {
            if ( !annotations->existsObject(activeObjID) )
                return;

            Object *obj = annotations->getObject(activeObjID);
            QList<QPointF> fixPoints = str2points(QString::fromStdString(obj->get("fixpoints")));

            // check the distance to the fix points
            int closestPoint = -1;
            float closestDistance = -1;
            float tmpDistance;
            for (int i = 0; i < fixPoints.count(); i++) {
                tmpDistance = sqrt(powf(fixPoints[i].x() - xyMouse.x(), 2) + powf(fixPoints[i].y() - xyMouse.y(), 2));
                if (closestPoint < 0 || tmpDistance < closestDistance) {
                    closestDistance = tmpDistance;
                    closestPoint = i;
                }
            }

            QPolygonF poly( fixPoints.toVector() );
            if (closestPoint >= 0 && closestDistance <= MARGIN / getZoomFactor()) {
                mousePos = CloseToFixPoint;
                activeFixPoint = closestPoint;
            }
            //            // mouse in polygon
            else if ( fixPoints.size() >= 2
                      && poly.containsPoint(xyMouse, Qt::OddEvenFill) )
            {
                mousePos = InPolygon;
                activeFixPoint = -1;
            }
            else {
                mousePos = Somewhere;
                activeFixPoint = -1;
            }

//            if ( fixPoints.empty() )
//                obj->clear("fixpoints");
//            else
//                obj->set("fixpoints", points2str(fixPoints).toStdString());
        }
        else if ( mouseMode == CreateRectangle ) {
            // mouse is not changing the box parameters .. adjust the mouse cursor
           setRectMousePos(xyMouse, activeObjID, true);
        }
    }

    // update the mouse cursor
    updateMouseCursor();
}

void AnnotationsPixmapWidget :: setRectMousePos ( QPointF & xyMouse, IA::ID id, bool check_in_box )
{
    // mouse is not changing the box parameters .. adjust the mouse cursor

    if ( !annotations->existsObject(id) )
        return;

    // init variables
    bool closeToLeft = false;
    bool closeToRight = false;
    bool closeToTop = false;
    bool closeToBottom = false;    

    Object *obj = annotations->getObject(id);
    QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));

    // check the distance to the rect
    if (xyMouse.y() >= box.top() - MARGIN / getZoomFactor() && xyMouse.y() <= box.bottom() + MARGIN / getZoomFactor()) {
        if (fabs(xyMouse.x() - box.left()) <= MARGIN / getZoomFactor())
            closeToLeft = true;
        if (fabs(xyMouse.x() - box.right()) <= MARGIN / getZoomFactor())
            closeToRight = true;
    }
    if (xyMouse.x() >= box.left() - MARGIN / getZoomFactor() && xyMouse.x() <= box.right() + MARGIN / getZoomFactor()) {
        if (fabs(xyMouse.y() - box.top()) <= MARGIN / getZoomFactor())
            closeToTop = true;
        if (fabs(xyMouse.y() - box.bottom()) <= MARGIN / getZoomFactor())
            closeToBottom = true;
    }
    // mouse in box?    
    if (closeToLeft) {
        if (closeToTop)
            mousePos = CloseToTopLeft;
        else if (closeToBottom)
            mousePos = CloseToBottomLeft;
        else
            mousePos = CloseToLeft;
    }
    else if (closeToRight) {
        if (closeToTop)
            mousePos = CloseToTopRight;
        else if (closeToBottom)
            mousePos = CloseToBottomRight;
        else
            mousePos = CloseToRight;
    }
    else if (closeToTop) {
        mousePos = CloseToTop;
    }
    else if (closeToBottom) {
        mousePos = CloseToBottom;
    }
    else if ( check_in_box && box.contains(xyMouse) ) {
        mousePos = InBox;
    }
    else {
        mousePos = Somewhere;
    }
}

void AnnotationsPixmapWidget :: mousePressEvent( QMouseEvent * event )
{    
    // accept only left click
    if ( event->button() == Qt::RightButton )
        return;
//    qDebug() << "old mode: " << mouseMode;
    // save for the mousePressRelease event
    old_mouse_mode = mouseMode;
    // get the mouse coordinate in the zoomed image
    QPointF xyMouse(event->x(), event->y());
    xyMouse = getMatrixInv().map(xyMouse);
    clicked_pos = xyMouse;

    // select the underlying obj w. the smallest area
    if ( mouseMode == Browsing ){
        IA::ID final_id = -1;
        float smallest_area = std::numeric_limits<float>::max();
//        foreach( Object *obj, annotations->getObjects() ) {
        BOOST_FOREACH( IA::ID id, visibleObjIDs ) {

            Object *obj = annotations->getObject(id);
            // at fix_point?
            QList<QPointF> fix_points = str2points(QString::fromStdString(obj->get("fixpoints")));

            // check the distance to the fix points
            int closestPoint = -1;
            float closestDistance = -1;
            float tmpDistance;
            for (int i = 0; i < fix_points.count(); i++) {
                tmpDistance = sqrt(powf(fix_points[i].x() - xyMouse.x(), 2) + powf(fix_points[i].y() - xyMouse.y(), 2));
                if (closestPoint < 0 || tmpDistance < closestDistance) {
                    closestDistance = tmpDistance;
                    closestPoint = i;
                }
            }
            // found a point of a polygon? -> leave
            if (closestPoint >= 0 && closestDistance <= MARGIN / getZoomFactor()) {               
                activeFixPoint = closestPoint;                
                final_id = id;
                break;
            }

            // check if mouse in poly
            QPolygonF poly( fix_points.toVector() );
            if ( !poly.isEmpty() ) {
                if ( poly.containsPoint(xyMouse, Qt::OddEvenFill) ) {
                    // compute area according to http://mathworld.wolfram.com/PolygonArea.html
                    float area = 0.0;
                    int s = fix_points.size();
                    for( int i = 0; i < s - 1; i++ ) {
                        area += fix_points[i].x() * fix_points[i+1].y() - fix_points[i+1].x() * fix_points[i].y();
                    }
                    area += fix_points[s-1].x() * fix_points[0].y() - fix_points[0].x() * fix_points[s-1].y();
                    area /= 2.0;
                    // can be negative if points were counterclockwise annotated
                    area = std::abs(area);
                    if ( area < smallest_area ) {
                        smallest_area = area;
                        final_id = id;
                    }
                }
            }

            // in box (+ margin)
            QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));
            if ( !box.isEmpty() ) {
                box.setRect(box.x()-MARGIN/getZoomFactor(), box.y()-MARGIN/getZoomFactor(),
                            box.width()+2*MARGIN/getZoomFactor(),
                            box.height()+2*MARGIN/getZoomFactor());
                if ( box.contains(xyMouse) )     {
                    float area = box.width() * box.height();
                    if ( area < smallest_area ) {
                        smallest_area = area;
                        final_id = id;
                    }
                }
            }           
        } // end foreach
        // found sth?
        if ( final_id != -1 ) {
            setActiveObject(final_id);
            mouseMode = Dragging;
            setRectMousePos( xyMouse, activeObjID, true );
            // search further
            if (mousePos == Somewhere) {
                checkClosestFixPoint( xyMouse, activeObjID );
            }
            if (mousePos == Somewhere) {
                QPolygonF poly( getFixPoints(annotations->getObject(activeObjID) ).toVector() );
                // in poly?
                if ( poly.containsPoint(xyMouse, Qt::OddEvenFill) ) {
                    mousePos = InPolygon;
                }
            }
        }
    }
    else if ( mouseMode == CreateRectangle ) {
        // only draw rectangle if we are not near an edge
        if ( !annotations->existsObject(activeObjID))
            return;

        Object *obj = annotations->getObject(activeObjID);        
        QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));

        if (mousePos == Somewhere
                || mousePos == InBox
                // TODO: guess we need these as well:
                || mousePos == InPolygon
                || mousePos == CloseToFixPoint)
        {
            // save the original bounding box
            //            _orgBox = box;

            // we start a new bounding box
            box.setLeft(xyMouse.x());
            box.setTop(xyMouse.y());
            box.setRight(xyMouse.x());
            box.setBottom(xyMouse.y());

            // set the mousePos .. i.e. we are now changing the bottom right corner
            mousePos = CloseToBottomRight;
        }

        // we start dragging
        mouseMode = Dragging;

//        // save changes
        if (box.isEmpty() && 0 == box.left() && 0 == box.right() && 0 == box.top() && 0 == box.bottom())
            obj->clear("bbox");
        else
            obj->set("bbox", rect2str(box).toStdString());

        // update the mouse cursor
        updateMouseCursor();
    }

    //    else if (mouseMode == DeleteFixPoint
    //             && mousePos == CloseToFixPoint
    //             && activeFixPoint >= 0 && activeFixPoint < fixPoints.count()) {
    //        // delete the active fix point
    //        fixPoints.removeAt(activeFixPoint);
    //        activeFixPoint = -1;
    //        mousePos = Somewhere;
    //    }
    else if (mouseMode == CreateFixPoint){
        if ( mousePos == CloseToFixPoint ) {
//            mouseMode == Dragging;
        }
        else {
            if ( !annotations->existsObject(activeObjID))
                return;

            Object *obj = annotations->getObject(activeObjID);           
            QList<QPointF> fix_points = str2points(QString::fromStdString(obj->get("fixpoints")));

            // create a new active fix point
            fix_points.append(xyMouse);
            activeFixPoint = fix_points.count() - 1;
//            mousePos = CloseToFixPoint;
            //            // send a signal that the bounding box has changed
            //            emit objectContentChanged(activeObjID);
            // update the mouse cursor
            //            updateMouseCursor();

            // save the changes
//            if ( fix_points.empty() )
//                obj->clear("fixpoints");
//            else
            obj->set("fixpoints", points2str(fix_points).toStdString());
            emit sthChanged();
        }
        // update the mouse cursor
    }
//    qDebug() << "current-mode: " << mouseMode;
    // request a repaint
    updateMouseCursor();
    repaint();
}

void AnnotationsPixmapWidget::mouseReleaseEvent(QMouseEvent * event)
{
    if ( event->button() == Qt::RightButton )
        return;
    if ( mouseMode == Dragging ) {
//        std::cerr << " we are dragging?\n";
        if (annotations->existsObject(activeObjID)) {

            Object *obj = annotations->getObject(activeObjID);
            Q_CHECK_PTR(obj);
            QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));

            // handle the left dragged over to right size, the top over the bottom
            // side, and vice versa
            if (box.left() > box.right()) {
                double tmp = box.left();
                box.setLeft(box.right());
                box.setRight(tmp);
            }
            if (box.top() > box.bottom()) {
                double tmp = box.top();
                box.setTop(box.bottom());
                box.setBottom(tmp);
            }

            // check whether the box is valid, otherwise restore the original box
//            if (box.width() * box.height() <= 10)
//                            box = _orgBox;
            if (box.isValid()) {
                obj->set("bbox", rect2str(box).toStdString());
            } else {
                obj->clear("bbox");
            }

            QPointF xyMouse(event->x(), event->y());
            xyMouse = getMatrixInv().map(xyMouse);
            if (xyMouse != clicked_pos){
                emit sthChanged();
            }

            //			_orgBox = QRectF();

            // TODO do we need this?
            // send the signal that the size of the bounding box has changed
            //            emit objectContentChanged(activeObjID);
        }
        // switch back to old mouse mode
        mouseMode = old_mouse_mode;
        // request a repaint
        repaint();
    }
}

void AnnotationsPixmapWidget::updateMouseCursor()
{
    // decide upon the mouse cursor for the different modes
    Qt::CursorShape newCursor = Qt::ArrowCursor;


//    if (annotations->existsObject(activeObjID)) {
        // TODO both cases can be merged
//        if (mouseMode == Browsing || mouseMode == CreateRectangle || mouseMode == Dragging) {
    switch (mousePos) {
        case CloseToLeft:
        case CloseToRight:
            newCursor = Qt::SizeHorCursor;
            break;
        case CloseToTop:
        case CloseToBottom:
            newCursor = Qt::SizeVerCursor;
            break;
        case CloseToTopLeft:
        case CloseToBottomRight:
            newCursor = Qt::SizeFDiagCursor;
            break;
        case CloseToTopRight:
        case CloseToBottomLeft:
            newCursor = Qt::SizeBDiagCursor;
            break;
        case CloseToFixPoint:
            newCursor = Qt::PointingHandCursor;
            break;
        default:
            newCursor = Qt::CrossCursor;
            break;
    }
    if ( (mouseMode == Browsing || mouseMode == Dragging)
         && (mousePos == InBox || mousePos == InPolygon) )
    {
        newCursor = Qt::SizeAllCursor;;
    }
//        }
//        else if (mouseMode == Browsing || mouseMode == CreateFixPoint) {
//            switch (mousePos) {
//            case CloseToFixPoint:
//                newCursor = Qt::PointingHandCursor;
//                break;
//            case InBox:
//                newCursor = Qt::SizeAllCursor;
//                break;
//            default:
//                newCursor = Qt::CrossCursor;
//                break;
//            }
//        }
//    }

    // set the cursor
    setCursor(newCursor);
}

bool AnnotationsPixmapWidget :: mouseOverObj() const
{
    if ( annotations->existsObject(activeObjID) && (mousePos == InBox || mousePos == InPolygon))
        return true;
    return false;
}
