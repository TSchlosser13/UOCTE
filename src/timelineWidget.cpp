#include "timelineWidget.hpp"

TimelineWidget::TimelineWidget(QWidget *parent):
    numSectors(9),
    numScansToShowAtOnce(5),
    numScans(0),
    laterality(0),
    valueRange(0,600),
    //colors { 0x5e3c99, 0xb2abd2, 0xf7f7f7, 0xfdb863, 0xe66101 }
    colors { 0x2c7bb6, 0xabd9e9, 0xffffbf, 0xfdae61, 0xd7191c }
{

    setWindowTitle("uocte Timeline");
    prepareLayout();
    initColorMap();
}

TimelineWidget::~TimelineWidget()
{
    customPlot->close();
    scans->clear();
}


void TimelineWidget::prepareLayout()
{
    patientInfo = new QLabel("<b>Patient info</b>");
    patientInfo->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    sectorView = new glSectorWidget();
    sectorView->setFixedSize(250, 250);

    /*
    QLabel* upperContourLabel = new QLabel ("Upper Contour: ");
    upperContourSpinBox = new QSpinBox (this);
    QLabel* lowerContourLabel = new QLabel ("Lower Contour: ");
    lowerContourSpinBox = new QSpinBox (this);
    */

    scanInfo = new QLabel("<b>Selected scan info</b>");
    scanInfo->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    QLabel *colorStopLabel = new QLabel("<b>Color scale</b>");
    colorStopLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    QLineEdit *colorAreas[5];
    for (int i = 0; i < 5; ++i) {
        colorAreas[i] = new QLineEdit();
        colorAreas[i]->setReadOnly(true);
        QPalette pal;
        pal.setColor(QPalette::Base, colors[i]);
        colorAreas[i]->setPalette(pal);
        colorAreas[i]->setObjectName(QString("colorArea%1").arg(i));
        colorAreas[i]->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    }
    QLineEdit *colorStops[4];
    for (int i = 0; i < 4; ++i) {
        colorStops[i] = new QLineEdit();
        QString a = QString("colorStop%1").arg(i);
        colorStops[i]->setObjectName(QString(a));
        //colorStops[i]->setFixedWidth(sectorView->getWidth());

        colorStops[i]->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    }
    QPushButton *colorStopsButton = new QPushButton("Apply");
    colorStopsButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    connect(colorStopsButton, SIGNAL(released()), this, SLOT(changeColorScale()));

    customPlot = new QCustomPlot();
    customPlot->setObjectName(QStringLiteral("customPlot"));
    customPlot->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    customPlot->addPlottable(colorMap);

    QLabel *zoomFactorLabel = new QLabel();
    zoomFactorLabel->setObjectName("zoomFactorLabel");
    zoomFactorLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );


    QScrollBar *scrollBar = new QScrollBar(Qt::Horizontal);
    scrollBar->setObjectName("scrollbar");
    connect(scrollBar, SIGNAL(sliderMoved(int)), this, SLOT(adjustXAxisToScrollbar(int)));
    blockScrollbar = new QSignalBlocker(scrollBar);
    blockScrollbar->unblock();

    QLabel *lateralityLabel = new QLabel("Laterality: ");
    lateralityLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    QRadioButton *leftLat = new QRadioButton("left");
    leftLat->setObjectName("leftLaterality");
    leftLat->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    QRadioButton *rightLat = new QRadioButton("right");
    rightLat->setObjectName("rightLaterality");
    //rightLat->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    //IMPORTANT to connect rightLat only
    connect(rightLat, SIGNAL(toggled(bool)), this, SLOT(changeLaterality(bool)));

    QCheckBox *gridToggle = new QCheckBox("Grid");
    gridToggle->setChecked(true);
    connect(gridToggle, SIGNAL(toggled(bool)), this, SLOT(drawGrid(bool)));

    QPushButton *chooseColorsButton = new QPushButton("Choose Colors");
    connect (chooseColorsButton, SIGNAL(released()), this, SLOT(chooseColors()));

    QVBoxLayout *colorPreviewLayout = new QVBoxLayout;
        for (int i = 4; i >= 0; --i)
            colorPreviewLayout->addWidget(colorAreas[i]);
    QVBoxLayout *colorEditLayout = new QVBoxLayout;
        for (int i = 3; i >= 0; --i)
           colorEditLayout->addWidget(colorStops[i]);
    QHBoxLayout *colorCombineLayout = new QHBoxLayout;
        colorCombineLayout->addLayout(colorPreviewLayout);
        colorCombineLayout->addLayout(colorEditLayout);
    QVBoxLayout *colorStopLayout = new QVBoxLayout;
        colorStopLayout->addWidget(colorStopLabel);
        colorStopLayout->addLayout(colorCombineLayout);
        colorStopLayout->addWidget(colorStopsButton);

    //TODO: Add selection of contours for sector calculations
    /*
    QHBoxLayout* chooseUpperContourLayout = new QHBoxLayout;
        chooseUpperContourLayout->addWidget(upperContourLabel);
        chooseUpperContourLayout->addWidget(upperContourSpinBox);
    QHBoxLayout* chooseLowerContourLayout = new QHBoxLayout;
        chooseLowerContourLayout->addWidget(lowerContourLabel);
        chooseLowerContourLayout->addWidget(lowerContourSpinBox);
    QVBoxLayout *chooseContourLayout = new QVBoxLayout;
    chooseContourLayout->addLayout(chooseUpperContourLayout);
    chooseContourLayout->addLayout(chooseLowerContourLayout);
    */

    QVBoxLayout *leftLayout = new QVBoxLayout;
        leftLayout->addWidget(patientInfo);
        leftLayout->addWidget(sectorView);
        //leftLayout->addLayout(chooseContourLayout);
        leftLayout->addWidget(scanInfo);
        leftLayout->addLayout(colorStopLayout);
        leftLayout->setSizeConstraint(QLayout::SetMinimumSize);
    QHBoxLayout *bottomRightLayout = new QHBoxLayout;
        bottomRightLayout->addWidget(lateralityLabel);
        bottomRightLayout->addWidget(leftLat);
        bottomRightLayout->addWidget(rightLat);
        bottomRightLayout->addWidget(gridToggle);
        bottomRightLayout->addWidget(chooseColorsButton);
    QHBoxLayout *aboveBottomRightLayout = new QHBoxLayout;
        aboveBottomRightLayout->addWidget(zoomFactorLabel);
        aboveBottomRightLayout->addWidget(scrollBar);
    QVBoxLayout *rightLayout = new QVBoxLayout;
        rightLayout->addWidget(customPlot);
        rightLayout->addLayout(aboveBottomRightLayout);
        rightLayout->addLayout(bottomRightLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout;
        mainLayout->addLayout(leftLayout);
        mainLayout->addLayout(rightLayout);
        //layout->setStretch(0, 3);
        //layout->setStretch(1, 7);
    this->setLayout(mainLayout);
    this->resize(3*sectorView->width(), customPlot->height());
}

