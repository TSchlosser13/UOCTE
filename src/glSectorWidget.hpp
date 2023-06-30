#ifndef GL_SECTOR_WIDGET_HPP
#define GL_SECTOR_WIDGET_HPP

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QMatrix4x4>

class glSectorWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    //Q_OBJECT

public:
      glSectorWidget(QWidget *parent = 0);
      ~glSectorWidget();

      void clear(); //sets visualization to init

      int getWidth() { return 2*(radius3 + 10); }
      int getHeight() { return 2*(radius3 + 10); }

      void setSectorValues(std::vector<double> &scan, std::vector<QColor> &colors);
      //mark the given sector by surrounding it with the complementary to its fill color
      //call after setSectorValues for correct complementary color calculation
      void markSector(int sector);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int  w ,int h);

private:
    //QMatrix4x4 m_projection;

    double scale;
    int radius1, radius2, radius3;
    int radius12; //mid between rad1 and rad2
    int radius23; //mid between rad2 and rad3
    QPoint center;

    int fontSize;
    int textWidth; //approximation based on fontSize, for 3 digits
    //QPainter::drawText() uses given point as bottom left of the text, so it has to be adjusted
    QPoint textOffset[9];

    //holds sector names on startup, selected sectorValues (double) later
    std::vector<QString> sectorValues;
    std::vector<QColor> sectorColors;

    int markedSector;
    QPen markingPen;
    QFont markingFont;
};

#endif // GL_SECTOR_WIDGET_HPP
