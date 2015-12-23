#ifndef ANNOTATIONPIXMAPWIDGET_H_
#define ANNOTATIONPIXMAPWIDGET_H_

#include "PixmapWidget.h"

#include <ImgAnnotations.h>

#define MARGIN 5


class QMouseEvent;

// our own pixmap widget .. which displays an image and bounding boxes
class AnnotationsPixmapWidget : public PixmapWidget
{
	Q_OBJECT

public:
	enum MouseMode {
		// normal mouse mode .. resizing and moving the bounding boxes
        CreateRectangle,
		// the mouse is exploring (browsing) the bounding boxes, i.e. selcting
		// a new box visually
		Browsing,
		// the user creates by clicking a new fix point .. clicking on the on a
		// point already existing will delete the point
        CreateFixPoint,
		// the mouse is dragging/resizing an object .. cannot be set via
		// setMouseMode()
		Dragging
	};

private:
	enum MousePosition {
		Somewhere, InBox, CloseToFixPoint,
		CloseToLeft, CloseToRight, CloseToTop, CloseToBottom,
        CloseToTopLeft, CloseToTopRight, CloseToBottomLeft, CloseToBottomRight,
        InPolygon
	};

	IA::ImgAnnotations *annotations;
	IA::IDList visibleObjIDs;
	IA::ID activeObjID;
    // current nearest fixpoint
	int activeFixPoint;
    // the marked fixpoint
    int markedFixPoint;
	QRectF _orgBox;

	MousePosition mousePos;
	MouseMode mouseMode;
    MouseMode old_mouse_mode;
    QPointF clicked_pos;
	QAbstractScrollArea *scrollArea;

public:
	AnnotationsPixmapWidget( IA::ImgAnnotations *annotations, QAbstractScrollArea* scrollArea, QWidget *parent=0 );
//	~AnnotationsPixmapWidget();
	MouseMode getMouseMode();
	void setMouseMode(MouseMode);
	void setVisibleObjects(const IA::IDList&);
	void setActiveObject(IA::ID);
    bool mouseOverObj() const;

public slots:
	void onVisibleObjectsChanged(const IA::IDList&);
	void onActiveObjectChanged(IA::ID);
    void onPasteObj(IA::ID);

signals:
	void objectContentChanged(IA::ID);
	void activeObjectChanged(IA::ID);
    void createNewObject();
    void sthChanged();

protected:
	void paintEvent(QPaintEvent*);
	void mouseMoveEvent(QMouseEvent * event);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);

private:
	void updateMouseCursor();
    bool haveActiveObject();
    void setRectMousePos(QPointF & xyMouse,
                         IA::ID activeObjID,
                         bool check_in_box);
    void checkClosestFixPoint(QPointF & xyMouse,
                        IA::ID activte_obj_id);
    QList<QPointF> getFixPoints(IA::Object *obj);
    QTimer *timer;
};

#endif /*ANNOTATIONPIXMAPWIDGET_H_*/