void TimelineWidget::initColorMap()
{
    //set the View over the Graph
    //show all 9 sectors vertically
    customPlot->yAxis->setRange(0.5, numSectors +0.5);

    this->findChild<QLineEdit*>(QString("colorStop0"))->setText("150");
    this->findChild<QLineEdit*>(QString("colorStop1"))->setText("210");
    this->findChild<QLineEdit*>(QString("colorStop2"))->setText("310");
    this->findChild<QLineEdit*>(QString("colorStop3"))->setText("350");
    changeColorScale();

    //gradient.setColorInterpolation( QCPColorGradient::ColorInterpolation::ciHSV);

    //show Color Legend
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setGradient(gradient);
    colorScale->setDataRange(valueRange);
    colorScale->setRangeZoom(true);
    colorMap->setColorScale(colorScale);

    //colorMap->rescaleDataRange(true);
    colorMap->setInterpolate(false);

    //customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);

    //make xAxis scrollable (without scrollbar)
    customPlot->setInteraction(QCP::iRangeDrag, true);
    customPlot->setInteraction(QCP::iRangeZoom, true);
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal);
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal);
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(checkXAxisRange(QCPRange)));
    //connect(customPlot->xAxis2, SIGNAL(rangeChanged(QCPRange)), this, SLOT(changeBoundedRange(QCPRange)));
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(adjustScrollbarToXAxis(QCPRange)));
    blockXAxis = new QSignalBlocker(customPlot->xAxis);
    blockXAxis->unblock();

    //x-Axis properties
    customPlot->xAxis->setLabel("Dates");
    customPlot->xAxis->setUpperEnding(QCPLineEnding::esLineArrow);
    customPlot->xAxis->setAutoTicks(false);
    //customPlot->xAxis->setAutoTickStep(false);
    //customPlot->xAxis->setTickStep(1);
    customPlot->xAxis->setAutoSubTicks(false);
    customPlot->xAxis->setSubTickCount(1);
    customPlot->xAxis->setAutoTickLabels(false);
    customPlot->xAxis->setTickLabelRotation(30);
    customPlot->xAxis->setTickLength(0, 2);

    //sector names on yAxis
    customPlot->yAxis->setLabel("Sectors");
    customPlot->yAxis->setLowerEnding(QCPLineEnding::esBar);
    customPlot->yAxis->setUpperEnding(QCPLineEnding::esBar);
    QVector<double> yTicks { 1., 2., 3., 4., 5., 6., 7., 8., 9. };
    QVector<QString> yLabels { "outer bottom", "outer left", "inner bottom", "inner left", "center", "inner right", "inner top", "outer right", "outer top" };
    customPlot->yAxis->setAutoTicks(false);
    customPlot->yAxis->setTickVector(yTicks);
    customPlot->yAxis->setAutoSubTicks(false);
    customPlot->yAxis->setSubTickCount(1);
    customPlot->yAxis->setAutoTickLabels(false);
    customPlot->yAxis->setTickVectorLabels(yLabels);
    customPlot->yAxis->setTickLength(0, 2);

    //draw grid over cells
    customPlot->moveLayer(customPlot->layer("grid"), customPlot->layer("main"));
    customPlot->xAxis->grid()->setPen(Qt::NoPen);
    customPlot->xAxis->grid()->setSubGridVisible(true);
    customPlot->xAxis->grid()->setSubGridPen(QPen(Qt::black));
    customPlot->yAxis->grid()->setPen(Qt::NoPen);
    customPlot->yAxis->grid()->setSubGridVisible(true);
    customPlot->yAxis->grid()->setSubGridPen(QPen(Qt::black));

    //prepare marking of a clicked scan
    customPlot->xAxis2->setVisible(true);
    customPlot->xAxis2->setAutoTicks(false);
    customPlot->xAxis2->setTickVector(QVector<double>(0));
    //customPlot->xAxis2->setAutoTickLabels(false);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->xAxis2->setAutoSubTicks(false);
    customPlot->xAxis2->setSubTickCount(0);
    QPen markPen(Qt::black);
    markPen.setWidth(4);
    customPlot->xAxis2->grid()->setPen(markPen);
    customPlot->xAxis2->grid()->setVisible(true);

    //clicking on a column (scan) shall change the glSectorWidget respectively
    /* TODO: click on Rect to do it
    customPlot->setInteraction(QCP::iSelectPlottables, true);
    colorMap->setSelectedBrush(QBrush(Qt::blue));
    colorMap->setSelectedPen(QPen(Qt::blue));
    connect(customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(updateSectorViewValues(QCPAbstractPlottable*, QMouseEvent*)));
    //*/
    //clicking on a text does it
    customPlot->setInteraction(QCP::iSelectItems, true);
    connect(customPlot, SIGNAL(itemClick(QCPAbstractItem*,QMouseEvent*)), this, SLOT(updateSectorViewValues(QCPAbstractItem*,QMouseEvent*)));
    connect(customPlot, SIGNAL(itemClick(QCPAbstractItem*,QMouseEvent*)), this, SLOT(markClickedScan(QCPAbstractItem*,QMouseEvent*)));
}

