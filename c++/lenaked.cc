/*
 * Lenaked - Adding nikud to hebrew.
 *
 * A program by Dov Grobgeld <dov.grobgeld@gmail.com>
 * This program is distributed under the GPL v3.0 license.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <map>
#include <set>
#include <vector>
#include "plis/plis.h"
#include "config.h"
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <iostream>
#include <codecvt>
#include <locale>
#endif
using namespace std;
using namespace plis;

static const char ui_descr_menu_string[] =
#include "menu-top-xml.i"
;

GtkWidget *w_mw = nullptr;
GtkWidget *nikud_action_box = nullptr;
GtkWidget *w_text_view = nullptr;
GtkWidget *w_opt_table = nullptr;
GtkWidget *w_menubox = nullptr;
GtkTextBuffer *text_buffer = nullptr;
vector<GtkWidget *> tab_buttons;

static void cb_menu_open (GtkAction *action, gpointer data);
static void cb_menu_open_corpus (GtkAction *action, gpointer data);
static void cb_menu_save (GtkAction *action, gpointer data);
static void cb_menu_about (GtkAction *action, gpointer data);
static void menu_add_widget (GtkUIManager *ui, GtkWidget *widget, GtkContainer *container);

static GtkActionEntry entries[] = 
{
  { "FileMenuAction", NULL, "_File" },                  /* name, stock id, label */
  { "HelpMenuAction", NULL, "_Help" },  
  { "OpenAction", GTK_STOCK_OPEN,
    "_Open", "<control>O",    
    "Open",
    G_CALLBACK (cb_menu_open) },
  { "OpenCorpusAction", GTK_STOCK_OPEN,
    "_OpenCorpus", "<control>Q",    
    "OpenCorupus",
    G_CALLBACK (cb_menu_open_corpus) },
  { "SaveAction", GTK_STOCK_SAVE,
    "_Save", "<control>S",    
    "save",
    G_CALLBACK (cb_menu_save) },
  { "QuitAction", GTK_STOCK_QUIT,
    "_Quit", "<control>Q",    
    "Quit",
    G_CALLBACK (gtk_main_quit) },
  { "AboutAction", GTK_STOCK_ABOUT,
    "_About", NULL,
    "About",
    G_CALLBACK (cb_menu_about) },
};

static GtkToggleActionEntry toggle_entries[] =
{
};

void cb_replace_selection(GtkWidget *widget, gpointer data);
constexpr int opt_width = 8;

map<slip, llip> nikud_db;

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt); 
    
    vfprintf(stderr, fmt, ap);
    exit(-1);
}

static void read_db(slip db_filename)
{
    ifstream inf(db_filename.c_str());
    nikud_db.clear();
    slip S_;
    while(inf >> S_) {
        llip match;
        if (S_.m("^(\\S+)\\s*=>\\s*\\[(.*?)\\]",match)) {
            slip w = match[1];
            llip word_list = match[2].split(" ");
            nikud_db[w] = word_list;
        }
    }
}

static slip canon(slip w)
{
    slip w_out = "";
    char *str = g_strdup(w.c_str());
    const char *p = str;
    // Get rid of everything except the letters
    while(p) {
        gunichar ch = g_utf8_get_char_validated(p, w.length()-(p-str));
        if (ch==0xfffffffe)
            break;
        if (ch >= 0x5d0 && ch <= 0x5ea) {
            gchar outbuf[6];
            int len = g_unichar_to_utf8(ch, outbuf);
            for (int i=0; i<len; i++)
                w_out += outbuf[i];
        }
        p = g_utf8_next_char(p);
    }
    g_free(str);

    return w_out;
}

void destroy_buttons() {
    for (auto t : tab_buttons) {
        gtk_widget_destroy(t);
    }
    tab_buttons.clear();
}

