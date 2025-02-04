#include "painter.hpp"
#include <iostream>
using namespace term_winapi;

void painter::init() { 
    system("cls"); 
    /**
     * h_console после этого != 0
     */
    terminal_api_instance();
    
    if (h_console != 0) { 
        fprintf(stderr, "%s: h_console is nil.", __FUNCTION__);  
        return; 
    }

    // получаем размер окна
    get_window_size(window.w, window.h); 

    // Инициализируем буфер
    mesh_buffer_size    = (window.w + 1) * (window.h + 1) + 1   ;   // +1 for \0
    mesh_out_buffer     = new char[mesh_buffer_size]; 
    
    // Очищаем буфер
    flush_buffer();

    //Включаем спец. режим
    set_terminal_mode(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT, false);
    set_cursor_pos(0, 0); 
}

void painter::flush_buffer() { 
    memset(mesh_out_buffer, (int)'\0', sizeof(char) * mesh_buffer_size);
}

void painter::window_size(window_t &size) {
    size.w = window.w; 
    size.h = window.h; 
}

// for debugging
void painter::_debug_draw_borders(int width, int height) {
    system("cls");

    for (int i = 0; i < width; ++i) { 
        for (int j = 0; j < height; ++j) { 
            mesh_out_buffer[i * j] = 'b';
        }
    }

    set_cursor_pos(0, 0); 

    // Задаём текст на x, y позиции, курсор должен переместиться на x + len
    out_line(mesh_out_buffer, mesh_buffer_size, (scr_color)0xF, (scr_color)0xF); 

    flush_buffer();
}   
   
//  

void painter::generic_draw(pos_t x, pos_t y, pos_t x_end, pos_t y_end, scr_color fclr, scr_color bclr) { 
    if (x > window.w) { 
        fprintf(stderr, "%s: pos_t(x) out of range. max(x)=%d.\ncur_x = (%d);cur_y = (%d)\n\n", 
            __FUNCTION__, window.w, x, y); 
        return; 
    } 

    if (y > window.h) {
        fprintf(stderr, "%s: pos_t(y) out of range. max(x)=%d.\ncur_x = (%d);cur_y = (%d)\n", 
            __FUNCTION__, window.h, x, y); 
        return; 
    } 

    // Задаём позицию курсора
    set_cursor_pos((int)x, (int)y);
    
    for (pos_t row = y; row <= y_end; ++row) { 
        unsigned long line_size = x_end - x + 1;    
        char *line = new char[line_size]; 
        
        int symb = 0;
        for (pos_t col = x; col <= x_end; ++col) {
            line[symb] = mesh_out_buffer[row * window.w + col]; 
            //printf("[%c]", line[j]);
            symb++;
        }
        //printf("\n\0");

        //printf("[%lu]", line_size);

        if (out_line(line, line_size, fclr, bclr) != 0) { 
            fprintf(stderr, "%s: out_line problem\n", __FUNCTION__);
            delete[] line; 
            return; 
        } 

        delete[] line;

        set_cursor_pos(x, row + 1); 
    }

    set_cursor_pos(0, y_end + 1);

    flush_buffer();
}

painter::text_t painter::fmt(const char* fmt, ...) { 
    va_list args;  
    va_start(args, fmt); 

    text_t text; 

    // Используем vsnprintf для получения необходимой длины строки
    int required_len = vsnprintf(nullptr, 0, fmt, args) + 1; // +1 для нулевого терминатора (я бля забыл об этом и потратил кучу времени)
    
    text.data = new char[required_len];
    text.len = required_len;

    // Повторно инициализируем args для vsnprintf (нахуя это блять надо)
    va_start(args, fmt); 
    vsnprintf(text.data, text.len, fmt, args);
    va_end(args); 

    text.len = strlen(text.data); 

    return text; 
}

painter::figure_t* painter::new_figure() { 
    figure_t *fig = new figure_t;
    
    fig->x = 0;
    fig->y = 0; 
    fig->w = 0; 
    fig->h = 0; 

    fig->data = {
        .data = nullptr, 
        .len  = 0,
    };
    
    strncpy(fig->symbols.BE, "", sizeof(char[2]));
    strncpy(fig->symbols.BG, "", sizeof(char[2]));
    strncpy(fig->symbols.BL, "", sizeof(char[2]));
    strncpy(fig->symbols.BR, "", sizeof(char[2]));
    strncpy(fig->symbols.LE, "", sizeof(char[2]));
    strncpy(fig->symbols.RE, "", sizeof(char[2]));
    strncpy(fig->symbols.TE, "", sizeof(char[2]));
    strncpy(fig->symbols.TL, "", sizeof(char[2]));
    strncpy(fig->symbols.TR, "", sizeof(char[2]));

    fig->bclr = scr_color::LIGHTGRAY; 
    fig->fclr = scr_color::BLACK;

    return fig;
}

void painter::figure_type_set(figure_t* t, figure_types type) { 
    t->type = type; 
}

