/**
 * @file   ol_menu.c
 * @author Tiger Soldier <tigersoldi@gmail.com>
 * @date   Sun Aug 16 16:18:41 2009
 * 
 * @brief  Popup menu for OSD lyrics
 * 
 * 
 */

#include "ol_menu.h"
#include "ol_intl.h"
#include "ol_config.h"
#include "ol_about.h"
#include "ol_option.h"
#include "ol_keybindings.h"
#include "ol_commands.h"
#include "ol_app.h"
#include "ol_gui.h"
#include "ol_search_dialog.h"
#include "ol_lrc_parser.h"
#include "ol_player.h"
#include "ol_debug.h"

static void ol_config_changed (OlConfig *config, gchar *group, gchar *name, gpointer data);
static GtkWidget *popup_menu = NULL;
/* enum { */
/*   OL_MENU_LOCK, */
/*   OL_MENU_HIDE, */
/*   OL_MENU_PERFERENCE, */
/*   OL_MENU_ABOUT, */
/*   OL_MENU_QUIT, */
/*   OL_MENU_COUNT, */
/* }; */

static struct Menu
{
  GtkWidget *lock;
  GtkWidget *hide;
  GtkWidget *play;
  GtkWidget *pause;
  GtkWidget *stop;
  GtkWidget *prev;
  GtkWidget *next;
  GtkWidget *preference;
  GtkWidget *about;
  GtkWidget *quit;
} menu = {0};

/* static GtkMenuItem *items[OL_MENU_COUNT] = {0}; */
void ol_menu_lock (GtkWidget *widget, gpointer data);
void ol_menu_hide (GtkWidget *widget, gpointer data);
void ol_menu_quit (GtkWidget *widget, gpointer data);
void ol_menu_preference (GtkWidget *widget, gpointer data);
void ol_menu_play (GtkWidget *widget, gpointer data);
void ol_menu_pause (GtkWidget *widget, gpointer data);
void ol_menu_stop (GtkWidget *widget, gpointer data);
void ol_menu_prev (GtkWidget *widget, gpointer data);
void ol_menu_next (GtkWidget *widget, gpointer data);
void ol_menu_download (GtkWidget *widget, gpointer data);
void ol_menu_no_lyric (GtkWidget *widget, gpointer data);
void ol_menu_assign_lrc (GtkWidget *widget, gpointer data);
void ol_menu_advance_lrc (GtkWidget *widget, gpointer data);
void ol_menu_delay_lrc (GtkWidget *widget, gpointer data);

void
ol_menu_advance_lrc (GtkWidget *widget, gpointer data)
{
  ol_app_adjust_lyric_offset (-200);
}

void
ol_menu_delay_lrc (GtkWidget *widget, gpointer data)
{
  ol_app_adjust_lyric_offset (200);
}

void
ol_menu_download (GtkWidget *widget, gpointer data)
{
  /* if (ol_app_get_controller () != NULL) */
  /* { */
  OlMusicInfo *info = ol_app_get_current_music ();
  if (info != NULL)
    ol_search_dialog_show ();
  /* ol_app_download_lyric (info); */
  /* } */
}

void
ol_menu_no_lyric (GtkWidget *widget, gpointer data)
{
  OlMusicInfo *info = ol_app_get_current_music ();
  if (info != NULL)
    ol_app_assign_lrcfile (info, NULL, TRUE);
}

void
ol_menu_assign_lrc (GtkWidget *widget, gpointer data)
{
  static char *prev_path = NULL;
  OlMusicInfo *info = ol_app_get_current_music ();
  GtkFileFilter *lrc_filter = NULL;
  lrc_filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (lrc_filter, _("LRC files"));
  gtk_file_filter_add_pattern (lrc_filter, "*.lrc");
  if (info != NULL)
  {
    ol_errorf ("prev_path: %s\n", prev_path);
    GtkWidget *dialog = NULL;
    dialog = gtk_file_chooser_dialog_new (_("Choose LRC file to assign"),
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), lrc_filter);
    
    if (prev_path != NULL)
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), prev_path);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *filename;
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      ol_app_assign_lrcfile (info, filename, TRUE);
      g_free (filename);
      if (prev_path != NULL)
        g_free (prev_path);
      prev_path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    }
    gtk_widget_destroy (dialog);
    dialog = NULL;
  }
}

static void
ol_config_changed (OlConfig *config, gchar *group, gchar *name, gpointer data)
{
  ol_log_func ();
  ol_logf (OL_DEBUG, "  name:%s\n", name);
  if (strcmp (name, "locked") == 0)
  {
    gboolean locked = ol_config_get_bool (config, "OSD", "locked");
    if (menu.lock != NULL &&
        locked != gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu.lock)))
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu.lock),
                                      locked);
  }
  else if (strcmp (name, "visible") == 0)
  {
    gboolean visible = ol_config_get_bool (config, "General", name);
    if (menu.hide &&
        visible == gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu.hide)))
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu.hide),
                                      !visible);
  }
}

void
ol_menu_lock (GtkWidget *widget, gpointer data)
{
  OlConfig *config = ol_config_get_instance ();
  ol_assert (config != NULL);
  gboolean locked = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
  ol_config_set_bool (config, "OSD",  "locked", locked);
}

