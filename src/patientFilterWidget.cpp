#include "patientFilterWidget.hpp"

PatientFilterWidget::PatientFilterWidget(xmlPatientList *patientList)
{
    setWindowTitle("Choose Patient");

    QLabel *infoText = new QLabel("Convert OCT files first, to list them for a Timeline View");
    QLabel *queryLabel = new QLabel("ID:");
    QLineEdit *queryEdit = new QLineEdit();
    QPushButton* rejectButton = new QPushButton("cancel");
    QPushButton* acceptButton = new QPushButton("OK");

    idList = new QListWidget();

    QHBoxLayout *queryLayout = new QHBoxLayout();
        queryLayout->addWidget(queryLabel);
        queryLayout->addWidget(queryEdit);
    QHBoxLayout *bottomLayout = new QHBoxLayout();
        bottomLayout->addWidget(rejectButton);
        bottomLayout->addWidget(acceptButton);
    QVBoxLayout *mainLayout = new QVBoxLayout();
        mainLayout->addWidget(infoText);
        mainLayout->addLayout(queryLayout);
        mainLayout->addWidget(idList);
        mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);

    //load patient data
    patientIDs = patientList->getPatientIDs();
    //second list, because idList will delete its content, but we want to keep all IDs for future filtering
    filteredPatientIDs = patientIDs;
    idList->addItems(filteredPatientIDs);

    //filter data on ID input
    connect(queryEdit, SIGNAL(textChanged(QString)), this, SLOT(filterQuery(QString)));

    //finish buttons
    connect(idList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));
    connect(rejectButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
}

PatientFilterWidget::~PatientFilterWidget()
{
}

void PatientFilterWidget::filterQuery(QString text)
{
    filteredPatientIDs.clear();
    filteredPatientIDs = patientIDs.filter(text);
    idList->clear();
    idList->addItems(filteredPatientIDs);
}

QString PatientFilterWidget::getSelectedItem()
{
    if (idList->currentItem() == nullptr)
        return "";
    return idList->currentItem()->text();
}
