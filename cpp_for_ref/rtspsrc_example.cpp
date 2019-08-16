
/*
原型: 

gst-launch-1.0 rtspsrc lolcaiton=rtsp://192.168.50.246 user-id=admin user-pw=admin protocols=4 ! rtph264depay ! h264parse ! avdec_h264 ! videoscale ! videorate ! videoconvert ! autovideosink

gst-launch-1.0 rtspsrc location=rtsp://localhost:8554/test latency=300 ! decodebin ! autovideosink
*/
#include <gst/gst.h>

typedef struct CustomData_ {
	GstElement *pipeline;
	GstElement *rtspsrc;
	GstElement *queue;
	GstElement *depay;
	GstElement *parse;
	GstElement *video_decode;
	GstElement *videoscale;
	GstElement *videorate;
	GstElement *videoconvert;
	GstElement *autovideosink;
} CustomData;

int main(int argc, char *argv[])

{

    GMainLoop* loop=NULL;

    gst_init (&argc, &argv);


    loop = g_main_loop_new(NULL, FALSE);

    GstElement* pipeline = gst_pipeline_new(name); //建立pipeline

    GstElement* rtspsrc = gst_element_factory_make("rtspsrc","rtspsrc"); //rtsp 接收的元件

    // GstElement* queue = gst_element_factory_make("queue","queue"); //buffer

    GstElement* depay = gst_element_factory_make("rtph264depay","videodepay"); // h264在rtp的payload要解開

    GstElement* parse = gst_element_factory_make("h264parse","h264parse"); // parse h264 header

    GstElement* video_decode = gst_element_factory_make("avdec_h264","videodecode"); //解碼

    GstElement* videoscale = gst_element_factory_make("videoscale","videoscale");  //影像寬高調整的element

    GstElement* videorate = gst_element_factory_make("videorate","videorate");  // video 張數的控制

    GstElement* videoconvert = gst_element_factory_make("videoconvert","videoconvert");  // 色彩轉換

    GstElement* autovideosink = gst_element_factory_make("autovideosink","videosink");  // 影像輸出

    // Set properties
    g_object_set(G_OBJECT(rtspsrc), "location","192.168.50.246", "protocols", 4,NULL); //設定 rtsp參數

    g_object_set(G_OBJECT(rtspsrc), "user-id","admin", "user-pw","admin",NULL);  //設定 rtsp參數 

    gst_bin_add_many(GST_BIN(pipeline), rtspsrc, /*queue,*/ depay, parse, video_decode, videoscale, videorate, videoconvert, autovideosink, NULL);

    gst_element_link_many(/*queue,*/ depay, parse, video_decode, videoscale, videorate, videoconvert, autovideosink, NULL);

	/*
	 * Here we have two way to connect the pad of rtspsrc and depay
	 * we call g_signal_connect but apply two different callback function
	 */
	// WAY 1
    // g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(on_pad_added), depay); 

    // WAY 2
	// g_signal_connect (rtspsrc, "pad-added", G_CALLBACK (pad_added_handler), &appData);

	/* Set the pipeline to "playing" state*/
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_main_loop_run(loop);

}


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


static void pad_added_handler (GstElement *src, GstPad *new_pad, myData_t *data) 
{
	// More control of link two pad (comes from tutorial)
	GstPad *sink_pad = gst_element_get_static_pad (data->depayloader, "sink");
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