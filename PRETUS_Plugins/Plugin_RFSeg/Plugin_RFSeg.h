#pragma once

#include <Plugin.h>
#include "Worker_RFSeg.h"
#include "Widget_RFSeg.h"
#include <QtVTKVisualization.h>
#include <vector>

class Plugin_RFSeg : public Plugin {
    Q_OBJECT

public:
    typedef Worker_RFSeg WorkerType;
    typedef Widget_RFSeg WidgetType;
    typedef QtVTKVisualization ImageWidgetType;
    Plugin_RFSeg(QObject* parent = 0);

    QString GetPluginName(void){ return "RF Seg";}
    QString GetPluginDescription(void) {return "Automatic Rectus Femoris segmentation using Unet - impl. by Kerdegari et al.";}
    void SetCommandLineArguments(int argc, char* argv[]);

    void Initialize(void);

protected:
    virtual void SetDefaultArguments();

    template <class T> QString VectorToQString(std::vector<T> vec);
    template <class T> std::vector<T> QStringToVector(QString str);

public Q_SLOTS:
    virtual void slot_configurationReceived(ifind::Image::Pointer image);
    virtual void slot_saveResults();

};