void create_matches()
{
    destroy_buttons();
    auto imark = gtk_text_buffer_get_insert(text_buffer);
    auto smark = gtk_text_buffer_get_selection_bound(text_buffer);
    GtkTextIter ins_iter, sel_iter;
    gtk_text_buffer_get_iter_at_mark(text_buffer, &ins_iter, imark);
    gtk_text_buffer_get_iter_at_mark(text_buffer, &sel_iter, smark);
    gchar *sel_text = gtk_text_buffer_get_text(text_buffer, &ins_iter, &sel_iter, TRUE);
    slip can_text = canon(sel_text);

    llip list;
    list.push_back(sel_text);
    g_free(sel_text);
    if (nikud_db.count(can_text)) {
        for (auto v : nikud_db[can_text])
            list.push_back(v);
    }

    set<slip> seen;
    
    for (auto l: list) {
        if (seen.count(l))
            continue;
        auto button = gtk_button_new_with_label(l);
        gtk_widget_set_name(button, "heb-button");
        g_signal_connect(button, "clicked", G_CALLBACK(cb_replace_selection), NULL);
        tab_buttons.push_back(button);
        seen.insert(l);
    }

    int idx = 0;

    for (auto b : tab_buttons) {
	int col = idx % opt_width;
	int row = int(idx/opt_width);
        gtk_table_attach(GTK_TABLE(w_opt_table),b,
                         col, col+1,
                         row,row+1,
                         GtkAttachOptions(GTK_FILL|GTK_EXPAND),
                         GtkAttachOptions(GTK_FILL|GTK_EXPAND),
                         0,0);
        gtk_widget_show(b);
        idx++;
    }
    //    print "sel_text = $sel_text\n";
}

void cb_start(GtkWidget *widget, gpointer data)
{
    // sweep one word at a time

    // mark the first word
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(text_buffer, &iter, 0);
    gtk_text_buffer_place_cursor(text_buffer, &iter);

    // Move selection mark one word forwards
    gtk_text_iter_forward_word_end(&iter);
    auto smark = gtk_text_buffer_get_mark(text_buffer, "selection_bound");
    gtk_text_buffer_move_mark(text_buffer, smark, &iter);

    create_matches();
}

void cb_license(GtkWidget */*widget*/, gpointer parent)
{
    auto dialog = gtk_dialog_new_with_buttons("License",
                                              GTK_WINDOW(parent),
                                              GTK_DIALOG_MODAL,
                                              GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                              NULL);
    auto text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    
    gtk_widget_set_size_request(dialog,500,300);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), text_view,
                       TRUE, TRUE, 0);
    auto buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_insert_at_cursor(buf,
        "Lenaked is free software: you can redistribute it and/or modify it "
        "under the terms of the GNU General Public License as published by the "
        "Free Software Foundation; either version 3 of the License, or (at your "
        "option) any later version.\n\n"
        "Lenaked is distributed in the hope that it will be useful, but WITHOUT "
        "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
        "FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License "
        "for more details.\n\n"
        "You should have received a copy of the GNU General Public License along "
        "with Lenaked. If not, see: http://www.gnu.org/licenses/\n",
        -1);
    gtk_widget_show(text_view);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void cb_next(GtkWidget */*widget*/, gpointer /*data*/)
{
    GtkTextIter iter;

    // Move selection mark one word forwards
    auto smark = gtk_text_buffer_get_selection_bound(text_buffer);
    gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, smark);
    gtk_text_iter_forward_word_end(&iter);
    gtk_text_iter_backward_word_start(&iter);
    gtk_text_buffer_place_cursor(text_buffer, &iter);
    gtk_text_iter_forward_word_end(&iter);
    gtk_text_buffer_move_mark(text_buffer, smark, &iter);
    create_matches();
}

void cb_prev(GtkWidget *widget, gpointer data)
{
    // Move selection mark one word forwards
    auto smark = gtk_text_buffer_get_mark(text_buffer, "selection_bound");
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(text_buffer, &iter, smark);
    gtk_text_iter_backward_word_starts(&iter, 2);
    gtk_text_buffer_place_cursor(text_buffer, &iter);
    gtk_text_iter_forward_word_end(&iter);
    gtk_text_buffer_move_mark(text_buffer, smark, &iter);
    create_matches();
}

