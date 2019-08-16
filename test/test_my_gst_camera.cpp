
#include "gst_camera.h"
#include "gst_camera_param.h"

int main(int argc, char const *argv[])
{
    
    GstCameraParam params;
    params.launchStr_ = "rtspsrc location=\"rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?\" ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink";

    // Constructing and initializing GstCamera
    GstCamera gstCamera{params};

    if( !gstCamera.Open() ) {
        printf("failed to open gstcamera\n");
    }

    while(true) {
        gstCamera.Capture();
    }

    return 0;
}
