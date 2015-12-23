#ifndef PREPARECHECKPAGE_H
#define PREPARECHECKPAGE_H

#include <QWizardPage>
#include <QFutureWatcher>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PrepareCheckPage;
}

class PrepareCheckPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareCheckPage(puhma::AttributeEmbed *ae, QWidget *parent = 0);
    ~PrepareCheckPage();
    bool isComplete() const;
    void initializePage();
    int nextId() const;

public slots:
    void checked();
    void disableBack();

signals:
    void goNext();

private:
    puhma::AttributeEmbed *ae;
    Ui::PrepareCheckPage *ui;
    QFutureWatcher<int> watcher;
    int checkstatus;
};

#endif // PREPARECHECKPAGE_H
