#include "propertydialog.h"
#include "ui_propertydialog.h"
#include "functions.h"

#include <QComboBox>
#include <QDebug>
#include <QSignalMapper>
#include <QString>
#include <QStandardItemModel>

PropertyDialog::PropertyDialog(  QWidget *parent,
		IA::ImgAnnotations & _annotation,
		QStringList _properties
		) :
    QDialog(parent),
    ui(new Ui::PropertyDialog),
    annotations(_annotation)
{
    ui->setupUi(this);
    properties = _properties;
    properties.removeFirst();
}

PropertyDialog::~PropertyDialog()
{
    delete ui;
}

void PropertyDialog :: fillMenuFromAnnotation()
{
    ui->tableWidget->setRowCount(properties.size());
    ui->tableWidget->horizontalHeader()->stretchLastSection();
    ui->tableWidget->setVerticalHeaderLabels(properties);
//    ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    QSignalMapper* signal_mapper = new QSignalMapper(this);

    int row = 0;    
    QList<QComboBox*> all_combos;
    foreach ( QString property, properties ) {
        // get all values according to the properties
        QStringList values = std2qt( annotations.getAllObjPropertyValues(property.toStdString()) );

        // copied from createMap()
//        foreach( QString value, values )
//        {
//            QSet<IA::ID> set;
//            foreach( IA::Object * obj, annotations.getObjects() )
//            {
//                if (obj->isSet(property.toStdString())
//                        && obj->get(property.toStdString()) == value.toStdString())
//                {
//                    set.insert(obj->getID());
//                }
//            }
//            map[QString("%1-%2").arg(property,value)] = set;
//        }
        // end copied from createMap()

        all_values.push_back(values);
        values.sort();
        // add them to a combobox
        QComboBox * combo = new QComboBox(ui->tableWidget);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        combo->addItems(values);
        combo->setEditable(true);
        combo->clearEditText();
        // TODO: if obj is already annotated, we should put here its annotation
//        combo->setEditText();
//        combo->setMinimumContentsLength(20);

        // connect to signalmapper such that we get informed when sth changed
        connect( combo, SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()) );
        connect( combo, SIGNAL(editTextChanged(QString)), signal_mapper, SLOT(map()) );
        signal_mapper->setMapping( combo, row );       
        ui->tableWidget->setCellWidget( row, 0, combo );

        // check-icon-item
        // alternatively, the combobox can be made checkable by
        // adding a new model to it (but didn't work for me)
        QTableWidgetItem * item = new QTableWidgetItem();
        item->setFlags(Qt::ItemIsEnabled);
        ui->tableWidget->setItem(row, 1, item);
        item->setCheckState(Qt::Unchecked);

        all_combos.push_back(combo);
        row++;
    }
    connect( signal_mapper, SIGNAL(mapped(int)), this, SLOT(changed(int)) );
    connect( signal_mapper, SIGNAL(mapped(int)), this, SLOT(editTextGiven(int)) );

    adjustComboBoxSize(all_combos);

//    ui->tableWidget->setColumnWidth(1, 18);
    ui->tableWidget->resizeColumnsToContents();    
    row_edited = std::vector<bool>(properties.size(), false);
}

// creates map of property-value <-> set of ids
void PropertyDialog :: createMap()
{
    foreach ( QString property, properties )
    {
        QStringList values = std2qt( annotations.getAllObjPropertyValues(property.toStdString()) );
        foreach( QString value, values )
        {
            QSet<IA::ID> set;
            foreach( IA::Object * obj, annotations.getObjects() )
            {
                if (obj->isSet(property.toStdString())
                        && obj->get(property.toStdString()) == value.toStdString())
                {
                    set.insert(obj->getID());
                }
            }
            map[QString("%1-%2").arg(property,value)] = set;
        }
    }
}

void PropertyDialog :: adjustComboBoxSize(QList<QComboBox *> & all_combos)
{
    // adjust combobox size
    // get maximum size
    int max_width = 0;
    foreach ( QComboBox* combo, all_combos ) {
        int width = combo->minimumSizeHint().width();
        if ( width > max_width ) {
            max_width = width;
        }
    }
    // set maximum size
    foreach ( QComboBox* combo, all_combos ) {
        combo->setMinimumWidth(max_width);
    }
}

// according to:
// http://stackoverflow.com/questions/8766633/how-to-determine-the-correct-size-of-a-qtablewidget
QSize PropertyDialog :: myGetQTableWidgetSize(QTableWidget *t) {
   int w = t->verticalHeader()->width() + 4; // +4 seems to be needed
   for (int i = 0; i < t->columnCount(); i++)
      w += t->columnWidth(i); // seems to include gridline (on my machine)
   int h = t->horizontalHeader()->height() + 3;
   for (int i = 0; i < t->rowCount(); i++)
      h += t->rowHeight(i);
   return QSize(w, h);
}

void PropertyDialog :: myRedoGeometry(QWidget *w) {
   const bool vis = w->isVisible();
   const QPoint pos = w->pos();
   w->hide();
   w->show();
   w->setVisible(vis);
   if (vis && !pos.isNull())
       w->move(pos);
}

