#include "Worker_RFSeg.h"
#include <iostream>
#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <QDir>

#include <itkImportImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionIteratorWithIndex.h>
#include "pngutils.hxx"
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>

Worker_RFSeg::Worker_RFSeg(QObject *parent) : Worker(parent){
    this->python_folder = "";
    this->modelname = "model_5l_AugPlus.pth";
    this->params.out_size[0] = 128;
    this->params.out_size[1] = 128;
    this->params.out_spacing[0] = 9.0;
    this->params.out_spacing[1] = 6.0;
    this->params.origin = Worker::WorkerParameters::OriginPolicy::Centre;
    this->output_filename = "output.txt";
    // TODO: I should probably do something about the spacing

    this->mAreaBuffer.clear();
    this->mMaxAreaBufferSize = 20;
    this->m_store_next_image.resize(2);
    this->m_store_next_image[0] = -1;
    this->m_store_next_image[1] = -1;

    mAbsoluteCropBounds = false; // by default relative
    mAratio = {275, 175};
    //mCropBounds = {500, 200, 1100, 700}; // for 1920 × 1080
    mCropBounds = {0.25, 0.2, 0.55, 0.65}; // for 1920 × 1080
    mDesiredSize = {128, 128};
}

QString Worker_RFSeg::getOutput_filename() const
{
    return output_filename;
}

void Worker_RFSeg::setOutput_filename(const QString &value)
{
    output_filename = value;
}

