/**
 * @file   ol_player_guayadeque02.h
 * @author Sarlmol Apple <sarlmolapple@gmail.com>
 * @date   Fri Aug  20 10:56:12 2010
 * 
 * @brief  Supports guayadeque0.2.5
 * 
 * 
 */
#include <dbus/dbus-glib.h>
#include "ol_player_guayadeque02.h"
#include "ol_utils.h"
#include "ol_utils_dbus.h"
#include "ol_player_mpris.h"
#include "ol_elapse_emulator.h"
#include "ol_debug.h"

static const char *SERVICE = "org.mpris.guayadeque";
static const char *icon_paths[] = {
  "/usr/share/app-install/icons/guayadeque.png",
  "/usr/share/pixmaps/guayadeque.png"
};

static OlPlayerMpris *mpris = NULL;
//static OlElapseEmulator *elapse_emulator = NULL;

static OlPlayerMpris* ol_player_guayadeque02_get_mpris ();

static gboolean ol_player_guayadeque02_get_music_info (OlMusicInfo *info);
static gboolean ol_player_guayadeque02_get_played_time (int *played_time);
static gboolean ol_player_guayadeque02_get_music_length (int *len);
static gboolean ol_player_guayadeque02_get_activated ();
static enum OlPlayerStatus ol_player_guayadeque02_get_status ();
static int ol_player_guayadeque02_get_capacity ();
static gboolean ol_player_guayadeque02_play ();
static gboolean ol_player_guayadeque02_pause ();
static gboolean ol_player_guayadeque02_stop ();
static gboolean ol_player_guayadeque02_prev ();
static gboolean ol_player_guayadeque02_next ();
static gboolean ol_player_guayadeque02_seek (int pos_ms);
//static void ol_player_guayadeque02_ensure_elapse (int elapsed_time);
static const char *ol_player_guayadeque02_get_icon_path ();
static const char *_get_icon_path (void);

/*static void
ol_player_guayadeque02_ensure_elapse (int elapsed_time)
{
  if (elapse_emulator == NULL)
  {
    elapse_emulator = g_new (OlElapseEmulator, 1);
    if (elapse_emulator != NULL)
      ol_elapse_emulator_init (elapse_emulator, elapsed_time, 1000);
  }
  }*/

static OlPlayerMpris*
ol_player_guayadeque02_get_mpris ()
{
  if (mpris == NULL)
  {
    mpris = ol_player_mpris_new (SERVICE);
  }
  return mpris;
}

static gboolean
ol_player_guayadeque02_get_music_info (OlMusicInfo *info)
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_get_music_info (mpris, info);
}

static gboolean
ol_player_guayadeque02_get_played_time (int *played_time)
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_get_played_time (mpris, played_time);
  /*int amarok_time = 0;
  if (!ol_player_mpris_get_played_time (mpris, &amarok_time))
    return FALSE;
  ol_player_guayadeque02_ensure_elapse (amarok_time);

  if (0)
  {
    enum OlPlayerStatus status = ol_player_guayadeque02_get_status ();
    if (status == OL_PLAYER_PLAYING)
      *played_time = ol_elapse_emulator_get_real_ms (elapse_emulator, amarok_time);
    else
      *played_time = ol_elapse_emulator_get_last_ms (elapse_emulator, amarok_time);
  }
  else
  {
    *played_time = ol_elapse_emulator_get_real_ms (elapse_emulator, amarok_time);
    }
    return TRUE;*/
}

static gboolean
ol_player_guayadeque02_get_music_length (int *len)
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_get_music_length (mpris, len);
}

static gboolean
ol_player_guayadeque02_get_activated ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_get_activated (mpris);
}

static enum OlPlayerStatus
ol_player_guayadeque02_get_status ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_get_status (mpris);
}

static int
ol_player_guayadeque02_get_capacity ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_get_capacity (mpris);
}

static gboolean
ol_player_guayadeque02_play ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_play (mpris);
}

static gboolean
ol_player_guayadeque02_pause ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_pause (mpris);
}

static gboolean
ol_player_guayadeque02_stop ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_stop (mpris);
}

static gboolean
ol_player_guayadeque02_prev ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_prev (mpris);
}

static gboolean
ol_player_guayadeque02_next ()
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_next (mpris);
}

static gboolean
ol_player_guayadeque02_seek (int pos_ms)
{
  OlPlayerMpris *mpris = ol_player_guayadeque02_get_mpris ();
  return ol_player_mpris_seek (mpris, pos_ms);
}

static const char *
ol_player_guayadeque02_get_icon_path ()
{
  int i;
  for (i = 0; i < ol_get_array_len (icon_paths); i++)
  {
    if (ol_path_is_file (icon_paths[i]))
      return icon_paths[i];
  }
  return NULL;
}

struct OlPlayer*
ol_player_guayadeque02_get ()
{
  ol_log_func ();
  struct OlPlayer *controller = ol_player_new ("guayadeque 0.2.5");
  ol_player_set_cmd (controller, "guayadeque02");
  controller->get_music_info = ol_player_guayadeque02_get_music_info;
  controller->get_activated = ol_player_guayadeque02_get_activated;
  controller->get_played_time = ol_player_guayadeque02_get_played_time;
  controller->get_music_length = ol_player_guayadeque02_get_music_length;
  controller->get_capacity = ol_player_guayadeque02_get_capacity;
  controller->get_status = ol_player_guayadeque02_get_status;
  controller->play = ol_player_guayadeque02_play;
  controller->pause = ol_player_guayadeque02_pause;
  controller->stop = ol_player_guayadeque02_stop;
  controller->prev = ol_player_guayadeque02_prev;
  controller->next = ol_player_guayadeque02_next;
  controller->seek = ol_player_guayadeque02_seek;
  controller->get_icon_path = ol_player_guayadeque02_get_icon_path;
  return controller;
}