void
ol_menu_hide (GtkWidget *widget, gpointer data)
{
  OlConfig *config = ol_config_get_instance ();
  ol_assert (config != NULL);
  gboolean hide = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
  ol_config_set_bool (config, "General", "visible", !hide);
}

void
ol_menu_quit (GtkWidget *widget,
              gpointer data)
{
  gtk_main_quit ();
}

void
ol_menu_preference (GtkWidget *widget, gpointer data)
{
  ol_option_show ();
}

void
ol_menu_play (GtkWidget *widget, gpointer data)
{
  struct OlPlayer *player = ol_app_get_player ();
  if (player == NULL)
    return;
  ol_log_func ();
  ol_player_play (player);
}

void
ol_menu_pause (GtkWidget *widget, gpointer data)
{
  struct OlPlayer *player = ol_app_get_player ();
  if (player == NULL)
    return;
  ol_player_pause (player);
}

void
ol_menu_stop (GtkWidget *widget, gpointer data)
{
  struct OlPlayer *player = ol_app_get_player ();
  if (player == NULL)
    return;
  ol_player_stop (player);
}

void
ol_menu_prev (GtkWidget *widget, gpointer data)
{
  struct OlPlayer *player = ol_app_get_player ();
  if (player == NULL)
    return;
  ol_player_prev (player);
}

void
ol_menu_next (GtkWidget *widget, gpointer data)
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  struct OlPlayer *player = ol_app_get_player ();
  if (player == NULL)
    return;
  ol_player_next (player);
}

static void
ol_menu_init ()
{
  /* create accelerator group */
  GtkAccelGroup *accel = ol_keybinding_get_accel_group ();
  GtkWidget *item;
  OlConfig *config = ol_config_get_instance ();
  popup_menu = ol_gui_get_widget ("pop-menu");
  gtk_menu_set_accel_group (GTK_MENU (popup_menu), accel);
  menu.lock = ol_gui_get_widget ("menu-lock");
  if (menu.lock)
  {
    gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu.lock),
                                  "<OSD Lyrics>/Lock");
  }
  ol_config_changed (config, "OSD", "locked", NULL);

  menu.hide = ol_gui_get_widget ("menu-hide");
  if (menu.hide)
  {
    gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu.hide),
                                  "<OSD Lyrics>/Hide");
  }
  ol_config_changed (config, "General", "visible", NULL);

  menu.preference = ol_gui_get_widget ("menu-prefernce");
  menu.about = ol_gui_get_widget ("menu-about");
  menu.quit = ol_gui_get_widget ("menu-quit");

  menu.play = ol_gui_get_widget ("menu-play");
  menu.pause = ol_gui_get_widget ("menu-pause");
  menu.stop = ol_gui_get_widget ("menu-stop");
  menu.prev = ol_gui_get_widget ("menu-prev");
  menu.next = ol_gui_get_widget ("menu-next");
  
  g_signal_connect (config,
                    "changed",
                    G_CALLBACK (ol_config_changed),
                    NULL);
  gtk_widget_show_all (popup_menu);
}

static void
ol_menu_update_player_control ()
{
  ol_log_func ();
  struct OlPlayer *player = ol_app_get_player ();
  if (menu.play)
  {
    if (player != NULL)
    {
      if ((ol_player_get_capacity (player) & OL_PLAYER_STATUS) &&
          ol_player_get_status (player) == OL_PLAYER_PLAYING)
      {
        gtk_widget_hide (menu.play);
      }
      else
      {
        gtk_widget_show (menu.play);
        gtk_widget_set_sensitive (menu.play,
                                  ol_player_get_capacity (player) & OL_PLAYER_PLAY);
      }
    }
    else
    {
      gtk_widget_show (menu.play);
      gtk_widget_set_sensitive (menu.play, FALSE);
    }
  }
  if (menu.pause)
  {
    if (player != NULL)
    {
      if ((ol_player_get_capacity (player) & OL_PLAYER_STATUS))
      {
        if (ol_player_get_status (player) == OL_PLAYER_PLAYING)
          gtk_widget_show (menu.pause);
        else
          gtk_widget_hide (menu.pause);
      }
      else
      {
        gtk_widget_show(menu.pause);
      }
      gtk_widget_set_sensitive (menu.pause,
                                ol_player_get_capacity (player) & OL_PLAYER_PAUSE);
    }
    else
    {
      gtk_widget_hide (menu.pause);
    }
  }
  if (menu.stop)
    gtk_widget_set_sensitive (menu.stop,
                              player != NULL &&
                              (ol_player_get_capacity (player) & OL_PLAYER_STOP));
  if (menu.prev)
    gtk_widget_set_sensitive (menu.prev,
                              player != NULL &&
                              (ol_player_get_capacity (player) & OL_PLAYER_PREV));
  if (menu.next)
    gtk_widget_set_sensitive (menu.next,
                              player != NULL &&
                              (ol_player_get_capacity (player) & OL_PLAYER_NEXT));
}

GtkWidget*
ol_menu_get_popup ()
{
  ol_log_func ();
  if (popup_menu == NULL)
  {
    ol_menu_init ();
  }
  ol_menu_update_player_control ();
  return popup_menu;
}
