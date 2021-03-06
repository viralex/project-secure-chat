#include "gui.h"
#include "revision.h"
#include "threading/thread.h"

#define GUI_ICON "../etc/psc.png"

enum
{
  COLUMN_STRING1,
  COLUMN_STRING2,
  COLUMNS
};

pthread_mutex_t  mutex_guichange;

struct gui_res
{
    GtkWidget *window;
    GtkWidget *vbox_main;

    /* menubar (MOTHER OF NAMES) */
    GtkWidget *menubar, *filemenu,*helpmenu, *file;
    GtkWidget *connect,*open, *font, *sep, *quit;
    GtkWidget *help, *about;
    GtkAccelGroup *accel_group = NULL;

    /* toolbar */
    GtkWidget *toolbar;
    GtkToolItem *toolbar_connect;
    GtkToolItem *toolbar_refresh;
    GtkToolItem *toolbar_reset;
    GtkToolItem *toolbar_separator;
    GtkToolItem *toolbar_exit;

    /* paned */
    GtkWidget *paned_main;

    GtkWidget *dialog;
    GtkWidget *text_entry;
    GtkWidget *status_bar;

    /* user list */
    GtkListStore *model_user_list;
    GtkWidget *view_user_list;
    GtkCellRenderer *renderer_user_list;
    GtkTreeSelection *selection_user_list;
    GtkWidget *scrolledwindow_user_list;
        
    /* Chat */
    GtkWidget *hbox_chat;
    GtkWidget *view_chat;
    GtkWidget *label_nick;
    GtkWidget *scrolledwindow_chat;
    GtkTextBuffer *tbuf_chat;
    
    /* input */
    GtkWidget *hbox_inputs;
    GtkWidget *vbox_inputs;
    GtkWidget *entry_command;
    GtkWidget *button_send;
} gres;

void push_status_bar(const gchar*);
void add_message_to_chat(gpointer data, gchar *str, gchar type);
void add_user_to_list(gpointer data, gchar *str, gchar *lev);
void remove_user_from_list(gpointer data, const gchar *str);
void remove_all_users_from_list(gpointer data);
bool request_auth(gpointer parent);

void* GuiThread(void* arg)
{
    gui_res* gres = (gui_res*) arg;

    bool prev = false;

    if (CFG_GET_BOOL("autoconnect"))
    {
        bool proceed = false;
        INFO("debug","GUI: autoconnecting...\n");
        gdk_threads_enter();
        proceed = request_auth(gres->window);
        gdk_threads_leave();
        if(proceed)
            c_core->Connect();
    }
    
    while(1)
    {
        gdk_threads_enter();
        
        if(c_core->GetSession()->IsConnected() && !prev)
        {
            gtk_tool_button_set_label(
                GTK_TOOL_BUTTON(gres->toolbar_connect),
                "Disconnect");
            push_status_bar("Connected with server!");
        }
        
        if(!c_core->GetSession()->IsConnected() && prev)
        {
            gtk_tool_button_set_label(
                GTK_TOOL_BUTTON(gres->toolbar_connect),
                "Connect");
            push_status_bar("Disconnected from server!");
            remove_all_users_from_list(gres->view_user_list);
        }
        
        prev = c_core->GetSession()->IsConnected();

        Message_t msg = c_core->GetEvent();
        
        if(msg.data.length() > 0)
        {
            if (msg.type != 'J')
            {
                if (msg.data[msg.data.length()-1] != '\n')
                    msg.data.append("\n");
                add_message_to_chat(gres->tbuf_chat,
                                   (gchar*) msg.data.c_str(), msg.type);
            }
            
            if (msg.type == 'j' || msg.type == 'J')
            {
                add_user_to_list(gres->view_user_list,
                                 (gchar*) msg.user.c_str(),
                                 (gchar*) "*");
            }
            else if (msg.type == 'l')
            {
                remove_user_from_list(gres->view_user_list,
                                     (gchar*) msg.user.c_str());
            }
            else if (msg.type == 'L')
            {
                remove_all_users_from_list(gres->view_user_list);
            }
        }
        else
            INFO("debug", "GUI: message is null\n ");
        
        gdk_threads_leave();
        
        if(!c_core->EmptyEvents())
        {
            INFO("debug","GUI: There's another event to be handled\n");
            continue;
        }
        
        c_core->WaitEvent();
    }

    pthread_exit(NULL);
}

