/*
    Pads allow information to enter and leave an element. The Capabilities (or Caps, for short) of a Pad, then, specify what kind of information can travel through the Pad

                                                                -----------------------------   -----------------------------
                                                                |GstElemeent(audio-decoder) |   |GstElemeent(audio-sink)    |
                                                                |---------         ---------|   |---------                  |  
                                                                | GstPad |         | GstPad |-->| GstPad |                  |  
                              -------------------------    ---->| (sink) |         | (src)  |   | (sink) |                  |  
 --------------------------   | GstElement(demuxer)   |    |    |---------         ---------|   |---------                  |  
 | GstElement(source)     |   |              ---------|    |    |                           |   |                           |   
 |                        |   |              |GstPad  |-----    -----------------------------   -----------------------------   
 |                --------|   |---------     |(src_01)|                                                                     
 |                |GstPad |-->| GstPad |     -------- |                                                                     
 |                |(src)  |   | (sink) |     ---------|                                                                        
 |                --------|   |---------     |GstPad  |-----    -----------------------------   -----------------------------   
 |                        |   |              |(src_02)|    |    |GstElemeent(video-decoder) |   |GstElemeent(video-sink)    |   
 |                        |   |              ---------|    |    |---------         ---------|   |---------                  |   
 --------------------------   -------------------------    ---->| GstPad |         | GstPad |-->| GstPad |                  |   
                                                                | (sink) |         | (src)  |   | (sink) |                  |   
                                                                |---------         ---------|   |---------                  |   
                                                                |                           |   |                           |   
                                                                -----------------------------   -----------------------------   


The main complexity when dealing with demuxers is that they cannot produce any information until they have received some data and have a chance to look at the container to see what is inside. This is, demuxers start with no source pads to which other elements can link, and thus the pipeline must necessarily teminate at them.

The solution is to build the pipeline from the source down to the demuxer, and set it to run(PLAY). **When demuxer has received enough information to know about the number and kind of streams in the container, it will start creating source pads**. This is the right time for us to finish building the pipeline and attach it to the newly added demuxer demuxer pads.


This example is used to demonstration link to  

 */

#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline;
    GstElement *source;
    GstElement *convert;
    GstElement *sink;
} CustomData;

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) {
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements
     * 
     * gst_element_factory_make(type, name) is a shortcut 
     *  for `gst_element_factory_find()` + `gst_element_factory_create()` 
     */
    data.source = gst_element_factory_make ("uridecodebin", "source");
    data.convert = gst_element_factory_make ("audioconvert", "convert");
    data.sink = gst_element_factory_make ("autoaudiosink", "sink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("test-pipeline");

    if (!data.pipeline || !data.source || !data.convert || !data.sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline. Note that we are NOT linking the source at this
     *   point. We will do it later. 
     * 
     * At this moment, "urldecodebin" GstElement(data.source) has no Pads to begin with
     *  because the program do not start to access video data, The src Pad will be 
     *  added after pipeline Start "PLAYING" 
     * So Now, demuxer provide nothing because source element(data.source) does not have 
     *   enough information(even does not have src Pad)
     * 
     * At this case, we call src Pad of GstElement "urldecodebin" is "Sometimes Pads"
     */
    gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.convert , data.sink, NULL);
    // Link convert and sink only
    if (!gst_element_link (data.convert, data.sink)) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    /* Set the URI to play */
    g_object_set (data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

    /* Connect to the pad-added signal 
     *
     * g_signal_connect is used to notify(execute the callback function) when the registered event has happened here, 
     *  we register "pad-added" event for executing "pad_added_handler" callback function.
     * 
     * because we connect(or register) to "pad-added" signal(or event) and 
     *  pad_added_callback(GstElement* self, GstPad* new_pad, gpoint* user_Data) will be called when signal is emitted
     *  
     * and we cast our callback function "pad_added_handler" to G_CALLBACK and let the pointer of "pad_added_callback"
     *  point to "pad_added_handler"
     * 
     * NOTE: you have to figure the argument of Callback function of the signal you want to connect
     *          and implement your custom function that can match it. 
     */
    
    g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);



    /* Start playing 
     *
     * After set pipeline to PLAY(GST_STATE_PLAYING), the source element(data.source) is reading data from the source(ex. file, url)
     * 
     * When our source element(data.source) has(read) enough information to start producing data, it will create source pads
     *  (that means "pad-added" event is triggered and "pad_added_handler" will be called.)
     */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus (data.pipeline);
    do {
        msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
            (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS) );

        /* Parse message */
        if (msg != NULL) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    /* We are only interested in state-changed messages from the pipeline */
                    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                    g_print ("Pipeline state changed from %s to %s:\n",
                        gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                    }
                    break;
                default:
                    /* We should not reach here */
                    g_printerr ("Unexpected message received.\n");
                    break;
            }
            gst_message_unref (msg);
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (data.pipeline);
    return 0;
}


/* This function will be called by the "pad-added" signal */
// The first parameter of a signal handler is always the object that has triggered it.
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) 
{
    GstPad *sink_pad = gst_element_get_static_pad (data->convert, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
        g_print ("We are already linked. Ignoring.\n");
        goto exit;
    }

    /* Check the new pad's type */
    // In this example, we are only interested in pads producing audio 
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
        g_print ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
        goto exit;
    }

    /* Attempt the link 
     * gst_pad_link(GstPad, GstPad): link two pads in the same bin(pipeline)
     * 
     * new_pad  : src pad of data.source
     * sink_pad : sink pad of data.convert 
     */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } 
    else {
        g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL){ gst_caps_unref (new_pad_caps); }

    /* Unreference the sink pad */
    gst_object_unref (sink_pad);
}