#include <stdio.h>
#include <gst/gst.h>

int main (int argc, char *argv[])
{
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Build the pipeline */
    pipeline =
        gst_parse_launch
        ("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
        NULL);

    /* Start playing */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);

    // program will hang here until 
    //  1. ERROR occurs
    //  2. End Of Source
    // occurs    
    msg =
        // gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        // GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Free resources */
    if (msg != NULL) {
        printf("no msg\n"); 
        gst_message_unref (msg); 
    }
    printf("msg\n");

    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
return 0;
}