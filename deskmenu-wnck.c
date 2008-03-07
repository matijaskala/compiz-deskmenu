#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "deskmenu-wnck.h"

/* borrowed from libwnck selector.c */
static GdkPixbuf *
wnck_selector_dimm_icon (GdkPixbuf *pixbuf)
{
  int x, y, pixel_stride, row_stride;
  guchar *row, *pixels;
  int w, h;
  GdkPixbuf *dimmed;

  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);

  if (gdk_pixbuf_get_has_alpha (pixbuf)) 
    dimmed = gdk_pixbuf_copy (pixbuf);
  else
    dimmed = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);

  pixel_stride = 4;

  row = gdk_pixbuf_get_pixels (dimmed);
  row_stride = gdk_pixbuf_get_rowstride (dimmed);

  for (y = 0; y < h; y++)
    {
      pixels = row;
      for (x = 0; x < w; x++)
        {
          pixels[3] /= 2;
          pixels += pixel_stride;
        }
      row += row_stride;
    }

  return dimmed;
}
#define SELECTOR_MAX_WIDTH 50   /* maximum width in characters */

static gint
wnck_selector_get_width (GtkWidget  *widget,
                         const char *text)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  gint char_width;
  PangoLayout *layout;
  PangoRectangle natural;
  gint max_width;
  gint screen_width;
  gint width;

  gtk_widget_ensure_style (widget);

  context = gtk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context, widget->style->font_desc,
                                       pango_context_get_language (context));
  char_width = pango_font_metrics_get_approximate_char_width (metrics);
  pango_font_metrics_unref (metrics);
  max_width = PANGO_PIXELS (SELECTOR_MAX_WIDTH * char_width);

  layout = gtk_widget_create_pango_layout (widget, text);
  pango_layout_get_pixel_extents (layout, NULL, &natural);
  g_object_unref (G_OBJECT (layout));

  screen_width = gdk_screen_get_width (gtk_widget_get_screen (widget));

  width = MIN (natural.width, max_width);
  width = MIN (width, 3 * (screen_width / 4));

  return width;
}

/* end borrowing */

static void
dmwin_set_weight (DeskmenuWindow *dmwin, PangoWeight weight)
{
    PangoFontDescription *font_desc;
    font_desc = pango_font_description_new ();
    pango_font_description_set_weight (font_desc, weight);
    gtk_widget_modify_font (dmwin->label, font_desc);
    pango_font_description_free (font_desc);
}

static void
dmwin_set_decoration (DeskmenuWindow *dmwin, gchar *ante, gchar *post)
{
    gchar *name;
    name = g_strconcat (ante, wnck_window_get_name (dmwin->window), post, NULL);

    gtk_label_set_text (GTK_LABEL (dmwin->label), name);

    gtk_widget_set_size_request (dmwin->label,
        wnck_selector_get_width (dmwin->windowlist->menu, name), -1);

    g_free (name);
}

static void
activate_window (GtkWidget  *widget,
                 WnckWindow *window)
{
    WnckWorkspace *workspace;
    guint32 timestamp;

    timestamp = gtk_get_current_event_time ();

    workspace = wnck_window_get_workspace (window);

    if (workspace)
        wnck_workspace_activate (workspace, timestamp);
    wnck_window_activate (window, timestamp);
}

static gint
dmwin_for_window (DeskmenuWindow *dmwin, 
                  WnckWindow *window)
{
    if (dmwin->window == window)
        return 0;
    else
        return -1; /* keep searching */
}

static void
window_name_changed (WnckWindow *window, 
                     DeskmenuWindow *dmwin)
{
    if (wnck_window_is_minimized (window))
    {
        if (wnck_window_is_shaded (window))
            dmwin_set_decoration (dmwin, "=", "=");
        else
            dmwin_set_decoration (dmwin, "[", "]");
    }
    else
        dmwin_set_decoration (dmwin, "", "");
}

static void
window_icon_changed (WnckWindow *window, 
                     DeskmenuWindow* dmwin)
{
    GdkPixbuf *pixbuf;
    gboolean free_pixbuf;

    pixbuf = wnck_window_get_mini_icon (window); 
    free_pixbuf = FALSE;    
    if (wnck_window_is_minimized (window))
    {
        pixbuf = wnck_selector_dimm_icon (pixbuf);
        free_pixbuf = TRUE;
    }

    gtk_image_set_from_pixbuf (GTK_IMAGE (dmwin->image), pixbuf);

    if (free_pixbuf)
        g_object_unref (pixbuf);
}

