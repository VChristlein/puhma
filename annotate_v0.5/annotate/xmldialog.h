#pragma once
#ifndef XMLDIALOG_H
#define XMLDIALOG_H

#include <QDialog>

namespace Ui {
class XmlDialog;
}

class XmlOptions : public QDialog
{
    Q_OBJECT
    
public:
    explicit XmlOptions(QWidget *parent,
                        QString xml_header,
                        QString xml_footer,
                        QString zone_header,
                        QString zone_snippet,
                        QString zone_footer,
                        QString prefix,
                        bool one_file);
    ~XmlOptions();

    QString getXmlHeader() const;
    QString getXmlFooter() const;
    QString getZone_header() const;
    QString getZone_snippet() const;
    QString getZone_footer() const;
    QString getPrefix() const;
    bool getOne_file() const;

    void setXml_header(const QString &value);
    void setXml_footer(const QString &value);
    void setZone_header(const QString &value);
    void setZone_snippet(const QString &value);
    void setZone_footer(const QString &value);
    void setPrefix(const QString &value);
    void setOne_file(bool value);

private:
    Ui::XmlDialog *ui;
    QString xml_header,
        xml_footer,
        zone_header,
        zone_snippet,
        zone_footer,
        prefix;
    bool one_file;

    void setValues();
public slots:
    int exec();
private slots:
    void on_copyHeaderButton_clicked();
    void on_copyFooterButton_clicked();
    void on_buttonBox_accepted();
};

#endif
