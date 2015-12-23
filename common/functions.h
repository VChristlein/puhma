#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <QString>
#include <QPointF>
#include <QList>
#include <QRectF>

QRectF str2rect(const QString &str);
QString rect2str(const QRectF &rect);
QList<QPointF> str2points(const QString &str);
QString points2str(const QList<QPointF> &points);

#endif /*FUNCTIONS_H_*/
