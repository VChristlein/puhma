#include "snippetsdialog.h"
#include "ui_snippetsdialog.h"

#include <iostream>
#include <QFileDialog>
#include <QErrorMessage>
#include <QDebug>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "ImgAnnotations.h"
#include "imageChoper.h"

SnippetsDialog:: SnippetsDialog(QWidget *parent,
                                const QString & _database_path,
                                const IA::ImgAnnotations & _ia,
                                const QVector<QString> & _snippet_labels,
                                const QVector<QString> & _snippet_text,
                                const QString & _output_folder)
    : QDialog(parent),
      database_path(_database_path),
      ia(_ia),
      snippet_labels(_snippet_labels),
      snippet_text(_snippet_text),
      output_folder(_output_folder),
      ui(new Ui::SnippetsDialog)
{

    ui->setupUi(this);

    if( output_folder.isEmpty() )
        output_folder = "./";

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onOKButton_triggered()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    ui->snippetOutputFolderLineEdit->setText(output_folder);
    ui->snippetComboBox->addItems(snippet_labels.toList());
    ui->snippetComboBox->adjustSize();
}

SnippetsDialog::~SnippetsDialog()
{
    delete ui;
}

QString SnippetsDialog::getOutput_folder() const
{
    return output_folder;
}

int SnippetsDialog :: exec()
{    
    layout()->setSizeConstraint(QLayout::SetMinimumSize);

    return QDialog::exec();
}

void SnippetsDialog :: onOKButton_triggered()
{
    if ( ui->snippetOutputFolderLineEdit->text().isEmpty() ){
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Please insert an inputfolder");
        return;
    }
    if ( ui->snippetTextEdit->toPlainText().isEmpty() ){
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Please use a valid (not empty) snippet");
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // outputfolder
    output_folder = ui->snippetOutputFolderLineEdit->text();
    QDir out_dir(output_folder);
    // create if needed
    if (!out_dir.exists()
            && !out_dir.mkpath(output_folder))
    {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Cannot create output folder");
        return;
    }

    // resizing?
    bool resize = false;
    int height = 0;
    if ( ui->heightCheckBox->isChecked() )
    {
        resize = true;
        height = ui->heightSpinBox->value();
    }
    bool fill = false;
    // fill color for cut-out snippet
    uchar fill_color = 0;
    if (ui->whiteRadioButton->isChecked()){
        fill_color = 255;
        fill = true;
    } else if ( ui->blackRadioButton->isChecked() ){
        fill = true;
    }

    // get all snippet ids and extract the snippets
    IA::DirList dirs = ia.getDirs();
    // note: probably we don't need to iterate over the dirs since everything
    // should be saved at the database_path
    for( IA::Dir* dir : dirs )
    {
        IA::FileList files = dir->getFiles();
        for( IA::File * file : files)
        {
            QString img_fname = QFileInfo(database_path,
                                          QFileInfo(QString::fromStdString(dir->getDirPath()),
                                                    QString::fromStdString(file->getFilePath())).filePath()).canonicalFilePath();

            cv::Mat img;
            bool visited = false;
            for( IA::Object *obj : file->getObjects() )
            {
                if (obj->get("snippet_raw") == ui->snippetTextEdit->toPlainText().toStdString())
                {
                    if( !ui->valueLineEdit->text().isEmpty()
                            && ui->valueLineEdit->text().toStdString() != obj->get("value") )
                        continue;

                    // only load new image if not visited
                    if( !visited ){                        
                        img = cv::imread( img_fname.toStdString() );
                        if( img.empty() ){
                            qDebug() << "cannot read image: " << img_fname;
                            continue;
                        }
                        if( img.depth() != CV_8U){
                            qDebug() << "unsuported image fileformat for:" << img_fname;
                            continue;
                        }
                        visited = true;
                    }

                    // get bbox and mask
                    cv::Mat1b mask;
                    cv::Rect bbox = ImageChoper::getBBox(obj, mask);
                    if( bbox.area() == 0 ){
                        continue;
                    }
                    cv::Mat snip = img(bbox);
                    cv::Mat snip_result;
                    // fill non-masked pixels
                    if( !mask.empty() && fill ){
                        snip_result = snip.clone();
                        for( int y = 0; y < snip.rows; y++){
                            for( int x = 0; x < snip.cols; x++){
                                if ( mask(y,x) > 0)
                                    continue;
                                if( snip.type() == CV_8UC3 ){
                                    snip_result.at<cv::Vec3b>(y,x) = cv::Vec3b(fill_color,fill_color,fill_color);
                                }
                                else if( snip.type() == CV_8UC1 ){
                                    snip_result.at<uchar>(y,x) = fill_color;
                                } else {
                                    qDebug() << "WARNING: unsuported image format";
                                    continue;
                                }
                            }
                        }
                    } else {
                        snip_result = snip;
                    }
                    if( resize ){
                        int width = std::max(1, static_cast<int>(snip.cols * (static_cast<float>(height) / snip.rows)));
//                        cv::Mat tmp;
                        cv::resize(snip_result, snip_result, cv::Size(width, height));
//                        snip_result = tmp;
                    }
                    // write snippet
                    QFileInfo finfo(img_fname);
                    QString base = finfo.baseName();                    
                    base += '_'
                            + QString::number(obj->getID())
                            + "__"
                            + QString::fromStdString(obj->get("value"))
                            + ".png";

                    QFileInfo fi(output_folder, base);
                    cv::imwrite(fi.filePath().toStdString(), snip_result);
                }
            }
        }
    }

    QApplication::restoreOverrideCursor();

    //    ProcessDialog *process_dialog = new ProcessDialog(this, &puhma::AttributeEmbed::prepare, *ae);

    //    process_dialog->show();
    //    QTimer::singleShot(0, process_dialog, SLOT(process()));
    //    process_dialog->exec();
    //    delete process_dialog;
    this->accept();
}

void SnippetsDialog::on_OutputFolderSelector_clicked()
{
    QString file = QFileDialog::getExistingDirectory(this,
                                                     "Select Output Folder",
                                                     output_folder,
                                                     QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
    ui->snippetOutputFolderLineEdit->setText(file);
}

void SnippetsDialog::on_snippetComboBox_currentIndexChanged(int index)
{
    ui->snippetTextEdit->setPlainText(snippet_text[index]);
}
