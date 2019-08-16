#include "gst_camera_param.h"
#include "gst_camera.h"

#include <cstring> // memset
#include <algorithm>

#include "cudaMappedMemory.h"

GstCamera::GstCamera(): width_{0}, height_{0}, depth_{0}
{

}
GstCamera::GstCamera(GstCameraParam params): width_{0}, height_{0}, depth_{0}
{
    Init(params);
}

GstCamera::~GstCamera()
{

}

inline bool launchGstreamer(GstElement** pipeline, std::string launchStr)
{

    GError* err = NULL;
    *pipeline = gst_parse_launch(launchStr.c_str(), &err);
    if( !(*pipeline) ) {

		g_print ("Error for launch: %s\n", err->message);
		g_error_free (err);
        
		gst_object_unref (*pipeline);
        return false;
    }
    return true;
}


bool GstCamera::initGstCheck()
{
    frame_count = 0; // For test

    GError* err;
    if( !gst_init_check(0, NULL, &err) ) {
        g_print ("failed to initialize gstreamer library with gst_init(): %s\n", err->message);
		g_error_free (err);
        return false;
    }
    uint32_t ver[] = {0, 0, 0, 0};
    gst_version(&ver[0], &ver[1], &ver[2], &ver[3]);
    printf("Initialized gstreamer, Ver %u.%u.%u.%u\n", ver[0], ver[1], ver[2], ver[3]);

    gst_debug_remove_log_function(gst_debug_log_default);
	
	if( true )
	{
		// gst_debug_add_log_function(rilog_debug_function, NULL, NULL);
		gst_debug_set_active(true);
		gst_debug_set_colored(false);
	}
    return true;
}

std::string GstCamera::searchAppsinkName(std::string launchStr)
{
    std::string target = "";
    std::vector<std::string> elems = mtsai::utils::split(launchStr, '!');
    for(std::string &elm : elems) {
        std::vector<std::string> elm_names = mtsai::utils::split(elm, ' ');
        std::vector<std::string>::iterator itr = std::find_if(elm_names.begin(), elm_names.end(), [](std::string a){
            a.erase(remove_if(a.begin(), a.end(), isspace), a.end());
            return (a == "appsink");
        });
        if(itr != elm_names.end()) {
            target = elm_names.back();
            target.erase(remove_if(target.begin(), target.end(), isspace), target.end());
            target = mtsai::utils::split(target, '=').back();
            break;
        }
    }
    return target;
}

bool GstCamera::Init(GstCameraParam params)
{
    if(!initGstCheck()) {
        return false;
    }

    for(uint32_t i=0; i<GST_CAMERA_RING_BUFFER_SIZE; ++i) {
        ringBufferCPU_[i] = nullptr;
        ringBufferGPU_[i] = nullptr;

    }

    launchStr_ = params.launchStr_;
    printf("launch string: %s\n", launchStr_.c_str());

    // Search appsink name and must be "mysink"
    std::string appsink_name= searchAppsinkName(launchStr_);
    printf("appsink_name: %s\n", appsink_name.c_str());
    if(appsink_name != "mysink") {
        printf("app sink name is not mysink\n");
        return false;
    }
    
    // Launch Gstreamer with command string
    if( !launchGstreamer(&pipeline_, launchStr_) ) {
        return false;
    }

    bus_ = gst_pipeline_get_bus( GST_PIPELINE(pipeline_) );

    GstElement* appsinkElement = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
	appsink_ = GST_APP_SINK(appsinkElement);

    // app_sin callback
    GstAppSinkCallbacks cb;
	memset(&cb, 0, sizeof(GstAppSinkCallbacks));
    cb.eos         = onEOS;
	cb.new_preroll = onPreroll;
	cb.new_sample  = onBuffer;
    gst_app_sink_set_callbacks(appsink_, &cb, (void*)this, NULL);
}

