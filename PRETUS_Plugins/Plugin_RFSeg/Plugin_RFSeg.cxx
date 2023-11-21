#include "Plugin_RFSeg.h"
#include <generated/plugin_rfseg_config.h>
#include <ifindImagePeriodicTimer.h>
#include <QObject>
#include <QCheckBox>
#include <QPushButton>

Q_DECLARE_METATYPE(ifind::Image::Pointer)
Plugin_RFSeg::Plugin_RFSeg(QObject *parent) : Plugin(parent)
{
    {
        WorkerType::Pointer worker_ = WorkerType::New();
        worker_->python_folder = std::string(RFSeg::getPythonFolder());
        this->worker = worker_;
    }
    this->mStreamTypes = ifind::InitialiseStreamTypeSetFromString("Input");
    this->setFrameRate(25); // fps
    this->Timer->SetDropFrames(true);

    {
        // create widget
        WidgetType * mWidget_ = new WidgetType;
        this->mWidget = mWidget_;

        WorkerType *w = std::dynamic_pointer_cast< WorkerType >(this->worker).get();

        QObject::connect(mWidget_, &WidgetType::signal_measurement_finished,
                         w, &Worker_RFSeg::slot_storeNextImage);

        QObject::connect(mWidget_, &WidgetType::signal_measurement_finished,
                         w, &Worker_RFSeg::slot_storeNextImage);

        QObject::connect(w, &WorkerType::signal_thumbnail,
                         mWidget_, &WidgetType::slot_UpdateThumbnail);

        QObject::connect(mWidget_->mSaveButton, &QPushButton::released,
                         this, &Plugin_RFSeg::slot_saveResults);


    }
    {
        // create image widget
        ImageWidgetType * mWidget_ = new ImageWidgetType;
        this->mImageWidget = mWidget_;
        this->mImageWidget->SetStreamTypes(ifind::InitialiseStreamTypeSetFromString(this->GetCompactPluginName().toStdString()));
        this->mImageWidget->SetWidgetLocation(ImageWidgetType::WidgetLocation::visible); // by default, do not show

        // set image viewer default options:
        // overlays, colormaps, etc
        ImageWidgetType::Parameters default_params = mWidget_->Params();
        default_params.SetBaseLayer(0); // use the input image as background image
        default_params.SetOverlayLayer(-1); // show 1 layer on top of the background
        default_params.SetLutId(5);
        default_params.SetShowColorbar(false);
        mWidget_->SetParams(default_params);


        WidgetType *wid = reinterpret_cast< WidgetType *>(this->mWidget);

        QObject::connect(wid->mShowOverlayCheckbox, &QCheckBox::toggled,
                         mWidget_, &ImageWidgetType::EnableOverlay);
    }
    this->SetDefaultArguments();

}

void Plugin_RFSeg::Initialize(void){

    Plugin::Initialize();
    reinterpret_cast< ImageWidgetType *>(this->mImageWidget)->Initialize();
    this->worker->Initialize();
    // Retrieve the list of classes and create a blank image with them as meta data.
    ifind::Image::Pointer configuration = ifind::Image::New();
    configuration->SetMetaData<std::string>("PythonInitialized",this->GetPluginName().toStdString());
    Q_EMIT this->ConfigurationGenerated(configuration);

    this->Timer->Start(this->TimerInterval);
}

void Plugin_RFSeg::slot_saveResults(){
    WidgetType *wid = reinterpret_cast< WidgetType *>(this->mWidget);
    auto timings = wid->mTimes_msec;
    auto areas = wid->mAreas_mm;
    std::vector< std::vector<std::string> > is_valid(timings.size());
    for (int i =0; i < timings.size(); i++){
        is_valid[i].resize(timings[i].size());
        for (int j =0; j < timings[i].size(); j++){
            is_valid[i][j] = wid->mThumbnailsCB[i][j]->isChecked() ? "yes" : "no";
        }
    }

    WorkerType *w = std::dynamic_pointer_cast< WorkerType >(this->worker).get();
    w->saveResults(timings, areas, is_valid);
}


void Plugin_RFSeg::slot_configurationReceived(ifind::Image::Pointer image){
    Plugin::slot_configurationReceived(image);
    if (image->HasKey("PythonInitialized")){
        std::string whoInitialisedThePythonInterpreter = image->GetMetaData<std::string>("PythonInitialized");
        std::cout << "[WARNING from "<< this->GetPluginName().toStdString() << "] Python interpreter already initialized by \""<< whoInitialisedThePythonInterpreter <<"\", no initialization required."<<std::endl;
        this->worker->setPythonInitialized(true);
    }

    if (image->HasKey("Python_gil_init")){
        std::cout << "[WARNING from "<< this->GetPluginName().toStdString() << "] Python Global Interpreter Lock already set by a previous plug-in."<<std::endl;
        this->worker->set_gil_init(1);
    }

    /// Pass on the message in case we need to "jump" over plug-ins
    Q_EMIT this->ConfigurationGenerated(image);
}

