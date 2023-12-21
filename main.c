#include <gst/gst.h>
#include <stdio.h>

typedef struct _CustomData {
    GstElement *playbin;
    gboolean
        playing,
        terminate,
        seek_enabled,
        seek_done;
    gint64 duration;
} CustomData;

static void handle_message(CustomData*,GstMessage*);

int main(int argc, char const *argv[]){
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    data.playing = FALSE;
    data.terminate = FALSE;
    data.seek_done = FALSE;
    data.seek_enabled = FALSE;
    data.duration = GST_CLOCK_TIME_NONE;

    gst_init(&argc,&argv);

    /* Create the elements */
    data.playbin = gst_element_factory_make("playbin","playbin");

    if (!data.playbin){
        g_printerr("Could not create playbin.\n");
        return -1;
    }

    /* Set URI */
    gchar uri[1<<10];
    g_print("Enter uri for playing:");
    fgets(uri,sizeof(uri),stdin);
    g_object_set(data.playbin,"uri",uri,NULL);

    ret = gst_element_set_state(data.playbin,GST_STATE_PLAYING);
    if (ret = GST_STATE_CHANGE_FAILURE){
        g_printerr("Unable to set pipeline to palying state.\n");
        gst_object_unref(data.playbin);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(data.playbin);
    do {
        msg = gst_bus_timed_pop_filtered(
            bus,
            GST_CLOCK_TIME_NONE,
            GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_DURATION
        );

        if (msg!=NULL){
            handle_message(&data,msg);
        } else {
            if (data.playing) {
                gint64 current = -1;

                /* Query the current position of the stream. */
                if (!gst_element_query_position(data.playbin,GST_FORMAT_TIME,&current)){
                    g_printerr("Could not query current position.\n");
                }

                /* If we don't know it yet, query the stream duration */
                if (!GST_CLOCK_TIME_IS_VALID(data.duration)){
                    if (!gst_element_query_duration(data.playbin,GST_FORMAT_TIME,&data.duration)){
                        g_printerr("Could not query duration.\n");
                    }
                }

                g_print(
                    "Position %"GST_TIME_FORMAT" / %"GST_TIME_FORMAT"\r",
                    GST_TIME_ARGS(current),
                    GST_TIME_ARGS(data.duration)
                );

                if (data.seek_enabled && !data.seek_done && current>10*GST_SECOND){
                    g_print("\nReached 10s, performing seek...\n");
                    gst_element_seek_simple(
                        data.playbin,
                        GST_FORMAT_TIME,
                        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT ,
                        30*GST_SECOND
                    );
                    data.seek_done = TRUE;
                }
            }
        }
    } while (!data.terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(data.playbin,GST_STATE_NULL);
    g_object_unref(data.playbin);

    return 0;
}

static void handle_message(CustomData *data, GstMessage *msg){
    //TODO
}
