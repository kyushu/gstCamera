// gint
main (gint   argc,
      gchar *argv[])
{
    GstElement *pipeline, *videosrc, *conv,*enc, *pay, *udp;

    // init GStreamer
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);

    // setup pipeline
    pipeline = gst_pipeline_new ("pipeline");

    videosrc = gst_element_factory_make ("videotestsrc", "source");

    conv = gst_element_factory_make ("videoconvert", "conv");

    enc = gst_element_factory_make("x264enc", "enc");

    pay = gst_element_factory_make("rtph264pay", "pay");
    g_object_set(G_OBJECT(pay), "config-interval", 1, NULL);

    udp = gst_element_factory_make("udpsink", "udp");
    g_object_set(G_OBJECT(udp), "host", "127.0.0.1", NULL);
    g_object_set(G_OBJECT(udp), "port", "5000", NULL);

    gst_bin_add_many (GST_BIN (pipeline), videosrc, conv, enc, pay, udp, NULL);

    if (gst_element_link_many (videosrc, conv, enc, pay, udp, NULL) != TRUE)
    {
        return -1;
    }

    // play
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    g_main_loop_run (loop);

    // clean up
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));
    g_main_loop_unref (loop);

    return 0;
}