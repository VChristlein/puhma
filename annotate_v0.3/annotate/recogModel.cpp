#include "recogModel.h"
#include "pcagmm.h"
#include "fisher.h"
#include "trans_phoc.h"


RecogModel :: RecogModel(puhma::AttributeEmbed *ae){
    puhma::PCAGMM *pcagmm = new puhma::PCAGMM(ae->config, ae->filePathManager, ae->st); 
    pcagmm->get(ae->dataprepare, pca, meansEM, covsEM, weightsEM);
    puhma::FV *fv = new puhma::FV(ae->config, ae->filePathManager, ae->st);
    puhma::TransPhoc *tp = new puhma::TransPhoc(ae->config, ae->filePathManager);
    attM = new puhma::AttriModel(tp->getSize().width,fv->getSize().width, ae->filePathManager);
    attM->reload();
}
