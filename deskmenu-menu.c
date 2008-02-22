/*
 * compiz-deskmenu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * compiz-deskmenu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2008 Christopher Williams <christopherw@verizon.net> 
 */

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1

#include <libwnck/libwnck.h>
#include "deskmenu-menu.h"
#include "deskmenu-glue.h"


G_DEFINE_TYPE(Deskmenu, deskmenu, G_TYPE_OBJECT)


GQuark
deskmenu_error_quark (void)
{
    static GQuark quark = 0;
    if (!quark)
        quark = g_quark_from_static_string ("deskmenu_error");
    return quark;
}


static void
quit (GtkWidget *widget,
      gpointer   data)
{
    gtk_main_quit ();
}


static void
activate_window (GtkWidget  *widget,
                 WnckWindow *window)
{
    WnckWorkspace *workspace;
    guint32 timestamp;

    timestamp = gtk_get_current_event_time ();

    workspace = (WnckWorkspace *) wnck_window_get_workspace (window);

    if (workspace)
        wnck_workspace_activate (workspace, timestamp);
    wnck_window_activate (window, timestamp);
}

void
deskmenu_populate_windows_menu (Deskmenu *deskmenu)
{
    GList *list, *tmp;
    char *name, *ante, *post;
    GtkWidget *item;
    WnckWindow *window;

    list = gtk_container_get_children (GTK_CONTAINER (deskmenu->windows_menu));

    for (tmp = list; tmp != NULL; tmp = tmp->next)
    {
        gtk_widget_destroy (tmp->data);
    }

    list = wnck_screen_get_windows (deskmenu->screen);

    for (tmp = list; tmp != NULL; tmp = tmp->next)
    {
        window = tmp->data;

        if (wnck_window_is_skip_tasklist (window))
            continue;

        if (wnck_window_is_shaded (window))
        {
            ante = "=";
            post = "=";

        }
        else
        {
            if (wnck_window_is_minimized (window))
            {
                ante = "[";
                post = "]";
            }
            else
            {
                ante = "";
                post = "";
            }
        }

        name = g_strconcat (ante, wnck_window_get_name (window), post, NULL);

        item = gtk_image_menu_item_new_with_label (name);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_pixbuf (
                (GdkPixbuf *) wnck_window_get_mini_icon (window)));
        g_signal_connect (G_OBJECT (item), "activate", 
            G_CALLBACK (activate_window), window);
        gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->windows_menu), item);
        g_free (name);

        
    }

    gtk_widget_show_all (deskmenu->menu);

}


void
launcher_activated (GtkWidget *widget,
                    gchar     *command)
{
    GError *error = NULL;
	if (!gdk_spawn_on_screen (gdk_screen_get_default (),
                              g_get_home_dir (),
                              g_strsplit (command, " ", 0),
                              NULL, G_SPAWN_SEARCH_PATH,
                              NULL, NULL, NULL, &error))
    {
        GtkWidget *message = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE, "%s", error->message);
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (message);
    }
        
}


/* The handler functions. */