QMap<QString, QString> PropertyDialog :: getNewValues()
{
    QMap<QString, QString> value_of_property;
    for ( int i = 0; i < properties.size(); i++ ) {
         QComboBox* curr_combo = static_cast<QComboBox*>( ui->tableWidget->cellWidget(i, 0) );
         QString value = curr_combo->currentText();
         value_of_property[ properties[i] ] = value;
    }
    return value_of_property;
}

int PropertyDialog :: exec()
{    
    layout()->setSizeConstraint(QLayout::SetMinimumSize);
    fillMenuFromAnnotation();
//    createMap();

    // adjust size of widget to size of table
    myRedoGeometry(this);
    ui->tableWidget->setMaximumSize(myGetQTableWidgetSize(ui->tableWidget));
    ui->tableWidget->setMinimumSize(ui->tableWidget->maximumSize()); // optional

    return QDialog::exec();
}

void PropertyDialog :: editTextGiven(int row)
{
    // check the row
    QTableWidgetItem * item = ui->tableWidget->item(row, 1);
    // individual icon:
//    QIcon icon;
//    icon.addFile( QString::fromUtf8(":/resources/check.png"), QSize(), QIcon::Normal, QIcon::Off );
//    item->setIcon(icon);
    item->setCheckState(Qt::Checked);
    row_edited[row] = true;
}

// if sth changed adjust the others accordingly, i.e. try to decrease the list of possibilities
void PropertyDialog :: changed(int row)
{
//    int row = _row.toInt();
    QComboBox* curr_combo = static_cast<QComboBox*>( ui->tableWidget->cellWidget(row, 0) );
//    qDebug() << row << curr_combo->currentText();
    if ( curr_combo->currentText().isEmpty() ){    
        return;
    }

    QString property_value = QString("%1-%2").arg(properties.at(row), curr_combo->currentText());

    std::map< QString, QSet<IA::ID> >::iterator it;
    it = map.find(property_value);
    // don't need to adjust as no real change occured
    if ( it == map.end() || it->second.isEmpty() )
        return;

    QSet<IA::ID> set = it->second;
    // get all property-value-sets which are already set
    for( int r = 0 ; r < ui->tableWidget->rowCount(); r++ )
    {
        // skip already used row
        if ( r == row )
            continue;
        // get combobox at this row
        QComboBox * combo = static_cast<QComboBox*>( ui->tableWidget->cellWidget(r, 0) );
        // but only those which are already set
        if ( combo->currentText().isEmpty())
            continue;

        // get the set
        QString prop_val = QString("%1-%2").arg(properties.at(r), combo->currentText());
        std::map< QString, QSet<IA::ID> >::iterator it_tmp;
        it_tmp = map.find(prop_val);
        if ( it == map.end() || it->second.isEmpty() )
            continue;
        // intersect the ids
        set = set.intersect(it->second);
    }

    // block signals
    for( int r = 0 ; r < ui->tableWidget->rowCount(); r++ )
    {
        QComboBox* combo = static_cast<QComboBox*>( ui->tableWidget->cellWidget(r, 0) );
        combo->blockSignals(true);
    }

    // use values as new options for the combo boxes
    for( int r = 0 ; r < ui->tableWidget->rowCount(); r++ )
    {
        QComboBox* combo = static_cast<QComboBox*>( ui->tableWidget->cellWidget(r, 0) );
        if ( r == row )
            continue;
        QSet<QString> values;
        // get the values from the id-set
        QString prop = properties.at(r);
        std::string property = prop.toStdString();
        foreach ( IA::ID id, set )
        {
            IA::Object * obj = annotations.getObject(id);
            if ( obj->isSet(property)
                 && !obj->get(property).empty() )
            {
                values.insert( QString::fromUtf8(obj->get(property).c_str()) );
            }
        }

        // make two groups: the preferred ones and the non-preferred-ones (hidden_values)
        QStringList original_values = all_values[r];
        QSet<QString> orig_values_set = original_values.toSet();
        QSet<QString> hidden_values_set = orig_values_set.subtract(values);

        QStringList hidden_values = hidden_values_set.toList();
        QStringList preferred_values = values.toList();
        hidden_values.sort();
        preferred_values.sort();
//        qDebug() << preferred_values << hidden_values;

        // backup current-text
        QString text = combo->currentText();        
        combo->clear();
        // TODO they could also be added as sets with insertItems & QRole
        // Does not work for me
//        QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
//        foreach(QString text, preferred_values) {
//           QStandardItem* item = new QStandardItem( text );
//           QFont font = item->font();
//           font.setBold( true );
//           item->setFont( font );

//           itemModel->appendRow( item );
//        }
//        combo->setModel( itemModel );

        combo->addItems(preferred_values);
        combo->insertSeparator(preferred_values.size());
        combo->addItems(hidden_values);
        // make the preferred values bold
        // doesn't work for me (probably due to the standard style)
//        for(int i = 0; i < preferred_values.size(); i++){
//            combo->setItemData(i, QFont("Arial", 10, QFont::Bold), Qt::FontRole);
//        }
        combo->clearEditText();
        // for those which have been set, replay the currenttext
        if ( row_edited[r] )
            combo->setEditText(text);
    }

    // mark row as visited with a check-icon
    editTextGiven(row);

    // un-block signals
    for( int r = 0 ; r < ui->tableWidget->rowCount(); r++ )
    {
        QComboBox* combo = static_cast<QComboBox*>( ui->tableWidget->cellWidget(r, 0) );
        combo->blockSignals(false);
    }
}
