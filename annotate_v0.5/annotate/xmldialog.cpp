#include "xmldialog.h"
#include "ui_xmldialog.h"

#include <QDebug>
#include <QString>

XmlOptions::XmlOptions(  QWidget *parent,
                         QString _xml_header,
                         QString _xml_footer,
                         QString _zone_header,
                         QString _zone_snippet,
                         QString _zone_footer,
                         QString _prefix,
                         bool _one_file) :
    QDialog(parent),
    ui(new Ui::XmlDialog),
    xml_header(_xml_header),
    xml_footer(_xml_footer),
    zone_header(_zone_header),
    zone_snippet(_zone_snippet),
    zone_footer(_zone_footer),
    prefix(_prefix),
    one_file(_one_file)
{
    ui->setupUi(this);
    setValues();

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_buttonBox_accepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

XmlOptions::~XmlOptions()
{
    delete ui;
}

void XmlOptions:: setValues()
{
    ui->xmlFooterTextEdit->setText(xml_footer);
    ui->xmlHeaderTextEdit->setText(xml_header);
    ui->zoneFooterTextEdit->setText(zone_footer);
    ui->zoneSnippetTextEdit->setText(zone_snippet);
    ui->zoneHeaderTextEdit->setText(zone_header);
    ui->prefixLineEdit->setText(prefix);
    ui->oneFileCheckBox->setChecked(one_file);
}

QString XmlOptions::getXmlHeader() const
{
    return xml_header;
}
QString XmlOptions::getXmlFooter() const
{
    return xml_footer;
}
QString XmlOptions::getZone_header() const
{
    return zone_header;
}
QString XmlOptions::getZone_snippet() const
{
    return zone_snippet;
}
QString XmlOptions::getZone_footer() const
{
    return zone_footer;
}
QString XmlOptions::getPrefix() const
{
    return prefix;
}
bool XmlOptions::getOne_file() const
{
    return one_file;
}
void XmlOptions::setXml_header(const QString &value)
{
    xml_header = value;
}
void XmlOptions::setXml_footer(const QString &value)
{
    xml_footer = value;
}
void XmlOptions::setZone_header(const QString &value)
{
    zone_header = value;
}
void XmlOptions::setZone_snippet(const QString &value)
{
    zone_snippet = value;
}
void XmlOptions::setZone_footer(const QString &value)
{
    zone_footer = value;
}
void XmlOptions::setPrefix(const QString &value)
{
    prefix = value;
}
void XmlOptions::setOne_file(bool value)
{
    one_file = value;
}

int XmlOptions :: exec()
{    
    setValues();

    return QDialog::exec();
}

void XmlOptions::on_copyHeaderButton_clicked()
{
   ui->zoneHeaderTextEdit->setText(ui->xmlHeaderTextEdit->toPlainText());
}

void XmlOptions::on_copyFooterButton_clicked()
{
    ui->zoneFooterTextEdit->setText(ui->xmlFooterTextEdit->toPlainText());
}

void XmlOptions::on_buttonBox_accepted()
{
    xml_footer = ui->xmlFooterTextEdit->toPlainText();
    xml_header = ui->xmlHeaderTextEdit->toPlainText();
    zone_footer = ui->zoneFooterTextEdit->toPlainText();
    zone_snippet = ui->zoneSnippetTextEdit->toPlainText();
    zone_header = ui->zoneHeaderTextEdit->toPlainText();
    prefix = ui->prefixLineEdit->text();
    one_file = ui->oneFileCheckBox->isChecked();
    this->accept();
}

