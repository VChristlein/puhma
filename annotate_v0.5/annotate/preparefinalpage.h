#ifndef PREPAREFINALPAGE_H
#define PREPAREFINALPAGE_H

#include <QWizardPage>

namespace Ui {
class PrepareFinalPage;
}

class PrepareFinalPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareFinalPage(QWidget *parent = 0);
    ~PrepareFinalPage();
    void initializePage();

public slots:
    void disableBack();
private:
    Ui::PrepareFinalPage *ui;
};

#endif // PREPAREFINALPAGE_H
