/*
  Copyright (c) 2023 djcj <djcj@gmx.de>

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions: 

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software. 

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <string>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <features.h>
#include <ftw.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#ifdef __BSD__
#include <limits.h>
#include <sys/sysctl.h>
#endif

#include "launcher.hpp"
#include "res.h"  /* fallback icon resource */


#define LOG(MSG, ...)  if (launcher::verbose()) {printf(MSG "\n", __VA_ARGS__);}


bool launcher::m_verbose = false;

int movebox::handle(int e)
{
    switch (e) {
        case FL_PUSH:
            fl_cursor(FL_CURSOR_MOVE);
            m_event_x = Fl::event_x();
            m_event_y = Fl::event_y();
            return e;  /* return non-zero */
        case FL_DRAG:
            window()->position(
                Fl::event_x_root() - m_event_x,
                Fl::event_y_root() - m_event_y);
            break;
        case FL_RELEASE:
            fl_cursor(FL_CURSOR_DEFAULT);
            break;
    }

    return Fl_Widget::handle(e);
}

int logobutton::handle(int e)
{
    switch (e) {
        case FL_ENTER:
            set_color(m_col);
            break;
        case FL_LEAVE:
            set_color(MARATHON_GREEN);
            break;
    }

    return Fl_Button::handle(e);
}

void logobutton::set_color(Fl_Color col)
{
    if (!m_l) return;

    circle *o1 = m_l->logo1();
    circle *o2 = m_l->logo2();

    if (o1 && o2) {
        o1->color(col);
        o2->color(col);
        parent()->redraw();
    }
}

int launcher_window::handle(int e)
{
    if (m_l && e == FL_ENTER) {
        circle *o1 = m_l->logo1();
        circle *o2 = m_l->logo2();

        if (o1 && o2) {
            o1->color(MARATHON_GREEN);
            o2->color(MARATHON_GREEN);
            redraw();
        }
    }

    return Fl_Double_Window::handle(e);
}

void launcher::print_fltk_version()
{
    const int n = Fl::api_version();
    printf("Using FLTK v%d.%d.%d - https://www.fltk.org\n", n/10000, (n/100) % 100, n%100);
}

inline void launcher::error_message(const char *msg)
{
    fl_message_title("Error");
    fl_alert("%s", msg);
}

inline int launcher::command(const char *cmd)
{
    Fl::flush();
    LOG("+ %s", cmd);
    return system(cmd);
}

/* returns "$HOME/.alephone/"; assert if m_home is NULL */
inline std::string launcher::confdir() const
{
    /* this error message will be more meaningful
     * than the one from std::string() */
    assert(m_home != NULL);

    return std::string(m_home) + "/.alephone/";
}

bool launcher::default_icon_png(const char *path)
{
    if (m_png) delete m_png;
    m_png = new Fl_PNG_Image(path);

    if (m_png->fail()) {
        LOG("cannot load: %s", path);
        delete m_png;
        m_png = NULL;
        return false;
    }

    LOG("loaded: %s", path);
    Fl_Window::default_icon(m_png);

    return true;
}

/* look for PNG icons in the following order:
 * <application path> + ".png"
 * $HOME/.alephone/alephone.png
 * /usr/share/icons/hicolor/<...>/apps/alephone.png
 * /usr/share/pixmaps/alephone.png
 */
void launcher::load_default_icon()
{
    /* look for PNG file with the same name as the executable
     * plus ".png" extension */

    std::string prog = get_progname_png();

    if (!prog.empty() && default_icon_png(prog.c_str())) {
        return;
    }

    std::string path = get_self_exe_png();

    if (!path.empty() && path != prog && default_icon_png(path.c_str())) {
        return;
    }

    /* look for ~/.alephone/alephone.png */
    path = confdir() + "alephone.png";
    if (default_icon_png(path.c_str())) return;

    /* look for "alephone.png" in system directories */
#define GET_HICOLOR(RES) default_icon_png("/usr/share/icons/hicolor/" RES "x" RES "/apps/alephone.png")

    if (GET_HICOLOR("512")) return;
    if (GET_HICOLOR("256")) return;
    if (GET_HICOLOR("128")) return;
    if (GET_HICOLOR("64")) return;
    if (GET_HICOLOR("48")) return;
    if (GET_HICOLOR("32")) return;
    if (GET_HICOLOR("24")) return;
    if (GET_HICOLOR("22")) return;
    if (GET_HICOLOR("16")) return;

    if (default_icon_png("/usr/share/pixmaps/alephone.png")) return;

    /* fall back to embedded default icon */

    /* "input-gaming.svg" from Tango Icon Library, converted to PNG
     * released into the Public Domain
     * http://tango.freedesktop.org/Tango_Icon_Library
     */
    if (m_png) delete m_png;
    m_png = new Fl_PNG_Image(NULL, input_gaming_png, input_gaming_png_len);

    if (m_png->fail()) {
        delete m_png;
        m_png = NULL;
    } else {
        Fl_Window::default_icon(m_png);
    }
}

