#define MOO_DO_NOT_MANGLE_GLIB_FUNCTIONS
#include <mooglib/moo-glib.h>
#include <mooglib/moo-time.h>
#include <mooglib/moo-stat.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __WIN32__
#include <io.h>
#endif // __WIN32__

const mgw_errno_t MGW_E_NOERROR = { MGW_ENOERROR };
const mgw_errno_t MGW_E_EXIST   = { MGW_EEXIST };


static mgw_time_t convert_time_t (time_t t)
{
    mgw_time_t result = { t };
    return result;
}


static void convert_g_stat_buf (const GStatBuf* gbuf, MgwStatBuf* mbuf)
{
    mbuf->atime = convert_time_t (gbuf->st_atime);
    mbuf->mtime = convert_time_t (gbuf->st_mtime);
    mbuf->ctime = convert_time_t (gbuf->st_ctime);

    mbuf->size = gbuf->st_size;

    mbuf->isreg = S_ISREG (gbuf->st_mode);
    mbuf->isdir = S_ISDIR (gbuf->st_mode);

#ifdef S_ISLNK
    mbuf->islnk = S_ISLNK (gbuf->st_mode);
#else
    mbuf->islnk = 0;
#endif

#ifdef S_ISSOCK
    mbuf->issock = S_ISSOCK (gbuf->st_mode);
#else
    mbuf->issock = 0;
#endif

#ifdef S_ISFIFO
    mbuf->isfifo = S_ISFIFO (gbuf->st_mode);
#else
    mbuf->isfifo = 0;
#endif

#ifdef S_ISCHR
    mbuf->ischr = S_ISCHR (gbuf->st_mode);
#else
    mbuf->ischr = 0;
#endif

#ifdef S_ISBLK
    mbuf->isblk = S_ISBLK (gbuf->st_mode);
#else
    mbuf->isblk = 0;
#endif
}


#define call_with_errno(err__, func__, rtype__, ...)    \
({                                                      \
    rtype__ result__;                                   \
    errno = 0;                                          \
    result__ = (func__) (__VA_ARGS__);                  \
    if ((err__) != NULL)                                \
        (err__)->value = errno;                         \
    result__;                                           \
})


const char *
mgw_strerror (mgw_errno_t err)
{
    return g_strerror (err.value);
}

GFileError
mgw_file_error_from_errno (mgw_errno_t err)
{
    return g_file_error_from_errno (err.value);
}


int
mgw_stat (const gchar *filename, MgwStatBuf *buf, mgw_errno_t *err)
{
    GStatBuf gbuf = { 0 };
    int result = call_with_errno (err, g_stat, int, filename, &gbuf);
    convert_g_stat_buf (&gbuf, buf);
    return result;
}

int
mgw_lstat (const gchar *filename, MgwStatBuf *buf, mgw_errno_t *err)
{
    GStatBuf gbuf = { 0 };
    int result = call_with_errno (err, g_lstat, int, filename, &gbuf);
    convert_g_stat_buf (&gbuf, buf);
    return result;
}


const struct tm *
mgw_localtime (const mgw_time_t *timep)
{
    time_t t = timep->value;
    return localtime(&t);
}

#ifdef __WIN32__
static struct tm *
localtime_r (const time_t *timep,
             struct tm *result)
{
    struct tm *res;
    res = localtime (timep);
    if (res)
        *result = *res;
    return res;
}
#endif

const struct tm *
mgw_localtime_r (const mgw_time_t *timep, struct tm *result, mgw_errno_t *err)
{
    time_t t = timep->value;
    return call_with_errno (err, localtime_r, struct tm*, &t, result);
}

mgw_time_t
mgw_time (mgw_time_t *t, mgw_errno_t *err)
{
    time_t t1;
    mgw_time_t result = { call_with_errno (err, time, time_t, &t1) };
    if (t != NULL)
        t->value = t1;
    return result;
}


guint64
mgw_ascii_strtoull (const gchar *nptr, gchar **endptr, guint base, mgw_errno_t *err)
{
    return call_with_errno (err, g_ascii_strtoull, guint64, nptr, endptr, base);
}