bool GstCamera::Open()
{
    const GstStateChangeReturn result = gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    if(result == GST_STATE_CHANGE_ASYNC) {
        // no idea how to handle
    }
    else if(result != GST_STATE_CHANGE_SUCCESS) {
        printf("gstreamer failed to set pipeline state to PLAYING (error %u)\n", result);
		return false;
    }

    return true;

}
void GstCamera::Capture()
{

}

void GstCamera::checkFrameBuffer()
{
    printf("\t%ld\n", frame_count );
    frame_count += 1;

    GstSample* gstSample = gst_app_sink_pull_sample(appsink_);
    if(!gstSample) {
        printf("gstreamer camera -- gst_app_sink_pull_sample() returned NULL...\n");
		return;
    }
    GstBuffer* gstBuffer = gst_sample_get_buffer(gstSample);
	
	if( !gstBuffer )
	{
		printf("gstreamer camera -- gst_sample_get_buffer() returned NULL...\n");
		return;
	}
    // retrieve
	GstMapInfo map; 

	if(	!gst_buffer_map(gstBuffer, &map, GST_MAP_READ) ) 
	{
		printf("gstreamer camera -- gst_buffer_map() failed...\n");
		return;
	}

	void* gstData = map.data; //GST_BUFFER_DATA(gstBuffer);
	const uint32_t gstSize = map.size; //GST_BUFFER_SIZE(gstBuffer);
	if( !gstData )
	{
		printf("gstreamer camera -- gst_buffer had NULL data pointer...\n");
        gst_sample_unref(gstSample);
		return;
	}
    // retrieve caps
	GstCaps* gstCaps = gst_sample_get_caps(gstSample);
	
	if( !gstCaps )
	{
		printf("gstreamer camera -- gst_buffer had NULL caps...\n");
		gst_sample_unref(gstSample);
		return;
	}
    GstStructure* gstCapsStruct = gst_caps_get_structure(gstCaps, 0);
	
	if( !gstCapsStruct )
	{
		printf("gstreamer camera -- gst_caps had NULL structure...\n");
		gst_sample_unref(gstSample);
		return;
	}

    // get width & height of the buffer
	int width  = 0;
	int height = 0;
	
	if( !gst_structure_get_int(gstCapsStruct, "width", &width) ||
		!gst_structure_get_int(gstCapsStruct, "height", &height) )
	{
		printf("gstreamer camera -- gst_caps missing width/height...\n");
		gst_sample_unref(gstSample);
		return;
	}

    if( width < 1 || height < 1 ) {
        printf("gstreamer camera -- width < 1  or height < 1...\n");
        gst_sample_unref(gstSample);
		return;
    }
	width_  = width;
	height_ = height;
	depth_  = (gstSize * 8) / (width_ * height_);
	frameSize_   = gstSize;
    printf("width : %d\n", width_);
    printf("height: %d\n", height_);
    printf("depth : %d\n", depth_);
    printf("size  : %d\n", frameSize_);
    
    // make sure ringbuffer is allocated
	if( !ringBufferCPU_[0] )
	{
		for( uint32_t n=0; n < GST_CAMERA_RING_BUFFER_SIZE; n++ )
		{
			if( !cudaAllocMapped(&ringBufferCPU_[n], &ringBufferGPU_[n], gstSize) ) {
                printf(LOG_CUDA "gstreamer camera -- failed to allocate ringbuffer %u  (size=%u)\n", n, gstSize);
            }
		}
		
		printf(LOG_CUDA "gstreamer camera -- allocated %u ringbuffers, %u bytes each\n", GST_CAMERA_RING_BUFFER_SIZE, gstSize);
	}
	
	// copy to next ringbuffer
	const uint32_t nextRingbuffer = (latestRingBuffer_ + 1) % GST_CAMERA_RING_BUFFER_SIZE;		
	//printf(LOG_GSTREAMER "gstreamer camera -- using ringbuffer #%u for next frame\n", nextRingbuffer);
	memcpy(ringBufferCPU_[nextRingbuffer], gstData, frameSize_);
    gst_buffer_unmap(gstBuffer, &map); 
	//gst_buffer_unref(gstBuffer);
    gst_sample_unref(gstSample);
	
	
	// update and signal sleeping threads
	// Step1. Lock for update the latest index of RingBuffer and Retrived flag
    std::unique_lock<std::mutex> lkRing(ringMutex_);
	latestRingBuffer_ = nextRingbuffer;
	latestRetrived_  = false;

    // Step2. unlock after updating
    lkRing.unlock();
    waitEvent_.notify_all();
}