void TimelineWidget::setPatientData(QString name, QString birth, QString sex, std::vector<std::tuple<std::string /*date*/, std::string /*laterality*/,
                                    std::vector<double> /*sectorvalues*/> >  scans)
{
    if (scans.empty())
        return;

    leftScans.clear();
    rightScans.clear();

    patientInfo->setText(QString("<b>Patient info</b><br>Name: %1<br>Birth date: %2<br>Sex: %3").
                         arg(name).arg(birth).arg(sex));

    //sort scans by laterality
    for(auto it = scans.begin(); it != scans.end(); ++it)
    {
        if (std::get<1>(*it) == "L")
            leftScans.push_back(*it);
        else if (std::get<1>(*it) == "R")
            rightScans.push_back(*it);
        else {
            leftScans.push_back(*it);
            rightScans.push_back(*it);
        }
    }

    QGraphicsColorizeEffect *uncheckableGraphics = new QGraphicsColorizeEffect();
    uncheckableGraphics->setColor(Qt::gray);

    if (leftScans.empty()) {
        this->findChild<QRadioButton*>("leftLaterality")->setCheckable(false);
        this->findChild<QRadioButton*>("leftLaterality")->setGraphicsEffect(uncheckableGraphics);
        this->numScans = rightScans.size();
        this->scans = &rightScans;
        this->findChild<QRadioButton*>("rightLaterality")->setChecked(true);
    }
    else if (rightScans.empty()) {
        this->findChild<QRadioButton*>("rightLaterality")->setCheckable(false);
        this->findChild<QRadioButton*>("rightLaterality")->setGraphicsEffect(uncheckableGraphics);
        this->numScans = leftScans.size();
        this->scans = &leftScans;
        this->findChild<QRadioButton*>("leftLaterality")->setChecked(true);
    }
    else { //scans contains both left and right eyes
        //start visualization with left eye
        this->numScans = leftScans.size();
        this->scans = &leftScans;
        this->findChild<QRadioButton*>("leftLaterality")->setChecked(true);
    }

    /* TEST PURPOSE: RANDOM DATA
    this->numScans = 10;
    this->sectorValues.resize(numScans);
    for(int i = 0; i < this->numScans; ++i)
        this->sectorValues[i].resize(this->numSectors);

    //random dummy data
    //TODO: change to take data out of xml
    qsrand(time(NULL));
    for (int x = 0; x < this->numScans; ++x) {
        for (int y = 0; y < 9; ++y)
            this->sectorValues[x][y] = 250 + qrand()%100;
    }
    */
}

