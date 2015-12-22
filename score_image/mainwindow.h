#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QStringList files;
    int index;
    QFile *log_file;

    void showCurrentImage();
private slots:
    void on_pushButtonBrowse_clicked();
    void on_pushButton_ok_clicked();
    void on_pushButton_fail_clicked();
    void on_pushButton_logFile_clicked();
    void on_pushButton_notPerfect_clicked();
};

#endif // MAINWINDOW_H