GdkPixbuf *create_pixbuf(const gchar * filename)
{
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   pixbuf = gdk_pixbuf_new_from_file(filename, &error);

   if(!pixbuf)
   {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
   }

   return pixbuf;
}

GdkPixbuf *reduce_pixbuf(GdkPixbuf * pixbuf, gint x, gint y)
{
    GdkPixbuf *pixbuf_reduced = NULL;
    
    if(pixbuf)
    {
        pixbuf_reduced = gdk_pixbuf_scale_simple(pixbuf, 
                                                 x,y,
                                                 GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT(pixbuf));                                            
    }

    return pixbuf_reduced;
}

void show_about()
{
  GdkPixbuf *pixbuf = reduce_pixbuf(create_pixbuf(GUI_ICON), 96, 96);

  GtkWidget *dialog = gtk_about_dialog_new();
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), _REVISION);
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
      "(c) 2012-2013 Alessandro Rosetti Daniele Lazzarini");
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
      "project secure chat");
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
      "http://code.google.com/p/project-secure-chat/");
  gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
  g_object_unref(pixbuf), pixbuf = NULL;
  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

void entry_pwd_response(GtkWidget * widget, gpointer dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
}

bool request_auth(gpointer parent)
{
    bool ret;
    GtkWidget* dialog = gtk_dialog_new_with_buttons ("Enter Password",
                                                 GTK_WINDOW(parent),
                                                 (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                 GTK_STOCK_OK,
                                                 GTK_RESPONSE_ACCEPT,
                                                 GTK_STOCK_CANCEL,
                                                 GTK_RESPONSE_REJECT,
                                                 NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));    
    GtkWidget *entry_name = gtk_entry_new();
    GtkWidget *entry_pwd = gtk_entry_new();
    GtkWidget *label_name = gtk_label_new("username");
    GtkWidget *label_pwd = gtk_label_new("password");
    GtkWidget *image;
    GdkPixbuf *pixbuf = reduce_pixbuf(create_pixbuf(GUI_ICON), 48, 48);
    if(pixbuf)
    {
        image = gtk_image_new_from_pixbuf(pixbuf);
        gtk_container_add (GTK_CONTAINER (content_area), image);
    }

    gtk_entry_set_invisible_char (GTK_ENTRY(entry_pwd), '*');
    gtk_entry_set_visibility (GTK_ENTRY(entry_pwd), FALSE);

    gtk_container_set_border_width(GTK_CONTAINER (content_area), 5);

    gtk_entry_set_text(GTK_ENTRY(entry_name), (gchar*) c_core->GetSession()->GetUsername()->c_str());
     
    gtk_container_add (GTK_CONTAINER (content_area), label_name);        
    gtk_container_add (GTK_CONTAINER (content_area), entry_name);

    gtk_container_add (GTK_CONTAINER (content_area), label_pwd);
    gtk_container_add (GTK_CONTAINER (content_area), entry_pwd);

    if(c_core->GetSession()->GetUsername()->length())
        gtk_widget_grab_focus (GTK_WIDGET(entry_pwd));
    else
        gtk_widget_grab_focus (GTK_WIDGET(entry_name));

    gtk_widget_show_all (dialog);
    
    g_signal_connect(G_OBJECT(entry_pwd), "activate", G_CALLBACK(entry_pwd_response), dialog);
    
    gint result = gtk_dialog_run(GTK_DIALOG (dialog));

    gchar *text_name = (gchar*) gtk_entry_get_text(GTK_ENTRY(entry_name));
    gchar *text_pwd = (gchar*) gtk_entry_get_text(GTK_ENTRY(entry_pwd));
    
    switch (result)
    {
        case GTK_RESPONSE_ACCEPT:
            INFO("debug" "GUI: login dialog -> %s\n", text_name);
            
            if (strlen(text_name) && strlen(text_pwd))
            {
                c_core->GetSession()->SetUsername(text_name);
                c_core->GetSession()->SetPassword(text_pwd);
                
                gtk_label_set_text(GTK_LABEL(gres.label_nick), text_name);
                
                if(!(ret = c_core->GetSession()->TestRsa()))
                {
                    add_message_to_chat(gres.tbuf_chat,
                                (gchar*) "Wrong Password! (RSA test failed)\n", 'e');
                }
            }
            else
            {
                add_message_to_chat(gres.tbuf_chat,
                                (gchar*) "Invalid User/Password insertion\n", 'e');
                ret = false;
            }
        break;

        default:
            ret = false;
        break;
    }
    
    if(pixbuf)
        g_object_unref (G_OBJECT(pixbuf));
    gtk_widget_destroy(dialog);
    
    return ret;
}

