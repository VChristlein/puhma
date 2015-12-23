#include "prepareprocesspage.h"
#include "ui_prepareprocesspage.h"

#include<QDebug>
#include<QtConcurrentRun>
#include<QTimer>
#include<QAbstractButton>

PrepareProcessPage::PrepareProcessPage(puhma::AttributeEmbed *ae,
                                                     void (puhma::AttributeEmbed::*f)(),
                                                     QWidget *parent) :
    QWizardPage(parent), f(f), ae(ae),
    ui(new Ui::PrepareProcessPage)
{
    ui->setupUi(this);
    pt = new ProcessThread(ae,f);
    sc = new StatusChecker(ae,pt);
    connect(sc,SIGNAL(doUp()),this,SLOT(updateStatusLabel()));
    connect(pt,SIGNAL(finished()),parent,SLOT(next()));
    //connect(pt,SIGNAL(finished()),pt,SLOT(deleteLater()));
    connect(pt,SIGNAL(finished()),this,SLOT(killSC()));
    connect(this,SIGNAL(scKilled()),pt,SLOT(deleteLater()));
    connect(sc,SIGNAL(finished()),sc,SLOT(deleteLater()));
    //connect(&watcher, SIGNAL(finished()), parent, SLOT(next()));
}

void PrepareProcessPage::initializePage(){
    //qDebug()<<f;
    QTimer::singleShot(0, this, SLOT(disableBack()));
    //qDebug()<<"main thread:"<<QThread::currentThreadId();
    pt->start();
    sc->start();
}

void PrepareProcessPage::rejectclicked(){
    //qDebug()<<"called";
    pt->terminate();
    pt->wait();
}

void PrepareProcessPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

bool PrepareProcessPage::isComplete() const{
    return false;
}

void PrepareProcessPage :: updateStatusLabel(){
    ui->lab_status2->setText(QString::fromStdString(ae->getStatistic()->getStatus()));
    //qDebug()<<"called";
}

PrepareProcessPage::~PrepareProcessPage()
{
    delete ui;
}

void PrepareProcessPage::killSC(){
    sc->quit();
    sc->wait();
    emit scKilled();
}