static void
window_state_changed (WnckWindow      *window,
                      WnckWindowState  changed_state,
                      WnckWindowState  new_state,
                      DeskmenuWindow  *dmwin)
{
    if (changed_state
        & (WNCK_WINDOW_STATE_DEMANDS_ATTENTION | WNCK_WINDOW_STATE_URGENT))
    {
        if (wnck_window_or_transient_needs_attention (window))
            dmwin_set_weight (dmwin, PANGO_WEIGHT_BOLD);
        else
            dmwin_set_weight (dmwin, PANGO_WEIGHT_NORMAL);
    }

    if (changed_state
        & (WNCK_WINDOW_STATE_MINIMIZED | WNCK_WINDOW_STATE_SHADED))
        window_name_changed (window, dmwin);

    if (changed_state & WNCK_WINDOW_STATE_SKIP_TASKLIST)
    {
        if (wnck_window_is_skip_tasklist (window))
        {
            gtk_widget_hide_all (dmwin->item);
            gtk_widget_set_no_show_all (dmwin->item, TRUE);
        }
        else
        {
            gtk_widget_set_no_show_all (dmwin->item, FALSE);
            gtk_widget_show_all (dmwin->item);
        }
    }
}

static void
windowlist_check_empty (DeskmenuWindowlist *windowlist)
{
    if (!windowlist->windows && !windowlist->empty_item)
    {
        windowlist->empty_item = gtk_menu_item_new_with_label ("None");
        gtk_widget_set_sensitive (windowlist->empty_item, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (windowlist->menu),
            windowlist->empty_item);
        gtk_widget_show (windowlist->empty_item);
    }
    else if (windowlist->empty_item)
    {
        gtk_widget_destroy (windowlist->empty_item);
        windowlist->empty_item = NULL;
    }
}

static DeskmenuWindow*
deskmenu_windowlist_window_new (WnckWindow *window,
                                DeskmenuWindowlist *windowlist)
{
    DeskmenuWindow *dmwin;

    dmwin = g_slice_new0 (DeskmenuWindow);
    dmwin->window = window;
    dmwin->windowlist = windowlist;

    dmwin->item = gtk_image_menu_item_new ();

    dmwin->label = gtk_label_new (NULL);
    gtk_container_add (GTK_CONTAINER (dmwin->item), dmwin->label);

    gtk_misc_set_alignment (GTK_MISC (dmwin->label), 0.0, 0.5);
    gtk_label_set_ellipsize (GTK_LABEL (dmwin->label), PANGO_ELLIPSIZE_END);

    dmwin->image = gtk_image_new ();

    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (dmwin->item),
        dmwin->image);

    g_signal_connect (G_OBJECT (dmwin->item), "activate", 
        G_CALLBACK (activate_window), window);

    gtk_menu_shell_append (GTK_MENU_SHELL (windowlist->menu), 
        dmwin->item);

    window_name_changed (window, dmwin);
    window_icon_changed (window, dmwin);

    if (wnck_window_or_transient_needs_attention (window))
        dmwin_set_weight (dmwin, PANGO_WEIGHT_BOLD);
    else
        dmwin_set_weight (dmwin, PANGO_WEIGHT_NORMAL);

    g_signal_connect (G_OBJECT (window), "name-changed", 
        G_CALLBACK (window_name_changed), dmwin);

    g_signal_connect (G_OBJECT (window), "icon-changed", 
        G_CALLBACK (window_icon_changed), dmwin);

    g_signal_connect (G_OBJECT (window), "state-changed", 
        G_CALLBACK (window_state_changed), dmwin);

    gtk_widget_set_no_show_all (dmwin->item,
        wnck_window_is_skip_tasklist (window));

    gtk_widget_show_all (dmwin->item);

    return dmwin;
}

static void
screen_window_opened (WnckScreen *screen, WnckWindow *window,
                      DeskmenuWindowlist *windowlist)
{
    DeskmenuWindow *dmwin;
    dmwin = deskmenu_windowlist_window_new (window, windowlist);
    windowlist->windows = g_list_prepend (windowlist->windows, dmwin);

    windowlist_check_empty (windowlist);
}

static void
screen_window_closed (WnckScreen *screen, WnckWindow *window,
                      DeskmenuWindowlist *windowlist)
{
    DeskmenuWindow *dmwin;
    GList *list;

    list = g_list_find_custom (windowlist->windows, window,
        (GCompareFunc) dmwin_for_window);

    if (!list)
        return;

    dmwin = list->data;

    gtk_widget_destroy (dmwin->image);
    gtk_widget_destroy (dmwin->label);
    gtk_widget_destroy (dmwin->item);
    g_slice_free (DeskmenuWindow, dmwin);
    windowlist->windows = g_list_remove (windowlist->windows, dmwin);

    windowlist_check_empty (windowlist);
}