void TimelineWidget::drawTimeline()
{
    //no patient data loaded
    if (this->scans->empty())
        return;

    //show "numScansToShowAtOnce" datasets horizontally at once
    customPlot->xAxis->setRange(xAxisOffset, numScansToShowAtOnce +xAxisOffset);

    //colorMap->clearData();
    //between 0 and 10 are 9 cells, so the size needs to be increased by 1
    colorMap->data()->setSize(numScans+1, numSectors+1);
    //move the cells to be aligned to the coordination system
    colorMap->data()->setRange(QCPRange(1, numScans+1), QCPRange(1, numSectors+1));


    //draw calculated sectorValues into cells
    for (int x = 0; x < numScans; ++x)
        for (int y = 0; y < numSectors; ++y)
            colorMap->data()->setCell(x, y, std::get<2>(scans->at(x)).at(y));

    //dates of scans on xAxis
    QVector<double> xTicks;
    QVector<QString> xLabels;
    for(int i = 0; i < numScans; ++i) {
        xLabels << QString::fromStdString(std::get<0>(scans->at(i)));
        xTicks << (double)(i+1);
    }
    customPlot->xAxis->setTickVectorLabels(xLabels);
    customPlot->xAxis->setTickVector(xTicks);

    //write sectors value on each cell
    customPlot->clearItems();
    QVector<QVector<QCPItemText*> > cellText (numScans,QVector<QCPItemText*>(numSectors));
    for (int x = 0; x < numScans; ++x)
        for (int y = 0; y < numSectors; ++y) {
            cellText[x][y] = new QCPItemText(customPlot);
            cellText[x][y]->setSelectedFont(QFont("Arial", 9, QFont::Bold));
            cellText[x][y]->setSelectedColor(Qt::black);
            customPlot->addItem(cellText[x][y]);
            cellText[x][y]->setText(QString::number(std::get<2>(scans->at(x)).at(y)));
            cellText[x][y]->position->setCoords(x+1, y+1);
        }

    customPlot->replot();
}

