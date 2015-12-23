#ifndef PROPERTYDIALOG_H
#define PROPERTYDIALOG_H

#include <QDialog>
#include <QMap>
#include <ImgAnnotations.h>

class AnnotationsPixmapWidget;
class QComboBox;
class QTableWidget;


namespace Ui {
class PropertyDialog;
}

class PropertyDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PropertyDialog(QWidget *parent,
							IA::ImgAnnotations & annotations,
                            QStringList properties
                            );
    ~PropertyDialog();
    // according to:
    // http://stackoverflow.com/questions/8766633/how-to-determine-the-correct-size-of-a-qtablewidget
    static QSize myGetQTableWidgetSize(QTableWidget *t);
    static void myRedoGeometry(QWidget *w);

    QMap<QString,QString> getNewValues();
private:    
    Ui::PropertyDialog *ui;
    IA::ImgAnnotations & annotations;
    /// list of properties
    QStringList properties;
    /// list of values to each property
    QList<QStringList> all_values;
    /// of property-value pairs to set of ids
    std::map< QString, QSet<IA::ID> > map;

    /// fills the menu from the annotations
    void fillMenuFromAnnotation();
    /// creates the map 'map'
    void createMap();
    /// adjust the size of the combo boxes to their maximum content
    void adjustComboBoxSize(QList<QComboBox*> & combos);
    // holds whether a row is edited or not
    std::vector<bool> row_edited;
public slots:
    int exec();
private slots:
    // which row changed in the table widget
    void changed(int row);
    void editTextGiven(int row);
};

#endif // PROPERTYDIALOG_H
