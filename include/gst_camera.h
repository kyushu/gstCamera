
#ifndef _GST_CAMERA_
#define _GST_CAMERA_

#include <mutex>
#include <condition_variable>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "gst_camera_param.h"
#include "mt_utils.h"

static const int GST_CAMERA_RING_BUFFER_SIZE = 16;

class GstCamera
{
public:
    GstCamera();
    GstCamera(GstCameraParam params);
    ~GstCamera();

    bool Init(GstCameraParam params);
    bool Open();
    void Capture();

private:
    bool initGstCheck();
    std::string searchAppsinkName(std::string launchStr);
    void checkBusMsg();
    void checkFrameBuffer();

    // Callback function
    static void onEOS(GstAppSink* sink, void* user_data);
    static GstFlowReturn onPreroll(GstAppSink* sink, void* user_data);
    static GstFlowReturn onBuffer(GstAppSink* sink, void* user_data);

    GstBus* bus_;
    GstAppSink* appsink_;
    GstElement* pipeline_;
    std::string launchStr_;

    int width_;
    int height_;
    int depth_;
    int frameSize_;
    
    void* ringBufferCPU_[GST_CAMERA_RING_BUFFER_SIZE];
    void* ringBufferGPU_[GST_CAMERA_RING_BUFFER_SIZE];

    std::condition_variable waitEvent_;
    std::mutex waitMutex_;
    std::mutex ringMutex_;

    u_int32_t latestRingBuffer_;
    bool latestRetrived_;

    // test count
    unsigned long frame_count;
};

#endif // _GST_CAMERA_