void select_font(gpointer parent)
{
    GtkWidget* dialog = gtk_font_chooser_dialog_new("Font", GTK_WINDOW(parent));
    
    gtk_dialog_run(GTK_DIALOG (dialog));
    
    PangoFontDescription *font_desc = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));
    
    if (font_desc)
        gtk_widget_modify_font(gres.view_chat, font_desc);

    gtk_widget_destroy(dialog);
}

void show_message(gchar *message)
{
   GtkWidget *dialog = gtk_dialog_new_with_buttons (_REVISION,
                                         0,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_NONE,
                                         NULL);
                                         
   GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
   GtkWidget *label = gtk_label_new (message);
   g_signal_connect_swapped (dialog,
                             "response",
                             G_CALLBACK (gtk_widget_destroy),
                             dialog);
   gtk_container_add (GTK_CONTAINER (content_area), label);
   gtk_widget_show_all (dialog);
   gtk_widget_destroy(dialog);
}

void on_destroy(gpointer user_data)
{
    gtk_main_quit();
}

bool find_user_from_list(gpointer data, const gchar *str)
{
    GtkTreeModel            *model;
    GtkTreeIter             iter;
    gboolean                r;
    gchar                   *value;
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));
    
    if (gtk_tree_model_get_iter_first (model, &iter) == FALSE)
        return false;
        
    for (r = gtk_tree_model_get_iter_first (model, &iter); 
         r == TRUE; 
         r = gtk_tree_model_iter_next (model, &iter))
    {                
        gtk_tree_model_get (model, &iter, COLUMN_STRING1, &value,  -1);
        
        if (g_ascii_strcasecmp(value, str) == 0)
        {
            if (value) 
                g_free (value);        
            return true;
        }
    
        if (value) 
            g_free (value);        
    }
    
    return false;
}

void add_user_to_list(gpointer data, gchar *str, gchar *lev)
{

  GtkTreeView *view = GTK_TREE_VIEW(data);
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  GtkTreeIter iter;
  
  if (!str || find_user_from_list(data, str))
      return;
  
  //pthread_mutex_lock(&mutex_guichange);
  gtk_list_store_append(GTK_LIST_STORE(model), &iter);
  gtk_list_store_set(GTK_LIST_STORE(model),
                     &iter,
                     COLUMN_STRING1,
                     str,
                     COLUMN_STRING2,
                     lev,
                     -1);
  //pthread_mutex_unlock(&mutex_guichange);
}

