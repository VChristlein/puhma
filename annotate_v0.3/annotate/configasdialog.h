#ifndef CONFIGASDIALOG_H
#define CONFIGASDIALOG_H

#include <QDialog>
#include <QMap>
//#include <ImgAnnotations.h>

#ifndef Q_MOC_RUN
#include "attributes_config.h"
#include "attributes_core.h"
#endif

#include "processdialog.h"

//class AnnotationsPixmapWidget;
class QComboBox;


namespace Ui {
class ConfigASDialog;
}

class ConfigASDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigASDialog(QWidget *parent, puhma::AttriConfig* const config,
            puhma::AttributeEmbed** ae);
    ~ConfigASDialog();

private:    
    Ui :: ConfigASDialog *ui;

    puhma :: AttriConfig* const config;
    puhma :: AttributeEmbed** ae;
public slots:
    int exec();
private slots:
    void on_OKButton_triggered();
};

#endif // CONFIGASDIALOG_H
