#include "chooseContoursWidget.hpp"

ChooseContoursWidget::ChooseContoursWidget(int numContours)
{
    setWindowTitle("Choose contours for export");

    QPushButton* selectAllButton = new QPushButton("Select All");
    QPushButton* deselectAllButton = new QPushButton("Deselect All");

    contourList = new QListWidget();

    QLabel *colorLabel = new QLabel("Color (\"blue\", \"#RGB\"):");
    colorEdit = new QLineEdit("lightgreen");
    colorInvalid = new QLabel("The input is not a valid color.");
    colorInvalid->setStyleSheet("QLabel { color : red; }");
    colorInvalid->hide();

    QPushButton* rejectButton = new QPushButton("Cancel");
    QPushButton* acceptButton = new QPushButton("Export");

    QHBoxLayout *topLayout = new QHBoxLayout();
        topLayout->addWidget(selectAllButton);
        topLayout->addWidget(deselectAllButton);
    QHBoxLayout *colorLayout = new QHBoxLayout();
        colorLayout->addWidget(colorLabel);
        colorLayout->addWidget(colorEdit);
    QHBoxLayout *bottomLayout = new QHBoxLayout();
        bottomLayout->addWidget(rejectButton);
        bottomLayout->addWidget(acceptButton);
    QVBoxLayout *mainLayout = new QVBoxLayout();
        mainLayout->addLayout(topLayout);
        mainLayout->addWidget(contourList);
        mainLayout->addLayout(colorLayout);
        mainLayout->addWidget(colorInvalid);
        mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);

    //list all contours available
    for (int contourID = 0; contourID < numContours; ++contourID)
    {
        QListWidgetItem *listItem = new QListWidgetItem(QString("Contour ").append(QString::number(contourID)), contourList);
        listItem->setCheckState(Qt::Unchecked);
        contourList->addItem(listItem);
    }

    connect(selectAllButton, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(deselectAllButton, SIGNAL(clicked()), this, SLOT(deselectAll()));
    connect(rejectButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(validate()));
}

ChooseContoursWidget::~ChooseContoursWidget()
{
    delete contourList;
}

void ChooseContoursWidget::selectAll()
{
    for(int i = 0; i < contourList->count(); ++i)
        contourList->item(i)->setCheckState(Qt::Checked);
}

void ChooseContoursWidget::deselectAll()
{
    for(int i = 0; i < contourList->count(); ++i)
        contourList->item(i)->setCheckState(Qt::Unchecked);
}

void ChooseContoursWidget::validate()
{
    if (QColor::isValidColor(colorEdit->text()))
        accept();
    else
        colorInvalid->show();
}

std::vector<int> ChooseContoursWidget::getContourList()
{
    std::vector<int> selectedContours;
    for (int i = 0; i < contourList->count(); ++i)
    {
        if (contourList->item(i)->checkState() == Qt::Checked)
            selectedContours.push_back(i);
    }
    return selectedContours;
}

QColor ChooseContoursWidget::getContourColor()
{
    if (QColor::isValidColor(colorEdit->text()))
        return colorEdit->text();
}
