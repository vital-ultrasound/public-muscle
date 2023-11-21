#pragma once

#include <Plugin.h>
#include "Worker_pixelSize.h"
#include "Widget_pixelSize.h"
#include <QtVTKVisualization.h>

class Plugin_pixelSize : public Plugin {
    Q_OBJECT

public:
    typedef Worker_pixelSize WorkerType;
    typedef Widget_pixelSize WidgetType;
    typedef QtVTKVisualization ImageWidgetType;
    Plugin_pixelSize(QObject* parent = 0);

    QString GetPluginName(void){ return "Pixel Size";}
    QString GetPluginDescription(void) {return "PIxel size computation.";}
    void SetCommandLineArguments(int argc, char* argv[]);

    void Initialize(void);

protected:
    virtual void SetDefaultArguments();


public Q_SLOTS:
    virtual void slot_configurationReceived(ifind::Image::Pointer image);


};
