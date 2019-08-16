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

inline bool createElement(std::string factoryName, std::string name, GstElement** gstElm) 
{
    *gstElm = gst_element_factory_make(factoryName.c_str(), name.c_str());
    if(!gstElm) {
        g_printerr ("%s could not be created.\n", name.c_str());
        return false;
    }
    return true;
}

typedef struct CustomData_ {
	GstElement* pipeline;
	GstElement* rtspsrc;
	GstElement* queue;
	GstElement* depay;
	GstElement* parse;
	GstElement* video_decode;
	GstElement* videoscale;
	GstElement* videorate;
	GstElement* videoconvert;
    GstElement* tee;
    GstElement* play_queue;
    GstElement* data_queue;
	GstElement* autovideosink;
    GstElement* decodebin;
    GstElement* app_sink;

    GstPad *tee_play_pad, *tee_data_pad;
    GstPad *queue_play_pad, *queue_data_pad;
    GstPad *video_raw_pad;
} CustomData;

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data);

// Callback function
static void onEOS(GstAppSink* sink, void* user_data);
static GstFlowReturn onPreroll(GstAppSink* sink, void* user_data);
static GstFlowReturn onBuffer(GstAppSink* sink, void* user_data);
static GstFlowReturn newBufferList(GstAppSink* sink, gpointer user_data);


bool buildPipeline(CustomData* customData, std::string name)
{
    // Create pipeline
    customData->pipeline = gst_pipeline_new(name.c_str());
    if(!customData->pipeline) { 
        g_printerr ("pipelin could not be created.\n");
        return false;
    }
    // connect to an RTSP server and read Data, strictly follows RGC 2326
    if(!createElement("rtspsrc", "rtspsrc", &(customData->rtspsrc)) ) { return false;}
    // GstElement* queue = gst_element_factory_make("queue","queue"); //buffer

    // Extract H264 video from RTP packets (RFC3984)
    if(!createElement("rtph264depay", "videodepay", &(customData->depay)) ) { return false;}

    // Parse H.264 streams 
    if(!createElement("h264parse", "h264parse", &(customData->parse)) ) { return false;}

    // libav h264 decoder
    if(!createElement("avdec_h264", "videodecode", &(customData->video_decode)) ) { return false;}

    // "videoscale" is used to resize video frames.
    // by defaut, it will try to negotiate to the same size on the source and sink pad
    if(!createElement("videoscale", "videoscale", &(customData->videoscale)) ) { return false;}

    // "videorate" is used to adjust frame rate, 
    // by default, it will simpley negotiate the same framerate on  its source and sink pad.
    if(!createElement("videorate", "videorate", &(customData->videorate)) ) { return false;}


    // tee for multiplex
    if(!createElement("tee", "tee", &(customData->tee)) ) { return false;}

    // Path 1
    // 
    // queue for playback real time video
    if(!createElement("queue", "play_queue", &(customData->play_queue)) ) { return false;}

    // "videoconvert" will convert video to a format understood by the video sink.
    if(!createElement("videoconvert", "videoconvert", &(customData->videoconvert)) ) { return false;}

    // autovideosink will automatically detects an appropriate video sink to use
    if(!createElement("autovideosink", "videosink", &(customData->autovideosink)) ) { return false;}

    // Path2
    //
    // queue for app sink data
    if(!createElement("queue", "data_queue", &(customData->data_queue)) ) { return false;}

    // appsink is ued to extract out decoded image
    if(!createElement("appsink", "app_sink", &(customData->app_sink)) ) { return false;}

    return true;
}

void setProperties(CustomData* customData)
{
    // Setup
    // location  : 
    // protocols : 4
    // user-id   : 
    // user-pw   :
    g_object_set(G_OBJECT(customData->rtspsrc), "location","rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?",NULL);
    // g_object_set(G_OBJECT(customData->rtspsrc), "user-id","admin", "user-pw","admin",NULL);  //設定 rtsp參數 
}



