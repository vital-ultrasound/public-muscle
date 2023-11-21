#pragma once

#include <Worker.h>
#include <QQueue>
#include <memory>
#include <vector>
#include <QPixmap>

/// For image data. Change if image data is different
#include <ifindImage.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

class Worker_RFSeg  : public Worker{
    Q_OBJECT

public:

    typedef Worker_RFSeg            Self;
    typedef std::shared_ptr<Self>       Pointer;

    /** Constructor */
    static Pointer New(QObject *parent = 0) {
        return Pointer(new Self(parent));
    }

    ~Worker_RFSeg();

    void Initialize();

    /// parameters must be only in the parent class
    std::string python_folder;
    std::string modelname;

    std::vector<double> cropBounds() const;
    void setCropBounds(const std::vector<double> &cropBounds);

    std::vector<float> aratio() const;
    void setAratio(const std::vector<float> &aratio);

    std::vector<int> desiredSize() const;
    void setDesiredSize(const std::vector<int> &desiredSize);

    bool absoluteCropBounds() const;
    void setAbsoluteCropBounds(bool absoluteCropBounds);

    void saveResults(std::vector<std::vector<float> > &timings, std::vector<std::vector<float> > &areas, std::vector<std::vector<std::string> > &is_valid);

    QString getOutput_filename() const;
    void setOutput_filename(const QString &value);

    int getContourThickness() const;
    void setContourThickness(int newContourThickness);

public Q_SLOTS:
    virtual void slot_storeNextImage(int legside, int examid);

Q_SIGNALS:
    void signal_thumbnail(QPixmap pix, int legside, int exam_id);

protected:
    Worker_RFSeg(QObject* parent = 0);

    void doWork(ifind::Image::Pointer image);

    QQueue<float> mAreaBuffer;
    int mMaxAreaBufferSize;

    /**
     * @brief mCropBounds
     * x0. y0. width, height
     */
    std::vector<double> mCropBounds;
    std::vector<float> mAratio;
    std::vector<int> mDesiredSize;
    bool mAbsoluteCropBounds;

    QString output_filename;


private:

    /// Python Functions
    py::object PyImageProcessingFunction;
    py::object PyPythonInitializeFunction;
    PyGILState_STATE gstate;
    std::vector< std::vector< ifind::Image::Pointer> > mMeasuredImages;
    std::vector<int> m_store_next_image;

    QPixmap imageToPixmap(ifind::Image::Pointer in, int wp=128, int hp=128);
    QImage imageToQImage(ifind::Image::Pointer in, int layer, bool do_overlay=false);
    int mContourThickness;

};
