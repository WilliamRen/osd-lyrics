/*
 * Copyright (C) 2009  Tiger Soldier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include "config.h"
#include "ol_osd_window.h"
#include "ol_lrc_parser.h"
#include "ol_lrc_utility.h"
#include "ol_player.h"
#include "ol_utils.h"
#include "ol_lrc_fetch.h"
#include "ol_trayicon.h"
#include "ol_intl.h"
#include "ol_config.h"
#include "ol_osd_module.h"
#include "ol_keybindings.h"

#define REFRESH_INTERVAL 100
#define MAX_PATH_LEN 1024
static const gchar *LRC_PATH = ".lyrics";

static OlPlayerController *controller = NULL;
static OlMusicInfo music_info = {0};
static gchar *previous_title = NULL;
static gchar *previous_artist = NULL;
static gint previous_duration = 0;
static gint previous_position = -1;
static LrcQueue *lrc_file = NULL;
static OlOsdModule *module = NULL;

static void ensure_lyric_dir ();
static gint refresh_music_info (gpointer data);
static void check_music_change (int time);
static void change_music ();
/** 
 * @brief duplicates the string and replace all invalid characters to `_' 
 * 
 * @param str string needs to be replaced
 * 
 * @return replaced string, should be free with g_free
 */
static char* replace_invalid_str (const char *str);
static gboolean is_file_exist (const char *filename);
/** 
 * Gets a music's full path filename
 * 
 * @param music_info The info of the music
 * @param pathname the returned full path filename
 */
void get_lyric_path_name (OlMusicInfo *music_info, char *pathname);
gboolean download_lyric (OlMusicInfo *music_info);

/** 
 * @brief Gets the real lyric of the given lyric
 * A REAL lyric is the nearest lyric to the given lyric, whose text is not empty
 * If the given lyric text is not empty, the given lyric is a real lyric
 * If not real lyric available, returns NULL
 * @param lrc An LrcInfo
 * 
 * @return The real lyric of the lrc. returns NULL if not available
 */
LrcInfo* get_real_lyric (LrcInfo *lrc);

static char*
replace_invalid_str (const char *str)
{
  if (str == NULL)
    return NULL;
  char *ret = g_strdup (str);
  char *rep = ret;
  while ((rep = strchr (rep, '/')) != NULL)
  {
    *rep = '_';
  }
  return ret;
}

static void
ensure_lyric_dir ()
{
  char *pathname = ol_path_alloc ();
  const char *home_dir = g_get_home_dir ();
  if (previous_artist == NULL)
  {
    sprintf (pathname, "%s/%s/", home_dir, LRC_PATH);
  }
  g_mkdir_with_parents (pathname, 0755);
  free (pathname);
}

void
get_lyric_path_name (OlMusicInfo *music_info, char *pathname)
{
  if (pathname == NULL)
    return;
  const char *home_dir;
  if (lrc_file != NULL)
  {
    free (lrc_file);
    lrc_file = NULL;
  }
  if (previous_title == NULL)
    return;
  home_dir = g_get_home_dir ();
  char *title = replace_invalid_str (previous_title);
  if (previous_artist == NULL)
  {
    sprintf (pathname, "%s/%s/%s.lrc", home_dir, LRC_PATH, title);
  }
  else
  {
    char *artist = replace_invalid_str (previous_artist);
    sprintf (pathname, "%s/%s/%s-%s.lrc", home_dir, LRC_PATH, artist, title);
    g_free (artist);
  }
  g_free(title);
  printf ("lrc file name:%s\n", pathname);
}

gboolean download_lyric (OlMusicInfo *music_info)
{
  int lrc_count;
  OlConfig *config = ol_config_get_instance ();
  char *name = ol_config_get_string (config, "Download", "download-engine");
  fprintf (stderr, "Download engine: %s\n", name);
  OlLrcFetchEngine *engine = ol_lrc_fetch_get_engine (name);
  if (engine == NULL)
    return FALSE;
  /* g_free (name); */
  OlLrcCandidate *candidates = engine->search (music_info, &lrc_count, "UTF-8");
  printf ("downloading...\n");
  if (lrc_count == 0 || candidates == NULL)
  {
    printf ("download failed\n");
    return FALSE;
  }
  else
  {
    char pathname[MAX_PATH_LEN];
    get_lyric_path_name (music_info, pathname);
    engine->download (&candidates[0], pathname, "UTF-8");
    printf ("download %s success\n", pathname);
    return TRUE;
  }
}

