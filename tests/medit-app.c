/* This file has been generated with opag 0.8.0.  */
/*
 *   medit-app.c
 *
 *   Copyright (C) 2004-2006 by Yevgen Muntyan <muntyan@math.tamu.edu>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   See COPYING file that comes with this distribution.
 */

#include "medit-ui.h"
#include <mooapp/mooapp.h>
#include <mooutils/mooutils-fs.h>
#include <mooutils/mooutils-misc.h>
#include <mooutils/moostock.h>
#include <moo-version.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define DEFAULT_NEW_INSTANCE 0


int _medit_parse_options (const char *const program_name,
                          const int         argc,
                          char **const      argv);

/********************************************************
 * command line parsing code generated by Opag
 * http://www.zero-based.org/software/opag/
 */
#ifndef STR_ERR_UNKNOWN_LONG_OPT
# define STR_ERR_UNKNOWN_LONG_OPT   "%s: unrecognized option `--%s'\n"
#endif

#ifndef STR_ERR_LONG_OPT_AMBIGUOUS
# define STR_ERR_LONG_OPT_AMBIGUOUS "%s: option `--%s' is ambiguous\n"
#endif

#ifndef STR_ERR_MISSING_ARG_LONG
# define STR_ERR_MISSING_ARG_LONG   "%s: option `--%s' requires an argument\n"
#endif

#ifndef STR_ERR_UNEXPEC_ARG_LONG
# define STR_ERR_UNEXPEC_ARG_LONG   "%s: option `--%s' doesn't allow an argument\n"
#endif

#ifndef STR_ERR_UNKNOWN_SHORT_OPT
# define STR_ERR_UNKNOWN_SHORT_OPT  "%s: unrecognized option `-%c'\n"
#endif

#ifndef STR_ERR_MISSING_ARG_SHORT
# define STR_ERR_MISSING_ARG_SHORT  "%s: option `-%c' requires an argument\n"
#endif

#define STR_HELP_UNIQUE "\
  -u, --unique        Use running instance of application\n"

#define STR_HELP_NEW_APP "\
  -n, --new-app       Run new instance of application\n"

#define STR_HELP_LOG "\
  -l, --log[=FILE]    Show debug output or write it to FILE\n"

#define STR_HELP_VERSION "\
      --version       Display version information and exit\n"

#define STR_HELP_HELP "\
  -h, --help          Display this help text and exit\n"

#define STR_HELP "\
  -u, --unique        Use running instance of application\n\
  -n, --new-app       Run new instance of application\n\
  -l, --log[=FILE]    Show debug output or write it to FILE\n\
      --version       Display version information and exit\n\
  -h, --help          Display this help text and exit\n"

/* Set to 1 if option --unique (-u) has been specified.  */
char _medit_opt_unique;

/* Set to 1 if option --new-app (-n) has been specified.  */
char _medit_opt_new_app;

/* Set to 1 if option --log (-l) has been specified.  */
char _medit_opt_log;

/* Set to 1 if option --version has been specified.  */
char _medit_opt_version;

/* Set to 1 if option --help (-h) has been specified.  */
char _medit_opt_help;

/* Argument to option --log (-l), or a null pointer if no argument.  */
const char *_medit_arg_log;

/* Parse command line options.  Return index of first non-option argument,
   or -1 if an error is encountered.  */
