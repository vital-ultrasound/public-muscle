#pragma once
#include <QWidget>
#include <ifindImage.h>
#include <QtPluginWidgetBase.h>
#include <QVector2D>
#include <vector>
#include <string>

class QLabel;
class QPushButton;
class QSpinBox;
//class QSlider;

class Widget_pixelSize : public QtPluginWidgetBase
{
    Q_OBJECT

public:

    Widget_pixelSize(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual void SendImageToWidgetImpl(ifind::Image::Pointer image);

    //QSlider *mSlider;
    QPushButton *mPickPointsButton;

public Q_SLOTS:
    virtual void ProcessPickedPoint(QVector2D &vec);
    virtual void ProcessNewSpacing(double spacing);

Q_SIGNALS:
    void signal_spacingComputed(double spacing);

private:
    // raw pointer to new object which will be deleted by QT hierarchy
    QLabel *mLabel;
    QLabel *mPointCoordLabel;
    QLabel *mIntervalLabel;
    QSpinBox *mIntervalSelector;
    bool mIsBuilt;

    /**
     * @brief Build the widget
     */
    void Build();
    std::vector<QVector2D> mPickedPoints;
    std::string PickedCoordinatesToLabel(double spacing);

};