void cb_stop(GtkWidget *widget, gpointer data)
{
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(text_buffer, &iter);
    gtk_text_buffer_place_cursor(text_buffer, &iter);
    destroy_buttons();
}

void cb_replace_selection(GtkWidget *widget, gpointer data)
{
    const gchar *text = gtk_button_get_label(GTK_BUTTON(widget));
    auto imark = gtk_text_buffer_get_insert(text_buffer);
    auto smark = gtk_text_buffer_get_selection_bound(text_buffer);
    GtkTextIter ins_iter, sel_iter;
    gtk_text_buffer_get_iter_at_mark(text_buffer, &ins_iter, imark);
    gtk_text_buffer_get_iter_at_mark(text_buffer, &sel_iter, smark);
    gtk_text_buffer_delete(text_buffer, &ins_iter,&sel_iter);
    gtk_text_buffer_insert_at_cursor(text_buffer, text, -1);

    cb_next(nullptr, nullptr);
}

void create_widgets()
{
    GError *error = NULL;
    GtkWidget *box = gtk_vbox_new(0,0);

    w_mw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_add(GTK_CONTAINER(w_mw), box);

    g_signal_connect(w_mw,"destroy",gtk_main_quit, NULL);
    gtk_window_set_title(GTK_WINDOW(w_mw),"Lenaked");
    gtk_widget_set_size_request(w_mw,500,600);

    auto action_group = gtk_action_group_new ("TestActions");
    w_menubox = gtk_vbox_new(0,0);

    auto menu_manager = gtk_ui_manager_new ();
    gtk_ui_manager_insert_action_group (menu_manager,
                                        action_group, 0);
    gtk_action_group_add_actions (action_group,
                                  entries, G_N_ELEMENTS(entries), w_mw);
    gtk_action_group_add_toggle_actions (action_group,
                                         toggle_entries,
                                         G_N_ELEMENTS(toggle_entries),
                                         w_mw);
    gtk_ui_manager_add_ui_from_string (menu_manager,
                                       ui_descr_menu_string,
                                       -1,
                                       &error);
    if (error)
        die("Got error: %s\n", error->message);

    /* This signal is necessary in order to place widgets from the UI manager
     * into the menu_box */
    g_signal_connect(menu_manager, 
                     "add_widget", 
                     G_CALLBACK (menu_add_widget), 
                     w_menubox
                     );
    gtk_box_pack_start(GTK_BOX(box),
                       w_menubox, FALSE, FALSE, 0);
    gtk_widget_show(w_menubox);

    gtk_widget_show(gtk_ui_manager_get_widget(menu_manager,
                                              "/menubar"));
    // Add a menu.
    auto paned = gtk_vpaned_new();
    gtk_box_pack_start(GTK_BOX(box), paned, TRUE, TRUE, 0);

    auto sw = gtk_scrolled_window_new(NULL,NULL);
    w_text_view = gtk_text_view_new();
    gtk_widget_set_size_request(sw, 500,300);
    gtk_widget_set_name(w_text_view,"heb-text");
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w_text_view), GTK_WRAP_WORD);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w_text_view));
    gtk_text_buffer_insert_at_cursor(text_buffer, "שלום לכל העולם!",-1);
    gtk_container_add(GTK_CONTAINER(sw), w_text_view);
    gtk_paned_add1(GTK_PANED(paned), sw);

    // A second pane for button box
    auto vbox2 = gtk_vbox_new(0,0);
    gtk_paned_add2(GTK_PANED(paned), vbox2);

    auto hbox = gtk_hbox_new(0,0);
    auto button_start = gtk_button_new_with_label("Start");
    g_signal_connect(button_start, "clicked", G_CALLBACK(cb_start), NULL);
    auto button_stop = gtk_button_new_with_label("Stop");
    g_signal_connect(button_stop, "clicked", G_CALLBACK(cb_stop), NULL);
    auto button_next = gtk_button_new_with_label("Next");
    g_signal_connect(button_next, "clicked", G_CALLBACK(cb_next), NULL);
    auto button_prev = gtk_button_new_with_label("Prev");
    g_signal_connect(button_prev, "clicked", G_CALLBACK(cb_prev), NULL);
    for (auto b : {button_start,button_stop,button_next,button_prev})
        gtk_box_pack_start(GTK_BOX(hbox), b, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), gtk_hseparator_new(), FALSE, FALSE, 0);
    nikud_action_box = hbox;

    w_opt_table = gtk_table_new(5,5,1);
    gtk_box_pack_start(GTK_BOX(vbox2), w_opt_table, TRUE, TRUE, 0);
    tab_buttons.push_back(gtk_button_new_with_label("Hello"));
    gtk_table_attach(GTK_TABLE(w_opt_table),
                     tab_buttons[0],
                     0,1,0,1,
                     GtkAttachOptions(GTK_FILL|GTK_EXPAND),
                     GtkAttachOptions(GTK_FILL|GTK_EXPAND),
                     0,0);

    gtk_widget_show_all(w_mw);
}

