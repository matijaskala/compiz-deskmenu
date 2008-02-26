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

#define HAVE_WNCK 1

#if HAVE_WNCK
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>
#include "deskmenu-wnck.h"
#endif

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

void
deskmenu_construct_item (Deskmenu *deskmenu)
{
    DeskmenuItem *item = deskmenu->current_item;
    GtkWidget *menu_item;

    switch (item->type)
    {
        case DESKMENU_ITEM_LAUNCHER:
            menu_item = gtk_image_menu_item_new_with_label
                (g_strstrip (item->name->str));
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), 
                gtk_image_new_from_icon_name (g_strstrip (item->icon->str),
                    GTK_ICON_SIZE_MENU));
            g_signal_connect (G_OBJECT (menu_item), "activate",
                G_CALLBACK (launcher_activated), g_strstrip (g_strdup
                    (item->command->str)));
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), 
                menu_item);
            break;

        case DESKMENU_ITEM_SEPARATOR:
            menu_item = gtk_separator_menu_item_new ();
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), 
                menu_item);
            break;
#if HAVE_WNCK
        case DESKMENU_ITEM_WINDOWLIST:
            menu_item = gtk_menu_item_new_with_label ("Windows");

            DeskmenuWindowlist *windowlist = deskmenu_windowlist_new ();

            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                windowlist->menu);
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
                menu_item);
            break;

        case DESKMENU_ITEM_VIEWPORTLIST:
            menu_item = gtk_menu_item_new_with_label ("Viewports");

            DeskmenuVplist *vplist = deskmenu_vplist_new ();
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                vplist->menu);
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
                menu_item);
            break;
#endif
        case DESKMENU_ITEM_RELOAD:
            menu_item = gtk_image_menu_item_new_with_label ("Reload");
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), 
                gtk_image_new_from_stock (GTK_STOCK_REFRESH, 
                    GTK_ICON_SIZE_MENU));
            g_signal_connect (G_OBJECT (menu_item), "activate", 
                G_CALLBACK (quit), NULL);    
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), 
                menu_item);
            break;

        default:
            break;
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
    DeskmenuElementType element_type;
    const gchar **ncursor = attr_names, **vcursor = attr_values;
    GtkWidget *item, *menu;

    if (deskmenu->stop_parsing)
        return;

    element_type = (DeskmenuElementType) GPOINTER_TO_INT (g_hash_table_lookup 
        (deskmenu->element_hash, element_name));

    switch (element_type)
    {
        case DESKMENU_ELEMENT_MENU:
            if (!deskmenu->menu)
            {
                deskmenu->menu = gtk_menu_new ();
                g_object_set_data (G_OBJECT (deskmenu->menu), "parent menu",
                    NULL);
                deskmenu->current_menu = deskmenu->menu;
            }
            else
            {
                gchar *name = NULL;
                while (*ncursor)
                {
                    if (strcmp (*ncursor, "name") == 0)
                        name = g_strdup (*vcursor);
                    else
                        g_set_error (error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, 
                            "Unknown attribute: %s", *ncursor);
                    ncursor++;
                    vcursor++;
                }
                if (name)
                    item = gtk_menu_item_new_with_label (name);
                else
                    item = gtk_menu_item_new_with_label ("");
                gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), 
                    item);
                menu = gtk_menu_new ();
                g_object_set_data (G_OBJECT (menu), "parent menu", 
                    deskmenu->current_menu);
                deskmenu->current_menu = menu;
                gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), 
                    deskmenu->current_menu);
                g_free (name);
            }
            break;
        
        case DESKMENU_ELEMENT_ITEM:
            deskmenu->current_item = g_slice_new0 (DeskmenuItem);
                while (*ncursor)
                {
                    if (strcmp (*ncursor, "type") == 0)
                        deskmenu->current_item->type = (DeskmenuItemType) 
                            GPOINTER_TO_INT (g_hash_table_lookup 
                                (deskmenu->item_hash, *vcursor));
                    else
                        g_set_error (error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, 
                            "Unknown attribute: %s", *ncursor);
                    ncursor++;
                    vcursor++;
                }
            break;

        case DESKMENU_ELEMENT_NAME:
        case DESKMENU_ELEMENT_ICON:
        case DESKMENU_ELEMENT_COMMAND:
            if (deskmenu->current_item)
                deskmenu->current_item->current_element = element_type;
            break;

        default:
            g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, 
                "Unknown element: %s", element_name);
            break;
    }
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

    if (deskmenu->stop_parsing)
        return;

    if (!(item && item->current_element))
        return;

    switch (item->current_element)
    {
        case DESKMENU_ELEMENT_NAME:
            if (!item->name)
                item->name = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->name, text, text_len);
            break;

        case DESKMENU_ELEMENT_ICON:
            if (!item->icon)
                item->icon = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->icon, text, text_len);
            break;

        case DESKMENU_ELEMENT_COMMAND:
            if (!item->command)
                item->command = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->command, text, text_len);
            break;

        default:
            break;
    }

}

