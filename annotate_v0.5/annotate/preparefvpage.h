#ifndef PREPAREFVPAGE_H
#define PREPAREFVPAGE_H

#include <QWizardPage>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PrepareFVPage;
}

class PrepareFVPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareFVPage(puhma::AttributeEmbed* ae, QWidget *parent = 0);
    ~PrepareFVPage();
    void initializePage();
    bool validatePage();

public slots:
    void disableBack();
    //void input_folder_select();

private:
    puhma::AttributeEmbed *ae;
    Ui::PrepareFVPage *ui;
};

#endif // PREPAREFVPAGE_H
