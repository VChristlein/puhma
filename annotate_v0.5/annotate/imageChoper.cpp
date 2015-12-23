#include "imageChoper.h"
#include "functions.h"

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QDebug>
#include <QRegExp>
#include <QRectF>
#include <QPointF>
#include <QStringList>

cv::Rect ImageChoper:: getBBox(const IA::Object *obj, cv::Mat1b & mask)
{
    if (obj->get("fixpoints").empty() && obj->get("bbox").empty() ){
        qDebug() << "Obj" << obj->getID() << "doesnt have fixpoints nor bbox";
        return cv::Rect();
    }

    if(!(obj->get("fixpoints").empty()))
    {
        //fixpoints
        QList<QPointF> fixPoints = str2points(QString::fromStdString(obj->get("fixpoints")));
        //get bbox from fixpoints
        std::vector<cv::Point> cv_points;
        for(const QPointF & point : fixPoints){
            cv_points.push_back(cv::Point(static_cast<int>(round(point.x())),
                                          static_cast<int>(round(point.y()))));
        }
        cv::Rect bbox = cv::boundingRect(cv_points);
        // create mask
        mask = cv::Mat1b::zeros(bbox.height, bbox.width);
        // relate points to origin
        for( size_t i = 0; i < cv_points.size(); i++){
            cv_points[i] -= bbox.tl();
        }

        // really stupid syntax of fillPoly...
        const int n_points = static_cast<int>(cv_points.size());
        const cv::Point *ppt = &cv_points[0];
        cv::fillPoly(mask, &ppt, &n_points, 1, cv::Scalar::all(255));
        return bbox;
    }

    // else
    //get bbox
    QRectF box = str2rect(QString::fromStdString(obj->get("bbox")));
    cv::Rect bbox = cv::Rect(static_cast<int>(box.left()),
                             static_cast<int>(box.top()),
                             static_cast<int>(box.width()),
                             static_cast<int>(box.height()));
    return bbox;
}

cv::Mat ImageChoper:: chopOneFromImage(IA::ImgAnnotations *ia,
                                        IA::ID id,
                                        const std::string & curPath)
{


    IA::Object * obj = ia->getObject(id);
    cv::Mat1b mask;
    cv::Rect bbox = getBBox(obj, mask);
    if (bbox.area() == 0)
        return cv::Mat();

    cv::Mat raw = cv::imread(curPath);

    // mask outer polygon pixels
//    cv::Mat chopped(bbox.height, bbox.width, raw.type(), 255);
//    cv::Mat tmp = cv::Mat(raw, bbox);
//    if( !mask.empty() )
//        tmp.copyTo(chopped, mask);
//    else
//        chopped = tmp;
//    return chopped;

    // just return snippet
    return cv::Mat(raw, bbox);
}
