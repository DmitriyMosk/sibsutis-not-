#ifndef _LIB_PAINTER
#define _LIB_PAINTER
    #include <stdio.h>
    #include <cstdint> 
    #include <cstdarg>
    #include "terminal_api.hpp" 
    
    /**
     * TODO: docs
     */
    namespace painter {
        using namespace term_winapi;

        /**
         * TODO: docs
         */
        static char *mesh_out_buffer;

        /**
         * TODO: docs
         */
        static size_t mesh_buffer_size; 

        /**
         * TODO: docs
         */
        typedef struct window_t { 
            int w, h; 
        } window_t; 

        /**
         * TODO: docs
         */
        typedef uint16_t pos_t      ; 

        /**
         * TODO: docs
         */
        typedef uint16_t fig_size_t ;

        /**
         * TODO: docs
         */
        typedef scr_color fig_clr_t ;

        /**
         * TODO: docs
         */
        typedef struct text_t { 
            char*           data ; 
            unsigned long   len  ;
        } text_t; 

        static window_t window; 

        /**
         * TODO: docs
         */
        void init(); 

        /**
         * TODO: docs
         */
        void window_size(window_t &window);

        /**
         * TODO: docs
         */
        void flush_buffer();

        /**
         * TODO: docs
         */
        void generic_draw(pos_t x, pos_t y, pos_t x_end, pos_t y_end, scr_color fclr, scr_color bclr); 

        /**
         * TODO: docs
         */
        text_t fmt(const char* fmt, ...);

        /**
         * TODO: docs  
         */

        /**
         *  Для отладки. Отрисовывает края экрана
         */ 
        void _debug_draw_borders(int width, int height); 

        /**
         * TODO: docs
         */
        typedef struct figure_symbol_t {
            char TL[2];        ///< Верхний левый угол    (Top Left corner)
            char TE[2];        ///< Верхняя сторона       (Top Edge)
            char TR[2];        ///< Верхний правый угол   (Top Right corner)
            char BE[2];        ///< Нижняя сторона        (Bottom Edge)
            char BL[2];        ///< Нижний левый угол     (Bottom Left corner)
            char LE[2];        ///< Левая сторона         (Left Edge)
            char BR[2];        ///< Нижний правый угол    (Bottom Right corner)
            char RE[2];        ///< Правая сторона        (Right Edge)
            char BG[2];        ///< Фон                   (Background)
        } figure_symbol_t; 

        /**
         * TODO: docs
         */
        enum figure_types { 
            SQUARE, 
            RECTANGLE, 
            TEXT,
        }; 

        /**
         * TODO: docs
         */
        typedef struct figure_t { 
            figure_symbol_t symbols         ; 
            figure_types    type            ;
            fig_size_t      w, h            ;
            fig_clr_t       fclr, bclr      ; 
            pos_t           x, y            ;

            /**
             * for type == TEXT
             */
            text_t          data            ; 
        } figure_t; 

        /**
         * Figure constructor 
         * TODO: docs
         */
        figure_t* new_figure();

        /**
         * TODO: docs
         */
        void figure_type_set(figure_t* t, figure_types type);

        /**
         * TODO: docs
         */
        void figure_symbol_set(figure_t* t, const char* symbol_type, const char* new_symbol);

        /**
         * TODO: docs
         */
        void figure_text_set(figure_t* t, text_t text); 

        /**
         * TODO: docs
         */
        void figure_width_set(figure_t* t, fig_size_t width);

        /**
         * TODO: docs
         */
        void figure_height_set(figure_t* t, fig_size_t height); 
        
        /**
         * Render functions
         */
        
        /**
         * TODO: docs
         */
        void render_pos(figure_t* t, pos_t x, pos_t y); 

        /**
         * TODO: docs
         */
        void render_clr(figure_t* t, fig_clr_t fclr, fig_clr_t bclr); 

        /**
         * TODO: docs
         */
        void render_draw(figure_t* t);
    }

#endif  


/**
 *  fig = new_figure()
 * 
 *  figure_type         (fig, RECTANGLE | SQUARE | TEXT)
 *  figure_symbol_set   (fig, "TL", "#")
 *  figure_text_set     (fig, fmt("GGWP %d", 10));
 * 
 *  render_pos  (fig, pos_t x, pos_t y); 
 *  render_color(fig, fclr, bclr);  
 *  render_draw (fig)
 * 
 *  /change_figure_attrs
 *  
 *  /example change figure colors
 *  render_color(fig, fclr, bclr); 
 *  render_draw (fig) 
 * 
 *  /example change figure text 
 *  figure_text_set(fig, fmt("GGWP2 %d", 10)); 
 *  render_draw (fig); 
 *  
 *  fig2 = new_figure(); 
 *  figure_extends(fig1, fig2)
 *  figure_text_set(fig, fmt("GGWP %d", 10));
 */