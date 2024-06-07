#ifndef PATIENTFILTER_WIDGET_HPP
#define PATIENTFILTER_WIDGET_HPP

#include <QDebug>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QString>

#include "xmlPatientList.hpp"

class PatientFilterWidget : public QDialog
{
    Q_OBJECT

public:
    PatientFilterWidget(xmlPatientList* patientList);
    ~PatientFilterWidget();

    QString getSelectedItem();

private:
    QListWidget *idList;
    QStringList patientIDs;
    QStringList filteredPatientIDs;

private slots:
    void filterQuery(QString text);
};

#endif //PATIENTFILTER_WIDGET_HPP
