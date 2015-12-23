#include "preparecheckpage.h"
#include "ui_preparecheckpage.h"
#include "prepareWizard.h"

#include<QDebug>
#include<QTimer>
#include<QtConcurrentRun>
#include<QAbstractButton>

PrepareCheckPage::PrepareCheckPage(puhma::AttributeEmbed *ae, QWidget *parent) :
    QWizardPage(parent), ae(ae),
    ui(new Ui::PrepareCheckPage)
{
    ui->setupUi(this);

    QObject::connect(&watcher, SIGNAL(finished()),this, SLOT(checked()));
    QObject::connect(this, SIGNAL(goNext()), parent, SLOT(next()));
}

void PrepareCheckPage::initializePage(){
    checkstatus = -1;
    watcher.setFuture(QtConcurrent::run(ae, &puhma::AttributeEmbed::check));
    QTimer::singleShot(0, this, SLOT(disableBack()));
}

bool PrepareCheckPage::isComplete() const{
    return false;
}

void PrepareCheckPage::checked(){
    checkstatus = watcher.result();
    emit goNext();
}

void PrepareCheckPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

int PrepareCheckPage::nextId() const{
    if(checkstatus == 9){
        return PrepareWizard::Page_FINAL;
    }else if(checkstatus == 0){
        return PrepareWizard::Page_DATASET;
    }else{
        return PrepareWizard::Page_PHOC;
    }
}

PrepareCheckPage::~PrepareCheckPage()
{
    delete ui;
}
