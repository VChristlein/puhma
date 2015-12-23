#include "imageChoper.h"
#include "functions.h"

#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QRegExp>
#include <QRectF>
#include <QPointF>
#include <QStringList>

cv::Mat ImageChoper :: chopOneFromImage(IA::ImgAnnotations *ia,
                                        IA::ID id,
                                        std::string curPath)
{
    cv::Mat1b mask;
    cv::Rect bbox;
    if(!(ia->getObject(id)->get("fixpoints").empty()))
    {
            //fixpoints
            QList<QPointF> fixPoints = str2points(QString::fromStdString(ia->getObject(id)->get("fixpoints")));
            //get bbox from fixpoints
            std::vector<cv::Point> cv_points;
            for(auto point:fixPoints){
                cv_points.push_back(cv::Point(static_cast<int>(point.x()),
                            static_cast<int>(point.y())));
            }
            bbox = cv::boundingRect(cv_points);
            // create mask
            mask = cv::Mat1b::zeros(bbox.height,bbox.width);
            // relate points to origin
            for( size_t i = 0; i < cv_points.size(); i++){
                cv_points[i] -= bbox.tl();
            }
            cv::fillPoly(mask, cv_points, cv::Scalar::all(255));
    }
    else if(!(ia->getObject(id)->get("bbox").empty()))
    {
        //get bbox
        QRectF box = str2rect(QString::fromStdString(ia->getObject(id)->get("bbox")));
        bbox = cv::Rect(static_cast<int>(box.left()),
                static_cast<int>(box.top()),
                static_cast<int>(box.width()),
                static_cast<int>(box.height()));
    }
    else{
        return cv::Mat();
    }
    cv::Mat raw = cv::imread(curPath);

    cv::Mat chopped(bbox.height, bbox.width, raw.type(), 255);
    cv::Mat tmp = cv::Mat(raw, bbox);
    if( !mask.empty() )
        tmp.copyTo(chopped, mask);
    else
        chopped = tmp;

    return chopped;
}
