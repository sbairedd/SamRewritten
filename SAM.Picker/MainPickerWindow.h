#pragma once

#include <gtk/gtk.h>
#include <iostream>

class MainPickerWindow {
public:
    //Todo: destructor
    MainPickerWindow();
    void reset_game_list();
    void add_to_game_list(const char app_name[]);
    void confirm_game_list();
    GtkWidget* get_main_window() { return m_main_window; };

private:
    GtkWidget *m_main_window;
    GtkListBox *m_game_list;  
    GtkBuilder *m_builder;
};