gboolean
is_file_exist (const char *filename)
{
  if (filename == NULL)
    return FALSE;
  struct stat buf;
  printf ("stat:%d\n", stat (filename, &buf));
  return stat (filename, &buf) == 0;
}

void
change_music ()
{
  printf ("%s\n",
          __FUNCTION__);
  gchar file_name[MAX_PATH_LEN];
  get_lyric_path_name (&music_info, file_name);
  if (module != NULL)
  {
    ol_osd_module_set_music_info (module, &music_info);
    ol_osd_module_set_duration (module, previous_duration);
  }
  ol_osd_module_set_lrc (module, NULL);
  if (!is_file_exist (file_name))
  {
    if (!download_lyric (&music_info) || !is_file_exist (file_name))
    return;
  }
  lrc_file = ol_lrc_parser_get_lyric_info (file_name);
  ol_osd_module_set_lrc (module, lrc_file);
}

void
check_music_change (int time)
{
  /* fprintf (stderr, "%s\n", __FUNCTION__); */
  /* checks whether the music has been changed */
  gboolean changed = FALSE;
  gboolean stop = FALSE;
  /* fprintf (stderr, "%d-%d\n", previous_position, time); */
  if (previous_position >=0 && time >= previous_position &&
      previous_title != NULL)
    return;
  /* compares the previous title with current title */
  if (controller && !controller->get_music_info (&music_info))
  {
    controller = NULL;
  }
  guint duration = 0;
  if (controller && !controller->get_music_length (&duration))
  {
    controller = NULL;
  }
  if (music_info.title == NULL)
  {
    if (previous_title != NULL)
    {
      g_free (previous_title);
      previous_title = NULL;
    }
    stop = TRUE;
  }
  else if (previous_title == NULL)
  {
    changed = TRUE;
    previous_title = g_strdup (music_info.title);
  }
  else if (strcmp (previous_title, music_info.title) != 0)
  {
    changed = TRUE;
    g_free (previous_title);
    previous_title = g_strdup (music_info.title);
  }
  /* compares the previous artist with current  */
  if (music_info.artist == NULL)
  {
    if (previous_artist != NULL)
    {
      g_free (previous_artist);
      previous_artist = NULL;
      changed = TRUE;
    }
  }
  else if (previous_artist == NULL)
  {
    changed = TRUE;
    previous_artist = g_strdup (music_info.artist);
  }
  else if (strcmp (previous_artist, music_info.artist) != 0)
  {
    changed = TRUE;
    g_free (previous_artist);
    previous_artist = g_strdup (music_info.artist);
  }
  /* compares the previous duration */
  /* FIXME: because the a of banshee, some lyrics may return different
     duration for the same song when plays to different position, so the
     comparison is commented out temporarily */
  /* if (previous_duration != duration) */
  /* { */
  /*   printf ("change6:%d-%d\n", previous_duration, duration); */
  /*   changed = TRUE; */
    previous_duration = duration;
  /* } */
  if (stop)
  {
    /* if (osd != NULL && GTK_WIDGET_MAPPED (osd)) */
    /*   gtk_widget_hide (GTK_WIDGET (osd)); */
    /* return; */
  }
  if (changed)
  {
    change_music ();
  }
}

gint
refresh_music_info (gpointer data)
{
  /* fprintf (stderr, "%s\n", __FUNCTION__); */
  if (controller == NULL)
  {
    controller = ol_player_get_active_player ();
  }
  guint time = 0;
  if (controller && !controller->get_played_time (&time))
  {
    controller = NULL;
  }
  check_music_change (time);
  previous_position = time;
  if (controller == NULL)
  {
    previous_position = -1;
    return TRUE;
  }
  ol_osd_module_set_played_time (module, time);
  return TRUE;
}

int
main (int argc, char **argv)
{

#if ENABLE_NLS
  /* Set the text message domain.  */
  printf ("initializing gettext: " PACKAGE " at " LOCALEDIR "\n");
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  /* textdomain (PACKAGE); */
  printf ("%s\n", _("_Lock"));
#endif
  
  gtk_init (&argc, &argv);
  ensure_lyric_dir ();
  ol_player_init ();
  module = ol_osd_module_new ();
  ol_trayicon_inital ();
  ol_keybinding_init ();
  ol_lrc_fetch_init ();
  ol_get_string_from_hash_table (NULL, NULL);
  g_timeout_add (REFRESH_INTERVAL, refresh_music_info, NULL);
  gtk_main ();
  ol_player_free ();
  ol_osd_module_destroy (module);
  module = NULL;
  return 0;
}