void TimelineWidget::checkXAxisRange(QCPRange boundedRange)
{
    double lowerRangeBound = xAxisOffset;
    double upperRangeBound = (this->numScans > this->numScansToShowAtOnce) ?
                             this->numScans +xAxisOffset : this->numScansToShowAtOnce +xAxisOffset;

    //prevent zooming too far out
    if (boundedRange.size() > upperRangeBound-lowerRangeBound)
    {
        boundedRange = QCPRange(lowerRangeBound, upperRangeBound);
    }
    //prevent zooming too far in (because labels disappear and widget becomes huge)
    else if (boundedRange.size() < 1) {
        boundedRange.upper = boundedRange.lower + 1;
    }

    //prevent dragging too far left or right
    double oldSize = boundedRange.size();
    if (boundedRange.lower < lowerRangeBound)
    {
        boundedRange.lower = lowerRangeBound;
        boundedRange.upper = lowerRangeBound+oldSize;
    }
    if (boundedRange.upper > upperRangeBound)
    {
        boundedRange.lower = upperRangeBound-oldSize;
        boundedRange.upper = upperRangeBound;
    }

    //qDebug() << QString("check - lower: %1, upper: %2").arg(boundedRange.lower).arg(boundedRange.upper);
    blockXAxis->reblock();
    this->customPlot->xAxis->setRange(boundedRange);
    blockXAxis->unblock();
    this->customPlot->xAxis2->setRange(boundedRange);
}

void TimelineWidget::adjustScrollbarToXAxis(QCPRange xAxisRange)
{
    //setValue() shall not lead to a call of adjustXAxisToScrollbar()
    blockXAxis->reblock();

    //qDebug() << QString("xAxis - lower: %1, upper: %2").arg( xAxisRange.lower).arg(xAxisRange.upper);
    zoomFactor = numScans / (xAxisRange.upper - xAxisRange.lower);
    if (zoomFactor < 1) zoomFactor = 1;

    this->findChild<QLabel*>("zoomFactorLabel")->setText(QString("Zoom: %1x").arg(zoomFactor, 0, 'f', 2));

    int scrollBarSize = scrollBarScaling* (xAxisRange.upper - xAxisRange.lower);
    blockScrollbar->reblock();
    this->findChild<QScrollBar*>("scrollbar")->setRange(0, scrollBarScaling*numScans - scrollBarSize);
    this->findChild<QScrollBar*>("scrollbar")->setValue(scrollBarScaling * (xAxisRange.lower -xAxisOffset));
    blockScrollbar->unblock();

    this->findChild<QScrollBar*>("scrollbar")->setPageStep(scrollBarSize);

    blockXAxis->unblock();
}

void TimelineWidget::adjustXAxisToScrollbar(int value)
{
    //xAxis->setRange() shall not lead to a call of adjustScrollbarToXAxis()
    blockScrollbar->reblock();

    double rangeSize = customPlot->xAxis->range().size();
    //qDebug() << QString("scroll - lower: %1, upper: %2").arg(value/100.0 + xAxisOffset).arg(value/100.0 + rangeSize + xAxisOffset);
    blockXAxis->reblock();
    this->customPlot->xAxis->setRange(value/100.0 + xAxisOffset, value/100.0 + rangeSize + xAxisOffset);
    this->customPlot->xAxis2->setRange(value/100.0 + xAxisOffset, value/100.0 + rangeSize + xAxisOffset);
    blockXAxis->unblock();
    customPlot->replot();

    blockScrollbar->unblock();
}

