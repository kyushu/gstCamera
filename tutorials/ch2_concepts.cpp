
/*
    pipeline: source -> filter -> sink ->
    source elements: data producers
    sink   elements: data consumers

 */


#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *sink;
    GstElement *filter;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements */
    // new elements can be created with 
    //        gst_element_factory_make(type, name)
    //              type:   videotestsrc
    //                      videoconvert
    //                      autovideosink
    //              name:   if you pass NULL, GStream will provide a unique name
    //                      for you.
    // source = gst_element_factory_make ("uridecodebin", "source");
    source = gst_element_factory_make ("videotestsrc", "source");
    sink = gst_element_factory_make ("autovideosink", "sink");
    filter = gst_element_factory_make("vertigotv", "filter");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !source || !sink || !filter) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    // call gst_bin_add_many(GST_BIN, element, element, ..., NULL)
    // to add the elements to the pipeline, this function accepts a list
    // of elements to be added, ending with NULL.
    gst_bin_add_many (GST_BIN (pipeline), source, sink, NULL);

    // These elements are not linked with each other yet. 
    // we need to use gst_element_link() to link them together
    // first parameters is the sourec, and the second one the destination.
    // link must be established following the data flow(source element to sink element).
    if (gst_element_link (source, sink) != TRUE) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }
    // if (gst_element_link (source, filter) != TRUE) {
    //     g_printerr ("Elements could not be linked.\n");
    //     gst_object_unref (pipeline);
    //     return -1;
    // }
    // if (gst_element_link (filter, sink) != TRUE) {
    //     g_printerr ("Elements could not be linked.\n");
    //     gst_object_unref (pipeline);
    //     return -1;
    // }

    /* Modify the source's properties */
    // g_object_set() accepts a NULL-terminated list of property-name, 
    // property-value pairs, so multiple properties can be changed in one go
    // this line of code set pattern = 0, to display pattern 0
    // if you set the value to 1, it will display different content(pattern)
    g_object_set (source, "pattern", 0, NULL);
    // g_object_set (source, "pattern", 1, NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    // the program wait here until the video is end or error occurred.
    bus = gst_element_get_bus (pipeline);
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

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
                break;
            case GST_MESSAGE_EOS:
                g_print ("End-Of-Stream reached.\n");
                break;
            default:
                /* We should not reach here because we only asked for ERRORs and EOS */
                g_printerr ("Unexpected message received.\n");
                break;
        }
        gst_message_unref (msg);
    }

    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}