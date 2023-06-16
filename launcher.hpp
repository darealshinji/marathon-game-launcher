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

#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_draw.H>

/* disable "deprecated" warning for fl_ask :-) */
//#include <FL/fl_ask.H>
extern int fl_ask(const char *, ...) __fl_attr((__format__(__printf__, 1, 2)));
extern void fl_alert(const char *, ...) __fl_attr((__format__(__printf__, 1, 2)));
extern void fl_message(const char *, ...) __fl_attr((__format__(__printf__, 1, 2)));
extern void fl_message_title(const char *title);

#define MARATHON_GREEN   fl_rgb_color( 69, 199,   5)
#define MARATHON_BLUE    fl_rgb_color(  6, 118, 230)
#define MARATHON_YELLOW  fl_rgb_color(227, 188,   0)
#define MARATHON_GRAY    fl_rgb_color(149, 149, 149)

#define BOXTYPE FL_THIN_UP_BOX

class circle;
class movebox;
class logobutton;
class launcher_window;
class launcher;


/* simple class to draw circles;
 * used to create the Marathon logo */
class circle : public Fl_Widget
{
private:
    int m_d = 0;

public:

    circle(int X, int Y, int D, Fl_Color C)
    : Fl_Widget(X,Y,0,0), m_d(D)
    {
        color(C);
    }

    virtual ~circle() {}

protected:

    int d() const {return m_d;}

    void draw() {
        fl_color(color());
        fl_draw_circle(x(), y(), d(), color());
    }
};

/* simple class that allows to move the
 * window when clicked on it */
class movebox : public Fl_Widget
{
private:

    int m_event_x = 0;
    int m_event_y = 0;

public:

    movebox(int X, int Y, int W, int H)
    : Fl_Widget(X,Y,W,H)
    {}

    virtual ~movebox() {}

protected:

    int handle(int e);
    void draw() {}
};

/* subclass of Fl_Button that changes the color of the
 * Marathon logo when the mouse button is above it */
class logobutton : public Fl_Button
{
private:

    Fl_Color m_col = MARATHON_GREEN;
    launcher *m_l = NULL;

public:

    logobutton(int X, int Y, int W, int H, Fl_Color col, launcher *o, const char *L)
    : Fl_Button(X,Y,W,H,L), m_col(col), m_l(o)
    {
        box(BOXTYPE);
        labelsize(16);
    }

    virtual ~logobutton() {}

private:

    int handle(int e);
    void set_color(Fl_Color col);
};

/* subclass of Fl_Double_Window that sets the Marathon logo color
 * back to normal when re-entering the window */
class launcher_window : public Fl_Double_Window
{
private:

    launcher *m_l = NULL;

public:

    launcher_window(int W, int H, launcher *o, const char *L)
    : Fl_Double_Window(W,H,L), m_l(o)
    {}

    virtual ~launcher_window() {}

private:

    int handle(int e);
};

/* the launcher application */
class launcher
{
private:

    const char *m_home = NULL;
    Fl_PNG_Image *m_png = NULL;
    Fl_Double_Window *m_win = NULL;
    circle *m_cirlce_o1 = NULL;
    circle *m_cirlce_o2 = NULL;

    static bool m_verbose;
    const char *m_script = NULL;

    void make_window(bool system_colors);

public:

    static void print_fltk_version();

    launcher(bool system_colors)
    {
        /* need to set m_home and allocate
         * m_win before anything else */
        m_home = getenv("HOME");
        print_fltk_version();
        make_window(system_colors);
    }

    ~launcher() {
        if (m_win) delete m_win;
        if (m_png) delete m_png;
    }

    int run();
    bool download();

    void script(const char *p);
    circle *logo1() const {return m_cirlce_o1;}
    circle *logo2() const {return m_cirlce_o2;}

    void verbose(bool b) {m_verbose = b;}
    static bool verbose() {return m_verbose;}

    static std::string get_progname_png();
    static std::string get_self_exe_png();

private:

    static void error_message(const char *msg);
    static int command(const char *cmd);

    std::string confdir() const;
    void load_default_icon();
    bool default_icon_png(const char *path);
    bool all_directories_exist();
    bool is_full_directory(const char *path);
    bool remove_data(const char *dir);

    static void download_cb(Fl_Widget *o, void *p);
    static void launch_cb(Fl_Widget *o, void *p);
};

#endif /* LAUNCHER_HPP */
