#include <assert.h>
#include <stdio.h>
#include <glib-object.h>
#include "config.h"
#include "ol_config.h"
#include "ol_config_property.h"
#include "ol_utils.h"
#include "ol_marshal.h"
#include "ol_debug.h"

#define OL_CONFIG_GET_PRIVATE(obj)   (G_TYPE_INSTANCE_GET_PRIVATE \
                                      ((obj),                     \
                                       OL_TYPE_CONFIG,            \
                                       OlConfigPrivate))
const char CONFIG_FILE_NAME[] = PACKAGE_NAME ".conf";
G_DEFINE_TYPE (OlConfig, ol_config, G_TYPE_OBJECT);

static void ol_config_despose (GObject *obj);
static void ol_config_finalize (GObject *object);
static void ol_config_emit_change (OlConfig *config,
                                   const char *group,
                                   const char *name);
static void ol_config_do_change (OlConfig *config);
static OlConfig* ol_config_new ();
static const char* ol_config_get_path ();

static OlConfig* instance = NULL;

typedef struct _OlConfigPrivate OlConfigPrivate;

struct _OlConfigPrivate
{
  GKeyFile *config;
};

static void
ol_config_init (OlConfig *self)
{
  OlConfigPrivate *priv = OL_CONFIG_GET_PRIVATE (self);
  priv->config = g_key_file_new ();
  if (!g_key_file_load_from_file (priv->config, ol_config_get_path (),
                                  G_KEY_FILE_KEEP_COMMENTS, NULL))
  {
  }
  int i;
  for (i = 0; i < ol_get_array_len (config_bool); i++)
  {
    if (!g_key_file_has_key (priv->config, config_bool[i].group, config_bool[i].name, NULL))
    {
      g_key_file_set_boolean (priv->config, config_bool[i].group, config_bool[i].name,
                              config_bool[i].default_value);
    }
  }
  for (i = 0; i < ol_get_array_len (config_int); i++)
  {
    if (!g_key_file_has_key (priv->config, config_int[i].group, config_int[i].name, NULL))
    {
      g_key_file_set_integer (priv->config, config_int[i].group, config_int[i].name,
                              config_int[i].default_value);
    }
  }
  for (i = 0; i < ol_get_array_len (config_double); i++)
  {
    if (!g_key_file_has_key (priv->config, config_double[i].group, config_double[i].name, NULL))
    {
      g_key_file_set_double (priv->config, config_double[i].group, config_double[i].name,
                              config_double[i].default_value);
    }
  }
  for (i = 0; i < ol_get_array_len (config_str); i++)
  {
    if (!g_key_file_has_key (priv->config, config_str[i].group, config_str[i].name, NULL))
    {
      g_key_file_set_string (priv->config, config_str[i].group, config_str[i].name,
                              config_str[i].default_value);
    }
  }
  for (i = 0; i < ol_get_array_len (config_str_list); i++)
  {
    ol_debugf ("%s\n", config_str_list[i].name); 
    if (!g_key_file_has_key (priv->config, config_str_list[i].group, config_str_list[i].name, NULL))
    {
      int len = 0;
      if (config_str_list[i].len > 0)
      {
        len = config_str_list[i].len;
      }
      else
      {
        while (config_str_list[i].default_value[len] != NULL)
          len++;
      }
      ol_debugf ("name:%s len:%d\n", config_str_list[i].name, len);
      g_key_file_set_string_list (priv->config,
                                  config_str_list[i].group,
                                  config_str_list[i].name,
                                  config_str_list[i].default_value,
                                  len);
    }
  }
}

