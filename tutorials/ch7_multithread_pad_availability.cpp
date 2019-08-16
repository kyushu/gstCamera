
/*  
                                    -------------  --------------  ---------------  ---------
                                    |a-queue    |  |a-convert   |  |a-resample   |  |a-sink |
                                --->|sink    src|->|sink     src|->|sink      src|->|sink   |
--------------  -----------    |    |           |  |            |  |             |  |       |
|audio-src   |  |tee   src|----|    -------------  --------------  ---------------  ---------
|         src|->|sink     |
|            |  |      src|----|    -------------  --------------  ---------------  ---------
--------------  -----------    |    |v-queue    |  |visual      |  |v-convert    |  |v-sink |
                                --->|sink    src|->|sink     src|->|sink      src|->|sink   |
                                    |           |  |            |  |             |  |       |
                                    -------------  --------------  ---------------  ---------
                                    




*/


#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *audio_source, *tee, *audio_queue, *audio_convert, *audio_resample, *audio_sink;
    GstElement *video_queue, *visual, *video_convert, *video_sink;
    GstBus *bus;
    GstMessage *msg;
    GstPad *tee_audio_pad, *tee_video_pad;
    GstPad *queue_audio_pad, *queue_video_pad;

    /* 
     * Initialize GStreamer 
     */
    gst_init (&argc, &argv);

    /*
     * Create the elements 
     */
    audio_source = gst_element_factory_make ("audiotestsrc", "audio_source");
    tee = gst_element_factory_make ("tee", "tee");
    audio_queue = gst_element_factory_make ("queue", "audio_queue");
    audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
    audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
    audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
    video_queue = gst_element_factory_make ("queue", "video_queue");
    
    // wavescope: consumes an audio signale and renders a waveform(oscilloscope)
    visual = gst_element_factory_make ("wavescope", "visual");
    
    video_convert = gst_element_factory_make ("videoconvert", "csp");
    video_sink = gst_element_factory_make ("autovideosink", "video_sink");

    /* 
     * Create the empty pipeline
     */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !audio_source || !tee || !audio_queue || !audio_convert || !audio_resample || !audio_sink ||
        !video_queue || !visual || !video_convert || !video_sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* 
     * Configure elements 
     */
    // set property "frequency" of audio_source(audiotestsrc GstElement) is 215.0 hz
    g_object_set (audio_source, "freq", 215.0f, NULL);
    // set properties "shader" and style of visual(wavescope GstElement)
    g_object_set (visual, "shader", 0, "style", 1, NULL);

    
    /* 
     * Add all elements into bin(pipeline) 
     */ 
    gst_bin_add_many (GST_BIN (pipeline), audio_source, tee, audio_queue, audio_convert, audio_resample, audio_sink,
        video_queue, visual, video_convert, video_sink, NULL);

    /* 
     * Link all elements that can be automatically linked because they have "Always" pads 
     */  
    if (gst_element_link_many (audio_source, tee, NULL) != TRUE ||
        gst_element_link_many (audio_queue, audio_convert, audio_resample, audio_sink, NULL) != TRUE ||
        gst_element_link_many (video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
    return -1;
    }

    /* 
     * Manually link the Tee, which has "Request" pads 
     * 
     *  For Request Pads, you have to link them manually
     */
    tee_audio_pad = gst_element_get_request_pad (tee, "src_%u");
    g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
    queue_audio_pad = gst_element_get_static_pad (audio_queue, "sink");
    tee_video_pad = gst_element_get_request_pad (tee, "src_%u");
    g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
    queue_video_pad = gst_element_get_static_pad (video_queue, "sink");
    if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
        gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
        g_printerr ("Tee could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }
    gst_object_unref (queue_audio_pad);
    gst_object_unref (queue_video_pad);

    /* Start playing the pipeline */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS) );

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad (tee, tee_audio_pad);
    gst_element_release_request_pad (tee, tee_video_pad);
    gst_object_unref (tee_audio_pad);
    gst_object_unref (tee_video_pad);

    /* Free resources */
    if (msg != NULL) {
        gst_message_unref (msg);
    }
    
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);

    gst_object_unref (pipeline);
    return 0;
}