gdouble
mgw_ascii_strtod (const gchar *nptr, gchar **endptr, mgw_errno_t *err)
{
    return call_with_errno (err, g_ascii_strtod, double, nptr, endptr);
}


MGW_FILE *
mgw_fopen (const char *filename, const char *mode, mgw_errno_t *err)
{
    return (MGW_FILE*) call_with_errno (err, g_fopen, FILE*, filename, mode);
}

int mgw_fclose (MGW_FILE *file)
{
    return fclose ((FILE*) file);
}

gsize
mgw_fread(void *ptr, gsize size, gsize nmemb, MGW_FILE *stream, mgw_errno_t *err)
{
    return call_with_errno (err, fread, gsize, ptr, size, nmemb, (FILE*) stream);
}

gsize
mgw_fwrite(const void *ptr, gsize size, gsize nmemb, MGW_FILE *stream)
{
    return fwrite (ptr, size, nmemb, (FILE*) stream);
}

int
mgw_ferror (MGW_FILE *file)
{
    return ferror ((FILE*) file);
}

char *
mgw_fgets(char *s, int size, MGW_FILE *stream)
{
    return fgets(s, size, (FILE*) stream);
}


MgwFd
mgw_open (const char *filename, int flags, int mode)
{
    MgwFd fd = { g_open (filename, flags, mode) };
    return fd;
}

int
mgw_close (MgwFd fd)
{
    return close (fd.value);
}

gssize
mgw_write(MgwFd fd, const void *buf, gsize count)
{
    return write (fd.value, buf, count);
}

int
mgw_pipe (MgwFd *fds)
{
    int t[2];

#ifndef __WIN32__
    int result = pipe (t);
#else
    int result = _pipe(t, 4096, O_BINARY);
#endif

    fds[0].value = t[0];
    fds[1].value = t[1];
    return result;
}

void
mgw_perror (const char *s)
{
    perror (s);
}


int
mgw_unlink (const char *path, mgw_errno_t *err)
{
    return call_with_errno (err, g_unlink, int, path);
}

int
mgw_remove (const char *path, mgw_errno_t *err)
{
    return call_with_errno (err, g_remove, int, path);
}

int
mgw_rename (const char *oldpath, const char *newpath, mgw_errno_t *err)
{
    return call_with_errno (err, g_rename, int, oldpath, newpath);
}

int
mgw_mkdir (const gchar *filename, int mode, mgw_errno_t *err)
{
    return call_with_errno (err, g_mkdir, int, filename, mode);
}

int
mgw_mkdir_with_parents (const gchar *pathname, gint mode, mgw_errno_t *err)
{
    return call_with_errno (err, g_mkdir_with_parents, int, pathname, mode);
}

int
mgw_access (const char *path, mgw_access_mode_t mode)
{
    int gmode = F_OK;
    if (mode.value & MGW_R_OK)
        gmode |= R_OK;
    if (mode.value & MGW_W_OK)
        gmode |= W_OK;
    if (mode.value & MGW_X_OK)
        gmode |= X_OK;
    return g_access (path, gmode);
}


gboolean
mgw_spawn_async_with_pipes (const gchar *working_directory,
                            gchar **argv,
                            gchar **envp,
                            GSpawnFlags flags,
                            GSpawnChildSetupFunc child_setup,
                            gpointer user_data,
                            GPid *child_pid,
                            MgwFd *standard_input,
                            MgwFd *standard_output,
                            MgwFd *standard_error,
                            GError **error)
{
    return g_spawn_async_with_pipes (working_directory, argv, envp, flags,
                                     child_setup, user_data, child_pid,
                                     standard_input ? &standard_input->value : NULL,
                                     standard_output ? &standard_output->value : NULL,
                                     standard_error ? &standard_error->value : NULL,
                                     error);
}

GIOChannel *
mgw_io_channel_unix_new (MgwFd fd)
{
    return g_io_channel_unix_new (fd.value);
}

#ifdef __WIN32__
GIOChannel *
mgw_io_channel_win32_new_fd (MgwFd fd)
{
    return g_io_channel_win32_new_fd (fd.value);
}
#endif // __WIN32__