void remove_user_from_list(gpointer data, const gchar *str)
{
    GtkTreeModel            *model;
    GtkTreeIter             iter;
    gboolean                r;
    gchar                   *value;
    
    if (!str || !find_user_from_list(data, str))
        return;
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));
           
    for (r = gtk_tree_model_get_iter_first (model, &iter); 
         r == TRUE; 
         r = gtk_tree_model_iter_next (model, &iter))
    {                
        gtk_tree_model_get (model, &iter, COLUMN_STRING1, &value,  -1);
        
        if (g_ascii_strcasecmp(value, str) == 0) 
        {
            gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
            
            if (value) 
                g_free (value); 
            
            break;
        }

        if (value) 
            g_free (value);        
    }
}

void remove_all_users_from_list(gpointer data)
{
    GtkTreeModel            *model = gtk_tree_view_get_model (GTK_TREE_VIEW (data));
    GtkTreeIter             iter;

    while (gtk_tree_model_get_iter_first (model, &iter))   
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

void scroll_down(GtkWidget *scrolled)
{
    assert(scrolled);
    GtkAdjustment* adjustment = 
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
    
    gdouble val = gtk_adjustment_get_upper(adjustment) - 
                  gtk_adjustment_get_page_size(adjustment);
    
    gtk_adjustment_set_value(adjustment, val);
}

void add_message_to_chat(gpointer data, gchar *str, gchar type)
{
    //pthread_mutex_lock(&mutex_guichange);
    GtkTextBuffer *text_view_buffer = GTK_TEXT_BUFFER(data);
    GtkTextIter textiter;

    INFO("debug","GUI: Adding Message: \"%s\" to chat\n", (char*) str);

    gtk_text_buffer_get_end_iter(text_view_buffer, &textiter);
    switch(type)
    {
        case 'j': //user join
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_join_fg", "bold", NULL);
        break;
        case 'l': //user leave
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_leave_fg", "bold", NULL);
        break;
        case 'm': //message received
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_msg_fg", NULL);
        break;
        case 'M': //message sent
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_msg_fg", "bold", NULL);
        break;
        case 'w': //whisp received
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_whisp_fg", NULL);
        break;
        case 'W': //whisp sent
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_whisp_fg", "bold", NULL);
        break;
        case 'e': // server communication type 1
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_sys_msg_fg", "bold", NULL);
        break;
        case 's': //server communication type 2
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "chat_sys_msg_fg", "bold", NULL);
        break;
        default:
            return;
        break;
    }

    scroll_down(gres.scrolledwindow_chat);
    //pthread_mutex_unlock(&mutex_guichange);
}

void button_send_click(gpointer data, gchar *str, gchar type)
{
    Message_t msg;
    gchar *text = (gchar*) gtk_entry_get_text(GTK_ENTRY(gres.text_entry));

    if (c_core->HandleSend(text))
    {
        gtk_entry_set_text (GTK_ENTRY(gres.text_entry), "");
    }
}

void push_status_bar(const gchar *str)
{
    //pthread_mutex_lock(&mutex_guichange);
    guint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(gres.status_bar), "info");
    gtk_statusbar_pop(GTK_STATUSBAR(gres.status_bar), id);
    gtk_statusbar_push(GTK_STATUSBAR(gres.status_bar), id, str);
    //pthread_mutex_unlock(&mutex_guichange);
}

void toolbar_reset_click(gpointer data)
{
    //pthread_mutex_lock(&mutex_guichange);
    GtkTextBuffer *text_view_buffer = GTK_TEXT_BUFFER(gres.tbuf_chat);
    GtkTextIter textiter;
    gtk_text_buffer_get_end_iter(text_view_buffer, &textiter);
    gtk_text_buffer_set_text(text_view_buffer, "", 0);
    //pthread_mutex_unlock(&mutex_guichange);
}

