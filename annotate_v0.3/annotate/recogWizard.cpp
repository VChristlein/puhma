
#include "recogWizard.h"

#include <QLabel>
#include <QDebug>
#include <QVBoxLayout>
#include <QStringListModel>
#include <QErrorMessage>

/*---------------------RecogProcessPage--------------------------*/
RecogProcessPage :: RecogProcessPage(RecogModel **rm, puhma::AttributeEmbed *ae, 
        const cv::Mat &img, RecogResult *rr, QWidget *parent)
    :QWizardPage(parent), rm(rm), ae(ae), img(img), rr(rr){
    setTitle("Process...");

    QLabel *label = new QLabel("Recognition task is now processing... "
                               "Please wait...");
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
    rt = new RecogThread(rm, ae, img, rr);
    connect(rt, SIGNAL(finished()), parent, SLOT(next()));
    setCommitPage(true);
}

void RecogProcessPage :: showEvent(QShowEvent *ev)
{
    QWizardPage::showEvent(ev);
    rt->start();
}

/*void RecogProcessPage :: processdone(){
    qDebug()<<"called3";
}*/

bool RecogProcessPage :: isComplete() const
{
   return false;
}

/*--------------------RecogResultPage-------------*/
RecogResultPage::RecogResultPage(RecogResult *rr, QWidget *parent):
    rr(rr), QWizardPage(parent),ui(new Ui::RecogResultPage)
{
    ui->setupUi(this);
}

void RecogResultPage::initializePage(){
    QList<QString> model;
    for(auto el : rr->candi){
        model.append(QString::fromStdString(el));
    }
    ui->listView->setModel(new QStringListModel(model));
}

bool RecogResultPage ::validatePage(){
    QModelIndexList list = ui->listView->selectionModel()->selectedIndexes();
    if(list.size()==0){
        QErrorMessage *error = new QErrorMessage(this);
        std::stringstream ss;
        ss << "Please choose one from the list.";
        error->showMessage(QString(ss.str().c_str() ));
        return false;
    }
    //rr->result = 
    rr->result = (list[0].data(Qt::DisplayRole ).toString());
    return true;
}

/*-------------------RecogWizard---------------------*/
RecogWizard::RecogWizard(RecogModel **rm, RecogResult *rr, const cv::Mat &img, 
        puhma::AttributeEmbed *ae, QWidget *parent)
    :QWizard(parent), rm(rm), rr(rr), img(img), ae(ae){
    setWindowTitle("Recognition");
    setPage(Page_Process, new RecogProcessPage(rm, ae, img, rr, this));
    setPage(Page_Result, new RecogResultPage(rr,this));
}
