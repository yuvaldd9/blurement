/**
 * SECTION:element-blurement
 *
 * FIXME:Describe blurement here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! blurement ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstblurement.h"

GST_DEBUG_CATEGORY_STATIC (gst_blurement_debug);
#define GST_CAT_DEFAULT gst_blurement_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
      "video/x-raw, "
      "format = (string)RGB, "
      "width = (int) [ 1, MAX ], "
      "height = (int) [ 1, MAX ], "
      "framerate = (fraction) [ 0/1, MAX ]"
    )    
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
      "video/x-raw, "
      "format = (string)RGB, "
      "width = (int) [ 1, MAX ], "
      "height = (int) [ 1, MAX ], "
      "framerate = (fraction) [ 0/1, MAX ]"
    )    
    );

#define gst_blurement_parent_class parent_class
G_DEFINE_TYPE (GstBlurement, gst_blurement, GST_TYPE_ELEMENT);

GST_ELEMENT_REGISTER_DEFINE (blurement, "blurement", GST_RANK_NONE,
    GST_TYPE_BLUREMENT);

static void gst_blurement_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_blurement_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_blurement_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_blurement_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the blurement's class */
static void
gst_blurement_class_init (GstBlurementClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_blurement_set_property;
  gobject_class->get_property = gst_blurement_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_X,
    g_param_spec_int ("x", "X Position", "X coordinate of blur area",
                      0, G_MAXINT, 0, G_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class, PROP_Y,
    g_param_spec_int ("y", "Y Position", "Y coordinate of blur area",
                      0, G_MAXINT, 0, G_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class, PROP_WIDTH,
    g_param_spec_int ("width", "Width", "Width of blur area",
                      1, G_MAXINT, 32, G_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
    g_param_spec_int ("height", "Height", "Height of blur area",
                      1, G_MAXINT, 32, G_PARAM_READWRITE));
      

  gst_element_class_set_details_simple (gstelement_class,
      "Blurement",
      "FIXME:Generic",
      "FIXME:Generic Template Element", "didi <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void
gst_blurement_init (GstBlurement * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_blurement_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_blurement_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
}

static void
gst_blurement_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBlurement *filter = GST_BLUREMENT (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_X:
      filter->blur_x = g_value_get_int (value);
      break;
    case PROP_Y:
      filter->blur_y = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      filter->blur_width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      filter->blur_height = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_blurement_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBlurement *filter = GST_BLUREMENT (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
      case PROP_X:
      g_value_set_int (value, filter->blur_x);
      break;
    case PROP_Y:
      g_value_set_int (value, filter->blur_y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, filter->blur_width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, filter->blur_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_blurement_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstBlurement *filter;
  gboolean ret;

  filter = GST_BLUREMENT (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
 static GstFlowReturn
 gst_blurement_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
 {
   GstBlurement *filter = GST_BLUREMENT (parent);
 
   GstMapInfo map;
   if (!gst_buffer_map(buf, &map, GST_MAP_READWRITE))
     return GST_FLOW_ERROR;
 
   GstCaps *caps = gst_pad_get_current_caps(filter->sinkpad);
   GstStructure *structure = gst_caps_get_structure(caps, 0);
 
 
   const gchar *format = gst_structure_get_string(structure, "format");
   if (!format || g_strcmp0(format, "RGB") != 0) {
     GST_ERROR_OBJECT(filter, "Unsupported format: %s", format);
     gst_buffer_unmap(buf, &map);
     return GST_FLOW_NOT_NEGOTIATED;
   }
    
   gint frame_width = 0, frame_height = 0;
   gst_structure_get_int(structure, "width", &frame_width);
   gst_structure_get_int(structure, "height", &frame_height);
   gst_caps_unref(caps);
 
   if (!format || g_strcmp0(format, "RGB") != 0) {
     GST_ERROR_OBJECT(filter, "Only RGB format is supported in this implementation");
     gst_buffer_unmap(buf, &map);
     return GST_FLOW_NOT_NEGOTIATED;
   }
 
   gint x = filter->blur_x;
   gint y = filter->blur_y;
   gint w = filter->blur_width;
   gint h = filter->blur_height;
 
   // Clip blur rectangle
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   if (x + w > frame_width) w = frame_width - x;
   if (y + h > frame_height) h = frame_height - y;
 
   if (w <= 0 || h <= 0) {
     gst_buffer_unmap(buf, &map);
     return gst_pad_push(filter->srcpad, buf);  // Nothing to blur
   }
 
   unsigned char *data = map.data;
   int row_stride = frame_width * 3;
 
   unsigned long long sum_r = 0, sum_g = 0, sum_b = 0;
   int pixel_count = w * h;
 
   // Average color over the rectangle
   #pragma omp parallel for reduction(+:sum_r,sum_g,sum_b)
   for (int j = 0; j < h; ++j) {
     for (int i = 0; i < w; ++i) {
       int px = x + i;
       int py = y + j;
       int offset = py * row_stride + px * 3;
 
       sum_r += data[offset + 0];
       sum_g += data[offset + 1];
       sum_b += data[offset + 2];
     }
   }
 
   unsigned char avg_r = sum_r / pixel_count;
   unsigned char avg_g = sum_g / pixel_count;
   unsigned char avg_b = sum_b / pixel_count;
 
   // Set region to average color
   #pragma omp parallel for
   for (int j = 0; j < h; ++j) {
     for (int i = 0; i < w; ++i) {
       int px = x + i;
       int py = y + j;
       int offset = py * row_stride + px * 3;
 
       data[offset + 0] = avg_r;
       data[offset + 1] = avg_g;
       data[offset + 2] = avg_b;
     }
   }
 
   gst_buffer_unmap(buf, &map);
   return gst_pad_push(filter->srcpad, buf);
 }
 


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
blurement_init (GstPlugin * blurement)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template blurement' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_blurement_debug, "blurement",
      0, "Template blurement");

  return GST_ELEMENT_REGISTER (blurement, blurement);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstblurement"
#endif

/* gstreamer looks for this structure to register blurements
 *
 * exchange the string 'Template blurement' with your blurement description
 */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    blurement,
    "blurement",
    blurement_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
