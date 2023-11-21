#include "Worker_pixelSize.h"
#include <iostream>
#include <QDebug>

//#include <pybind11/embed.h>
//#include <pybind11/numpy.h>

#include <itkImportImageFilter.h>
#include "pngutils.hxx"

Worker_pixelSize::Worker_pixelSize(QObject *parent) : Worker(parent){
    this->mSpacing = 1.0;
}

void Worker_pixelSize::Initialize(){

    if (this->params.verbose){
        std::cout << "Worker_pixelSize::Initialize()"<<std::endl;
    }
}

Worker_pixelSize::~Worker_pixelSize(){

}

void Worker_pixelSize::slot_UpdateSpacing(double spacing){
    this->mSpacing = spacing;
}

void Worker_pixelSize::doWork(ifind::Image::Pointer image){

    if (image == nullptr){
        if (this->params.verbose){
            std::cout << "Worker_pixelSize::doWork() - input image was null" <<std::endl;
        }
        return;
    }


    if (this->params.verbose){
        std::cout << "Worker_pixelSize::doWork()"<<std::endl;
    }

    /// Extract central slice and crop
    if (this->params.verbose){
        std::cout << "Worker_pixelSize::doWork() - adjust ratio"<<std::endl;
    }


    //GrayImageType2D::Pointer image_2d = this->get2dimage(image);;
    //image->SetMetaData<std::string>( mPluginName.toStdString() +"_area", QString::number(area_pixels).toStdString() );
    //image->SetMetaData<std::string>( mPluginName.toStdString() +"_areaM", QString::number(M).toStdString() );
    //image->SetMetaData<std::string>( mPluginName.toStdString() +"_aream", QString::number(m).toStdString() );
    image->SetMetaData<std::string>( mPluginName.toStdString() +"_spacing", QString::number(this->mSpacing).toStdString() );
    ifind::Image::SpacingType spacing;
    spacing.Fill(this->mSpacing);
    image->SetSpacingAllLAyers(spacing);
    Q_EMIT this->ImageProcessed(image);


    if (this->params.verbose){
        std::cout << "[VERBOSE] Worker_pixelSize::doWork() - image processed." <<std::endl;
    }

    //exit(-1);

}

