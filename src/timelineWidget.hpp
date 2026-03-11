#ifndef TIMELINE_WIDGET_HPP
#define TIMELINE_WIDGET_HPP


#include <QDebug>
#include <QGridLayout>
#include <QSignalBlocker>
#include <QWidget>
#include <time.h>

#include "qcustomplot.h"
#include "glSectorWidget.hpp"

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    TimelineWidget(QWidget *parent = 0);
    ~TimelineWidget();

    void setPatientData(QString name, QString birth, QString sex,
                        std::vector<std::tuple<std::string /*date*/, std::string /*laterality*/,
                        std::vector<double> /*sectorvalues*/> > scans);

    void drawTimeline();

private:
    void initColorMap();
    void prepareLayout();

    QCustomPlot *customPlot;
    QCPColorMap *colorMap;
    QCPRange valueRange;
    float zoomFactor;

    QCPColorGradient gradient;

    glSectorWidget *sectorView;

    QSpinBox* upperContourSpinBox;
    QSpinBox* lowerContourSpinBox;

    QLabel *scanInfo;
    QLabel *patientInfo;

    QWidget w;
    QGridLayout l;

    int numScans;
    const int numSectors;
    int numScansToShowAtOnce;
    bool laterality; //0 is Left, 1 is Right Eye

    //5 colors in order: critically low, borderline low, normal, borderline high, critically high
    std::vector<QRgb> colors;
    int colorStops[4];

    //Vector of Scans, each scan is a tuple of <date, laterality, 9 sectorValues>
    //scans points ever to leftScans or to rightScans
    std::vector<std::tuple<std::string,std::string,std::vector<double> > > *scans;
    std::vector<std::tuple<std::string,std::string,std::vector<double> > > leftScans;
    std::vector<std::tuple<std::string,std::string,std::vector<double> > > rightScans;

    //text label for each sector value
    //QVector<QVector<QCPItemText*> > cellText;

    QSignalBlocker *blockXAxis;
    QSignalBlocker *blockScrollbar;
    int scrollBarScaling = 100;
    double xAxisOffset = 0.5;

private slots:

    void adjustXAxisToScrollbar(int value);
    void adjustScrollbarToXAxis(QCPRange xAxisRange);
    void checkXAxisRange(QCPRange boundedRange);
    //void changePatientData(const char* patientID);

    void changeColorScale();
    void chooseColors();

    void changeLaterality(bool rightEye);

    void drawGrid(bool enabled);

    void updateSectorViewValues(QCPAbstractItem* item, QMouseEvent* event);
    void updateSectorViewValues(QCPAbstractPlottable* item, QMouseEvent* event);

    //mark scan which is clicked and used for the sectorWidget
    void markClickedScan(QCPAbstractItem *item, QMouseEvent* event);
};

#endif //TIMELINE_WIDGET_HPP
