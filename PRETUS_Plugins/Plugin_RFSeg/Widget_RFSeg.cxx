#include "Widget_RFSeg.h"
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QButtonGroup>
#include <sstream>
#include <QPixmap>
#include <iomanip>
//#include "QtInfoPanelLabelConFetalWeight.h"

Widget_RFSeg::Widget_RFSeg(
        QWidget *parent, Qt::WindowFlags f)
    : QtPluginWidgetBase(parent, f)
{

    this->mWidgetLocation = WidgetLocation::top_right;
    mStreamTypes = ifind::InitialiseStreamTypeSetFromString("RFSeg");
    mIsBuilt = false;
    mResetButtons = false;

    mLabel = new QLabel("Text not set", this);
    mLabel->setStyleSheet(sQLabelStyle);

    // create L and R buttons

    mLegButtons = new QButtonGroup();
    mNumberAcquisitionsPerLeg = 3;
    mLegButtons->setExclusive(true);
    mLegSelectionButton.resize(2); // left and right
    mTimes_begin.resize(2); // left and right
    mTimes_end.resize(2); // left and right
    mTimes_msec.resize(2);
    mAreas_mm.resize(2);// left and right
    mThumbnails.resize(2);// left and right
    mThumbnailsCB.resize(2);
    for (int i=0; i< mLegSelectionButton.size() ; i++){
        mLegSelectionButton[i].resize(mNumberAcquisitionsPerLeg);
        mTimes_begin[i].resize(mNumberAcquisitionsPerLeg);
        mTimes_end[i].resize(mNumberAcquisitionsPerLeg);
        mTimes_msec[i].resize(mNumberAcquisitionsPerLeg);
        mAreas_mm[i].resize(mNumberAcquisitionsPerLeg);
        mThumbnails[i].resize(mNumberAcquisitionsPerLeg);
        mThumbnailsCB[i].resize(mNumberAcquisitionsPerLeg);
        for (int j=0; j<mNumberAcquisitionsPerLeg; j++){
            mLegSelectionButton[i][j] = new QPushButton(this->GetButtonText(i, j, 0).c_str(), this);
            mLegSelectionButton[i][j]->setCheckable(true);
            mLegSelectionButton[i][j]->setStyleSheet(sQPushButtonStyle);
            mLegButtons->addButton(mLegSelectionButton[i][j]);
            mAreas_mm[i][j] = -1;
            mThumbnails[i][j] = new QLabel();
            mThumbnailsCB[i][j] = new QCheckBox();
            mThumbnailsCB[i][j]->setStyleSheet(sQCheckBoxStyle);
        }
    }


    mShowOverlayCheckbox = new QCheckBox("Show overlay", this);
    mShowOverlayCheckbox->setStyleSheet(sQCheckBoxStyle);

    mSaveButton = new QPushButton("Save", this);
    mSaveButton->setStyleSheet(sQPushButtonStyle);

    auto vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    this->setLayout(vLayout);
    vLayout->addWidget(mLabel);
    this->AddInputStreamComboboxToLayout(vLayout);
    this->AddImageViewCheckboxToLayout(vLayout);
}

std::string Widget_RFSeg::GetButtonText(int leg_id, int exam_id, float area_mm){
    std::stringstream ss;
    std::string legside = leg_id==0 ? "L" :  "R";
    ss << legside << exam_id << std:: endl;
    ss << std::fixed;
    if (area_mm == 0){
        ss << "N/A";
    } else {
        ss << std::setprecision(2) << area_mm/100 << "cm2 ";
        ss <<"("<< std::setprecision(2)  << mTimes_msec[leg_id][exam_id]/1000<< "s)";
    }

    return ss.str();
}

void Widget_RFSeg::Build(){


    QVBoxLayout * outmost_layout = reinterpret_cast <QVBoxLayout *>( this->layout());
    //outmost_layout->addWidget(mLabel, 1, Qt::AlignTop);


    {
        // create L and R buttons
        auto legbuttons_layout = new QHBoxLayout();

        for (int i=0; i< mLegSelectionButton.size() ; i++){
            auto legside_layout = new QVBoxLayout(this);
            for (int j=0; j<mNumberAcquisitionsPerLeg; j++){
                legside_layout->addWidget(mLegSelectionButton[i][j]);
            }
            legbuttons_layout->addLayout(legside_layout);
        }
        outmost_layout->addLayout(legbuttons_layout);

        QObject::connect(this->mLegButtons, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonPressed),
                         this, &Widget_RFSeg::slot_SomeButtonToggled);
        QObject::connect(this->mLegButtons, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonReleased),
                         this, &Widget_RFSeg::slot_SomeButtonToggledEnd);


    }
    {
        QHBoxLayout *layout  = new QHBoxLayout();
        mShowOverlayCheckbox->setChecked(true);
        layout->addWidget(mShowOverlayCheckbox);
        layout->addWidget(mSaveButton);
        outmost_layout->addLayout(layout);
    }

    { // thumbnails
        int widget_width = this->width();
         QPixmap black_pixmap(int(widget_width/2), 128);
         black_pixmap.fill(Qt::gray);
         auto layouth = new QHBoxLayout();
         for (int i=0; i< mLegSelectionButton.size() ; i++){
             auto layoutv = new QVBoxLayout(this);
             for (int j=0; j<mNumberAcquisitionsPerLeg; j++){
                 auto th_layout = new QStackedLayout();
                 mThumbnails[i][j]->setPixmap(black_pixmap);

                 //mThumbnails[i][j]->setFixedWidth(widget_width);
                 mThumbnailsCB[i][j]->setFixedSize(int(widget_width/10), int(widget_width/10));
                 th_layout->addWidget(mThumbnails[i][j]);
                 th_layout->addWidget(mThumbnailsCB[i][j]);
                 th_layout->setStackingMode(QStackedLayout::StackAll);

                 //layoutv->addWidget(mThumbnails[i][j]);
                 layoutv->addLayout(th_layout);

             }
             layouth->addLayout(layoutv);
         }
         outmost_layout->addLayout(layouth);
    }
}

