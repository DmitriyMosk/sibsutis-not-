#include <iostream>
#include "Terminal.hpp"
//#include "painter.hpp"

/**
 * Объявляем флаг того, что терминал запущен
 */

using namespace painter; 

int main() {     
    // painter::init(); 

    // painter::_debug_draw_borders(painter::window.w, painter::window.h); 

    // figure_t* fig1 = new_figure(); 

    // figure_type_set(fig1, figure_types::TEXT); 
    // figure_text_set(fig1, fmt("One Two %d", 3)); 

    // render_pos  (fig1, 10, 10); 
    // render_draw (fig1);

    // figure_t* fig2 = new_figure(); 

    // figure_type_set(fig2, figure_types::RECTANGLE); 

    // figure_symbol_set(fig2, "BG", "b"); 
    // figure_symbol_set(fig2, "TE", "!"); 
    // figure_symbol_set(fig2, "TL", "#");
    // figure_symbol_set(fig2, "TR", "#");
    // figure_symbol_set(fig2, "BE", "+"); 
    // figure_symbol_set(fig2, "BR", "$"); 
    // figure_symbol_set(fig2, "BL", "$"); 
    // figure_symbol_set(fig2, "LE", "*"); 
    // figure_symbol_set(fig2, "RE", "&");

    // painter::window_t wsize; 
    // window_size(wsize); 

    // //std::cout << wsize.w << " " << wsize.h << std::endl;

    // figure_width_set(fig2, 132); 
    // figure_height_set(fig2, 3); 

    // render_pos      (fig2, 0, 0); 
    // render_draw     (fig2);

    // set_terminal_mode(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT, true);
    Terminal term = Terminal(); 
    
    //Block *term.NewBlock

    return term.Run(); 
    // return 0; 
}