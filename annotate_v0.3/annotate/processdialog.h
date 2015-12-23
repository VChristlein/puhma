#ifndef PROCESSDIALOG_H
#define PROCESSDIALOG_H

#include <QDialog>
#include <QMap>
//#include <ImgAnnotations.h>

#ifndef Q_MOC_RUN
#include "attributes_config.h"
#include "attributes_core.h"
#endif

//class AnnotationsPixmapWidget;
class QComboBox;

namespace Ui {
class ProcessDialog;
}

class ProcessDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ProcessDialog(QWidget *parent, void (puhma::AttributeEmbed::*f)(),
            puhma::AttributeEmbed *ae);
    ~ProcessDialog();
    void updateStatusLabel2(QString st);
private:    
    Ui :: ProcessDialog *ui;

    void (puhma::AttributeEmbed::*f)();
    puhma::AttributeEmbed *ae;
signals:
    //void askUpLabel(QString st);

public slots:
    //int exec();
    void process();
private slots:
    //void upLabel(QString st);
};

#endif // PROCESSDIALOG_H
