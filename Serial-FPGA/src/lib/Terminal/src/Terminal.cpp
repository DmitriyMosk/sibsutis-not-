#include "Terminal.hpp"

using namespace painter; 
using namespace term_winapi;

window_t     Terminal::window       = {
    .w = 0, 
    .h = 0,
}; 

volatile int Terminal::_t_stop_flag = 0;

Terminal::Terminal() {
    /**
     * Set signal handler
     */
    std::signal(SIGINT, sigint_handler);
    init();

    window_size(window);
}

Terminal::~Terminal() {

}

int Terminal::Run() { 
    /**
     * Init painter API
     */

    __draw_header();
    
    /**
     * Бесконечный цикл для бесконечной обработки
     */
    while (_t_stop_flag == 0) { 
        key_handler();
    }

    term_winapi::set_terminal_mode(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT, true);

    return EXIT_SUCCESS;
}

/**
 * 
 */
void Terminal::key_handler() { 

}

/**
 * Block Functions
 */
Terminal::Block* Terminal::NewBlock(uint32_t id, painter::figure_types type) { 
    Block* block = new Block; 

    if (id > 0xFFFF) { 

    }

    block->id       = id; 
    block->parent   = nullptr; 
    block->fig      = painter::new_figure();
    
    figure_type_set(block->fig, type); 

    return block;
}

void Terminal::SetBlockParent(Block* block, Block* parent) {
    block->parent = parent; 

    SetBlockSize(block, block->fig->w, block->fig->h);
}

void Terminal::SetBlockPos(Block* block, block_pos_t x, block_pos_t y) { 
    render_pos(block->fig, x, y); 
}

void Terminal::SetBlockSize(Block* block, block_size_t width, block_size_t height) { 
    if (width == 0xFFFF) { 
        figure_width_set(block->fig, window.w);
    } else if (width > window.w) { 
        figure_width_set(block->fig, window.w - (0xFFFF - width));
    } else { 
        figure_width_set(block->fig, width);
    }

    if (height == 0xFFFF) { 
        figure_height_set(block->fig, window.h);
    } else if (height > window.h) {
        figure_height_set(block->fig, window.h - (0xFFFF - height));
    } else { 
        figure_height_set(block->fig, height);
    }

    if (block->parent != nullptr) { 
        if (width == window.w) {
            figure_width_set(block->fig, block->parent->fig->w);
        } else if (width > block->parent->fig->w) { 
            //figure_width_set(block->fig, window.w);
        } else {
            figure_width_set(block->fig, width);
        }

        if (height == window.h) { 
            figure_height_set(block->fig, block->parent->fig->h);
        } else if (height > block->parent->fig->h) {
            //figure_height_set(block->fig, window.h);
        } else { 
            figure_height_set(block->fig, height);
        }
    }
}

void Terminal::SetBlockAlignX(Block* b, uint8_t align_x) { 
    b->align = (b->align & term_io::MASK_ALIGN_X);  

    SetBlockPos(b, b->fig->x, b->fig->y); 
}

void Terminal::GetBlockAlignX(Block* b, uint8_t &alig_x) {

}   

void Terminal::SetBlockAlignY(Block* b, uint8_t align_y) { 
    b->align = (b->align & term_io::MASK_ALIGN_X);

    SetBlockPos(b, b->fig->x, b->fig->y); 
}

void Terminal::GetBlockAlignY(Block* b, uint8_t &align_y) { 
    
}

void Terminal::BlockDraw(Block* b) { 
    render_draw(b->fig); 
}

/**
 * Default blocks
 */
void Terminal::__draw_header() { 
    Block* header = NewBlock(term_io::HEADER_ID, figure_types::RECTANGLE);
    
    figure_symbol_set(header->fig, "BG", " ");  
    figure_symbol_set(header->fig, "TE", " ");  
    figure_symbol_set(header->fig, "TL", " ");
    figure_symbol_set(header->fig, "TR", "#");
    figure_symbol_set(header->fig, "BE", " ");  
    figure_symbol_set(header->fig, "BR", "#"); 
    figure_symbol_set(header->fig, "BL", " "); 
    figure_symbol_set(header->fig, "LE", " "); 
    figure_symbol_set(header->fig, "RE", " ");

    SetBlockPos     (header, 0, 0);
    SetBlockSize    (header, 0xFFFF, 2); 
    
    SetBlockAlignX  (header, term_io::ALIGN_X_LEFT); 
    SetBlockAlignY  (header, term_io::ALIGN_Y_TOP);
    
    render_clr(header->fig, fig_clr_t::DARKGRAY, fig_clr_t::DARKGRAY);

    BlockDraw(header); 

    Block* header_title = NewBlock(term_io::HEADER_ID + 1, figure_types::TEXT); 
    SetBlockParent(header_title, header); 
    SetBlockDockX(header_title, term_io::DOCK_X_CENTER); 
    //SetBlockAlignX(header_title, )
}

void Terminal::__draw_center() {

}

void Terminal::__draw_bottom() { 
    
}