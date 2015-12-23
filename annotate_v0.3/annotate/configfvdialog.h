#ifndef CONFIGFVDIALOG_H
#define CONFIGFVDIALOG_H

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
class QTableWidget;


namespace Ui {
class ConfigFVDialog;
}

class ConfigFVDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigFVDialog(QWidget *parent, puhma::AttriConfig* const config,
            puhma::AttributeEmbed** ae);
    ~ConfigFVDialog();

    QString updateStatus(ProcessDialog *pd);
    //int doUp(ProcessDialog *pd);

private:    
    Ui :: ConfigFVDialog *ui;

    puhma :: AttriConfig* const config;
    puhma :: AttributeEmbed** ae;
public slots:
    int exec();
private slots:
    void on_OKButton_triggered();
    void output_folder_select();
    void input_folder_select();
};

#endif // CONFIGFVDIALOG_H
