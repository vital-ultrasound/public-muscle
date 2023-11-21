#pragma once

#include <Worker.h>
#include <QQueue>
#include <memory>
#include <vector>

/// For image data. Change if image data is different
#include <ifindImage.h>

class Worker_pixelSize  : public Worker{
    Q_OBJECT

public:

    typedef Worker_pixelSize            Self;
    typedef std::shared_ptr<Self>       Pointer;

    /** Constructor */
    static Pointer New(QObject *parent = 0) {
        return Pointer(new Self(parent));
    }

    ~Worker_pixelSize();

    void Initialize();

    double mSpacing;

public Q_SLOTS:
    virtual void slot_UpdateSpacing(double spacing);

protected:
    Worker_pixelSize(QObject* parent = 0);

    void doWork(ifind::Image::Pointer image);



};