void GstCamera::onEOS(GstAppSink* sink, void* user_data)
{
    printf("gst rtsp onEOS\n");
}

GstFlowReturn GstCamera::onPreroll(GstAppSink* sink, void* user_data)
{
    printf("gst rtsp onPreroll\n");
    return GST_FLOW_OK;
}

GstFlowReturn GstCamera::onBuffer(GstAppSink* sink, void* user_data)
{
    printf("%s\n", __func__);
    if(!user_data) {
        return GST_FLOW_ERROR;
    }

    GstCamera *dec = (GstCamera *)user_data;
    dec->checkFrameBuffer();
    dec->checkBusMsg();
    
    return GST_FLOW_OK;
}


// checkMsgBus
void GstCamera::checkBusMsg()
{
	while(true)
	{
		GstMessage* msg = gst_bus_pop(bus_);
        if( !msg ) {
			break;
		}
        
        switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_ERROR:
            {
                GError *err = NULL;
                gchar *debug_info = NULL;
                gst_message_parse_error (msg, &err, &debug_info);
                
                g_printerr ("BUS Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                g_printerr ("gstreamer Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error (&err);
                g_free (debug_info);
                break;
            }
            case GST_MESSAGE_EOS:
            {
                printf("gstreamer %s recieved EOS signal...\n", GST_OBJECT_NAME(msg->src));
                //g_main_loop_quit (app->loop);		// TODO trigger plugin Close() upon error
                break;
            }
            case GST_MESSAGE_STATE_CHANGED:
            {
                GstState old_state, new_state;
        
                gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
                
                printf("gstreamer changed state from %s to %s ==> %s\n",
                                gst_element_state_get_name(old_state),
                                gst_element_state_get_name(new_state),
                                GST_OBJECT_NAME(msg->src));
                break;
            }
            case GST_MESSAGE_STREAM_STATUS:
            {
                GstStreamStatusType streamStatus;
                gst_message_parse_stream_status(msg, &streamStatus, NULL);
                
                std::string statusStr = "";
                switch(streamStatus)
                {
                    case GST_STREAM_STATUS_TYPE_CREATE:	    statusStr = "CREATE";
                    case GST_STREAM_STATUS_TYPE_ENTER:		statusStr = "ENTER";
                    case GST_STREAM_STATUS_TYPE_LEAVE:		statusStr = "LEAVE";
                    case GST_STREAM_STATUS_TYPE_DESTROY:	statusStr = "DESTROY";
                    case GST_STREAM_STATUS_TYPE_START:		statusStr = "START";
                    case GST_STREAM_STATUS_TYPE_PAUSE:		statusStr = "PAUSE";
                    case GST_STREAM_STATUS_TYPE_STOP:		statusStr = "STOP";
                    default:						        statusStr = "UNKNOWN";
                }

                printf("gstreamer stream status %s ==> %s\n",
                                statusStr.c_str(), 
                                GST_OBJECT_NAME(msg->src));
                break;
            }
            case GST_MESSAGE_TAG: 
            {
                GstTagList *tags = NULL;

                gst_message_parse_tag(msg, &tags);
                printf("gstreamer %s missing gst_tag_list_to_string()\n", GST_OBJECT_NAME(msg->src));
                if( tags != NULL ) {
                    gst_tag_list_free(tags);
                }
                    
                break;
            }
            default:
            {
                printf("gstreamer msg %s ==> %s\n", gst_message_type_get_name(GST_MESSAGE_TYPE(msg)), GST_OBJECT_NAME(msg->src));
                break;
            }
        }
        
		gst_message_unref (msg);
	}
}