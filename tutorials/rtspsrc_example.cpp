/*

ref:
    https://www.freedesktop.org/software/gstreamer-sdk/data/docs/latest/gst-plugins-base-libs-0.10/gst-plugins-base-libs-appsink.html#gst-app-sink-set-callbacks
    https://gstreamer.freedesktop.org/documentation/applib/gstappsink.html?gi-language=c#gst_app_sink_pull_sample

in VLC network URL: rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?

workable command of gstreamer:
```
gst-launch-1.0 rtspsrc location="rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?" ! rtph264depay ! h264parse ! avdec_h264 ! videoscale ! videorate ! videoconvert ! autovideosink

# or
gst-launch-1.0 rtspsrc location="rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?" ! decodebin ! autovideosink
```


Command example: 
gst-launch-1.0 rtspsrc location=rtsp://192.168.50.246 user-id=admin user-pw=admin protocols=4 ! rtph264depay ! h264parse ! avdec_h264 ! videoscale ! videorate ! videoconvert ! autovideosink

gst-launch-1.0 rtspsrc location="rtsp://192.168.0.55:554/user=admin&password=&channel=1&stream=0.sdp?" ! rtph264depay ! h264parse ! avdec_h264 ! videoscale ! videorate ! videoconvert ! autovideosink

gst-launch-1.0 -v rtspsrc location="rtsp://admin:admin@192.168.0.55:554" latency=10 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink

gst-launch-1.0 rtspsrc location=rtsp://192.168.0.55:554/test latency=300 ! decodebin ! autovideosink
*/
#include <cstdio>
#include <string>
#include <string.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>



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
	GstElement* autovideosink;
    GstElement* app_sink;
} CustomData;

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data);

// Callback function
static void onEOS(GstAppSink* sink, void* user_data);
static GstFlowReturn onPreroll(GstAppSink* sink, void* user_data);
static GstFlowReturn onBuffer(GstAppSink* sink, void* user_data);
static GstFlowReturn newBufferList(GstAppSink* sink, gpointer user_data);

void buildPipeline(CustomData* customData, std::string name)
{
    // Create pipeline
    customData->pipeline = gst_pipeline_new(name.c_str());
    // connect to an RTSP server and read Data, strictly follows RGC 2326
    customData->rtspsrc = gst_element_factory_make("rtspsrc","rtspsrc");

    // GstElement* queue = gst_element_factory_make("queue","queue"); //buffer

    // Extract H264 video from RTP packets (RFC3984)
    customData->depay = gst_element_factory_make("rtph264depay","videodepay");

    // Parse H.264 streams 
    customData->parse = gst_element_factory_make("h264parse","h264parse");

    // libav h264 decoder
    customData->video_decode = gst_element_factory_make("avdec_h264","videodecode");

    // "videoscale" is used to resize video frames.
    // by defaut, it will try to negotiate to the same size on the source and sink pad
    customData->videoscale = gst_element_factory_make("videoscale","videoscale");

    // "videorate" is used to adjust frame rate, 
    // by default, it will simpley negotiate the same framerate on  its source and sink pad.
    customData->videorate = gst_element_factory_make("videorate","videorate");

    // "videoconvert" will convert video to a format understood by the video sink.
    customData->videoconvert = gst_element_factory_make("videoconvert","videoconvert");

    // autovideosink will automatically detects an appropriate video sink to use
    customData->autovideosink = gst_element_factory_make("autovideosink","videosink");

    // appsink is ued to extract out decoded image
    customData->app_sink = gst_element_factory_make ("appsink", "app_sink");
}

void setProperties(CustomData* customData)
{
    // Setup
    // location  : 
    // protocols : 4
    // user-id   : 
    // user-pw   :
    g_object_set(G_OBJECT(customData->rtspsrc), "location","192.168.50.246", "protocols", 4,NULL);
    g_object_set(G_OBJECT(customData->rtspsrc), "user-id","admin", "user-pw","admin",NULL);  //設定 rtsp參數 
}



int main(int argc, char *argv[])
{
    CustomData appData;
    GMainLoop* loop=NULL;

    gst_init(&argc, &argv);


    loop = g_main_loop_new(NULL, FALSE);

    // Build pipeline
    buildPipeline(&appData, "rstpRecv");
    
    // Set properties
    setProperties(&appData);
    
    // Add GstElement into pipeline(bin)
    gst_bin_add_many(GST_BIN(appData.pipeline), 
                                appData.rtspsrc, /*queue,*/ 
                                appData.depay, 
                                appData.parse, 
                                appData.video_decode, 
                                appData.videoscale, 
                                appData.videorate, 
                                appData.videoconvert, 
                                appData.autovideosink, 
                                NULL
                            );

    // Link GstElement in pipeline except rtspsrc
    gst_element_link_many(/*queue,*/ appData.depay, 
                            appData.parse, 
                            appData.video_decode, 
                            appData.videoscale, 
                            appData.videorate, 
                            appData.videoconvert, 
                            appData.autovideosink, 
                            NULL
                        );

	/*
	 * Here we have two way to connect the pad of rtspsrc and depay
	 * we call g_signal_connect but apply two different callback function
	 */
	// WAY 1
    g_signal_connect(appData.rtspsrc, "pad-added", G_CALLBACK(on_pad_added), appData.depay); 

    // WAY 2
	// g_signal_connect (appData.rtspsrc, "pad-added", G_CALLBACK (pad_added_handler), &appData);

    GstAppSinkCallbacks cb;
	memset(&cb, 0, sizeof(GstAppSinkCallbacks));
    cb.eos         = onEOS;
	cb.new_preroll = onPreroll;
	cb.new_sample  = onBuffer;
    gst_app_sink_set_callbacks((GstAppSink*)(&(appData.app_sink)), &cb, &appData, NULL);

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
    // printf("%s\n", __func__);
    if(!user_data) {
        return GST_FLOW_OK;
    }

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