void TimelineWidget::chooseColors()
{
    QDialog *changeDialog = new QDialog();
    changeDialog->setWindowTitle("Choose Colors");

    std::vector<std::vector<QRgb>> colorScales;

    colorScales.push_back(std::vector<QRgb>{ 0x018571, 0x80cdc1, 0xf5f5f5, 0xdfc27d, 0xa6611a });
    colorScales.push_back(std::vector<QRgb>{ 0x5e3c99, 0xb2abd2, 0xf7f7f7, 0xfdb863, 0xe66101 });
    colorScales.push_back(std::vector<QRgb>{ 0x0571b0, 0x92c5de, 0xf7f7f7, 0xf4a582, 0xca0020 });
    colorScales.push_back(std::vector<QRgb>{ 0x2c7bb6, 0xabd9e9, 0xffffbf, 0xfdae61, 0xd7191c });

    std::vector<QString> colorScaleNames;
    //names taken from ColorBewer2.org
    colorScaleNames.push_back("BrBG");
    colorScaleNames.push_back("PuOr");
    colorScaleNames.push_back("RdBu");
    colorScaleNames.push_back("RdYlBu");

    QVBoxLayout *mainLayout = new QVBoxLayout();

    QHBoxLayout *colorScalesLayout = new QHBoxLayout();

    //predefined color scales
    for(int i = 0; i < colorScales.size(); ++i)
    {
        QVBoxLayout *lay = new QVBoxLayout();

        QRadioButton *radio = new QRadioButton(colorScaleNames[i]);
        radio->setObjectName(colorScaleNames[i]);
        //mark first color as default setting
        if (i == 0)
            radio->setChecked(true);
        lay->addWidget(radio);

        QLineEdit *colorAreas[5];
        for (int j = 4; j >= 0; --j) {
            colorAreas[i] = new QLineEdit();
            colorAreas[i]->setReadOnly(true);
            QPalette pal;
            pal.setColor(QPalette::Base, colorScales[i][j]);
            colorAreas[i]->setPalette(pal);
            //colorAreas[i]->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
            colorAreas[i]->setFixedWidth(26);
            lay->addWidget(colorAreas[i]);
        }

        lay->setSpacing(0);
        colorScalesLayout->addLayout(lay);
    }

    mainLayout->addLayout(colorScalesLayout);


    //custom color scale
    /*QLabel *name = new QLabel("Custom:");
    mainLayout->addWidget(name);

    QHBoxLayout *lay = new QHBoxLayout();

    QRadioButton *radio = new QRadioButton();
    radio->setObjectName("Custom");
    lay->addWidget(radio);

    QLineEdit *colorAreas[5];
    for (int i = 0; i < 5; ++i) {
        colorAreas[i] = new QLineEdit();
        colorAreas[i]->setReadOnly(true);
        QPalette pal;
        pal.setColor(QPalette::Base, colors[i]);
        colorAreas[i]->setPalette(pal);
        colorAreas[i]->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        lay->addWidget(colorAreas[i]);
    }
    */

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *rejectButton = new QPushButton("Cancel");
    buttons->addWidget(rejectButton);
    QPushButton *acceptButton = new QPushButton("Apply");
    buttons->addWidget(acceptButton);

    connect(rejectButton, SIGNAL(clicked()), changeDialog, SLOT(reject()));
    connect(acceptButton, SIGNAL(clicked()), changeDialog, SLOT(accept()));

    mainLayout->addLayout(buttons);

    changeDialog->setLayout(mainLayout);

    if (changeDialog->exec() == QDialog::Accepted) {
        int index = 0;
        for(auto radio : changeDialog->findChildren<QRadioButton*>()) {
            if (radio->isChecked())
                break;
            ++index;
        }
        colors = colorScales[index];
        changeColorScale();

        //update colored bars shown for updating ColorScale values
        for (int i = 0; i < 5; ++i) {
            QPalette pal;
            pal.setColor(QPalette::Base, colors[i]);
            this->findChild<QLineEdit*>(QString("colorArea%1").arg(i))->setPalette(pal);
        }


        //Map Values to ColorGradient without Interpolation
        /*double eps = 0.00001;
        gradient.clearColorStops();

        gradient.setColorStopAt(0, colors[4]);
        for (int i = 4; i < 0; --i) {
            //map value to percentage between min and max (creating values between 0 and 1)
            double perc = (double)colorStops[i] / (valueRange.upper-valueRange.lower);
            gradient.setColorStopAt(perc - eps, colors[i]);
            gradient.setColorStopAt(perc, colors[i+1]);
        }
        gradient.setColorStopAt(1, colors[4]);

        colorMap->setGradient(gradient);
        colorMap->setDataRange(valueRange);
        customPlot->replot();
        */
    }
}