static void
ol_config_class_init (OlConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *ol_param_spec = NULL;
  
  ol_config_parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private (klass, sizeof (OlConfigPrivate));
  
  /* gobject_class->set_property = ol_config_set_property; */
  /* gobject_class->get_property = ol_config_get_property; */
  gobject_class->dispose = ol_config_despose;
  gobject_class->finalize = ol_config_finalize;
  /* initialize properties */
  int i;
  /* for (i = 0; i < ol_get_array_len (config_bool); i++) */
  /* { */
  /*   ol_param_spec = g_param_spec_boolean (config_bool[i].name, */
  /*                                         config_bool[i].nick, */
  /*                                         config_bool[i].description, */
  /*                                         config_bool[i].default_value, */
  /*                                         G_PARAM_READWRITE); */
  /*   g_object_class_install_property (gobject_class, */
  /*                                    config_bool[i].key, */
  /*                                    ol_param_spec); */
  /* } */
  /* for (i = 0; i < ol_get_array_len (config_int); i++) */
  /* { */
  /*   ol_param_spec = g_param_spec_int (config_int[i].name, */
  /*                                     config_int[i].nick, */
  /*                                     config_int[i].description, */
  /*                                     config_int[i].min, */
  /*                                     config_int[i].max, */
  /*                                     config_int[i].default_value, */
  /*                                     G_PARAM_READWRITE); */
  /*   g_object_class_install_property (gobject_class, */
  /*                                    config_int[i].key, */
  /*                                    ol_param_spec); */
  /* } */
  /* for (i = 0; i < ol_get_array_len (config_double); i++) */
  /* { */
  /*   ol_param_spec = g_param_spec_double (config_double[i].name, */
  /*                                        config_double[i].nick, */
  /*                                        config_double[i].description, */
  /*                                        config_double[i].min, */
  /*                                        config_double[i].max, */
  /*                                        config_double[i].default_value, */
  /*                                        G_PARAM_READWRITE); */
  /*   g_object_class_install_property (gobject_class, */
  /*                                    config_double[i].key, */
  /*                                    ol_param_spec); */
  /* } */
  /* for (i = 0; i < ol_get_array_len (config_str); i++) */
  /* { */
  /*   ol_param_spec = g_param_spec_string (config_str[i].name, */
  /*                                        config_str[i].nick, */
  /*                                        config_str[i].description, */
  /*                                        config_str[i].default_value, */
  /*                                        G_PARAM_READWRITE); */
  /*   g_object_class_install_property (gobject_class, */
  /*                                    config_str[i].key, */
  /*                                    ol_param_spec); */
  /* } */
  /* for (i = 0; i < ol_get_array_len (config_str_list); i++) */
  /* { */
  /*   ol_param_spec = g_param_spec_boxed (config_str_list[i].name, */
  /*                                       config_str_list[i].nick, */
  /*                                       config_str_list[i].description, */
  /*                                       G_TYPE_STRV, */
  /*                                       G_PARAM_READWRITE); */
  /*   g_object_class_install_property (gobject_class, */
  /*                                    config_str_list[i].key, */
  /*                                    ol_param_spec); */
  /* } */
  /* initialize singals */
  GType signal_type[2];
  /* signal_type[0] = OL_TYPE_CONFIG; */
  signal_type[0] = G_TYPE_STRING;
  signal_type[1] = G_TYPE_STRING;
  klass->signals[CHANGED] =
    g_signal_newv ("changed",
                   G_TYPE_FROM_CLASS (gobject_class),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   NULL /* closure */,
                   NULL /* accumulator */,
                   NULL /* accumulator data */,
                   ol_marshal_VOID__STRING_STRING,
                   G_TYPE_NONE /* return_type */,
                   2     /* n_params */,
                   signal_type  /* param_types */);
  ol_debugf ("id of changed signal is: %d\n", klass->signals[CHANGED]);
}

static void
ol_config_despose (GObject *obj)
{
  OlConfig *self = OL_CONFIG (obj);
  OlConfigPrivate *priv = OL_CONFIG_GET_PRIVATE (obj);
  if (priv->config != NULL)
  {
    /* g_hash_table_destroy (priv->config); */
    g_key_file_free (priv->config);
    priv->config = NULL;
  }
  G_OBJECT_CLASS (ol_config_parent_class)->dispose (obj);
}

static void
ol_config_finalize (GObject *object)
{
  G_OBJECT_CLASS (ol_config_parent_class)->finalize (object);
}

static void
ol_config_emit_change (OlConfig *config,
                       const char *group,
                       const char *name)
{
  GValue params[3] = {0};
  g_value_init (&params[0], G_OBJECT_TYPE (config));
  g_value_set_object (&params[0], G_OBJECT (config));
  g_value_init (&params[1], G_TYPE_STRING);
  g_value_set_string (&params[1], g_strdup (group));
  g_value_init (&params[2], G_TYPE_STRING);
  g_value_set_string (&params[2], g_strdup (name));
  g_signal_emitv (params, OL_CONFIG_GET_CLASS (config)->signals[CHANGED],
                  0, NULL);
}
 
static OlConfig*
ol_config_new ()
{
  return g_object_new (OL_TYPE_CONFIG, NULL);
}

OlConfig*
ol_config_get_instance ()
{
  if (instance == NULL)
    instance = ol_config_new ();
  return instance;
}

gboolean
ol_config_set_bool (OlConfig *config, const char *group, const char *name, gboolean value)
{
  ol_assert_ret (config != NULL, FALSE);
  ol_assert_ret (name != NULL, FALSE);
  g_key_file_set_boolean (OL_CONFIG_GET_PRIVATE (config)->config, group, name, value);
  ol_config_emit_change (config, group, name);
  ol_config_save (config);
}

