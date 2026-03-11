#include "glSectorWidget.hpp"

glSectorWidget::glSectorWidget(QWidget *parent) : QOpenGLWidget(parent) ,
  sectorValues {"ot", "ol", "ob", "or", "it", "il", "ib", "ir", "c" },
  sectorColors {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
  markedSector(-1),
  markingPen(QPen(Qt::white)),
  markingFont (QFont("Arial", fontSize, QFont::Bold))

{

}

glSectorWidget::~glSectorWidget()
{

}

void glSectorWidget::clear()
{
    sectorValues = {"ot", "ol", "ob", "or", "it", "il", "ib", "ir", "c" };
    sectorColors = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    markSector(-1);
    this->update();
}

void glSectorWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void glSectorWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QPainter p(this);

    p.setPen(QPen(Qt::black));
    p.setFont(QFont("Arial", fontSize));

    //draw sectors with their respective colors and values

    int startAngle = 45 * 16; //degree for drawPie() is given in 1/16th steps
    int spanAngle = 90 * 16;


    for (int i = 0; i < 9; ++i) {
        if (i == markedSector) {
            p.setPen(markingPen);
            p.setFont(markingFont);
        }
        else {
            p.setPen(QPen(Qt::black));
            p.setFont(QFont("Arial", fontSize));
        }
        p.setBrush(QBrush(sectorColors[i]));

        if (i < 4) //outer ring starting at top, going CCW
            p.drawPie(center.x() - radius3, center.y() -radius3,
                      2 * radius3, 2 * radius3,
                      startAngle, spanAngle);
        else if (i < 8) //inner ring starting at top, going CCW
            p.drawPie(center.x() - radius2, center.y() -radius2,
                      2 * radius2, 2 * radius2,
                      startAngle, spanAngle);
        else //center
            p.drawEllipse(center, radius1, radius1);

        p.drawText(textOffset[i], sectorValues[i]);
        startAngle += spanAngle;
    }


    /* circles and diagonales are automatically drawn by pies

    p.drawEllipse(center, radius1, radius1);
    p.drawEllipse(center, radius2, radius2);
    p.drawEllipse(center, radius3, radius3);

    //north east
    p.drawLine(center.x() + radius1 * std::sqrt(0.5),
               center.y() - radius1 * std::sqrt(0.5),
               center.x() + radius3 * std::sqrt(0.5),
               center.y() - radius3 * std::sqrt(0.5));
    //south east
    p.drawLine(center.x() + radius1 * std::sqrt(0.5),
               center.y() + radius1 * std::sqrt(0.5),
               center.x() + radius3 * std::sqrt(0.5),
               center.y() + radius3 * std::sqrt(0.5));
    //south west
    p.drawLine(center.x() - radius1 * std::sqrt(0.5),
               center.y() + radius1 * std::sqrt(0.5),
               center.x() - radius3 * std::sqrt(0.5),
               center.y() + radius3 * std::sqrt(0.5));
    //north west
    p.drawLine(center.x() - radius1 * std::sqrt(0.5),
               center.y() - radius1 * std::sqrt(0.5),
               center.x() - radius3 * std::sqrt(0.5),
               center.y() - radius3 * std::sqrt(0.5));
    */
    //this->resize(this->getWidth(), this->getHeight());
}

void glSectorWidget::resizeGL(int  w ,int h)
{
    //m_projection.setToIdentity();
    //m_projection.perspective(45.0f, w / float(h), 0.01f, 100.0f);

    scale = (w > h) ? h : w;
    radius3 = scale/2;
    radius2 = radius3/2;
    radius1 = radius2/3;

    radius12 = radius2 - 0.5*(radius2 - radius1);
    radius23 = radius3 - 0.5*(radius3 - radius2);
    center = QPoint(radius3, radius3);
    fontSize = std::max(int(scale / 20), 1);
    textWidth = fontSize * 1;
    //outer
    textOffset[0] = {center.x() - textWidth, center.y() - radius23 + fontSize/2};
    textOffset[1] = {center.x() - radius23 - textWidth, center.y() + fontSize/2};
    textOffset[2] = {center.x() - textWidth, center.y() + radius23 + fontSize/2};
    textOffset[3] = {center.x() + radius23 - textWidth, center.y() + fontSize/2};
    //inner
    textOffset[4] = {center.x() - textWidth, center.y() - radius12 + fontSize/2};
    textOffset[5] = {center.x() - radius12 - textWidth, center.y() + fontSize/2};
    textOffset[6] = {center.x() - textWidth, center.y() + radius12 + fontSize/2};
    textOffset[7] = {center.x() + radius12 - textWidth, center.y() + fontSize/2};
    //center
    textOffset[8] = {center.x() - textWidth, center.y() + fontSize/2};


    markingPen.setWidth(scale / 100);
    markingFont.setPointSize(fontSize);

    this->update();
}

void glSectorWidget::setSectorValues(std::vector<double> &scan, std::vector<QColor> &colors)
{
    sectorValues.clear();
    for (int i = 0; i < 9; ++i) {
        sectorValues.push_back(QString::number(scan[i]));
    }

    sectorColors.clear();
    sectorColors = colors;

    this->update();
}

void glSectorWidget::markSector(int sector)
{
    markedSector = sector; //removes mark, if sector does not exist

    if (sector < sectorValues.size() && sector >= 0) {
        QColor h = sectorColors[sector].toHsl();
        QColor r;
        r.setHsl((h.hue()+180)%360, 255, 128, 255);
    }

    //markingPen.setColor(r);
    markingPen.setColor(Qt::black);

    this->update();
}