static void
cb_menu_about(GtkAction *action,
	      gpointer data)
{
    GtkWidget *about_window;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *w_image;
    gchar *markup;
  
    about_window = gtk_dialog_new ();
    gtk_window_set_transient_for(GTK_WINDOW(about_window),
                                 GTK_WINDOW(w_mw));
    gtk_dialog_add_button (GTK_DIALOG (about_window),
                           GTK_STOCK_OK, GTK_RESPONSE_OK);
    gtk_dialog_set_default_response (GTK_DIALOG (about_window),
                                     GTK_RESPONSE_OK);
  
    gtk_window_set_title(GTK_WINDOW (about_window), "Lenaked");
  
    gtk_window_set_resizable (GTK_WINDOW (about_window), FALSE);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->vbox), vbox, FALSE, FALSE, 0);

    w_image = NULL;

    //    gtk_box_pack_start (GTK_BOX (vbox), w_image, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
    markup = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">Lenaked %s</span>\n\n"
                              "%s\n"
                              "\n"
                              "<span>%s\n"
                              "%sEmail: <tt>&lt;%s&gt;</tt>\n"
                              "Commit-Id: %s\n"
                              "Commit-Date: %s</span>\n"
                              ,
                              VERSION,
                              ARCH,
                              ("Copyright &#x00a9; Dov Grobgeld 2017\n"),
                              ("Programming by: Dov Grobgeld\n"),
                              ("dov.grobgeld@gmail.com"),
                              GIT_COMMIT_ID,
                              GIT_COMMIT_TIME);
    gtk_label_set_markup (GTK_LABEL (label), markup);
    gtk_label_set_selectable (GTK_LABEL (label), true);
    g_free (markup);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    auto box = gtk_hbox_new(0,0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
    auto license_button = gtk_button_new_with_label("License");
    gtk_box_pack_start (GTK_BOX (box), license_button, TRUE,FALSE,0);
    g_signal_connect(license_button, "clicked",
                     G_CALLBACK(cb_license), about_window);
  
    gtk_widget_show_all (about_window);
    gtk_dialog_run (GTK_DIALOG (about_window));
    gtk_widget_destroy (about_window);
}

void
cb_menu_open (GtkAction *action, gpointer data)
{
    GtkWidget *w_filechooser
        = gtk_file_chooser_dialog_new ("Choose file to process",
                                       GTK_WINDOW(w_mw),
                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                       NULL);

    gtk_window_set_position(GTK_WINDOW(w_filechooser), GTK_WIN_POS_CENTER_ON_PARENT);
 
    gtk_dialog_set_default_response ( GTK_DIALOG(w_filechooser),
                                      GTK_RESPONSE_ACCEPT);

    int response = gtk_dialog_run(GTK_DIALOG(w_filechooser));

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkTextIter start_iter, end_iter;
        gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
        gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
        gtk_text_buffer_delete(text_buffer, &start_iter,&end_iter);
        char *selected_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w_filechooser));
        slip text;
        slip_read_file(selected_file,text);
        gtk_text_buffer_insert_at_cursor(text_buffer, text, -1);
        g_free(selected_file);
    }

    gtk_widget_destroy(w_filechooser);

    return;
}