int _medit_parse_options (const char *const program_name, const int argc, char **const argv)
{
  static const char *const optstr__unique = "unique";
  static const char *const optstr__new_app = "new-app";
  static const char *const optstr__version = "version";
  static const char *const optstr__help = "help";
  int i = 0;
  _medit_opt_unique = 0;
  _medit_opt_new_app = 0;
  _medit_opt_log = 0;
  _medit_opt_version = 0;
  _medit_opt_help = 0;
  _medit_arg_log = 0;
  while (++i < argc)
  {
    const char *option = argv [i];
    if (*option != '-')
      return i;
    else if (*++option == '\0')
      return i;
    else if (*option == '-')
    {
      const char *argument;
      size_t option_len;
      ++option;
      if ((argument = strchr (option, '=')) == option)
        goto error_unknown_long_opt;
      else if (argument == 0)
        option_len = strlen (option);
      else
        option_len = argument++ - option;
      switch (*option)
      {
       case '\0':
        return i + 1;
       case 'h':
        if (strncmp (option + 1, optstr__help + 1, option_len - 1) == 0)
        {
          if (argument != 0)
          {
            option = optstr__help;
            goto error_unexpec_arg_long;
          }
          _medit_opt_help = 1;
          return i + 1;
        }
        goto error_unknown_long_opt;
       case 'l':
        if (strncmp (option + 1, "og", option_len - 1) == 0)
        {
          _medit_arg_log = argument;
          _medit_opt_log = 1;
          break;
        }
        goto error_unknown_long_opt;
       case 'n':
        if (strncmp (option + 1, optstr__new_app + 1, option_len - 1) == 0)
        {
          if (argument != 0)
          {
            option = optstr__new_app;
            goto error_unexpec_arg_long;
          }
          _medit_opt_new_app = 1;
          break;
        }
        goto error_unknown_long_opt;
       case 'u':
        if (strncmp (option + 1, optstr__unique + 1, option_len - 1) == 0)
        {
          if (argument != 0)
          {
            option = optstr__unique;
            goto error_unexpec_arg_long;
          }
          _medit_opt_unique = 1;
          break;
        }
        goto error_unknown_long_opt;
       case 'v':
        if (strncmp (option + 1, optstr__version + 1, option_len - 1) == 0)
        {
          if (argument != 0)
          {
            option = optstr__version;
            goto error_unexpec_arg_long;
          }
          _medit_opt_version = 1;
          return i + 1;
        }
       default:
       error_unknown_long_opt:
        fprintf (stderr, STR_ERR_UNKNOWN_LONG_OPT, program_name, option);
        return -1;
       error_unexpec_arg_long:
        fprintf (stderr, STR_ERR_UNEXPEC_ARG_LONG, program_name, option);
        return -1;
      }
    }
    else
      do
      {
        switch (*option)
        {
         case 'h':
          _medit_opt_help = 1;
          return i + 1;
         case 'l':
          if (option [1] != '\0')
          {
            _medit_arg_log = option + 1;
            option = "\0";
          }
          else
            _medit_arg_log = 0;
          _medit_opt_log = 1;
          break;
         case 'n':
          _medit_opt_new_app = 1;
          break;
         case 'u':
          _medit_opt_unique = 1;
          break;
         default:
          fprintf (stderr, STR_ERR_UNKNOWN_SHORT_OPT, program_name, *option);
          return -1;
        }
      } while (*++option != '\0');
  }
  return i;
}
/* end of generated code
 ********************************************************/


static void
usage (void)
{
    g_print ("Usage: %s [OPTIONS] [FILES]\n", g_get_prgname ());
    g_print ("Options:\n");

    g_print ("%s", STR_HELP_UNIQUE);
    g_print ("%s", STR_HELP_NEW_APP);
    g_print ("%s", STR_HELP_LOG);
    g_print ("%s", STR_HELP_VERSION);
    g_print ("%s", STR_HELP_HELP);
}

static void
version (void)
{
    g_print ("medit %s\n", MOO_VERSION);
}


int
main (int argc, char *argv[])
{
    MooApp *app;
    int opt_remain;
    MooEditor *editor;
    char **files;
    gpointer window;
    int retval;
    gboolean new_instance;

    gtk_init (&argc, &argv);
//     gdk_window_set_debug_updates (TRUE);

    opt_remain = _medit_parse_options (g_get_prgname (), argc, argv);

    if (opt_remain < 0)
    {
        usage ();
        return 1;
    }

    if (_medit_opt_help)
    {
        usage ();
        return 0;
    }
    else if (_medit_opt_version)
    {
        version ();
        return 0;
    }

    if (_medit_opt_log)
    {
        if (_medit_arg_log)
            moo_set_log_func_file (_medit_arg_log);
        else
            moo_set_log_func_window (TRUE);
    }

    if (_medit_opt_unique)
        new_instance = FALSE;
    else if (_medit_opt_new_app)
        new_instance = TRUE;
    else
#if DEFAULT_NEW_INSTANCE
        new_instance = TRUE;
#else
        new_instance = FALSE;
#endif

    files = moo_filenames_from_locale (argv + opt_remain);

    app = g_object_new (MOO_TYPE_APP,
                        "argv", argv,
                        "short-name", "medit",
                        "full-name", "medit",
                        "description", "medit is a text editor",
                        "website", "http://mooedit.sourceforge.net/",
                        "website-label", "http://mooedit.sourceforge.net/",
                        "default-ui", MEDIT_UI,
                        "logo", MOO_STOCK_MEDIT,
                        NULL);

    if ((!new_instance && moo_app_send_files (app, files)) ||
         !moo_app_init (app))
    {
        gdk_notify_startup_complete ();
        g_strfreev (files);
        g_object_unref (app);
        return 0;
    }

    editor = moo_app_get_editor (app);
    window = moo_editor_new_window (editor);

    if (files && *files)
    {
        char **p;

        for (p = files; p && *p; ++p)
            moo_editor_new_file (editor, window, NULL, *p, NULL);
    }

    g_strfreev (files);

    g_signal_connect_swapped (editor, "all-windows-closed",
                              G_CALLBACK (moo_app_quit), app);

    retval = moo_app_run (app);

    g_object_unref (app);
    return retval;
}
