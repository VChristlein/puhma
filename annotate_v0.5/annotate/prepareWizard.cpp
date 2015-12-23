#include "prepareWizard.h"
#include "preparetypepage.h"
#include "preparecheckpage.h"
#include "preparefinalpage.h"
#include "preparedatasetpage.h"
#include "preparephocpage.h"
#include "prepareprocesspage.h"
#include "preparefvpage.h"
#include "prepareaspage.h"
#include "preparecspage.h"

#include <QDebug>
#include <QAbstractButton>
#include <QTimer>

/*-------------------RecogWizard---------------------*/
PrepareWizard::PrepareWizard(puhma::AttriConfig *const config,
                             puhma::AttributeEmbed **ae, QWidget *parent)
    :QWizard(parent), config(config), ae(ae){
    if(*ae == nullptr){
        *ae = new puhma::AttributeEmbed(*config);
    }
    setWindowTitle("Prepare the recognition task");
    setPage(Page_TYPE, new PrepareTypePage(*ae, this));
    setPage(Page_CHECK, new PrepareCheckPage(*ae,this));

    setPage(Page_DATASET, new PrepareDatasetPage(*ae,this));
    setPage(Page_DATASETPROC, new PrepareProcessPage(*ae,&puhma::AttributeEmbed::prepare,this));

    setPage(Page_PHOC, new PreparePhocPage(*ae,this));
    setPage(Page_PHOCPROC, new PrepareProcessPage(*ae, &puhma::AttributeEmbed::labelEmbed,this));

    setPage(Page_FV, new PrepareFVPage(*ae,this));
    setPage(Page_FVPROC, new PrepareProcessPage(*ae,&puhma::AttributeEmbed::fvRep,this));

    setPage(Page_AS, new PrepareASPage(*ae,this));
    setPage(Page_ASPROC, new PrepareProcessPage(*ae,&puhma::AttributeEmbed::trainAS,this));

    setPage(Page_CS, new PrepareCSPage(*ae,this));
    setPage(Page_CSPROC, new PrepareProcessPage(*ae,&puhma::AttributeEmbed::calibrateNoEva,this));

    setPage(Page_FINAL, new PrepareFinalPage(this));

    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(fitContents(int)));
    //disconnect( button( QWizard::CancelButton ), SIGNAL( clicked() ), this, SLOT( reject() ) );
    //connect( button( QWizard::CancelButton ), SIGNAL( clicked() ), this, SLOT( cancelClicked() ) );
}

void PrepareWizard::fitContents(int id){
    adjustSize();
}

void PrepareWizard::reject(){
    if(currentId()==Page_DATASETPROC||
            currentId()==Page_PHOCPROC||
            currentId()==Page_FVPROC||
            currentId()==Page_ASPROC||
            currentId()==Page_CSPROC){
        ((PrepareProcessPage*)currentPage())->rejectclicked();
    }
    QDialog::reject();
}
