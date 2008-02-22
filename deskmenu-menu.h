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

#include "deskmenu-common.h"

typedef struct Deskmenu Deskmenu;
typedef struct DeskmenuClass DeskmenuClass;
typedef struct DeskmenuItem DeskmenuItem;

GType deskmenu_get_type (void);

struct DeskmenuItem
{
    gchar *type;
    gchar *current_element;
    GString *name;
    GString *icon;
    GString *command;
};

struct Deskmenu
{
  GObject parent;
  WnckScreen *screen;
  GtkWidget *menu;
  GtkWidget *windows_menu;
  GtkWidget *current_menu;
  DeskmenuItem *current_item;
  gchar *configpath;
};

struct DeskmenuClass
{
  GObjectClass parent;
};


#define DESKMENU_TYPE              (deskmenu_get_type ())
#define DESKMENU(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DESKMENU_TYPE, Deskmenu))
#define DESKMENU_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), DESKMENU_TYPE, DeskmenuClass))
#define IS_DESKMENU(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DESKMENU_TYPE))
#define IS_DESKMENU_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), DESKMENU_TYPE))
#define DESKMENU_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), DESKMENU_TYPE, DeskMenu))


typedef enum
{
  DESKMENU_ERROR_GENERIC
} DeskmenuError;

GQuark deskmenu_error_quark (void);
#define DESKMENU_ERROR deskmenu_error_quark ()


gboolean deskmenu_show(Deskmenu *deskmenu, GError **error);