DeskmenuWindowlist*
deskmenu_windowlist_new (void)
{
    DeskmenuWindowlist *windowlist;
    windowlist = g_slice_new0 (DeskmenuWindowlist);
    windowlist->screen = wnck_screen_get_default ();

    windowlist->menu = gtk_menu_new ();

    g_signal_connect (G_OBJECT (windowlist->screen), "window-opened",
        G_CALLBACK (screen_window_opened), windowlist);

    g_signal_connect (G_OBJECT (windowlist->screen), "window-closed",
        G_CALLBACK (screen_window_closed), windowlist);

    windowlist_check_empty (windowlist);

    return windowlist;
}

static void
deskmenu_vplist_goto (GtkWidget      *widget,
                      DeskmenuVplist *vplist)
{
    guint viewport;
    viewport = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), 
        "viewport"));
    guint column, row, i;

    column = viewport % vplist->hsize;
    if (!column)
        column = vplist->hsize - 1; /* needs to be zero-indexed */
    else
        column--;

    row = 0;
    if (viewport > vplist->hsize)
    {
        i = 0;
        while (i < (viewport - vplist->hsize))
        {
            i += vplist->hsize;
            row++;
        }
    }

    wnck_screen_move_viewport (vplist->screen,
        column * vplist->screen_width,
        row * vplist->screen_height);
}

static void
deskmenu_vplist_go_direction (GtkWidget      *widget,
                              DeskmenuVplist *vplist)
{
    WnckMotionDirection direction;
    direction = (WnckMotionDirection) GPOINTER_TO_UINT (g_object_get_data 
        (G_OBJECT (widget), "direction"));

    guint x = vplist->x, y = vplist->y;

    switch (direction)
    {
        case WNCK_MOTION_LEFT:
            if (vplist->screen_width > x)
                x = vplist->xmax;
            else
                x -= vplist->screen_width;
            break;
        case WNCK_MOTION_RIGHT:
            if (x + vplist->screen_width > vplist->xmax)
                x = 0;
            else
                x += vplist->screen_width;
            break;
        case WNCK_MOTION_UP:
            if (vplist->screen_height > y)
                y = vplist->ymax;
            else
                y -= vplist->screen_height;
            break;
        case WNCK_MOTION_DOWN:
            if (y + vplist->screen_height > vplist->ymax)
                y = 0;
            else
                y += vplist->screen_height;
            break;
        default:
            g_assert_not_reached ();
            break;
    }
    wnck_screen_move_viewport (vplist->screen, x, y);
}

static gboolean
deskmenu_vplist_can_move (DeskmenuVplist      *vplist,
                          WnckMotionDirection  direction)
{
    if (!vplist->wrap)
    {
        switch (direction)
        {
            case WNCK_MOTION_LEFT:
                return vplist->x;
            case WNCK_MOTION_RIGHT:
                return (vplist->x != vplist->xmax);
            case WNCK_MOTION_UP:
                return vplist->y;
            case WNCK_MOTION_DOWN:
                return (vplist->y != vplist->ymax);
            default:
                break;
        }
    }
    else
    {
        switch (direction)
        {
            case WNCK_MOTION_LEFT:
            case WNCK_MOTION_RIGHT:
                return (vplist->hsize > 1);
            case WNCK_MOTION_UP:
            case WNCK_MOTION_DOWN:
                return (vplist->vsize > 1);
            default:
                break;
        }
    }
    g_assert_not_reached ();
}

static GtkWidget*
deskmenu_vplist_make_go_item (DeskmenuVplist      *vplist,
                              WnckMotionDirection  direction,
                              gchar               *name,
                              gchar               *stock_id)
{
    GtkWidget *item;
    item = gtk_image_menu_item_new_with_mnemonic (name);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
        gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU));
    g_object_set_data (G_OBJECT (item), "direction",
        GINT_TO_POINTER (direction));
    g_signal_connect (G_OBJECT (item), "activate", 
        G_CALLBACK (deskmenu_vplist_go_direction), vplist);
    gtk_menu_shell_append (GTK_MENU_SHELL (vplist->menu), item);
    
    return item;
}

guint
deskmenu_vplist_get_vpid (DeskmenuVplist *vplist)
{
    guint column, row;
    column = vplist->x / vplist->screen_width;
    row = vplist->y / vplist->screen_height;
    
    return (row * vplist->hsize + column + 1);
}