/* recursively remove <dir> inside "$HOME/.alephone" without following
 * symbolic links; this is same as the command "rm -rf ~/.alephone/<dir>"
 */
bool launcher::remove_data(const char *dir)
{
    auto lambda = [] (const char *fpath, const struct stat *, int, struct FTW *) -> int {
        return remove(fpath);
    };

    const int flags = FTW_DEPTH | FTW_MOUNT | FTW_PHYS;
    std::string path = confdir() + dir;

    LOG("delete: %s", path.c_str());

    if (nftw(path.c_str(), lambda, 20, flags) != 0 && errno != ENOENT) {
        fl_message_title("Error");
        fl_alert("Failed to delete:\n%s", path.c_str());
        return false;
    }

    return true;
}

/* returns true ONLY if the given path is confirmed to
 * be a directory that is not empty (symbolic links are resolved)
 */
bool launcher::is_full_directory(const char *path)
{
    struct stat st;
    struct dirent *d;
    DIR *dirp;

    if (stat(path, &st) != 0 ||
        !S_ISDIR(st.st_mode) ||
        (dirp = opendir(path)) == NULL)
    {
        return false;
    }

    while ((d = readdir(dirp)) != NULL) {
        if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) {
            /* directory is NOT empty */
            closedir(dirp);
            return true;
        }
    }

    closedir(dirp);

    /* empty directory or something else */
    return false;
}

/* check if ALL game data directories exist:
 * ~/.alephone/data-marathon-master
 * ~/.alephone/data-marathon-2-master
 * ~/.alephone/data-marathon-infinity-master
 */
bool launcher::all_directories_exist()
{
    const char *paths[3] = {
        "data-marathon-master",
        "data-marathon-2-master",
        "data-marathon-infinity-master"
    };

    for (int i = 0; i < 3; i++) {
        std::string path = confdir() + paths[i];

        if (!is_full_directory(path.c_str())) {
            return false;
        }
    }

    return true;
}

/* use a custom script to download the Marathon game files;
 * checks for tools or the existence of paths must be done
 * by the script;
 * this can be used i.e. to download using a different tool
 * like such as git or to download from a different website
 */
void launcher::script(const char *p)
{
    if (!p || *p == 0) return;
    m_script = p;
    LOG("using custom download script: %s", m_script);
}

/* open xterm and download the game data;
 * returning "true" means the window icon should be reloaded
 */
