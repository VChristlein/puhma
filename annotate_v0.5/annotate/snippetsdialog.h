#pragma once
#ifndef SNIPPETS_DIALOG_H
#define SNIPPETS_DIALOG_H

#include <QDialog>

// forward declarations
namespace IA{
class ImgAnnotations;
}
namespace Ui {
class SnippetsDialog;
}

class SnippetsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SnippetsDialog(QWidget *parent,
                            const QString & database_path,
                            const IA::ImgAnnotations & ia,
                            const QVector<QString> & snippet_labels,
                            const QVector<QString> & snippet_text,
                            const QString  &output_folder);
    ~SnippetsDialog();
    QString getOutput_folder() const;

private:
    Ui :: SnippetsDialog *ui;
    const IA::ImgAnnotations & ia;
    const QVector<QString> & snippet_labels;
    const QVector<QString> & snippet_text;
    QString output_folder;
    const QString & database_path;
public slots:
    int exec();
private slots:
    void onOKButton_triggered();
    void on_OutputFolderSelector_clicked();
    void on_snippetComboBox_currentIndexChanged(int index);
};

#endif // SNIPPETS_DIALOG_H