static void
deskmenu_vplist_update (WnckScreen *screen, DeskmenuVplist *vplist)
{
    guint new_count, current;
    /* get dimensions */

    vplist->workspace = wnck_screen_get_workspace
        (vplist->screen, 0);

    vplist->screen_width = wnck_screen_get_width (screen);
    vplist->workspace_width = wnck_workspace_get_width
        (vplist->workspace);
    vplist->screen_height = wnck_screen_get_height (screen);
    vplist->workspace_height = wnck_workspace_get_height
        (vplist->workspace);

    vplist->x = wnck_workspace_get_viewport_x (vplist->workspace);
    vplist->y = wnck_workspace_get_viewport_y (vplist->workspace);
    vplist->xmax = vplist->workspace_width - vplist->screen_width;
    vplist->ymax = vplist->workspace_height - vplist->screen_height;
    vplist->hsize = vplist->workspace_width / vplist->screen_width;

    current = deskmenu_vplist_get_vpid (vplist);

    vplist->vsize = vplist->workspace_height / vplist->screen_height;

    new_count = vplist->hsize * vplist->vsize;

    gtk_widget_hide (vplist->go_left);
    gtk_widget_hide (vplist->go_right);
    gtk_widget_hide (vplist->go_up);
    gtk_widget_hide (vplist->go_down);

    gtk_widget_set_no_show_all (vplist->go_left,
        !deskmenu_vplist_can_move (vplist, WNCK_MOTION_LEFT));
    gtk_widget_set_no_show_all (vplist->go_right,
        !deskmenu_vplist_can_move (vplist, WNCK_MOTION_RIGHT));
    gtk_widget_set_no_show_all (vplist->go_up,
        !deskmenu_vplist_can_move (vplist, WNCK_MOTION_UP));
    gtk_widget_set_no_show_all (vplist->go_down,
        !deskmenu_vplist_can_move (vplist, WNCK_MOTION_DOWN));

    GtkWidget *item;
    guint i;

    if (new_count > vplist->old_count)
    {
        gchar *text;

        for (i = vplist->old_count; i < new_count; i++)
        {
            text = g_strdup_printf ("Viewport _%i", i + 1);
            item = gtk_menu_item_new_with_mnemonic (text);
            g_object_set_data (G_OBJECT (item), "viewport",
                GUINT_TO_POINTER (i + 1));
            g_signal_connect (G_OBJECT (item), "activate",
                G_CALLBACK (deskmenu_vplist_goto), vplist);
            gtk_menu_shell_append (GTK_MENU_SHELL (vplist->menu), item);
            g_ptr_array_add (vplist->goto_items, item);
            g_free (text);
        }

        vplist->old_count = new_count;
    }
    else if (new_count < vplist->old_count)
    {
        for (i = new_count; i < vplist->old_count; i++)
        {
            item = g_ptr_array_index (vplist->goto_items, i);
            gtk_widget_destroy (item);
        }
        g_ptr_array_remove_range (vplist->goto_items, new_count, 
            vplist->old_count - new_count);

        vplist->old_count = new_count;
    }

    if (current != vplist->old_vpid)
    {
        if (vplist->old_vpid && vplist->old_vpid <= new_count)
        {
            item = g_ptr_array_index (vplist->goto_items, vplist->old_vpid - 1);
            gtk_widget_set_sensitive (item, TRUE);
        }
        item = g_ptr_array_index (vplist->goto_items, current - 1);
        gtk_widget_set_sensitive (item, FALSE);

        vplist->old_vpid = current;
    }

    gtk_widget_show_all (vplist->menu);    

    return;
}

DeskmenuVplist*
deskmenu_vplist_new (void)
{
    DeskmenuVplist *vplist;
    vplist = g_slice_new0 (DeskmenuVplist);
    vplist->screen = wnck_screen_get_default ();

    vplist->menu = gtk_menu_new ();

    vplist->go_left = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_LEFT,
                  "Viewport _Left", GTK_STOCK_GO_BACK);

    vplist->go_right = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_RIGHT,
                  "Viewport _Right", GTK_STOCK_GO_FORWARD);

    vplist->go_up =  deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_UP,
                  "Viewport _Up", GTK_STOCK_GO_UP);

    vplist->go_down = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_DOWN,
                  "Viewport _Down", GTK_STOCK_GO_DOWN);

    gtk_menu_shell_append (GTK_MENU_SHELL (vplist->menu), 
        gtk_separator_menu_item_new ());

    vplist->goto_items = g_ptr_array_new ();

    g_signal_connect (G_OBJECT (vplist->screen), "viewports-changed",
        G_CALLBACK (deskmenu_vplist_update), vplist);

    return vplist;
}