void
start_element (GMarkupParseContext *context,
               const gchar         *element_name,
               const gchar        **attr_names,
               const gchar        **attr_values,
               gpointer             user_data,
               GError             **error)
{
    Deskmenu *deskmenu = DESKMENU (user_data);
    const gchar **ncursor = attr_names, **vcursor = attr_values;
    GtkWidget *item, *menu;

    if (strcmp (element_name, "menu") == 0)
    {
        if (!deskmenu->menu)
        {
            deskmenu->menu = gtk_menu_new ();
            g_object_set_data (G_OBJECT (deskmenu->menu), "parent menu", NULL);
            deskmenu->current_menu = deskmenu->menu;
        }
        else
        {
            gchar *name = NULL;
            while (*ncursor) {
                if (strcmp (*ncursor, "name") == 0)
                    name = g_strdup (*vcursor);
                else
                    g_set_error (error, G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, 
                        "Unknown attribute: %s", *ncursor);
                ncursor++;
                vcursor++;
            }
            item = gtk_menu_item_new_with_label (name);
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), item);
            menu = gtk_menu_new ();
            g_object_set_data (G_OBJECT (menu), "parent menu", deskmenu->current_menu);
            deskmenu->current_menu = menu;
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), deskmenu->current_menu);
            g_free (name);
        }

    }
    else if (strcmp (element_name, "item") == 0)
    {
        deskmenu->current_item = g_slice_new0 (DeskmenuItem);
            while (*ncursor) {
                if (strcmp (*ncursor, "type") == 0)
                    deskmenu->current_item->type = g_strdup (*vcursor);
                else
                    g_set_error (error, G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, 
                        "Unknown attribute: %s", *ncursor);
                ncursor++;
                vcursor++;
            }
    }
    else if (strcmp (element_name, "name") == 0)
    {
        deskmenu->current_item->current_element = "name";
    }
    else if (strcmp (element_name, "icon") == 0)
    {
        deskmenu->current_item->current_element = "icon";
    } 
    else if (strcmp (element_name, "command") == 0)
    {
        deskmenu->current_item->current_element = "command";
    }
    else
        g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, 
            "Unknown element: %s", element_name);
}

void
text (GMarkupParseContext *context,
      const gchar         *text,
      gsize                text_len,
      gpointer             user_data,
      GError             **error)
{
    Deskmenu *deskmenu = DESKMENU (user_data);
    DeskmenuItem *item = deskmenu->current_item;
    if (item && item->current_element)
    {
        if (strcmp (item->current_element, "name") == 0)
        {
            if (!item->name)
                item->name = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->name, text, text_len);
        }
        else if (strcmp (item->current_element, "icon") == 0)
        {
            if (!item->icon)
                item->icon = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->icon, text, text_len);
        } 
        else if (strcmp (item->current_element, "command") == 0)
        {
            if (!item->command)
                item->command = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->command, text, text_len);
        }
    }
}

void
end_element (GMarkupParseContext *context,
             const gchar         *element_name,
             gpointer             user_data,
             GError             **error)
{

    Deskmenu *deskmenu = DESKMENU (user_data);
    if (strcmp (element_name, "menu") == 0)
    {
        GtkWidget *parent = g_object_get_data (G_OBJECT (deskmenu->current_menu), "parent menu");
        deskmenu->current_menu = parent;
    }
    else if (strcmp (element_name, "item") == 0 && deskmenu->current_item)
    {

        /* finally make the item ^_^ */
        DeskmenuItem *item = deskmenu->current_item;
        GtkWidget *menu_item;
        if (strcmp (item->type, "launcher") == 0)
        {
            menu_item = gtk_image_menu_item_new_with_label (g_strstrip (item->name->str));
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), 
                gtk_image_new_from_icon_name (g_strstrip (item->icon->str), GTK_ICON_SIZE_MENU));
            g_signal_connect (G_OBJECT (menu_item), "activate",
                G_CALLBACK (launcher_activated), g_strstrip (g_strdup (item->command->str)));
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), menu_item);

        }
        else if (strcmp (item->type, "separator") == 0)
        {
            menu_item = gtk_separator_menu_item_new ();
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), menu_item);
        }
        else if (strcmp (item->type, "windowlist") == 0 && !deskmenu->windows_menu)
        {
            menu_item = gtk_menu_item_new_with_label ("Windows");
            deskmenu->windows_menu = gtk_menu_new ();
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), deskmenu->windows_menu);
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), menu_item);
        }
        else if (strcmp (item->type, "reload") == 0)
        {
            menu_item = gtk_image_menu_item_new_with_label ("Reload");
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), 
                gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
            g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (quit), NULL);    
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), menu_item);
        }

        g_free (item->type);

        if (item->name)
            g_string_free (item->name, TRUE);
        if (item->icon)
            g_string_free (item->icon, TRUE);
        if (item->command)
            g_string_free (item->command, TRUE);
        g_slice_free (DeskmenuItem, item);
        deskmenu->current_item = NULL;
    }
}

