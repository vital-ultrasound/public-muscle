#include "Plugin_pixelSize.h"
#include <generated/plugin_pixelsize_config.h>
#include <ifindImagePeriodicTimer.h>
#include <QObject>
#include <QPushButton>

Q_DECLARE_METATYPE(ifind::Image::Pointer)
Plugin_pixelSize::Plugin_pixelSize(QObject *parent) : Plugin(parent)
{
    {
        WorkerType::Pointer worker_ = WorkerType::New();
        this->worker = worker_;
    }
    this->mStreamTypes = ifind::InitialiseStreamTypeSetFromString("Input");
    this->setFrameRate(25); // fps
    this->Timer->SetDropFrames(true);

    {
        // create widget
        WidgetType * widget_ = new WidgetType;
        this->mWidget = widget_;

        //        // Connect widget and worker here
        //        WorkerType *w = std::dynamic_pointer_cast< WorkerType >(this->worker).get();
        //        QObject::connect(mWidget_->mPickPointsButton,
        //                &QPushButton::released, w,
        //                &WorkerType::slot_pointPicking);

    }
    {
        // create image widget
        ImageWidgetType * widget_ = new ImageWidgetType;
        this->mImageWidget = widget_;
        this->mImageWidget->SetStreamTypes(ifind::InitialiseStreamTypeSetFromString(this->GetCompactPluginName().toStdString()));
        this->mImageWidget->SetWidgetLocation(ImageWidgetType::WidgetLocation::visible); // by default, do not show

        // set image viewer default options:
        // overlays, colormaps, etc
        ImageWidgetType::Parameters default_params = widget_->Params();
        default_params.SetBaseLayer(0); // use the input image as background image
        //default_params.SetOverlayLayer(-1); // show 1 layer on top of the background
        //default_params.SetLutId(5);
        //default_params.SetShowColorbar(false);
        widget_->SetParams(default_params);

        // Connect widget and image worker here

        QObject::connect(widget_, &ImageWidgetType::PointPicked,
                         reinterpret_cast<WidgetType *>(this->mWidget), &WidgetType::ProcessPickedPoint);

        WorkerType *w = std::dynamic_pointer_cast< WorkerType >(this->worker).get();
        QObject::connect(reinterpret_cast<WidgetType *>(this->mWidget), &WidgetType::signal_spacingComputed,
                         w, &WorkerType::slot_UpdateSpacing);

        /*QObject::connect(reinterpret_cast<WidgetType *>(this->mWidget)->mPickPointsButton,
                &QPushButton::released, widget_,
                &ImageWidgetType::PickTwoPoints);
                */

    }
    this->SetDefaultArguments();

}

void Plugin_pixelSize::Initialize(void){

    Plugin::Initialize();
    reinterpret_cast< ImageWidgetType *>(this->mImageWidget)->Initialize();
    this->worker->Initialize();
    // Retrieve the list of classes and create a blank image with them as meta data.
    //ifind::Image::Pointer configuration = ifind::Image::New();
    //Q_EMIT this->ConfigurationGenerated(configuration);

    this->Timer->Start(this->TimerInterval);
}

void Plugin_pixelSize::slot_configurationReceived(ifind::Image::Pointer image){
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

void Plugin_pixelSize::SetDefaultArguments(){
    // arguments are defined with: name, placeholder for value, argument type,  description, default value
    mArguments.push_back({"pixelsize", "<mm per pixel>",
                          QString( Plugin::ArgumentType[2] ),
                          "Float value with pixels per mm if known.",
                          QString::number(std::dynamic_pointer_cast< WorkerType >(this->worker)->mSpacing).toStdString().c_str()});


}

void Plugin_pixelSize::SetCommandLineArguments(int argc, char* argv[]){
    Plugin::SetCommandLineArguments(argc, argv);
    InputParser input(argc, argv, this->GetCompactPluginName().toLower().toStdString());

    {const std::string &argument = input.getCmdOption("pixelsize");
        if (!argument.empty()){
        //            std::dynamic_pointer_cast< WorkerType >(this->worker)->mSpacing = atof(argument.c_str());
            double spacing = atof(argument.c_str());
            reinterpret_cast<WidgetType *>(this->mWidget)->ProcessNewSpacing(spacing);
        }}

    // no need to add above since already in plugin
    {const std::string &argument = input.getCmdOption("verbose");
        if (!argument.empty()){
            this->worker->params.verbose= atoi(argument.c_str());
        }}
}



extern "C"
{
#ifdef WIN32
__declspec(dllexport) Plugin* construct()
{
    return new Plugin_pixelSize();
}
#else
Plugin* construct()
{
    return new Plugin_pixelSize();
}
#endif // WIN32
}
