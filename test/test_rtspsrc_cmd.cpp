/*
in VLC network URL: rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?

workable command of gstreamer:
gst-launch-1.0 rtspsrc location="rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?" ! rtph264depay ! h264parse ! avdec_h264 ! videoscale ! videorate ! videoconvert ! autovideosink

*/
#include <cstdio>
#include <string>
#include <string.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

// Callback function
static void onEOS(GstAppSink* sink, void* user_data);
static GstFlowReturn onPreroll(GstAppSink* sink, void* user_data);
static GstFlowReturn onBuffer(GstAppSink* sink, void* user_data);
static GstFlowReturn newBufferList(GstAppSink* sink, gpointer user_data);

struct CustomApp
{
public:
    CustomApp(): count{0} {};
    ~CustomApp(){};
    long int count;

};


int main(int argc, char *argv[])
{

    GMainLoop* loop=NULL;

    gst_init(&argc, &argv);

    CustomApp myApp{};

    loop = g_main_loop_new(NULL, FALSE);

    // Build pipeline
    // std::string launchStr = "rtspsrc location=\"rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?\" ! rtph264depay ! h264parse ! avdec_h264 ! videoscale ! videorate ! video/x-raw format=RGB ! appsink name=mysink";
    // std::string launchStr = "rtspsrc location=\"rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?\" ! decodebin ! video/x-raw format=RGB ! appsink name=mysink";

    // std::string launchStr = "rtspsrc location=\"rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?\" ! rtph264depay ! h264parse ! avdec_h264 ! appsink name=mysink";

    // image frame is converted to 24 bit(3 byte)
    // std::string launchStr = "rtspsrc location=\"rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?\" ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink";

	std::string launchStr = "rtspsrc location=\"rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?\" ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink";
    
    GError* err = NULL;
    GstElement* pipeline = gst_parse_launch(launchStr.c_str(), &err);
    if( !pipeline ) {

		g_print ("Error: %s\n", err->message);
		g_error_free (err);
        
		gst_object_unref (pipeline);
        return -1;
    }
    GstElement* appsinkElement = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
	GstAppSink* app_sink = GST_APP_SINK(appsinkElement);

    // app_sin callback
    GstAppSinkCallbacks cb;
	memset(&cb, 0, sizeof(GstAppSinkCallbacks));
    cb.eos         = onEOS;
	cb.new_preroll = onPreroll;
	cb.new_sample  = onBuffer;
    gst_app_sink_set_callbacks((GstAppSink*)(app_sink), &cb, &myApp, NULL);


	/* Set the pipeline to "playing" state*/
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_main_loop_run(loop);

}


void onEOS(GstAppSink* sink, void* user_data)
{
    printf("gst rtsp onEOS\n");
}
GstFlowReturn onPreroll(GstAppSink* sink, void* user_data)
{
    printf("gst rtsp onPreroll\n");
    return GST_FLOW_OK;
}
GstFlowReturn onBuffer(GstAppSink* sink, void* user_data)
{
    printf("%s\n", __func__);
    if(!user_data) {
        return GST_FLOW_ERROR;
    }

    printf("\t%ld\n", ((CustomApp *)(user_data))->count );
    ((CustomApp *)(user_data))->count += 1;

    GstSample* gstSample = gst_app_sink_pull_sample(sink);
    if(!gstSample) {
        printf("gstreamer camera -- gst_app_sink_pull_sample() returned NULL...\n");
		return GST_FLOW_ERROR;
    }
    GstBuffer* gstBuffer = gst_sample_get_buffer(gstSample);
	
	if( !gstBuffer )
	{
		printf("gstreamer camera -- gst_sample_get_buffer() returned NULL...\n");
		return GST_FLOW_ERROR;
	}
    // retrieve
	GstMapInfo map; 

	if(	!gst_buffer_map(gstBuffer, &map, GST_MAP_READ) ) 
	{
		printf("gstreamer camera -- gst_buffer_map() failed...\n");
		return GST_FLOW_ERROR;
	}

	void* gstData = map.data; //GST_BUFFER_DATA(gstBuffer);
	const uint32_t gstSize = map.size; //GST_BUFFER_SIZE(gstBuffer);
	if( !gstData )
	{
		printf("gstreamer camera -- gst_buffer had NULL data pointer...\n");
        gst_sample_unref(gstSample);
		return GST_FLOW_ERROR;
	}
    // retrieve caps
	GstCaps* gstCaps = gst_sample_get_caps(gstSample);
	
	if( !gstCaps )
	{
		printf("gstreamer camera -- gst_buffer had NULL caps...\n");
		gst_sample_unref(gstSample);
		return GST_FLOW_ERROR;
	}
    GstStructure* gstCapsStruct = gst_caps_get_structure(gstCaps, 0);
	
	if( !gstCapsStruct )
	{
		printf("gstreamer camera -- gst_caps had NULL structure...\n");
		gst_sample_unref(gstSample);
		return GST_FLOW_ERROR;
	}

    // get width & height of the buffer
	int width  = 0;
	int height = 0;
	
	if( !gst_structure_get_int(gstCapsStruct, "width", &width) ||
		!gst_structure_get_int(gstCapsStruct, "height", &height) )
	{
		printf("gstreamer camera -- gst_caps missing width/height...\n");
		gst_sample_unref(gstSample);
		return GST_FLOW_ERROR;
	}

    if( width < 1 || height < 1 ) {
        printf("gstreamer camera -- width < 1  or height < 1...\n");
        gst_sample_unref(gstSample);
		return GST_FLOW_ERROR;
    }
	int mWidth  = width;
	int mHeight = height;
	int mDepth  = (gstSize * 8) / (width * height);
	int mSize   = gstSize;
    printf("width : %d\n", mWidth);
    printf("height: %d\n", mHeight);
    printf("depth : %d\n", mDepth);
    printf("size  : %d\n", mSize);
    // // make sure ringbuffer is allocated
	// if( !mRingbufferCPU[0] )
	// {
	// 	for( uint32_t n=0; n < NUM_RINGBUFFERS; n++ )
	// 	{
	// 		if( !cudaAllocMapped(&mRingbufferCPU[n], &mRingbufferGPU[n], gstSize) )
	// 			printf(LOG_CUDA "gstreamer camera -- failed to allocate ringbuffer %u  (size=%u)\n", n, gstSize);
	// 	}
		
	// 	printf(LOG_CUDA "gstreamer camera -- allocated %u ringbuffers, %u bytes each\n", NUM_RINGBUFFERS, gstSize);
	// }
	
	// // copy to next ringbuffer
	// const uint32_t nextRingbuffer = (mLatestRingbuffer + 1) % NUM_RINGBUFFERS;		
	
	// //printf(LOG_GSTREAMER "gstreamer camera -- using ringbuffer #%u for next frame\n", nextRingbuffer);
	// memcpy(mRingbufferCPU[nextRingbuffer], gstData, gstSize);
	// gst_buffer_unmap(gstBuffer, &map); 
	// //gst_buffer_unref(gstBuffer);
	// gst_sample_unref(gstSample);
	
	
	// // update and signal sleeping threads
	// // mRingMutex->lock();
	// std::unique_lock<std::mutex> lkRing(mRingMutex);
	// mLatestRingbuffer = nextRingbuffer;
	// mLatestRetrieved  = false;




    return GST_FLOW_OK;
}
// GstFlowReturn newBufferList(GstAppSink* sink, gpointer user_data)
// {
//     printf("gst rtsp onEOS\n");
// }