gboolean
ol_config_set_int (OlConfig *config, const char *group, const char *name, int value)
{
  ol_assert_ret (config != NULL, FALSE);
  ol_assert_ret (name != NULL && group != NULL, FALSE);
  g_key_file_set_integer (OL_CONFIG_GET_PRIVATE (config)->config, group, name, value);
  ol_config_emit_change (config, group, name);
  ol_config_save (config);
}

gboolean
ol_config_set_double (OlConfig *config, const char *group, const char *name, double value)
{
  ol_assert_ret (config != NULL, FALSE);
  ol_assert_ret (name != NULL, FALSE);
  g_key_file_set_double (OL_CONFIG_GET_PRIVATE (config)->config, group, name, value);
  ol_config_emit_change (config, group, name);
  ol_config_save (config);
}

gboolean
ol_config_set_string (OlConfig *config, const char *group, const char *name, const char* value)
{
  ol_assert_ret (config != NULL, FALSE);
  ol_assert_ret (name != NULL, FALSE);
  g_key_file_set_string (OL_CONFIG_GET_PRIVATE (config)->config, group, name, value);
  ol_config_emit_change (config, group, name);
  ol_config_save (config);
}

gboolean
ol_config_set_str_list (OlConfig *config,
                        const char *group,
                        const char *name,
                        const char **value,
                        int len)
{
  ol_assert_ret (config != NULL, FALSE);
  ol_assert_ret (name != NULL, FALSE);
  g_key_file_set_string_list (OL_CONFIG_GET_PRIVATE (config)->config, group, name, value, len);
  ol_config_emit_change (config, group, name);
  ol_config_save (config);
}

gboolean
ol_config_get_bool (OlConfig *config, const char *group, const char *name)
{
  ol_assert_ret (config != NULL, 0);
  ol_assert_ret (name != NULL, 0);
  gboolean value = g_key_file_get_boolean (OL_CONFIG_GET_PRIVATE (config)->config,
                                           group,
                                           name,
                                           NULL);
  ol_debugf ("[%s]%s:%d\n", group, name, value);
  return value;
}

int
ol_config_get_int (OlConfig *config, const char *group, const char *name)
{
  ol_assert_ret (config != NULL, 0);
  ol_assert_ret (name != NULL, 0);
  gint value = g_key_file_get_integer (OL_CONFIG_GET_PRIVATE (config)->config,
                                       group,
                                       name,
                                       NULL);
  ol_debugf ("[%s]%s:%d\n", group, name, value);
  return value;
}

double
ol_config_get_double (OlConfig *config, const char *group, const char *name)
{
  ol_assert_ret (config != NULL, 0.0);
  ol_assert_ret (name != NULL, 0.0);
  double value = g_key_file_get_double (OL_CONFIG_GET_PRIVATE (config)->config,
                                        group,
                                        name,
                                        NULL);
  ol_debugf ("[%s]%s:%lf\n", group, name, value);
  return value;
}

char*
ol_config_get_string (OlConfig *config, const char *group, const char *name)
{
  ol_assert_ret (config != NULL, NULL);
  ol_assert_ret (name != NULL, NULL);
  char *value = g_key_file_get_string (OL_CONFIG_GET_PRIVATE (config)->config,
                                        group,
                                        name,
                                        NULL);
  ol_debugf ("[%s]%s:%s\n", group, name, value);
  return value;
}

char**
ol_config_get_str_list (OlConfig *config,
                        const char *group,
                        const char *name,
                        int *len)
{
  ol_assert_ret (config != NULL, NULL);
  ol_assert_ret (name != NULL, NULL);
  char **value = g_key_file_get_string_list (OL_CONFIG_GET_PRIVATE (config)->config,
                                             group,
                                             name,
                                             len,
                                             NULL);
  return value;
}

const char*
ol_config_get_path ()
{
  static char* path = NULL;
  if (path == NULL)
  {
    path = g_strdup_printf ("%s/%s/%s", g_get_user_config_dir (), PACKAGE_NAME, CONFIG_FILE_NAME);
    ol_debugf ("config path: %s\n", path);
    char *dir = g_strdup_printf ("%s/%s/", g_get_user_config_dir (), PACKAGE_NAME);
    g_mkdir_with_parents (dir, 0755);
    g_free (dir);
  }
  return path;
}

void ol_config_save (OlConfig *config)
{
  OlConfigPrivate *priv = OL_CONFIG_GET_PRIVATE (config);
  gsize len;
  char *file_content = g_key_file_to_data (priv->config, &len, NULL);
  g_file_set_contents (ol_config_get_path (), file_content, len, NULL);
}