void Worker_RFSeg::Initialize(){

    if (this->params.verbose){
        std::cout << "Worker_RFSeg::Initialize()"<<std::endl;
    }

    if (!this->PythonInitialized){
        try {
            py::initialize_interpreter();
        }
        catch (py::error_already_set const &pythonErr) {
            std::cout << "[ERROR] Worker_RFSeg::Initialize() " << pythonErr.what();
        }

    }

    if (this->params.verbose){
        std::cout << "Worker_RFSeg::Initialize() - load model ..." << std::flush;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();
    {
        py::exec("import sys");
        std::string command = "sys.path.append('" + this->python_folder + "')";
        py::exec(command.c_str());

        py::object processing;
        try {
            processing = py::module::import("RFSeg_worker");
        } catch (pybind11::error_already_set & err) {
            std::cout << "[ERROR] Worker_RFSeg::Initialize() - error while importing RFSeg module" << std::endl;
            err.restore();
        }
        /// Check for errors
        if (PyErr_Occurred())
        {
            std::cout << "[ERROR] Worker_RFSeg::Initialize() " << std::endl;
            PyErr_Print();
            return;
        }

        /// grabbing the functions from module
        this->PyImageProcessingFunction = processing.attr("dowork");
        this->PyPythonInitializeFunction = processing.attr("initialize");
        py::tuple sz = py::make_tuple(128, 128);
        this->PyPythonInitializeFunction(sz,this->python_folder + "/model", this->modelname);

        this->PythonInitialized = true;
    }
    PyGILState_Release(gstate);

    if (this->params.verbose){
        std::cout << "loaded"<<std::endl;
    }

}

Worker_RFSeg::~Worker_RFSeg(){
    /// Finalize python stuff
    py::finalize_interpreter();
}

void Worker_RFSeg::slot_storeNextImage(int legside, int examid){
    if (legside+1 > mMeasuredImages.size()){
        mMeasuredImages.resize(legside+1);
    }
    if (examid+1 > mMeasuredImages[legside].size()){
        mMeasuredImages[legside].resize(examid+1, nullptr);
    }
    this->m_store_next_image[0] = legside;
    this->m_store_next_image[1] = examid;
}

void Worker_RFSeg::saveResults(std::vector<std::vector<float> > &timings_msec, std::vector<std::vector<float> > &areas_mm2, std::vector<std::vector<std::string> > &is_valid){

    // CHeck if the folder exists, else create it.

    QFileInfo file_info(this->output_filename);
    QString folder = file_info.absolutePath();

    if (!QDir(folder).exists()){
        std::cout << "Worker_RFSeg::saveResults - creating output folder "<< folder.toStdString() << std::endl;
        QDir().mkdir(folder);
    }

    // now write to
    std::ofstream out;
    out.open(this->output_filename.toStdString());

    // image to save
    out << "# Times (s)"<<std::endl;
    for (int legside = 0; legside < timings_msec.size(); legside++ ){
        std::string ls = (legside==0) ? "L" : "R";
        out << ls<<"\t";
        for (int examid = 0; examid < timings_msec[legside].size(); examid++ ){
            out << timings_msec[legside][examid]/1000<< "\t";
        }
        out <<std::endl;

    }
    out << "# Areas (cm2)"<<std::endl;
    for (int legside = 0; legside < timings_msec.size(); legside++ ){
        std::string ls = (legside==0) ? "L" : "R";
        out << ls<<"\t";
        for (int examid = 0; examid < timings_msec[legside].size(); examid++ ){
            out << areas_mm2[legside][examid]/100 << "\t";
        }
        out <<std::endl;
    }
    out << "# Valid image (yes/no)"<<std::endl;
    for (int legside = 0; legside < timings_msec.size(); legside++ ){
        std::string ls = (legside==0) ? "L" : "R";
        out << ls<<"\t";
        for (int examid = 0; examid < timings_msec[legside].size(); examid++ ){
            out << is_valid[legside][examid] << "\t";
        }
        out <<std::endl;
    }

    out.close();
    std::cout << "Worker_RFSeg::saveResults to "<< this->output_filename.toStdString()<<std::endl;

    // save images
    for (int legside = 0; legside < this->mMeasuredImages.size(); legside++ ){
        std::string ls = (legside==0) ? "L" : "R";
        for (int examid = 0; examid < this->mMeasuredImages[legside].size(); examid++ ){
            if (this->mMeasuredImages[legside][examid] == nullptr){
                continue;
            }
            if (this->mMeasuredImages[legside][examid]->GetNumberOfLayers() <2){
                continue;
            }
            QImage img_bck = this->imageToQImage(this->mMeasuredImages[legside][examid], 0);
            QImage img_segmentation = this->imageToQImage(this->mMeasuredImages[legside][examid], this->mMeasuredImages[legside][examid]->GetNumberOfLayers()-1);
            QImage img_overlay = this->imageToQImage(this->mMeasuredImages[legside][examid], this->mMeasuredImages[legside][examid]->GetNumberOfLayers()-1, true);

            QString filename_bck = this->output_filename + "_" + QString(ls.c_str()) + QString::number(examid) + "_background.png";
            img_bck.save(filename_bck);
            QString filename_segmentation = this->output_filename + "_" + QString(ls.c_str()) + QString::number(examid) + "_segmentation.png";
            img_segmentation.save(filename_segmentation);
            QString filename_overlay = this->output_filename + "_" + QString(ls.c_str()) + QString::number(examid) + "_overlay.png";
            img_overlay.save(filename_overlay);
        }
        //std::cout <<std::endl;
    }


}

void Worker_RFSeg::doWork(ifind::Image::Pointer image){


    if (!this->PythonInitialized){
        return;
    }

    if (!Worker::gil_init) {
        Worker::gil_init = 1;
        PyEval_InitThreads();
        PyEval_SaveThread();

        ifind::Image::Pointer configuration = ifind::Image::New();
        configuration->SetMetaData<std::string>("Python_gil_init","True");
        Q_EMIT this->ConfigurationGenerated(configuration);
    }

    if (image == nullptr){
        if (this->params.verbose){
            std::cout << "Worker_RFSeg::doWork() - input image was null" <<std::endl;
        }
        return;
    }


    if (this->params.verbose){
        std::cout << "Worker_RFSeg::doWork(). image spacing is "<<image->GetSpacing()[0]<< ", "<<image->GetSpacing()[1]<< std::endl;
    }

    /// Extract central slice and crop
    if (this->params.verbose){
        std::cout << "Worker_RFSeg::doWork() - adjust ratio"<<std::endl;
    }


    ifind::Image::Pointer image_ratio_adjusted;

    std::vector<int> absoluteCropBounds(4);
    if (this->absoluteCropBounds() == true){
        std::copy(mCropBounds.begin(), mCropBounds.end(), back_inserter(absoluteCropBounds));
    } else {
        // get the image size
        ifind::Image::SizeType imsize = image->GetLargestPossibleRegion().GetSize();
        absoluteCropBounds[0] = int(mCropBounds[0] * imsize[0]); // x0
        absoluteCropBounds[1] = int(mCropBounds[1] * imsize[1]); // y0
        absoluteCropBounds[2] = int(mCropBounds[2] * imsize[0]); // w
        absoluteCropBounds[3] = int(mCropBounds[3] * imsize[1]); // h

        if (this->params.verbose){
            std::cout << "\tWorker_RFSeg::doWork() computing absolute crop bounds"<<std::endl;
            std::cout << "\t\timage size is "<< imsize[0] << "x" << imsize[1]<<std::endl;
            std::cout << "\t\trelative crop bounds are "<< mCropBounds[0] << ":" << mCropBounds[1]<< ":" << mCropBounds[2]<< ":" << mCropBounds[3]<<std::endl;
            std::cout << "\t\tabsolute crop bounds are "<< absoluteCropBounds[0] << ":" << absoluteCropBounds[1]<< ":" << absoluteCropBounds[2]<< ":" << absoluteCropBounds[3]<<std::endl;
        }
    }

    /// Use the appropriate layer
    std::vector<std::string> layernames = image->GetLayerNames();
    int layer_idx = this->params.inputLayer;
    if (this->params.inputLayer <0){
        /// counting from the end
        layer_idx = image->GetNumberOfLayers() + this->params.inputLayer;
    }
    ifind::Image::Pointer layerImage = ifind::Image::New();
    layerImage->Graft(image->GetOverlay(layer_idx), layernames[layer_idx]);
    //layerImage->SetSpacing(image->GetSpacing());
    image_ratio_adjusted = this->CropImageToFixedAspectRatio(layerImage, &mAratio[0], &absoluteCropBounds[0]);

    if (this->params.verbose){
        std::cout << "Worker_RFSeg::doWork(). layerImage spacing is "<<layerImage->GetSpacing()[0]<< ", "<<layerImage->GetSpacing()[1]<< std::endl;
    }

    //png::save_ifind_to_png_file<ifind::Image>(image_ratio_adjusted, "/home/ag09/data/VITAL/cpp_in_adjusted.png");
    // now resample to 128 128
    if (this->params.verbose){
        std::cout << "Worker_RFSeg::doWork() - resample"<<std::endl;
    }
    ifind::Image::Pointer image_ratio_adjusted_resampled  = this->ResampleToFixedSize(image_ratio_adjusted, &mDesiredSize[0]);
    //png::save_ifind_to_png_file<ifind::Image>(image_ratio_adjusted_resampled, "/home/ag09/data/VITAL/cpp_in_adjusted_resampled.png");
    this->params.out_spacing[0] = this->params.out_spacing[0] * (this->params.out_size[0] - 1 )/ (128 - 1);
    this->params.out_spacing[1] = this->params.out_spacing[1] * (this->params.out_size[1] - 1 )/ (128 - 1);
    this->params.out_size[0] = 128;
    this->params.out_size[1] = 128;


    if (this->params.verbose){
        std::cout << "\tWorker_RFSeg::doWork() Resampling information: "<<std::endl;
        std::cout << "\t\tBefore resampling: "<<std::endl;
        std::cout << "\t\t\tSpacing: "<< image_ratio_adjusted->GetSpacing()[0] <<", "<<image_ratio_adjusted->GetSpacing()[1]<<std::endl;
        std::cout << "\t\t\tSize: "<< image_ratio_adjusted->GetLargestPossibleRegion().GetSize()[0] <<", "<< image_ratio_adjusted->GetLargestPossibleRegion().GetSize()[1]<<std::endl;
        std::cout << "\t\tAfter resampling: "<<std::endl;
        std::cout << "\t\t\tSpacing: "<< image_ratio_adjusted_resampled->GetSpacing()[0] <<", "<<image_ratio_adjusted_resampled->GetSpacing()[1]<<std::endl;
        std::cout << "\t\t\tSize: "<< image_ratio_adjusted_resampled->GetLargestPossibleRegion().GetSize()[0] <<", "<< image_ratio_adjusted_resampled->GetLargestPossibleRegion().GetSize()[1]<<std::endl;
    }

    GrayImageType2D::Pointer image_2d = this->get2dimage(image_ratio_adjusted_resampled);

    GrayImageType2D::Pointer rf_segmentation;

    /// Create a numpy array containing the image scalars
    /// Input dimensions are swapped as ITK and numpy have inverted orders
    std::vector <unsigned long> dims = {image_2d->GetLargestPossibleRegion().GetSize()[1], image_2d->GetLargestPossibleRegion().GetSize()[0]};
    if (!image_2d->GetBufferPointer() || (dims[0] < 50) || (dims[1] < 50))
    {
        qWarning() << "[worker_rfseg] image buffer is invalid";
        return;
    }

    if (this->params.verbose){
        std::cout << "\t\tAfter 2D conversion: "<<std::endl;
        std::cout << "\t\t\tSpacing: "<< image_2d->GetSpacing()[0] <<", "<<image_2d->GetSpacing()[1]<<std::endl;
        std::cout << "\t\t\tSize: "<< image_2d->GetLargestPossibleRegion().GetSize()[0] <<", "<< image_2d->GetLargestPossibleRegion().GetSize()[1]<<std::endl;
    }

    double area_pixels = 0;
    double area_mm = 0;
    this->gstate = PyGILState_Ensure();
    {

        py::array numpyarray(dims, static_cast<GrayImageType::PixelType*>(image_2d->GetBufferPointer()));

        py::object _function = this->PyImageProcessingFunction;
        /// predict biometrics
        py::array segmentation_array = py::array(_function(numpyarray));
        //area_pixels = out_tuple[1].cast<int>();
        //area_mm= area_pixels * image_2d->GetSpacing()[0] * image_2d->GetSpacing()[1];

        /// ---------- Get the segmentation of the fitted ellipse -----------------------

        typedef itk::ImportImageFilter< GrayImageType::PixelType, 2 >   ImportFilterType;
        ImportFilterType::SizeType imagesize;

        imagesize[0] = segmentation_array.shape(1);
        imagesize[1] = segmentation_array.shape(0);

        if (this->params.verbose){
            std::cout << "Worker_RFSeg::doWork() - image size is "<< imagesize[0]<< " x " << imagesize[1] <<std::endl;
        }


        ImportFilterType::RegionType region;
        ImportFilterType::IndexType start;
        start.Fill(0);

        region.SetIndex(start);
        region.SetSize(imagesize);

        /// Define import filter
        ImportFilterType::Pointer importer = ImportFilterType::New();
        importer->SetOrigin( image_2d->GetOrigin() );
        importer->SetSpacing( image_2d->GetSpacing() );
        importer->SetDirection( image_2d->GetDirection() );
        importer->SetRegion(region);
        /// Separate the regional scalar buffer
        /// @todo check if a memcpy is necessary here
        GrayImageType::PixelType* localbuffer = static_cast<GrayImageType::PixelType*>(segmentation_array.mutable_data());
        /// Import the buffer
        importer->SetImportPointer(localbuffer, imagesize[0] * imagesize[1], false);
        importer->Update();

        /// Disconnect the output from the filter
        /// @todo Check if that is sufficient to release the numpy buffer, or if the buffer needs to obe memcpy'ed
        rf_segmentation = importer->GetOutput();
        rf_segmentation->DisconnectPipeline();

        rf_segmentation->SetMetaDataDictionary(image_2d->GetMetaDataDictionary());

        /// ---------- Get the contour of the ellipse ------------------------------------
        /// Create a 3D image with the 2D slice
        GrayImageType::Pointer segmentation = get3dimagefrom2d(rf_segmentation);
        //png::save_ifind_to_png_file<GrayImageType>(segmentation, "/home/ag09/data/VITAL/segmentation.png");
        GrayImageType::Pointer segmentation_unresized= this->UndoResampleToFixedSize(segmentation, image, &absoluteCropBounds[0]);

        //GrayImageType::Pointer responsemap = this->UnAdjustImageSize(segmentation, image);
        //png::save_ifind_to_png_file<GrayImageType>(segmentation_unresized, "/home/ag09/data/VITAL/unresampled_seg.png");
        //GrayImageType::Pointer responsemap_raw = this->UndoCropImageToFixedAspectRatio(segmentation_unresized, image, &absoluteCropBounds[0]);

        GrayImageType::PixelType threshold = 150;

        using FilterType = itk::BinaryThresholdImageFilter<GrayImageType, GrayImageType>;
        auto filter = FilterType::New();
        filter->SetInput(this->UndoCropImageToFixedAspectRatio(segmentation_unresized, image, &absoluteCropBounds[0]));
        filter->SetLowerThreshold(threshold);
        //filter->SetUpperThreshold(UpperThreshold);
        filter->SetOutsideValue(0);
        filter->SetInsideValue(255);
        filter->Update();
        GrayImageType::Pointer responsemap_raw = filter->GetOutput();

        // compute the area properly
        {
            area_pixels = 0;

            using GrayIteratorType = itk::ImageRegionConstIterator<GrayImageType>;
            GrayIteratorType it( responsemap_raw, responsemap_raw->GetRequestedRegion() );

            for ( it.GoToBegin() ; !it.IsAtEnd(); ++it)
            {
                area_pixels += int(it.Value()>threshold) ; // only foreground pixels
            }
            area_mm= area_pixels * image->GetSpacing()[0] * image->GetSpacing()[1];
        }

        GrayImageType::Pointer responsemap;
        // first threshold the result
        // then extract the contour
        if (this->mContourThickness > 0){

            using StructuringElementType = itk::FlatStructuringElement<3>;
            StructuringElementType::RadiusType radius;
            radius.Fill(this->mContourThickness);
            StructuringElementType structuringElement = StructuringElementType::Ball(radius);

            // convert to binary and then use this.
            //using GrayscaleDilateImageFilterType = itk::GrayscaleDilateImageFilter<GrayImageType, GrayImageType, StructuringElementType>;
            //auto dilateFilter = GrayscaleDilateImageFilterType::New();
            //dilateFilter->SetInput(responsemap_raw);
            //dilateFilter->SetKernel(structuringElement);

            using GrayscaleErodeImageFilterType = itk::GrayscaleErodeImageFilter<GrayImageType, GrayImageType, StructuringElementType>;
            auto erodeFilter = GrayscaleErodeImageFilterType::New();
            erodeFilter->SetInput(responsemap_raw);
            erodeFilter->SetKernel(structuringElement);


            using SubtractImageFilterType = itk::SubtractImageFilter<GrayImageType, GrayImageType>;
            auto subtractFilter = SubtractImageFilterType::New();
            subtractFilter->SetInput1(responsemap_raw);
            subtractFilter->SetInput2(erodeFilter->GetOutput());
            subtractFilter->Update();

            //dilateFilter->Update();
            //erodeFilter->Update();

            responsemap = subtractFilter->GetOutput();

        } else {
            responsemap = responsemap_raw;
        }


        image->GraftOverlay(responsemap.GetPointer(), image->GetNumberOfLayers(), "Segmentation");
        image->SetMetaData<std::string>( mPluginName.toStdString() +"_output", QString::number(image->GetNumberOfLayers()).toStdString() );

        //png::save_ifind_to_png_file<ifind::Image>(image, "/home/ag09/data/VITAL/input_image.png");
        //png::save_ifind_to_png_file<GrayImageType>(responsemap, "/home/ag09/data/VITAL/segmentaiton_image.png");

        if (this->params.verbose){
            std::cout << "\tWorker_RFSeg::doWork() - done" <<std::endl;
        }
    }
    PyGILState_Release(this->gstate);

    //    png::save_ifind_to_png_file<ifind::Image>(image, "/home/ag09/data/VITAL/tmp2/image.png");
    //    png::save_ifind_to_png_file<GrayImageType>(image_ratio_adjusted, "/home/ag09/data/VITAL/tmp2/image_ratio_adjusted.png");
    //    png::save_ifind_to_png_file<GrayImageType>(image_ratio_adjusted_resampled, "/home/ag09/data/VITAL/tmp2/image_ratio_adjusted_resampled.png");
    //    png::save_ifind_to_png_file<GrayImageType2D>(image_2d, "/home/ag09/data/VITAL/tmp2/image_2d.png");


    // deal with the area buffer - ensure the size is appropriate
    if (this->params.verbose){
        std::cout << "\t\tAfter 2D conversion: "<<std::endl;
        std::cout << "\t\t\tSpacing: "<< image_2d->GetSpacing()[0] <<", "<<image_2d->GetSpacing()[1]<<std::endl;
        std::cout << "\t\t\tSize: "<< image_2d->GetLargestPossibleRegion().GetSize()[0] <<", "<< image_2d->GetLargestPossibleRegion().GetSize()[1]<<std::endl;
        std::cout << "\t\tNpixels: "<< area_pixels<<std::endl;
        std::cout << "\t\tArea: "<< area_mm / 100<<std::endl;
    }


    //this->mAreaBuffer.enqueue(area_pixels);
    this->mAreaBuffer.enqueue(area_mm);
    while (this->mAreaBuffer.size()>this->mMaxAreaBufferSize+1){
        this->mAreaBuffer.dequeue();
    }
    // compute min, max and average
    int M = -1;
    int m = 10000000;
    float area_average = 0;
    for (auto l : this->mAreaBuffer){
        if (l > M) M = l;
        if (l < m) m = l;
        area_average += l;
    }
    area_average /= this->mAreaBuffer.size();


    image->SetMetaData<std::string>( mPluginName.toStdString() +"_area", QString::number(area_mm).toStdString() );
    image->SetMetaData<std::string>( mPluginName.toStdString() +"_areaM", QString::number(M).toStdString() );
    image->SetMetaData<std::string>( mPluginName.toStdString() +"_aream", QString::number(m).toStdString() );
    image->SetMetaData<std::string>( mPluginName.toStdString() +"_areaav", QString::number(area_average).toStdString() );


    Q_EMIT this->ImageProcessed(image);

    if (this->m_store_next_image[0] > -1){
        this->mMeasuredImages[this->m_store_next_image[0]][this->m_store_next_image[1]] = image;
        QPixmap pix = imageToPixmap(image, 256, 256);
        Q_EMIT signal_thumbnail(pix, this->m_store_next_image[0], this->m_store_next_image[1]);

        this->m_store_next_image[0] = -1;
        this->m_store_next_image[1] = -1;
    }

    if (this->params.verbose){
        std::cout << "[VERBOSE] Worker_RFSeg::doWork() - image processed." <<std::endl;
    }

    //exit(-1);

}

QImage Worker_RFSeg::imageToQImage(ifind::Image::Pointer in, int layer, bool do_overlay){
    int w = in->GetLargestPossibleRegion().GetSize()[0];
    int h = in->GetLargestPossibleRegion().GetSize()[1];

    QImage background(in->GetOverlay(layer)->GetBufferPointer(), w, h, QImage::Format_Indexed8);

    if (do_overlay == true && layer > 0){
        QPixmap pix = imageToPixmap(in, -1, -1);
        background = pix.toImage();
    }

    return background;
}

int Worker_RFSeg::getContourThickness() const
{
    return mContourThickness;
}

void Worker_RFSeg::setContourThickness(int newContourThickness)
{
    mContourThickness = newContourThickness;
}

QPixmap Worker_RFSeg::imageToPixmap(ifind::Image::Pointer in, int wp, int hp){

    int w = in->GetLargestPossibleRegion().GetSize()[0];
    int h = in->GetLargestPossibleRegion().GetSize()[1];

    if (wp <= 0){
        wp = w;
    }
    if (hp <= 0){
        hp = h;
    }

    ifind::Image::PixelType *p0 = in->GetOverlay(0)->GetBufferPointer();
    ifind::Image::PixelType *p1 = in->GetOverlay(in->GetNumberOfLayers()-1)->GetBufferPointer();
    ifind::Image::PixelType *p2 =  new ifind::Image::PixelType[h*w];

    ifind::Image::PixelType *point = &p2[0];

    for (int i=0; i < h*w; ++i ){
        *point = (*p1 >128) ? *p1 : *p0;
        ++p0;
        ++p1;
        ++point;
    }

    //QImage background(in->GetOverlay(0)->GetBufferPointer(), w, h, QImage::Format_Indexed8);
    //QImage overlay(in->GetOverlay(in->GetNumberOfLayers()-1)->GetBufferPointer(), w, h, QImage::Format_Indexed8);
    QImage merged(p2, w, h, QImage::Format_Indexed8);

    ////std::cout << "save to file"<<std::endl;
    //background.save("/home/ag09/tmp/tmp.png");
    //overlay.save("/home/ag09/tmp/tmp2.png");
    //merged.save("/home/ag09/tmp/tmp3.png");

    //QIcon icon_bck;
    //icon_bck.addPixmap(QPixmap::fromImage(background));
    //QPixmap pix_bck  = icon_bck.pixmap(QSize(128, 128), QIcon::Normal, QIcon::On);

    //QIcon icon_seg;
    //icon_seg.addPixmap(QPixmap::fromImage(overlay));
    //QPixmap pix_seg  = icon_seg.pixmap(QSize(128, 128), QIcon::Normal, QIcon::On);

    QIcon icon;
    icon.addPixmap(QPixmap::fromImage(merged));
    QPixmap pix  = icon.pixmap(QSize(wp, hp), QIcon::Normal, QIcon::On);


    return pix;
}

bool Worker_RFSeg::absoluteCropBounds() const
{
    return mAbsoluteCropBounds;
}

void Worker_RFSeg::setAbsoluteCropBounds(bool absoluteCropBounds)
{
    mAbsoluteCropBounds = absoluteCropBounds;
}

std::vector<int> Worker_RFSeg::desiredSize() const
{
    return mDesiredSize;
}

void Worker_RFSeg::setDesiredSize(const std::vector<int> &desiredSize)
{
    mDesiredSize = desiredSize;
}

std::vector<float> Worker_RFSeg::aratio() const
{
    return mAratio;
}

void Worker_RFSeg::setAratio(const std::vector<float> &aratio)
{
    mAratio = aratio;
}

std::vector<double> Worker_RFSeg::cropBounds() const
{
    return mCropBounds;
}

void Worker_RFSeg::setCropBounds(const std::vector<double> &cropBounds)
{
    mCropBounds = cropBounds;
}