bool launcher::download()
{
    char buf[256];
    const char *fmt = "xterm "
        "-title 'Download (close window to abort)' " /* window title */
        "-geometry 100x30+%d+%d "                    /* size + position */
        "-l -lf ~/.alephone/download.log "           /* log output to file */
        "-e '";

    /* create ~/.alephone */
    mkdir(confdir().c_str(), 0775);

    /* delete log file */
    std::string s = confdir() + "download.log";
    LOG("delete: %s", s.c_str());
    remove(s.c_str());

    /* run custom download script */
    if (m_script) {
        fl_message_title("Custom download script");
        const char *msg = "Do you want to (re-)download the game files using this custom script?";

        /* always ask when using a custom script */
        if (fl_ask("%s\n\n>> %s", msg, m_script) == 0) {
            return false;
        }

        sprintf(buf, fmt, m_win->x_root(), m_win->y_root());
        s = std::string(buf) + "sh -c " + m_script;
        s += " ; set +x; echo; echo \"Press ENTER to close window\"; read x'";
        command(s.c_str());

        return true;
    }

    /* check for curl or wget */
    if (command("wget --version 2>/dev/null >/dev/null") != 0) {
        error_message("`wget' is required to download the game files.");
        return false;
    }

    /* ask the user if they want to download everything again */
    if (all_directories_exist()) {
        fl_message_title("Download again?");

        if (fl_ask("%s", "Do you want to re-download the game files?") == 0) {
            return false;
        }
    }

    /* delete existing data directories */
    if (!remove_data("data-marathon-master")) return false;
    if (!remove_data("data-marathon-2-master")) return false;
    if (!remove_data("data-marathon-infinity-master")) return false;

    /* delete icon */
    s = confdir() + "alephone.png";
    LOG("delete: %s", s.c_str());
    remove(s.c_str());

#define ICON_URL "https://raw.githubusercontent.com/Aleph-One-Marathon/alephone/5653d64ba12f2cf058abcd8fd9ec2f06bcae9839/flatpak/alephone.png"
#define REPO "https://github.com/Aleph-One-Marathon/data-marathon"
#define MARATHON_DL(x) "(wget -O- " REPO x "/archive/refs/heads/master.tar.gz | tar xfz - -C ~/.alephone)"

    const char *default_script =
        "set -x;"
        "("
            /* ignore error on icon download but not on game data */
            "wget -O ~/.alephone/alephone.png " ICON_URL " ;"
            MARATHON_DL("") " && "
            MARATHON_DL("-2") " && "
            MARATHON_DL("-infinity")
        ") || ("
            "set +x;"
            "echo;"
            "echo \"Press ENTER to close window\";"
            "read x"
        ")"
        "'";

    /* run command */
    sprintf(buf, fmt, m_win->x_root(), m_win->y_root());
    s = std::string(buf) + default_script;
    command(s.c_str());

    return true;
}

/* returns program invocation name + ".png", but only
 * if the invocation name contains path separators
 */
std::string launcher::get_progname_png()
{
#ifdef __BSD__
    size_t buflen = MAXPATHLEN * 2;
    char buf[buflen + 1];
#endif
    char *path = NULL;


#ifdef __GLIBC__

    path = program_invocation_name;

#elif defined(__FreeBSD__) || defined(__DragonFly__)

    int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ARGS, -1};

    if (sysctl(name, 4, buf, &buflen, NULL, 0) == 0) {
        path = buf;
    }

#elif defined(__NetBSD__)

    int name[4] = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV};

    if (sysctl(name, 4, buf, &buflen, NULL, 0) == 0) {
        path = buf;
    }

#elif defined(__OpenBSD__)

    int name[4] = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV};
    char **p = NULL;

    if (sysctl(name, 4, buf, &buflen, NULL, 0) == 0) {
        p = reinterpret_cast<char **>(buf);
        if (p && p[0]) path = p[0];
    }

#endif

    if (path && path[0] != 0 && strchr(path, '/')) {
        return std::string(path) + ".png";
    }

    return "";
}

/* returns resolved path to executable + ".png" */
std::string launcher::get_self_exe_png()
{
    std::string path;
    char *rp = NULL;

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__)

#if defined(__NetBSD__)
    int name[4] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_PATHNAME };
#else
    int name[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
#endif
    char buf[MAXPATHLEN+1];
    size_t buflen = MAXPATHLEN;

    if (sysctl(name, 4, buf, &buflen, NULL, 0) == 0) {
        rp = realpath(buf, NULL);
    }

#elif defined(__linux__)

    rp = realpath("/proc/self/exe", NULL);

#endif

    if (rp) {
        path = std::string(rp) + ".png";
        free(rp);
    }

    return path;
}

/* the "download" button was clicked */
void launcher::download_cb(Fl_Widget *o, void *p)
{
    launcher *l = reinterpret_cast<launcher *>(p);
    Fl::hide_all_windows();
    if (l->download()) l->load_default_icon();
    o->window()->show();
}

/* start a Marathon game; "alephone" is expected to be in PATH */
void launcher::launch_cb(Fl_Widget *o, void *p)
{
    Fl::hide_all_windows();

    if (command((const char *)p) != 0 &&
        command("alephone --version 2>/dev/null >/dev/null") != 0)
    {
        error_message("`alephone' is not in PATH");
    }

    o->window()->show();
}

