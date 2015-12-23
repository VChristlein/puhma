#pragma once

#include <QWizard>
#include <QWizardPage>

#ifndef Q_MOC_RUN
#include "attributes_config.h"
#include "attributes_core.h"
#endif

class PrepareWizard : public QWizard{
    Q_OBJECT 

public:
    enum{Page_TYPE,Page_CHECK,Page_DATASET,Page_DATASETPROC,
         Page_PHOC,Page_PHOCPROC,Page_FV, Page_FVPROC,
         Page_AS, Page_ASPROC, Page_CS, Page_CSPROC,
         Page_FINAL};
    PrepareWizard(puhma::AttriConfig* const config,
                  puhma::AttributeEmbed** ae,
                  QWidget *parent = 0);
    ~PrepareWizard(){};

public slots:
    void fitContents(int id);
    void reject();

private:
    puhma::AttriConfig* const config;
    puhma::AttributeEmbed **ae;
};