/* The list of what handler does what. */
static GMarkupParser parser = {
    start_element,
    end_element,
    text,
    NULL,
    NULL
};


/* Class init */
static void 
deskmenu_class_init (DeskmenuClass *deskmenu_class)
{
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (deskmenu_class),
        &dbus_glib_deskmenu_object_info);
}

/* Instance init */
static void 
deskmenu_init (Deskmenu *deskmenu)
{
    gchar *configpath;
    
    configpath = g_build_path (G_DIR_SEPARATOR_S,
                               g_get_user_config_dir (),
                               "compiz", "deskmenu", "menu.xml", NULL);

    deskmenu->screen = wnck_screen_get_default ();
    wnck_screen_force_update (deskmenu->screen);

    deskmenu->menu = NULL;
    deskmenu->windows_menu = NULL;
    deskmenu->current_menu = NULL;
    deskmenu->current_item = NULL;

    char *text;
    gsize length;
    GMarkupParseContext *context = g_markup_parse_context_new (&parser,
        0, deskmenu, NULL);


    gboolean success = FALSE;
    if (!g_file_get_contents (configpath, &text, &length, NULL))
    {
        const gchar* const *cursor = g_get_system_config_dirs ();
        gchar *path = NULL;
        while (*cursor)
        {
            g_free (configpath);
            g_free (path);
            path = g_strdup (*cursor);
            configpath = g_build_path (G_DIR_SEPARATOR_S,
                         path,
                         "compiz", "deskmenu", "menu.xml", NULL);

            if (g_file_get_contents (configpath, &text, &length, NULL))
            {
                success = TRUE;
                g_free (path);
                break;
            }
            cursor++;
        }
    }
    else
    {
        success = TRUE;
    }

    if (!success)
    {
        g_print ("Couldn't find a menu file\n");
        exit (255);
    }

    if (!g_markup_parse_context_parse (context, text, length, NULL))
    {
        int line, column;
        g_markup_parse_context_get_position (context, &line, &column);
        g_print ("Parse of %s failed at line %d, column %d \n", configpath, line, column);
        exit (255);
    }

    g_free(text);
    g_markup_parse_context_free (context);

    gtk_widget_show_all (deskmenu->menu);
}

/* The show method */
gboolean
deskmenu_show (Deskmenu *deskmenu,
               GError  **error)
{
    guint32 timestamp;
    timestamp = gtk_get_current_event_time ();

    if (deskmenu->windows_menu)
        deskmenu_populate_windows_menu (deskmenu);

    gtk_menu_popup (GTK_MENU (deskmenu->menu), 
                    NULL, NULL, NULL, NULL, 
                    0, timestamp);
    return TRUE;
}

/* Convenience function to print an error and exit */
static void
die (const char *prefix,
     GError     *error)
{
    g_error("%s: %s", prefix, error->message);
    g_error_free (error);
    exit(1);
}

int
main (int    argc,
      char **argv)
{
    DBusGConnection *connection;
    GError *error = NULL;
    GObject *deskmenu;

    g_type_init ();

    /* Obtain a connection to the session bus */
    connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (connection == NULL)
        die ("Failed to open connection to bus", error);

    gtk_init (&argc, &argv);

    deskmenu = g_object_new (DESKMENU_TYPE, NULL);

    dbus_g_connection_register_g_object (connection,
                                         DESKMENU_PATH_DBUS,
                                         deskmenu);

	if (!dbus_bus_request_name (dbus_g_connection_get_connection (connection),
						        DESKMENU_SERVICE_DBUS,
                                DBUS_NAME_FLAG_REPLACE_EXISTING,
						        NULL))
        return 1;

    gtk_main ();

    return 0;
}