void toolbar_connect_click(gpointer data)
{
    GtkToolButton *toolbar_connect = GTK_TOOL_BUTTON(data);
    if (!c_core->GetSession()->IsConnected())
    {
        INFO("debug", "connect button pressed\n");
        if(!request_auth(gres.window))
        {
            push_status_bar("Insert valid credentials!");
        }
        else
            if(!c_core->Connect())
                push_status_bar("Connection failed!");

        return;
    }

    if(c_core->GetSession()->IsConnected())
    {
        INFO("debug", "disconnect button pressed\n");
        c_core->GetSession()->ClearPassword();
        if(!c_core->Disconnect())
            push_status_bar("Disconnection failed!?");
    }
}

void main_gui(int argc, char **argv)
{
    /* inits */
    gdk_threads_init();
    gdk_threads_enter();
    gtk_init (&argc, &argv);
    pthread_mutex_init(&mutex_guichange, NULL);

    /* window */
    gres.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(gres.window), 0);
    gtk_window_set_urgency_hint (GTK_WINDOW(gres.window), TRUE);
    gtk_window_set_title (GTK_WINDOW (gres.window), _PROJECTNAME);
    gtk_window_set_default_size(GTK_WINDOW(gres.window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(gres.window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(gres.window), TRUE);

    /* setting window icon */
    gtk_window_set_icon(GTK_WINDOW(gres.window), create_pixbuf(GUI_ICON));

    gtk_widget_show(gres.window);

    g_signal_connect(G_OBJECT(gres.window), "delete_event", G_CALLBACK(on_destroy), NULL);
    g_signal_connect(G_OBJECT(gres.window), "destroy", G_CALLBACK(on_destroy), NULL);

    /* vbox principale */
    gres.vbox_main = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
    gtk_container_add(GTK_CONTAINER(gres.window), gres.vbox_main);
    gtk_container_set_border_width(GTK_CONTAINER(gres.vbox_main),0);

    /* accellgroup */
    gres.accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(gres.window), gres.accel_group);

    /* menubar */
    gres.menubar = gtk_menu_bar_new();
    gres.filemenu = gtk_menu_new();
    gres.helpmenu = gtk_menu_new();

    gres.file = gtk_menu_item_new_with_label("File");
    //gres.connect = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
    gres.open = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    gres.font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
    gres.sep = gtk_separator_menu_item_new();
    gres.quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, gres.accel_group);
    gres.help = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
    gres.about = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(gres.file), gres.filemenu);
    //gtk_menu_shell_append(GTK_MENU_SHELL(gres.filemenu), gres.connect);
    gtk_menu_shell_append(GTK_MENU_SHELL(gres.filemenu), gres.font);
    gtk_menu_shell_append(GTK_MENU_SHELL(gres.filemenu), gres.sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(gres.filemenu), gres.quit);
    gtk_menu_shell_append(GTK_MENU_SHELL(gres.menubar), gres.file);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(gres.help), gres.helpmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(gres.helpmenu), gres.about);
    gtk_menu_shell_append(GTK_MENU_SHELL(gres.menubar), gres.help);

    gtk_box_pack_start(GTK_BOX(gres.vbox_main), gres.menubar, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(gres.quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(gres.font), "activate", G_CALLBACK(select_font), G_OBJECT(gres.window));
    g_signal_connect(G_OBJECT(gres.about), "activate", G_CALLBACK(show_about), NULL);

    /* toolbar */
    gres.toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(gres.toolbar), GTK_TOOLBAR_BOTH);

    gtk_container_set_border_width(GTK_CONTAINER(gres.toolbar), 2);

    gres.toolbar_connect = gtk_tool_button_new_from_stock(GTK_STOCK_NETWORK);
    if (!c_core->GetSession()->IsConnected())
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(gres.toolbar_connect), "Connect");
    else
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(gres.toolbar_connect), "Disconnect");
    gtk_toolbar_insert(GTK_TOOLBAR(gres.toolbar), gres.toolbar_connect, -1);
    g_signal_connect(G_OBJECT(gres.toolbar_connect), "clicked", G_CALLBACK(toolbar_connect_click), NULL);

    //gres.toolbar_refresh = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
    //gtk_toolbar_insert(GTK_TOOLBAR(gres.toolbar), gres.toolbar_refresh, -1);
    //g_signal_connect(G_OBJECT(gres.toolbar_refresh), "clicked", G_CALLBACK(set_nick), G_OBJECT(gres.window));

    gres.toolbar_reset = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
    gtk_toolbar_insert(GTK_TOOLBAR(gres.toolbar), gres.toolbar_reset, -1);
    g_signal_connect(G_OBJECT(gres.toolbar_reset), "clicked", G_CALLBACK(toolbar_reset_click), NULL);

    gres.toolbar_separator = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(gres.toolbar), gres.toolbar_separator, -1);

    gres.toolbar_exit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
    gtk_toolbar_insert(GTK_TOOLBAR(gres.toolbar), gres.toolbar_exit, -1);
    g_signal_connect(G_OBJECT(gres.toolbar_exit), "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_box_pack_start(GTK_BOX(gres.vbox_main), gres.toolbar, FALSE, FALSE, 0);

    /* Paned */
    gres.paned_main = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(gres.vbox_main), gres.paned_main, TRUE, TRUE, 0);

    gres.scrolledwindow_chat = gtk_scrolled_window_new (NULL, NULL);
    gtk_paned_pack1 (GTK_PANED(gres.paned_main), gres.scrolledwindow_chat, true, true);

    gtk_container_set_border_width (GTK_CONTAINER (gres.scrolledwindow_chat), 2);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gres.scrolledwindow_chat),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);

    gres.view_chat = gtk_text_view_new();

    GdkRGBA color;
    gdk_rgba_parse (&color, CFG_GET_STRING("chat_bg").c_str());
    gtk_widget_override_background_color(GTK_WIDGET(gres.view_chat),
                                         GTK_STATE_FLAG_NORMAL, &color);

    PangoFontDescription *font_desc = pango_font_description_from_string(CFG_GET_STRING("chat_font").c_str());
    if (font_desc)
        gtk_widget_modify_font(gres.view_chat, font_desc);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(gres.view_chat), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (gres.view_chat), FALSE);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (gres.view_chat), 1);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (gres.view_chat), 1);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gres.view_chat), FALSE);
    gtk_container_add (GTK_CONTAINER (gres.scrolledwindow_chat), gres.view_chat);
    gres.tbuf_chat = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gres.view_chat));
    
    gtk_text_buffer_create_tag(gres.tbuf_chat, "chat_bg", "background", CFG_GET_STRING("chat_bg").c_str() , NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "chat_sys_msg_fg", "foreground", CFG_GET_STRING("chat_sys_msg_fg").c_str() , NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "chat_msg_fg", "foreground", CFG_GET_STRING("chat_msg_fg").c_str() , NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "chat_join_fg", "foreground", CFG_GET_STRING("chat_join_fg").c_str() , NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "chat_leave_fg", "foreground", CFG_GET_STRING("chat_leave_fg").c_str() , NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "chat_whisp_fg", "foreground", CFG_GET_STRING("chat_whisp_fg").c_str() , NULL);

    gtk_text_buffer_create_tag(gres.tbuf_chat, "gap", "pixels_above_lines", 30, NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "lmarg", "left_margin", 5, NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "black_fg", "foreground", "#000000", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "white_fg", "foreground", "#ffffff", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "blue_fg", "foreground", "#3200ff", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "magenta_fg", "foreground", "#ff32ff", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "green_fg", "foreground", "#55ff00", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "red_fg", "foreground", "#ff3200", NULL);
    
    gtk_text_buffer_create_tag(gres.tbuf_chat, "green_bg", "background", "#55ff00", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "blue_bg", "background", "#3200ff", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "red_bg", "background", "#ff3200", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "yellow_bg", "background", "#f7f732", NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "magenta_bg", "background", "#ff32ff", NULL);
    
    gtk_text_buffer_create_tag(gres.tbuf_chat, "italic", "style", PANGO_STYLE_ITALIC, NULL);
    gtk_text_buffer_create_tag(gres.tbuf_chat, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);

    gres.scrolledwindow_user_list = gtk_scrolled_window_new (NULL, NULL);
    gtk_paned_pack2 (GTK_PANED(gres.paned_main), gres.scrolledwindow_user_list, false, false);
    gtk_container_set_border_width (GTK_CONTAINER (gres.scrolledwindow_user_list), 2);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gres.scrolledwindow_user_list),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_widget_show (gres.scrolledwindow_user_list);

    gres.model_user_list     = gtk_list_store_new(COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    gres.view_user_list      = gtk_tree_view_new_with_model (GTK_TREE_MODEL(gres.model_user_list));
    gres.selection_user_list = gtk_tree_view_get_selection(GTK_TREE_VIEW(gres.view_user_list));

    gtk_tree_selection_set_mode(gres.selection_user_list, GTK_SELECTION_SINGLE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(gres.view_user_list), TRUE);

    gres.renderer_user_list = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(gres.view_user_list), /* vista */
                        -1,                  /* posizione della colonna */
                        "Name",  /* titolo della colonna */
                        gres.renderer_user_list,            /* cella inserita nella colonna */
                        "text",              /* attributo colonna */
                        COLUMN_STRING1,    /* colonna inserita  */
                        NULL);               /* fine ;-) */

    gres.renderer_user_list = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(gres.view_user_list),
                        -1,
                        "Type",
                        gres.renderer_user_list,
                        "text",
                        COLUMN_STRING2,
                        NULL);

    gtk_widget_show (gres.view_user_list);
    g_object_unref(gres.model_user_list);

    gtk_container_add (GTK_CONTAINER (gres.scrolledwindow_user_list), gres.view_user_list);
    gtk_container_set_border_width (GTK_CONTAINER (gres.view_user_list), 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (gres.view_user_list), TRUE);

    /* INPUTS */
    gres.hbox_inputs = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX (gres.vbox_main), gres.hbox_inputs, FALSE, FALSE, 0);
    
    gres.label_nick = gtk_label_new((gchar *) CFG_GET_STRING("username").c_str()); 
    gtk_misc_set_alignment(GTK_MISC(gres.label_nick), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX (gres.hbox_inputs), gres.label_nick, FALSE, FALSE, 2 );

    gres.entry_command = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX (gres.hbox_inputs), gres.entry_command, TRUE, TRUE, 5);
    
    gres.button_send = gtk_button_new_with_label("Send");
    gtk_widget_set_size_request (GTK_WIDGET (gres.button_send), 70, 30);
    gtk_box_pack_start(GTK_BOX (gres.hbox_inputs), gres.button_send, FALSE, FALSE, 0);

    gres.text_entry = gres.entry_command;
    g_signal_connect(G_OBJECT(gres.entry_command), "activate", G_CALLBACK(button_send_click), NULL);
    g_signal_connect(G_OBJECT(gres.button_send), "clicked", G_CALLBACK(button_send_click), NULL);

    /* status_bar */
    gres.status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX (gres.vbox_main), gres.status_bar, FALSE, FALSE, 0);

    /* end_widgets */
    gtk_widget_show_all(gres.window);

    /* default focus on command entry */
    gtk_widget_grab_focus (GTK_WIDGET(gres.text_entry));

    INFO ("debug", "GUI: starting GUI thread\n");
    pthread_t tid;
    StartThread(GuiThread, (void*)&gres, tid);

    INFO ("debug", "GUI: starting GTK+3\n");
    gtk_main();
    gdk_threads_leave();
    pthread_mutex_destroy(&mutex_guichange);

    return;
}