void
end_element (GMarkupParseContext *context,
             const gchar         *element_name,
             gpointer             user_data,
             GError             **error)
{

    DeskmenuElementType element_type;
    Deskmenu *deskmenu = DESKMENU (user_data);
    GtkWidget *parent;
    element_type = (DeskmenuElementType) GPOINTER_TO_INT (g_hash_table_lookup 
        (deskmenu->element_hash, element_name));

    if (deskmenu->stop_parsing)
        return;

    switch (element_type)
    {
        case DESKMENU_ELEMENT_MENU:
            parent = g_object_get_data (G_OBJECT (deskmenu->current_menu), 
                "parent menu");

            if (deskmenu->current_menu == deskmenu->menu) /* we're done */
                deskmenu->stop_parsing = TRUE;

            deskmenu->current_menu = parent;

            break;

        case DESKMENU_ELEMENT_ITEM:
            if (!deskmenu->current_item)
                break;
            /* finally make the item ^_^ */
            deskmenu_construct_item (deskmenu);

            /* free data used to make it */
            if (deskmenu->current_item->name)
                g_string_free (deskmenu->current_item->name, TRUE);
            if (deskmenu->current_item->icon)
                g_string_free (deskmenu->current_item->icon, TRUE);
            if (deskmenu->current_item->command)
                g_string_free (deskmenu->current_item->command, TRUE);
            g_slice_free (DeskmenuItem, deskmenu->current_item);
            deskmenu->current_item = NULL;
            break;

        default:
            break;
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

    deskmenu->show_hooks = g_slice_new0 (GHookList);

    g_hook_list_init (deskmenu->show_hooks, sizeof (GHook));


    deskmenu->menu = NULL;
    deskmenu->current_menu = NULL;
    deskmenu->current_item = NULL;
    deskmenu->stop_parsing = FALSE;

    deskmenu->item_hash = g_hash_table_new (g_str_hash, g_str_equal);

    g_hash_table_insert (deskmenu->item_hash, "launcher",
        GINT_TO_POINTER (DESKMENU_ITEM_LAUNCHER));
    g_hash_table_insert (deskmenu->item_hash, "separator",
        GINT_TO_POINTER (DESKMENU_ITEM_SEPARATOR));
#if HAVE_WNCK
    g_hash_table_insert (deskmenu->item_hash, "windowlist",
        GINT_TO_POINTER (DESKMENU_ITEM_WINDOWLIST));
    g_hash_table_insert (deskmenu->item_hash, "viewportlist",
        GINT_TO_POINTER (DESKMENU_ITEM_VIEWPORTLIST));
#endif
    g_hash_table_insert (deskmenu->item_hash, "reload",
        GINT_TO_POINTER (DESKMENU_ITEM_RELOAD));

    deskmenu->element_hash = g_hash_table_new (g_str_hash, g_str_equal);
    
    g_hash_table_insert (deskmenu->element_hash, "menu", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_MENU));
    g_hash_table_insert (deskmenu->element_hash, "item", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_ITEM));
    g_hash_table_insert (deskmenu->element_hash, "name", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_NAME));
    g_hash_table_insert (deskmenu->element_hash, "icon", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_ICON));
    g_hash_table_insert (deskmenu->element_hash, "command", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_COMMAND));

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
        exit (1);
    }

    if (!g_markup_parse_context_parse (context, text, length, NULL))
    {
        int line, column;
        g_markup_parse_context_get_position (context, &line, &column);
        g_print ("Parse of %s failed at line %d \n", configpath, line);
        exit (1);
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

    g_hook_list_invoke (deskmenu->show_hooks, FALSE);

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
    g_error ("%s: %s", prefix, error->message);
    g_error_free (error);
    exit (1);
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