void Widget_RFSeg::slot_UpdateThumbnail(QPixmap pixmap, int i, int j){
    int widget_width = this->width();
    auto scaledPixmap = pixmap.scaledToWidth(int(widget_width/2));
    this->mThumbnails[i][j]->setPixmap(scaledPixmap);
}

std::vector<int> Widget_RFSeg::GetCurrentButtonSideAndExperiment(){
    std::vector<int> returning(2);
    returning[0] = -1;
    returning[1] = -1;
    for (int i=0; i< mLegSelectionButton.size() ; i++){
        for (int j=0; j<mNumberAcquisitionsPerLeg; j++){
            if (mLegSelectionButton[i][j]->isChecked()){
                returning[0] = i;
                returning[1] = j;
                break;
            }
        }
    }
    return returning;
}

void Widget_RFSeg::slot_SomeButtonToggled(QAbstractButton *button){
    // this always returns the last checked button. So we set the end time for it
    std::vector<int> legside_and_expid = GetCurrentButtonSideAndExperiment();
    if (legside_and_expid[0]> -1){
        mTimes_end[legside_and_expid[0]][legside_and_expid[1]] = std::chrono::steady_clock::now();
        mTimes_msec[legside_and_expid[0]][legside_and_expid[1]]  =  std::chrono::duration_cast<std::chrono::milliseconds>(mTimes_end[legside_and_expid[0]][legside_and_expid[1]] - mTimes_begin[legside_and_expid[0]][legside_and_expid[1]]).count();

        // emit a signal to say that we have finished computing an image.
        Q_EMIT this->signal_measurement_finished(legside_and_expid[0], legside_and_expid[1]);
        // by default, check the box
        this->mThumbnailsCB[legside_and_expid[0]][legside_and_expid[1]]->setChecked(true);

    }
    if (button->isChecked()){
        mResetButtons = true;
    }
}


void Widget_RFSeg::slot_SomeButtonToggledEnd(QAbstractButton *button){
    // this always returns the newly checked button
    std::vector<int> legside_and_expid = GetCurrentButtonSideAndExperiment();
    if (legside_and_expid[0]> -1){
        mTimes_begin[legside_and_expid[0]][legside_and_expid[1]] = std::chrono::steady_clock::now();
    }

    if (mResetButtons){
        // This means we clicked a button that was clicked
        mResetButtons = false;
        mLegButtons->setExclusive(false);
        button->setChecked(false);
        mLegButtons->setExclusive(true);
    }
}





void Widget_RFSeg::UpdateLegMeasurements(float rf_measurement_mm){

    for (int i=0; i< mLegSelectionButton.size() ; i++){
        for (int j=0; j<mNumberAcquisitionsPerLeg; j++){
            if (mLegSelectionButton[i][j]->isChecked()){
                mLegSelectionButton[i][j]->setText(this->GetButtonText(i, j, rf_measurement_mm).c_str());
                mAreas_mm[i][j] = rf_measurement_mm;
                break;
            }
        }
    }
}


void Widget_RFSeg::SendImageToWidgetImpl(ifind::Image::Pointer image){

    if (mIsBuilt == false){
        mIsBuilt = true;
        this->Build();
    }

    std::stringstream stream;
    stream << "==" << this->mPluginName.toStdString() << "=="<<std::endl;
    //stream << "Receiving " << ifind::StreamTypeSetToString(this->mInputStreamTypes) << std::endl;
    if (image->HasKey("RFSeg_area")){
        stream << std::fixed;
        stream << "Area: "<<  std::setprecision(2) << QString(image->GetMetaData<std::string>("RFSeg_area").c_str()).toFloat() / 100 << " cm2";
        stream << " [" << std::setprecision(1) << QString(image->GetMetaData<std::string>("RFSeg_aream").c_str()).toFloat() / 100 ;
        stream << ", " <<  std::setprecision(1) << QString(image->GetMetaData<std::string>("RFSeg_areaM").c_str()).toFloat() / 100  <<"] cm2" <<std::endl;
        stream << "Area av: " << std::setprecision(2) << QString(image->GetMetaData<std::string>("RFSeg_areaav").c_str()).toFloat() / 100 << " cm2"<<std::endl;
    }
    stream << "Sending " << ifind::StreamTypeSetToString(this->mStreamTypes);

    // Update the buttons
    float rf_measurement_mm = QString(image->GetMetaData<std::string>("RFSeg_area").c_str()).toFloat();

    // emit a signal with:
    // check if a button is checked, if it is, then send:
    // - the image
    // - the area
    // - the running time

    this->UpdateLegMeasurements(rf_measurement_mm);


    mLabel->setText(stream.str().c_str());
    Q_EMIT this->ImageAvailable(image);
}
