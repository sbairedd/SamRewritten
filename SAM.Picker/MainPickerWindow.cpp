#include "MainPickerWindow.h"

MainPickerWindow::MainPickerWindow() 
: 
m_main_window(nullptr),
m_back_button(nullptr),
m_game_list(nullptr),
m_stats_list(nullptr),
m_builder(nullptr),
m_main_stack(nullptr),
m_game_list_view(nullptr),
m_stats_list_view(nullptr)
{
    GError *error = NULL;
    m_builder = gtk_builder_new();
    const char ui_file[] = "glade/main_window.glade";
    
    // Load the builder
    gtk_builder_add_from_file (m_builder, ui_file, &error);
    if(error != NULL) {
        std::cerr << "An error occurred opening the main window.. Make sure " << ui_file << " exists and is a valid file." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Load the required widgets through the builder
    m_game_list = GTK_LIST_BOX(gtk_builder_get_object(m_builder, "game_list"));
    m_stats_list = GTK_LIST_BOX(gtk_builder_get_object(m_builder, "stats_list"));
    m_main_window = GTK_WIDGET(gtk_builder_get_object(m_builder, "main_window"));
    m_main_stack = GTK_STACK(gtk_builder_get_object(m_builder, "main_stack"));
    m_game_list_view = GTK_SCROLLED_WINDOW(gtk_builder_get_object(m_builder, "game_list_view"));
    m_stats_list_view = GTK_SCROLLED_WINDOW(gtk_builder_get_object(m_builder, "stats_list_view"));
    m_back_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "back_button"));
    GtkWidget* game_placeholder = GTK_WIDGET(gtk_builder_get_object(m_builder, "game_placeholder"));
    GtkWidget* stats_placeholder = GTK_WIDGET(gtk_builder_get_object(m_builder, "stats_placeholder"));

    g_signal_connect(m_game_list, "row-activated", (GCallback)on_game_row_activated, NULL);
    gtk_builder_connect_signals(m_builder, NULL);
    

    // Show the placeholder widget right away, which is the loading widget
    gtk_list_box_set_placeholder(m_game_list, game_placeholder);
    gtk_list_box_set_placeholder(m_stats_list, stats_placeholder);
    gtk_widget_show(game_placeholder);
}
// => Constructor



/**
 * See https://stackoverflow.com/questions/9192223/remove-gtk-container-children-repopulate-it-then-refresh
 * used here and other methods, tells you how to iterate through widgets with a relationship.
 * 
 * This method will remove every game entry, only leaving the loading widget.
 */
void 
MainPickerWindow::reset_game_list() {
    for (std::map<unsigned long, GtkWidget*>::iterator it = m_rows.begin(); it != m_rows.end(); ++it)
    {
        gtk_widget_destroy( GTK_WIDGET(it->second) );
    }

    m_rows.clear();
}
// => reset_game_list


/**
 * Add a game to the list. Ignores warnings for the obsolete GtkArrow.
 * Such a classy widget, I don't get why I should bother creating a shitty 
 * gtkImage instead when it does just what I want out of the box.
 * The entry created is pushed on m_rows, to be easily acccessed later.
 * The new entry is not shown yet, call confirm_game_list for that.
 */
void 
MainPickerWindow::add_to_game_list(const Game_t& app) {
    // Because you can't clone widgets with GTK, I'm going to recreate 
    // GTK_LIST_BOX_ROW(gtk_builder_get_object(m_builder, "game_entry"));
    // By hand. Which is dead stupid.

    //Also, fuck the police I still use GtkArrow what you gonna do

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    GtkWidget *wrapper = gtk_list_box_row_new();
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *label = gtk_label_new(app.app_name.c_str());
    GtkWidget *game_logo = gtk_image_new_from_icon_name ("gtk-missing-image", GTK_ICON_SIZE_DIALOG);
    GtkWidget *nice_arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);

    #pragma GCC diagnostic pop

    gtk_widget_set_size_request(wrapper, -1, 80);

    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(game_logo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(label), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(nice_arrow), FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(wrapper), GTK_WIDGET(layout));

    gtk_list_box_insert(m_game_list, GTK_WIDGET(wrapper), -1);
    
    //Save the created row somewhere for EZ access
    m_rows.insert(std::pair<unsigned long, GtkWidget*>(app.app_id, wrapper));
}
// => add_to_game_list

