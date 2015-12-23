#pragma once

#include <QThread>

#include "recogModel.h"

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

class RecogThread : public QThread
{
    Q_OBJECT
    public:
        RecogThread(RecogModel **rm, puhma::AttributeEmbed *ae, cv::Mat img, RecogResult *rr);
        //void stop();
    protected:
        void run();
    private:
        RecogModel **rm;
        puhma::AttributeEmbed *ae;
        cv::Mat img;
        RecogResult *rr;
        //QString messageStr;
};