int main(int argc, char *argv[])
{
    CustomData appData;
    GMainLoop* loop=NULL;

    gst_init(&argc, &argv);


    loop = g_main_loop_new(NULL, FALSE);

    // Build pipeline
    if( !buildPipeline(&appData, "rstpRecv") ) {
        gst_object_unref (appData.pipeline);
        return -1;
    }
    
    // Set properties
    setProperties(&appData);
    
    // Add GstElement into pipeline(bin)
    gst_bin_add_many(GST_BIN(appData.pipeline), 
                                appData.rtspsrc, /*queue,*/ 
                                appData.depay, 
                                appData.parse, 
                                appData.video_decode, 
                                // appData.videoscale, 
                                // appData.videorate, 
                                appData.tee,
                                appData.play_queue,
                                appData.data_queue,
                                appData.videoconvert, 
                                appData.autovideosink, 
                                // appData.video_raw,
                                appData.app_sink,
                                NULL
                            );

    // Link GstElement in pipeline except rtspsrc
    gst_element_link_many(/*queue,*/ appData.depay, 
                            appData.parse, 
                            appData.video_decode, 
                            // appData.videoscale, 
                            // appData.videorate, 
                            appData.tee,
                            // appData.autovideosink, 
                            // appData.video_raw,
                            // appData.app_sink,
                            NULL
                        );
    if (gst_element_link_many(appData.play_queue, appData.videoconvert, appData.autovideosink, NULL) != true )
    {
        g_printerr ("Play Elements could not be linked.\n");
        gst_object_unref (appData.pipeline);
        return -1;
    }
    if (gst_element_link_many(appData.data_queue, appData.app_sink, NULL) != true )
    {
        g_printerr ("Data Elements could not be linked.\n");
        gst_object_unref (appData.pipeline);
        return -1;
    }

    appData.tee_play_pad = gst_element_get_request_pad (appData.tee, "src_%u");
    appData.queue_play_pad = gst_element_get_static_pad (appData.play_queue, "sink");
    appData.tee_data_pad = gst_element_get_request_pad (appData.tee, "src_%u");
    appData.queue_data_pad = gst_element_get_static_pad (appData.data_queue, "sink");
    if (gst_pad_link (appData.tee_play_pad, appData.queue_play_pad) != GST_PAD_LINK_OK ||
        gst_pad_link (appData.tee_data_pad, appData.queue_data_pad) != GST_PAD_LINK_OK) {
        g_printerr ("Tee could not be linked.\n");
        gst_object_unref (appData.pipeline);
        return -1;
    }

	/*
	 * Here we have two way to connect the pad of rtspsrc and depay
	 * we call g_signal_connect but apply two different callback function
	 */
	// WAY 1
    g_signal_connect(appData.rtspsrc, "pad-added", G_CALLBACK(on_pad_added), appData.depay); 

    // WAY 2
	// g_signal_connect (appData.rtspsrc, "pad-added", G_CALLBACK (pad_added_handler), &appData);

    // app_sin callback
    GstAppSinkCallbacks cb;
	memset(&cb, 0, sizeof(GstAppSinkCallbacks));
    cb.eos         = onEOS;
	cb.new_preroll = onPreroll;
	cb.new_sample  = onBuffer;
    gst_app_sink_set_callbacks((GstAppSink*)(appData.app_sink), &cb, &appData, NULL);


	/* Set the pipeline to "playing" state*/
    gst_element_set_state(appData.pipeline, GST_STATE_PLAYING);

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

// Callback Function 1
static void on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
	// Link two Element with named pad
	GstPad *sink_pad = gst_element_get_static_pad (GST_ELEMENT(data), "sink");
	if(gst_pad_is_linked(sink_pad)) {
		g_print("rtspsrc and depay are already linked. Ignoring\n");
		return;
	}
    gst_element_link_pads(element, gst_pad_get_name(pad), GST_ELEMENT(data), "sink");
}


// Callback Function 2
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data)
{
	// More control of link two pad (comes from tutorial)
	GstPad *sink_pad = gst_element_get_static_pad (data->depay, "sink");
	GstPadLinkReturn ret;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

	/* If our converter is already linked, we have nothing to do here */
	if (gst_pad_is_linked (sink_pad)) {
		g_print (" Sink pad from %s already linked. Ignoring.\n", GST_ELEMENT_NAME (src));
		goto exit;
	}

	/* Check the new pad's type */
	new_pad_caps = gst_pad_get_current_caps (new_pad);
	new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
	new_pad_type = gst_structure_get_name (new_pad_struct);
	/* Check the new pad's name */
	if (!g_str_has_prefix (new_pad_type, "application/x-rtp")) {
		g_print ("  It is not the right pad.  Need recv_rtp_src_. Ignoring.\n");
		goto exit;
	}

	/* Attempt the link */
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
		g_print ("  Type is '%s' but link failed.\n", new_pad_type);
	} else {
		g_print ("  Link succeeded (type '%s').\n", new_pad_type);
	}

exit:
	/* Unreference the new pad's caps, if we got them */
	if (new_pad_caps != NULL)
		gst_caps_unref (new_pad_caps);

	/* Unreference the sink pad */
	gst_object_unref (sink_pad);
}