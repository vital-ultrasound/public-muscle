#include "Widget_pixelSize.h"
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
//#include "QtInfoPanelLabelConFetalWeight.h"

Widget_pixelSize::Widget_pixelSize(
        QWidget *parent, Qt::WindowFlags f)
    : QtPluginWidgetBase(parent, f)
{

    this->mWidgetLocation = WidgetLocation::top_right;
    mStreamTypes = ifind::InitialiseStreamTypeSetFromString("pixelSize");
    mIsBuilt = false;
    mPickedPoints.push_back(QVector2D(0, 0));
    mPickedPoints.push_back(QVector2D(0, 50));


    mLabel = new QLabel("Text not set", this);
    mLabel->setStyleSheet(sQLabelStyle);

//    mPickPointsButton = new QPushButton("Pick ruler points", this);
//    mPickPointsButton->setStyleSheet(sQPushButtonStyle);
//    QHBoxLayout *buttonlayout = new QHBoxLayout();
//    buttonlayout->addWidget(mPickPointsButton);
//    buttonlayout->addStretch();

    QHBoxLayout *intervallayout = new QHBoxLayout();
    mIntervalLabel = new QLabel("mm between ticks", this);
    mIntervalLabel->setStyleSheet(sQLabelStyle);
    mIntervalSelector = new QSpinBox(this);
    mIntervalSelector->setRange(1, 200);
    mIntervalSelector->setSingleStep(5);
    mIntervalSelector->setValue(50);
    mIntervalSelector->setStyleSheet(sQSpinBoxStyle);
    intervallayout->addWidget(mIntervalLabel);
    intervallayout->addWidget(mIntervalSelector);
    intervallayout->addStretch();

    mPointCoordLabel = new QLabel(this->PickedCoordinatesToLabel(1).c_str(), this);
    mPointCoordLabel->setStyleSheet(sQLabelStyle);

    /*
    mSlider = new QSlider(Qt::Orientation::Horizontal);
    mSlider->setStyleSheet(QtPluginWidgetBase::sQSliderStyle);

    mSlider->setMaximum(101);
    mSlider->setMinimum(0);
    mSlider->setAutoFillBackground(true);
    */
    auto vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    this->setLayout(vLayout);
    vLayout->addWidget(mLabel);
    vLayout->addLayout(intervallayout);
    vLayout->addWidget(mPointCoordLabel);
    this->AddInputStreamComboboxToLayout(vLayout);
    this->AddImageViewCheckboxToLayout(vLayout);
    //vLayout->addLayout(buttonlayout);
}


std::string Widget_pixelSize::PickedCoordinatesToLabel(double spacing){
    int n_pp = this->mPickedPoints.size();
    std::stringstream ss;

    ss << "Picked points: "<< std::endl;
    ss << "\t"<< this->mPickedPoints[n_pp-2][0]<< ", "<< this->mPickedPoints[n_pp-2][1]<<std::endl;
    ss << "\t"<< this->mPickedPoints[n_pp-1][0]<< ", "<< this->mPickedPoints[n_pp-1][1]<<std::endl;
    ss << "Spacing: "<< spacing << "mm per pixel"<<std::endl;
    return ss.str();
}

void Widget_pixelSize::Build(){

//    auto labelFont = mLabel->font();
//    labelFont.setPixelSize(15);
//    labelFont.setBold(true);
//    mLabel->setFont(labelFont);

    QVBoxLayout * outmost_layout = reinterpret_cast <QVBoxLayout *>( this->layout());
    //outmost_layout->addWidget(mLabel, 1, Qt::AlignTop);

    /// Pick points
    {
       /* QtInfoPanelLabelConFetalWeight *infoPanel = new QtInfoPanelLabelConFetalWeight(this);
        infoPanel->SetStreamTypesFromStr("Biometrics");
        outmost_layout->addWidget(infoPanel, 1, Qt::AlignTop);

        QObject::connect(
                    this, &QtPluginWidgetBase::ImageAvailable,
                    infoPanel, &QtInfoPanelBase::SendImageToWidget);
                    */
    }


}

void Widget_pixelSize::ProcessPickedPoint(QVector2D &vec){
    //std::cout << "Widget_pixelSize::ProcessPickedPoint(QVector2D &vec ) picked: "<< vec[0]<<", "<< vec[1]<<std::endl;
    this->mPickedPoints.push_back(vec);
    int n_pp = this->mPickedPoints.size();
    std::stringstream ss;
    double distance_in_pixels = this->mPickedPoints[n_pp-1].distanceToPoint(this->mPickedPoints[n_pp-2]);
    double distance_in_mm = mIntervalSelector->value();
    double spacing = distance_in_mm / distance_in_pixels;
    this->ProcessNewSpacing(spacing);
}

void Widget_pixelSize::ProcessNewSpacing(double spacing){
    mPointCoordLabel->setText(this->PickedCoordinatesToLabel(spacing).c_str());
    Q_EMIT this->signal_spacingComputed(spacing);
}

void Widget_pixelSize::SendImageToWidgetImpl(ifind::Image::Pointer image){

    if (mIsBuilt == false){
        mIsBuilt = true;
        this->Build();
    }

    std::stringstream stream;
    stream << "==" << this->mPluginName.toStdString() << "=="<<std::endl;
    //stream << "Receiving " << ifind::StreamTypeSetToString(this->mInputStreamTypes) << std::endl;
    if (image->HasKey("pixelSize_value")){
        stream << "PixelSize: "<< image->GetMetaData<std::string>("pixelSize_value") << " mm";
    }
    stream << "Sending " << ifind::StreamTypeSetToString(this->mStreamTypes);



    mLabel->setText(stream.str().c_str());
    Q_EMIT this->ImageAvailable(image);
}
