#pragma once

#include <QWizard>
#include <QWizardPage>

#include "recogModel.h"
#include "recogThread.h"
#include "ui_recogResultPage.h"

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui{
    class RecogResultPage;
}

class RecogProcessPage : public QWizardPage{
    Q_OBJECT

public:
    RecogProcessPage(RecogModel **rm, puhma::AttributeEmbed *ae, 
            const cv::Mat &img,
            RecogResult *rr,
            QWidget *parent =0);
    bool isComplete() const;
private:
    RecogModel **rm;
    puhma::AttributeEmbed *ae;
    RecogThread *rt;
    const cv::Mat img; 
    RecogResult *rr;
protected:
    void showEvent(QShowEvent *ev);
signals:
    void processdone();
};

class RecogResultPage : public QWizardPage{
    Q_OBJECT
public:
    RecogResultPage(RecogResult *rr, QWidget *parent =0);
    void initializePage();
    bool validatePage();
private:
    Ui :: RecogResultPage *ui;
    RecogResult *rr;
};

class RecogWizard : public QWizard{
    Q_OBJECT 

    enum{Page_Process, Page_Result};
public:
    RecogWizard(RecogModel **rm, RecogResult *rr, const cv::Mat &img, 
            puhma::AttributeEmbed *ae, QWidget *parent = 0);
    ~RecogWizard(){};
private:
    RecogModel **rm;
    puhma::AttributeEmbed *ae;
    const cv::Mat img; 
    RecogResult *rr;
};