void TimelineWidget::changeColorScale()
{
    bool ok;
    double eps = 0.00001;
    int old = -1;
    for (int i = 0; i < 4; ++i) {
        colorStops[i] = this->findChild<QLineEdit*>(QString("colorStop%1").arg(i))->text().toInt(&ok);
        if (!ok || colorStops[i] < 0 || colorStops[i] > 2000) {
            QMessageBox wrongInput;
            wrongInput.addButton("OK", QMessageBox::AcceptRole);
            wrongInput.setText("Please insert numbers between 0 and 2000 only.");
            wrongInput.exec();
            return;
        }
        if (colorStops[i] < old) {
            QMessageBox wrongInput;
            wrongInput.addButton("OK", QMessageBox::AcceptRole);
            wrongInput.setText("Numbers must be sorted ascending.");
            wrongInput.exec();
            return;
        }
        old = colorStops[i];
    }

    //change view on scale depending on input
    valueRange.upper = old*1.25;

    //Map Values to ColorGradient without Interpolation
    gradient.clearColorStops();

    gradient.setColorStopAt(0, colors[0]);
    for (int i = 0; i < 4; ++i) {
        //map value to percentage between min and max (creating values between 0 and 1)
        double perc = (double)colorStops[i] / (valueRange.upper-valueRange.lower);
        gradient.setColorStopAt(perc - eps, colors[i]);
        gradient.setColorStopAt(perc, colors[i+1]);
    }
    gradient.setColorStopAt(1, colors[4]);

    colorMap->setGradient(gradient);
    colorMap->setDataRange(valueRange);
    customPlot->replot();
}

void TimelineWidget::changeLaterality(bool rightEye)
{
    this->laterality = rightEye;
    this->scans = rightEye ? &this->rightScans : &this->leftScans;
    this->numScans = rightEye ? this->rightScans.size() : this->leftScans.size();

    //remove marking
    foreach (QCPAbstractItem* item, customPlot->selectedItems())
        item->setSelected(false);
    customPlot->xAxis2->setTickVector(QVector<double>(0));
    sectorView->clear();

    drawTimeline();
}

void TimelineWidget::drawGrid(bool enabled)
{
    if (enabled == true) {
        customPlot->xAxis->grid()->setSubGridVisible(true);
        customPlot->yAxis->grid()->setSubGridVisible(true);
    }
    else {
        customPlot->xAxis->grid()->setSubGridVisible(false);
        customPlot->yAxis->grid()->setSubGridVisible(false);
    }
    customPlot->replot();
}

void TimelineWidget::updateSectorViewValues(QCPAbstractItem* item, QMouseEvent* event)
{
    int x = item->positions().at(0)->coords().x();
    int y = item->positions().at(0)->coords().y();
    std::vector<double> scan;
    std::vector<QColor> colors;

    /* from xml order to sectors draw order
     * "outer bottom", "outer left", "inner bottom", "inner left", "center",
     * "inner right", "inner top", "outer right", "outer top"
     * to
     * ot, ol, ob, or, it, il, ib, ir, c
     */
    QVector<int> index { 8, 1, 0, 7, 6, 3, 2, 5, 4 };
    //from timeline draw order to sector order
    QVector<int> reverseIndex { 2, 1, 6, 5, 8, 7, 4, 3, 0 };

    //get all values of the selected scan
    //positions start at 1, but scans at 0
    for (int i = 0; i < numSectors; ++i) {
        scan.push_back (std::get<2>(scans->at(x - 1)).at(index[i]));
        //mark selected sector
        /*if (index[i] == (y-1))
            colors.push_back(0x4400ffff);
        else*/
            colors.push_back(gradient.color(std::get<2>(scans->at(x - 1)).at(index[i]), valueRange));
    }

    sectorView->setSectorValues(scan, colors);
    sectorView->markSector(reverseIndex[y-1]);
    //colorMap->data()->cell(x,y)->setSelectedBrush(QBrush(Qt::blue));
    //item->setSelected(true);
}

//TODO: Try to select a cell in QCPColorMap
void TimelineWidget::updateSectorViewValues(QCPAbstractPlottable *item, QMouseEvent* event)
{
    int x = 0, y = 0;
    colorMap->data()->coordToCell(event->pos().x(), event->pos().x(), &x, &y);
    //qDebug() << QString("x: %1, y: %2").arg(x).arg(y);
    //qDebug() << item->objectName();
    item->setSelectedBrush(QBrush(Qt::blue));
    item->setSelectedPen(QPen(Qt::blue));
}

void TimelineWidget::markClickedScan(QCPAbstractItem* item, QMouseEvent* event)
{
    int markedScan = item->positions().at(0)->coords().x();
    customPlot->xAxis2->setTickVector(QVector<double>(2) << markedScan -xAxisOffset << markedScan +xAxisOffset);

    scanInfo->setText(QString("<b>Selected scan info</b><br>Date: %1<br>Laterality: %2").arg(std::get<0>(scans->at(markedScan-1)).c_str())
                           .arg(std::get<1>(scans->at(markedScan-1)).c_str()));
}
