typedef struct myDataTag {
    GstElement *pipeline;
    GstElement *rtspsrc;
    GstElement *depayloader;
    GstElement *decoder;
    *sink;
} myData_t;

myData_t appData;

appData->pipeline  = gst_pipeline_new ("videoclient");
appData->rtspsrc = gst_element_factory_make ("rtspsrc", "rtspsrc");
g_object_set (G_OBJECT (appData->rtspsrc), "location", "rtsp://192.168.1.10:554/myStreamPath", NULL);
appData->depayloader = gst_element_factory_make ("rtph264depay","depayloader");
appData->decoder = gst_element_factory_make ("h264dec", "decoder");
appData->sink = gst_element_factory_make ("autovideosink", "sink");

//then add all elements together
gst_bin_add_many (GST_BIN (appData->pipeline), appData->rtspsrc, appData->depayloader, appData->decoder, appData->sink, NULL);

//link everythink after source
gst_element_link_many (appData->depayloader, appData->decoder, appData->sink, NULL);

/*
 * Connect to the pad-added signal for the rtpbin.  This allows us to link
 * the dynamic RTP source pad to the depayloader when it is created.
 */
g_signal_connect (appData->rtspsrc, "pad-added", G_CALLBACK (pad_added_handler), &appData);

/* Set the pipeline to "playing" state*/
gst_element_set_state (appData->pipeline, GST_STATE_PLAYING);

/* pad added handler */
static void pad_added_handler (GstElement *src, GstPad *new_pad, myData_t *pThis) {
GstPad *sink_pad = gst_element_get_static_pad (pThis->depayloader, "sink");
GstPadLinkReturn ret;
GstCaps *new_pad_caps = NULL;
GstStructure *new_pad_struct = NULL;
const gchar *new_pad_type = NULL;

g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

/* Check the new pad's name */
if (!g_str_has_prefix (GST_PAD_NAME (new_pad), "recv_rtp_src_")) {
    g_print ("  It is not the right pad.  Need recv_rtp_src_. Ignoring.\n");
    goto exit;
}

/* If our converter is already linked, we have nothing to do here */
if (gst_pad_is_linked (sink_pad)) {
    g_print (" Sink pad from %s already linked. Ignoring.\n", GST_ELEMENT_NAME (src));
    goto exit;
}

/* Check the new pad's type */
new_pad_caps = gst_pad_get_caps (new_pad);
new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
new_pad_type = gst_structure_get_name (new_pad_struct);

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