/* create a window but don't show() it yet */
void launcher::make_window(bool system_colors)
{
    Fl_Widget *o;
    const int y = 220;

    if (system_colors) {
        Fl::get_system_colors();
    } else {
        const uchar u = 60;
        Fl::background(u,u,u);
        Fl::background2(u,u,u);
    }

    Fl::scheme("gtk+");

    /* window begin */
    m_win = new launcher_window(234, 145+y, this, "Marathon Launcher");
    m_win->begin();

    /* Aleph One logo */
    m_cirlce_o1 = new circle((m_win->w() - 200)/2, 10, 200, MARATHON_GREEN);
    new circle((m_win->w() - 130)/2, 18, 130, m_win->color());
    m_cirlce_o2 = new circle((m_win->w() - 105)/2, 30, 105, MARATHON_GREEN);
    new Fl_Box(FL_FLAT_BOX, (m_win->w() - 20)/2, 145, 20, 66, NULL);

    /* set this above the logo but below the buttons */
    new movebox(0, 0, m_win->w(), m_win->h());

    /* Marathon Trilogy */
    o = new logobutton(10, y, m_win->w()-20, 30, MARATHON_BLUE, this, "Marathon");
    o->callback(launch_cb, (void *)"alephone ~/.alephone/data-marathon-master");

    o = new logobutton(10, 30+y, m_win->w()-20, 30, MARATHON_YELLOW, this, "Marathon 2: Durandal");
    o->callback(launch_cb, (void *)"alephone ~/.alephone/data-marathon-2-master");

    o = new logobutton(10, 60+y, m_win->w()-20, 30, MARATHON_GRAY, this, "Marathon Infinity");
    o->callback(launch_cb, (void *)"alephone ~/.alephone/data-marathon-infinity-master");

    const int w2 = (m_win->w() - 20) / 2;
    const int y2 = m_win->h() - 40;

    /* Download Files */
    o = new Fl_Button(10, y2, w2-1, 30, "Download Files");
    o->box(BOXTYPE);
    o->labelsize(13);
    o->callback(download_cb, this);

    /* Github */
    auto github_cb = [] (Fl_Widget *) {
        command("xdg-open https://github.com/Aleph-One-Marathon 2>/dev/null >/dev/null");
    };

    o = new Fl_Button(w2+11, y2, w2, 30, "Visit Github");
    o->box(BOXTYPE);
    o->labelsize(13);
    o->callback(github_cb);

    /* window end */
    m_win->end();
    m_win->clear_visible_focus();

    /* screen center */
    m_win->position((Fl::w() - m_win->decorated_w()) / 2, (Fl::h() - m_win->decorated_h()) / 2);
}

/* load icon and show() the window */
int launcher::run()
{
    load_default_icon();
    m_win->show();

    return Fl::run();
}

static void print_help(const char *argv0)
{
    const char *msg =
        "usage: %s --help\n"
        "       %s [--verbose] [--download-script=SCRIPT] [--no-system-colors]\n"
        "\n"
        "SCRIPT must be a shell script that downloads the game data into the\n"
        "directories listed below.\n"
        "\n"
        "\n"
        "Aleph One config directory:\n"
        "  ~/.alephone\n"
        "\n"
        "Search/download paths for...\n"
        "  Marathon:           ~/.alephone/data-marathon-master\n"
        "  Marathon 2:         ~/.alephone/data-marathon-2-master\n"
        "  Marathon Infinity:  ~/.alephone/data-marathon-infinity-master\n"
        "\n"
        "Download log file:\n"
        "  ~/.alephone/download.log\n"
        "\n"
        "Icon lookup paths:\n";

    printf(msg, argv0, argv0);

    std::string prog = launcher::get_progname_png();

    if (!prog.empty()) {
        printf("  %s\n", prog.c_str());
    }

    std::string self = launcher::get_self_exe_png();

    if (!self.empty() && self != prog) {
        printf("  %s\n", self.c_str());
    }

    puts("  ~/.alephone/alephone.png  (will be overwritten on new game downloads)\n"
        "  /usr/share/icons/hicolor/<...>/apps/alephone.png\n"
        "  /usr/share/pixmaps/alephone.png\n");

    launcher::print_fltk_version();
}

int main(int argc, char **argv)
{
    bool arg_system_colors = true;
    bool arg_verbose = false;
    const char *arg_script = NULL;

    for (int i=1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            arg_verbose = true;
        } else if (strncmp(argv[i], "--download-script=", 18) == 0) {
            arg_script = argv[i] + 18;
        } else if (strcmp(argv[i], "--no-system-colors") == 0) {
            arg_system_colors = false;
        } else {
            fprintf(stderr, "unknown argument ignored: %s\n", argv[i]);
        }
    }

    launcher l(arg_system_colors);
    l.verbose(arg_verbose);
    l.script(arg_script);

    return l.run();
}