/**
 * Draws all the games that have not been shown yet
 */
void 
MainPickerWindow::confirm_game_list() {
    gtk_widget_show_all(GTK_WIDGET(m_game_list));
}
// => confirm_game_list

/**
 * Refreshes the icon for the specified app ID
 * if app_id is zero, it means the downloaded file isn't an app icon
 */
void 
MainPickerWindow::refresh_app_icon(const unsigned long app_id) {
    if(app_id == 0)
        return;

    //TODO make sure app_id is index of m_rows
    GList *children;
    GtkImage *img;
    GdkPixbuf *pixbuf;
    GError *error;
    std::string path(g_cache_folder);

    error = nullptr;
    path += "/";
    path += std::to_string(app_id);
    path += "/banner";

    children = gtk_container_get_children(GTK_CONTAINER(m_rows[app_id])); //children = the layout
    children = gtk_container_get_children(GTK_CONTAINER(children->data)); //children = first element of layout    
    //children = g_list_next(children);                                   //children = second element of layout...

    img = GTK_IMAGE(children->data);
    if( !GTK_IS_IMAGE(img) ) {
        std::cerr << "It looks like the GUI has been modified, or something went wrong." << std::endl;
        std::cerr << "Inform the developer or look at MainPickerWindow::refresh_app_icon." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    g_list_free(children);

    pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
    if (error != NULL) {
        std::cerr << "Error while loading an app's logo: " << std::endl;
        std::cerr << "AppId: " << app_id << std::endl;
        std::cerr << "Message: "  << error->message << std::endl;
    }
    else {
        //Quick and jerky, quality isn't key here
        // Is the excess of memory freed though?
        pixbuf = gdk_pixbuf_scale_simple(pixbuf, 146, 68, GDK_INTERP_NEAREST);
        gtk_image_set_from_pixbuf(img, pixbuf);
    }

}
// => refresh_app_icon

void
MainPickerWindow::filter_games(const char* filter_text) {
    const std::string text_filter(filter_text);
    std::string text_label;
        
    gtk_widget_show_all( GTK_WIDGET(m_game_list) );
    if(text_filter.empty()) {
        return;
    }

    for (std::map<unsigned long, GtkWidget*>::iterator it = m_rows.begin(); it != m_rows.end(); ++it)
    {
        GList *children;
        GtkLabel* label;
        children = gtk_container_get_children(GTK_CONTAINER(it->second)); //children = the layout
        children = gtk_container_get_children(GTK_CONTAINER(children->data)); //children = first element of layout
        children = g_list_next(children); //children = label
        label = GTK_LABEL( children->data );

        if( !GTK_IS_LABEL(label) ) {
            std::cerr << "The layout may have been modified, please tell me you're a dev, look at MainPikerWindow Blah blah TODO" << std::endl;
            exit(EXIT_FAILURE);
        }

        text_label = std::string( gtk_label_get_text( GTK_LABEL(label) ) );

        //Holy shit C++, why can't you even do case insensitive comparisons wtf
        //strstri is just a shitty workaround there's no way to do this properly
        if(!strstri(text_label, text_filter)) {
            gtk_widget_hide( GTK_WIDGET(it->second) );
        }
    }
}
// => filter_games

unsigned long 
MainPickerWindow::get_corresponding_appid_for_row(GtkListBoxRow *row) {
    for(std::map<unsigned long, GtkWidget*>::iterator it = m_rows.begin(); it != m_rows.end(); ++it)
    {
        if((gpointer)it->second == (gpointer)row) {
            return it->first;
        }
    }
    return 0;
}
// => get_corresponding_appid_for_row

void
MainPickerWindow::switch_to_stats_page() {
    gtk_widget_set_visible(GTK_WIDGET(m_back_button), TRUE);
    gtk_stack_set_visible_child(GTK_STACK(m_main_stack), GTK_WIDGET(m_stats_list_view));
}
// => switch_to_stats_page


void
MainPickerWindow::switch_to_games_page() {
    gtk_widget_set_visible(GTK_WIDGET(m_back_button), FALSE);
    gtk_stack_set_visible_child(GTK_STACK(m_main_stack), GTK_WIDGET(m_game_list_view));

    //TODO Clear achievments list
}
// => switch_to_games_page