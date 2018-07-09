/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef _HELIA_
#define _HELIA_

#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define HELIA_TYPE_APPLICATION             ( helia_get_type () )
#define HELIA_APPLICATION(obj)             ( G_TYPE_CHECK_INSTANCE_CAST ( (obj), HELIA_TYPE_APPLICATION, Helia ) )
#define HELIA_APPLICATION_CLASS(klass)     ( G_TYPE_CHECK_CLASS_CAST  ( (klass), HELIA_TYPE_APPLICATION, HeliaClass ) )
#define HELIA_IS_APPLICATION(obj)          ( G_TYPE_CHECK_INSTANCE_TYPE ( (obj), HELIA_TYPE_APPLICATION ) )
#define HELIA_IS_APPLICATION_CLASS(klass)  ( G_TYPE_CHECK_CLASS_TYPE  ( (klass), HELIA_TYPE_APPLICATION ) )
#define HELIA_APPLICATION_GET_CLASS(obj)   ( G_TYPE_INSTANCE_GET_CLASS  ( (obj), HELIA_TYPE_APPLICATION, HeliaClass ) )

typedef struct _HeliaClass HeliaClass;
typedef struct _Helia Helia;


struct _HeliaClass
{
	GtkApplicationClass parent_class;
};

struct _Helia
{
	GtkApplication parent_instance;

	const gchar *program;
};

GType helia_get_type (void) G_GNUC_CONST;
Helia *helia_new (void);

G_END_DECLS

#endif /* _APPLICATION_H_ */

