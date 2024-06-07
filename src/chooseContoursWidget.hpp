#ifndef CHOOSECONTOURS_WIDGET_HPP
#define CHOOSECONTOURS_WIDGET_HPP

#include <QColor>
#include <QDebug>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QString>

#include <vector>

class ChooseContoursWidget : public QDialog
{
    Q_OBJECT

public:
    ChooseContoursWidget(int numContours);
    ~ChooseContoursWidget();

    //returns a list of selected contours (as index), which shall appear in the drawn image
    std::vector<int> getContourList();
    //returns the chosen color to draw all selected contours
    QColor getContourColor();

private:
    QListWidget* contourList;
    QLineEdit *colorEdit;
    QLabel *colorInvalid;

private slots:
    void validate();
    void selectAll();
    void deselectAll();
};

#endif //CHOOSECONTOURS_WIDGET_HPP