void painter::figure_symbol_set(figure_t* t, const char* symbol_type, const char* new_symbol) { 
    if (strlen(new_symbol) > sizeof(char[2]) - 1) {
        fprintf(stderr, "%s: symbol to big: %s", __FUNCTION__, new_symbol); 
    }

    if (strcmp(symbol_type, "BE") == 0) { 
        strncpy(t->symbols.BE, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "BG") == 0) { 
        strncpy(t->symbols.BG, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "BL") == 0) { 
        strncpy(t->symbols.BL, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "BR") == 0) { 
        strncpy(t->symbols.BR, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "LE") == 0) { 
        strncpy(t->symbols.LE, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "RE") == 0) { 
        strncpy(t->symbols.RE, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "TE") == 0) { 
        strncpy(t->symbols.TE, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "TL") == 0) { 
        strncpy(t->symbols.TL, new_symbol, sizeof(char[2]));
    } else if (strcmp(symbol_type, "TR") == 0) { 
        strncpy(t->symbols.TR, new_symbol, sizeof(char[2]));
    } else { 
        fprintf(stderr, "%s: Unknown symbol type: %s", __FUNCTION__, symbol_type); 
    }
}

void painter::figure_text_set(figure_t* t, text_t text) {
    t->data = text; 
}

// render 

void painter::render_pos(figure_t* t, pos_t x, pos_t y) { 
    t->x = x; 
    t->y = y; 
}

void painter::render_clr(figure_t* t, fig_clr_t fclr, fig_clr_t bclr) { 
    t->fclr = fclr;
    t->bclr = bclr; 
}


void painter::figure_width_set(figure_t* t, fig_size_t width) {
    t->w = width; 
}   

void painter::figure_height_set(figure_t* t, fig_size_t height) {
    t->h = height; 
}

void painter::render_draw(figure_t* t) { 
    if (t->type == figure_types::SQUARE) {
        t->h = t->w; 
    }

    if (t->type == figure_types::TEXT) { 
        t->h = 1; 
        t->w = (fig_size_t) t->data.len; // надеюсь, не будет случая с длинной текста 0xFFFF
    }

    /**
     * sX = 2; w = 5 
     * [][][][][]   => eX = sX + w - 1
     * 2       6 
     * sY = 2; h = 1 => eY = sY + h - 1 [eY == sY]
     * 
     * sY = 2; h = 5 
     * [] 2
     * []
     * []
     * []
     * [] 6 => too
     */

    pos_t start_x(t->x), end_x(t->x + t->w - 1); // X - columns
    pos_t start_y(t->y), end_y(t->y + t->h - 1); // Y - rows

    int window_width = window.w;

    if (t->type == figure_types::RECTANGLE || t->type == figure_types::SQUARE) {
        // Верхняя сторона: TL, TE, TR
        for (pos_t col = start_x; col <= end_x; ++col) {
            mesh_out_buffer[start_y * window_width + col] = t->symbols.TE[0];

            if (col == start_x) {
                mesh_out_buffer[start_y * window_width + col] = t->symbols.TL[0];
                if (start_y == end_y) continue; // Если высота равна 1, пропустить дальше
            }
            if (col == end_x) {
                mesh_out_buffer[start_y * window_width + col] = t->symbols.TR[0];
                if (start_y == end_y) continue; // Если высота равна 1, пропустить дальше
            }
        }

        if (start_y != end_y) { 
            // Нижняя сторона: BL, BE, BR
            for (pos_t col = start_x; col <= end_x; ++col) {
                if (start_y != end_y) {
                    mesh_out_buffer[end_y * window_width + col] = t->symbols.BE[0];
                }
                if (col == start_x) {
                    mesh_out_buffer[end_y * window_width + col] = t->symbols.BL[0];
                    if (start_y == end_y) continue; // Если высота равна 1, пропустить дальше
                }
                if (col == end_x) {
                    mesh_out_buffer[end_y * window_width + col] = t->symbols.BR[0];
                    if (start_y == end_y) continue; // Если высота равна 1, пропустить дальше
                }
            }

            if (end_y - start_y >= 2) {
                // Левая сторона: LE
                if (start_x != end_x) {
                    for (pos_t row = start_y + 1; row < end_y; ++row) {
                        mesh_out_buffer[row * window_width + start_x] = t->symbols.LE[0];
                    }
                }

                // Правая сторона: RE
                if (start_x != end_x) {
                    for (pos_t row = start_y + 1; row < end_y; ++row) {
                        mesh_out_buffer[row * window_width + end_x] = t->symbols.RE[0];
                    }
                }

                // Внутренняя часть (фон)
                if (start_x != end_x && start_y != end_y) {
                    for (pos_t col = start_x + 1; col < end_x; ++col) {
                        for (pos_t row = start_y + 1; row < end_y; ++row) {
                            mesh_out_buffer[row * window_width + col] = t->symbols.BG[0];
                        }
                    }
                }
            }
        } 
    }
    
    if (t->type == figure_types::TEXT) { 
        for (pos_t col = start_x; col <= end_x; ++col) { 
            mesh_out_buffer[start_y * window_width + col] = t->data.data[col - start_x];
        }
    }

    generic_draw(start_x, start_y, end_x, end_y, t->fclr, t->fclr);
}