void
cb_menu_open_corpus (GtkAction *action, gpointer data)
{
    GtkWidget *w_filechooser
        = gtk_file_chooser_dialog_new ("Choose corpus file",
                                       GTK_WINDOW(w_mw),
                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                       NULL);

    gtk_window_set_position(GTK_WINDOW(w_filechooser), GTK_WIN_POS_CENTER_ON_PARENT);
 
    gtk_dialog_set_default_response ( GTK_DIALOG(w_filechooser),
                                      GTK_RESPONSE_ACCEPT);

    int response = gtk_dialog_run(GTK_DIALOG(w_filechooser));

    if (response == GTK_RESPONSE_ACCEPT) {
        char *selected_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w_filechooser));
        read_db(selected_file);
        g_free(selected_file);
    }

    gtk_widget_destroy(w_filechooser);

    return;
}

static void cb_menu_save (GtkAction *action, gpointer data)
{
    GtkWidget *w_filechooser
        = gtk_file_chooser_dialog_new ("Choose file to process",
                                       GTK_WINDOW(w_mw),
                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                       NULL);

    gtk_window_set_position(GTK_WINDOW(w_filechooser), GTK_WIN_POS_CENTER_ON_PARENT);
 
    gtk_dialog_set_default_response ( GTK_DIALOG(w_filechooser),
                                      GTK_RESPONSE_ACCEPT);

    int response = gtk_dialog_run(GTK_DIALOG(w_filechooser));

    if (response == GTK_RESPONSE_ACCEPT) {
        char *selected_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w_filechooser));
        GtkTextIter start_iter, end_iter;
        gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
        gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
        gchar *all_text = gtk_text_buffer_get_text(text_buffer, &start_iter, &end_iter, TRUE);
        ofstream fh(selected_file);
        fh.write(all_text, strlen(all_text));
        fh.close();
        g_free(selected_file);
    }
    gtk_widget_destroy(w_filechooser);

    return;
}

/* Implement a handler for GtkUIManager's "add_widget" signal. The UI manager
 * will emit this signal whenever it needs you to place a new widget it has. */
static void
menu_add_widget (GtkUIManager *ui, GtkWidget *widget, GtkContainer *container)
{
    gtk_box_pack_start (GTK_BOX (container), widget, FALSE, FALSE, 0);
    gtk_widget_show (widget);
}

#define CASE(s) if (!strcmp(s, S_))

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    int argp = 1;

    gtk_rc_parse_string(
                        "style \"big\" {\n"
                        "    font_name =\"serif 30\"\n"
                        "}\n"
                        "\n"
                        "style \"heb-button\" {\n"
                        "    font_name = \"Sans 20\"\n"
                        "}\n"
                        "style \"heb-text\" {\n"
                        "    font_name = \"Sans 18\"\n"
                        "}\n"
                        "widget \"*heb-button*\" style \"heb-button\"\n"
                        "widget \"*heb-text*\" style \"heb-text\"\n"
                        );
    
    while(argp < argc && argv[argp][0] == '-') {
        char *S_ = argv[argp++];

        CASE("-help") {
            printf(" - ~@\n\n"
                   "Syntax:\n"
                   "    ~(pwdleaf) [] ...\n"
                   "\n"
                   "Options:\n"
                   "    -x x    Foo\n");
            exit(0);
        }
        die("Unknown option %s!\n", S_);
    }

#ifdef _WIN32
    char buffer[MAX_PATH];
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    GetModuleFileName(NULL, buffer, MAX_PATH);
    slip path(buffer);
    path.s("(.*)\\\\.*$","$1");  // Though there probably is a basename in glib...
    read_db(path + "\\..\\nikud-db.utf8");
#else
    read_db("../nikud-db.utf8");
#endif
    create_widgets();
    
    gtk_main();

    exit(0);
    return(0);
}
