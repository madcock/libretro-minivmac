#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <retro_dirent.h>

/* ===============================================================
 *
 *                          GUI
 *
 * ===============================================================*/

#define MAX_PATH_LEN 512
struct file_browser {
    /* path */
    char file[MAX_PATH_LEN];
    char home[MAX_PATH_LEN];
    char desktop[MAX_PATH_LEN];
    char directory[MAX_PATH_LEN];

    /* directory content */
    char **files;
    char **directories;
    size_t file_count;
    size_t dir_count;
};


static struct file_browser browser;

#ifdef __unix__
#include <unistd.h>
#endif

#ifndef _WIN32
# include <pwd.h>
#endif

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

#ifndef __LIBRETRO__
static char*
file_load(const char* path, size_t* siz)
{
    char *buf;
    FILE *fd = fopen(path, "rb");
    if (!fd) die("Failed to open file: %s\n", path);
    fseek(fd, 0, SEEK_END);
    *siz = (size_t)ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buf = (char*)calloc(*siz, 1);
    fread(buf, *siz, 1, fd);
    fclose(fd);
    return buf;
}
#endif

static char*
str_duplicate(const char *src)
{
    char *ret;
    size_t len = strlen(src);
    if (!len) return 0;
    ret = (char*)malloc(len+1);
    if (!ret) return 0;
    memcpy(ret, src, len);
    ret[len] = '\0';
    return ret;
}

static void
dir_free_list(char **list, size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        free(list[i]);
    free(list);
}

static char**
dir_list(const char *dir, int return_subdirs, size_t *count)
{
    size_t n = 0;
    char buffer[MAX_PATH_LEN];
    char **results = NULL;
    size_t capacity = 32;
    size_t size;
    struct RDIR *zh = NULL;

    assert(dir);
    assert(count);
    strncpy(buffer, dir, MAX_PATH_LEN);
    n = strlen(buffer);

    if (n > 0 && (buffer[n-1] != '/'))
        buffer[n++] = '/';

    size = 0;

    zh = retro_opendir(dir);
    if (zh != NULL) {
        int nonempty = 1;
	nonempty = retro_readdir(zh);
        if (!nonempty) return NULL;

        do {
            char *p;
            int is_subdir = 0;
	    const char *name = NULL;
	    name = retro_dirent_get_name(zh);
            if (name[0] == '.')
                continue;

	    is_subdir = retro_dirent_is_dir(zh, NULL);

            if ((return_subdirs && is_subdir) || (!is_subdir && !return_subdirs)){
                if (!size) {
                    results = (char**)calloc(sizeof(char*), capacity);
                } else if (size >= capacity) {
                    void *old = results;
                    capacity = capacity * 2;
                    results = (char**)realloc(results, capacity * sizeof(char*));
                    assert(results);
                    if (!results) free(old);
                }
                p = str_duplicate(name);
                results[size++] = p;
            }
        } while (zh != NULL && retro_readdir(zh));
    }
    retro_closedir(zh);
    *count = size;
    return results;
}

static void
file_browser_reload_directory_content(struct file_browser *browser, const char *path)
{
    strncpy(browser->directory, path, MAX_PATH_LEN);
    dir_free_list(browser->files, browser->file_count);
    dir_free_list(browser->directories, browser->dir_count);
    browser->files = dir_list(path, 0, &browser->file_count);
    browser->directories = dir_list(path, 1, &browser->dir_count);
}

static void
file_browser_init(struct file_browser *browser)
{
    memset(browser, 0, sizeof(*browser));

    {
        /* load files and sub-directory list */
        const char *home = getenv("HOME");
#if defined(PS2) || defined(_3DS) || defined(SWITCH) || defined(SF2000) // TODO: Fix directory
    	if (!home) home = "/";
#elif defined(GEKKO)
    	if (!home) home = "/sd:";
#elif defined(VITA)
	if (!home) home = "/ux0:";
#elif defined(_WIN32)
        if (!home) home = getenv("USERPROFILE");
#else
        if (!home) home = getpwuid(getuid())->pw_dir;
        {
            size_t l;
            strncpy(browser->home, home, MAX_PATH_LEN);
            l = strlen(browser->home);
            strcpy(browser->home + l, "/");
            strcpy(browser->directory, browser->home);

        }
#endif
        {
            size_t l;
            strcpy(browser->desktop, browser->home);
            l = strlen(browser->desktop);
            strcpy(browser->desktop + l, "Desktop/");
        }
        browser->files = dir_list(browser->directory, 0, &browser->file_count);
        browser->directories = dir_list(browser->directory, 1, &browser->dir_count);
    }
}

static void
file_browser_free(struct file_browser *browser)
{
    if (browser->files)
        dir_free_list(browser->files, browser->file_count);
    if (browser->directories)
        dir_free_list(browser->directories, browser->dir_count);
    browser->files = NULL;
    browser->directories = NULL;
    memset(browser, 0, sizeof(*browser));
}

void filebrowser_init()
{
   
    file_browser_init(&browser);
}

void filebrowser_free()
{

    file_browser_free(&browser);

}

