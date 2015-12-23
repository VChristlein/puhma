#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QMap>
//#include <ImgAnnotations.h>

#ifndef Q_MOC_RUN
#include "attributes_config.h"
#include "attributes_core.h"
#endif


//class AnnotationsPixmapWidget;
class QComboBox;
class QTableWidget;


namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigDialog(QWidget *parent, puhma::AttriConfig* const config,
            puhma::AttributeEmbed** ae);
    ~ConfigDialog();
private:    
    Ui :: ConfigDialog *ui;

    puhma :: AttriConfig* const config;
    puhma :: AttributeEmbed** ae;
public slots:
    int exec();
private slots:
    void on_OKButton_triggered();
    void output_folder_select();
    void input_folder_select();
    void gt_file_select();
    void dataset_type_changed();
    void trainset_select();
    void valiset_select();
    void testset_select();
};

#endif // PROPERTYDIALOG_H
