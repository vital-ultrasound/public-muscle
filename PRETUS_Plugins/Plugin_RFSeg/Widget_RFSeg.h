#pragma once
#include <QWidget>
#include <ifindImage.h>
#include <QtPluginWidgetBase.h>
#include <vector>
#include <QAbstractButton>
#include <chrono>
#include <QPixmap>

class QLabel;
class QPushButton;
class QCheckBox;
class QButtonGroup;
//class QSlider;

class Widget_RFSeg : public QtPluginWidgetBase
{
    Q_OBJECT

public:

    Widget_RFSeg(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual void SendImageToWidgetImpl(ifind::Image::Pointer image);

    QCheckBox *mShowOverlayCheckbox;
    QPushButton *mSaveButton;
    std::vector< std::vector< float> > mTimes_msec;
    std::vector< std::vector< float> > mAreas_mm;
    std::vector< std::vector<QCheckBox *> > mThumbnailsCB;

public Q_SLOTS:
    virtual void slot_SomeButtonToggled(QAbstractButton *button);
    virtual void slot_SomeButtonToggledEnd(QAbstractButton *button);
    virtual void slot_UpdateThumbnail(QPixmap pixmap, int i, int j);

Q_SIGNALS:
    void signal_measurement_finished(int legside, int exam_id); // leg, n exam

private:
    // raw pointer to new object which will be deleted by QT hierarchy
    QLabel *mLabel;
    bool mIsBuilt;

    QButtonGroup *mLegButtons;
    std::vector< std::vector<QPushButton *> > mLegSelectionButton;
    std::vector< std::vector<QLabel *> > mThumbnails;

    std::vector< std::vector< std::chrono::steady_clock::time_point> > mTimes_begin;
    std::vector< std::vector< std::chrono::steady_clock::time_point> > mTimes_end;

    int mNumberAcquisitionsPerLeg;
    bool mResetButtons;

    /**
     * @brief Build the widget
     */
    void Build();
    std::string GetButtonText(int leg_id, int exam_id, float area_mm);
    void UpdateLegMeasurements(float rf_measurement_mm);

    std::vector<int> GetCurrentButtonSideAndExperiment();

};