void Plugin_RFSeg::SetDefaultArguments(){
    // arguments are defined with: name, placeholder for value, argument type,  description, default value

    mArguments.push_back({"modelname", "<*.h5>",
                          QString( Plugin::ArgumentType[3] ),
                          "Model file name (without folder).",
                          QString(std::dynamic_pointer_cast< WorkerType >(this->worker)->modelname.c_str())});

    mArguments.push_back({"cropbounds", "xmin:ymin:width:height",
                          QString( Plugin::ArgumentType[3] ),
                          "set of four colon-delimited numbers with the pixels to define the crop bounds",
                          this->VectorToQString<double>(std::dynamic_pointer_cast< WorkerType >(this->worker)->cropBounds()).toStdString().c_str()});

    mArguments.push_back({"abscropbounds", "0/1",
                          QString( Plugin::ArgumentType[0] ),
                          "whether the crop bounds are provided in relative values (0 - in %) or absolute (1 -in pixels)",
                          QString::number(std::dynamic_pointer_cast< WorkerType >(this->worker)->absoluteCropBounds()).toStdString().c_str()});

    mArguments.push_back({"output", "<filename>",
                          QString( Plugin::ArgumentType[3] ),
                          "path to the output filename where results will be written",
                          std::dynamic_pointer_cast< WorkerType >(this->worker)->getOutput_filename().toStdString().c_str()});

    mArguments.push_back({"showcontour", "<val>",
                          QString( Plugin::ArgumentType[1] ),
                          "Whether to show the segmentation as a mask (0) or as a contour (>0), where the entered value is the thickness of the contour",
                          QString::number(std::dynamic_pointer_cast< WorkerType >(this->worker)->getContourThickness())});

}

void Plugin_RFSeg::SetCommandLineArguments(int argc, char* argv[]){
    Plugin::SetCommandLineArguments(argc, argv);
    InputParser input(argc, argv, this->GetCompactPluginName().toLower().toStdString());

    {const std::string &argument = input.getCmdOption("modelname");
        if (!argument.empty()){
            std::dynamic_pointer_cast< WorkerType >(this->worker)->modelname = argument.c_str();
        }}

    {const std::string &argument = input.getCmdOption("cropbounds");
        if (!argument.empty()){
            std::dynamic_pointer_cast< WorkerType >(this->worker)->setCropBounds(this->QStringToVector<double>(argument.c_str()));
        }}

    {const std::string &argument = input.getCmdOption("abscropbounds");
        if (!argument.empty()){
            std::dynamic_pointer_cast< WorkerType >(this->worker)->setAbsoluteCropBounds(atoi(argument.c_str()));
        }}

    {const std::string &argument = input.getCmdOption("output");
        if (!argument.empty()){
            std::dynamic_pointer_cast< WorkerType >(this->worker)->setOutput_filename(argument.c_str());
        }}

    {const std::string &argument = input.getCmdOption("showcontour");
        if (!argument.empty()){
            std::dynamic_pointer_cast< WorkerType >(this->worker)->setContourThickness(atoi(argument.c_str()));
        }}

    // no need to add above since already in plugin
    {const std::string &argument = input.getCmdOption("verbose");
        if (!argument.empty()){
            std::dynamic_pointer_cast< WorkerType >(this->worker)->params.verbose= atoi(argument.c_str());
        }}
}


template <class T>
QString Plugin_RFSeg::VectorToQString(std::vector<T> vec){
    QString out;

    for (T val : vec){
        out.push_back(QString::number(val) + ":");
    }
    // remove the last ':'
    int pos = out.lastIndexOf(QChar(':'));

    return out.left(pos);
}

template <class T>
std::vector<T> Plugin_RFSeg::QStringToVector(QString str){
    std::vector<T> out;

    QStringList str_list = str.split(":");
    for (QString val : str_list){
        out.push_back(T(val.toDouble()));
    }

    return out;
}

extern "C"
{
#ifdef WIN32
__declspec(dllexport) Plugin* construct()
{
    return new Plugin_RFSeg();
}
#else
Plugin* construct()
{
    return new Plugin_RFSeg();
}
#endif // WIN32
}
