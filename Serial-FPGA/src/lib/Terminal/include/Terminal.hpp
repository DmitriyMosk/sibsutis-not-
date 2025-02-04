#ifndef _LIB_TERMINAL
#define _LIB_TERMINAL

#include "painter.hpp"
#include <csignal>

#define TERMINAL_VERSION "2.3"

using namespace painter; 

namespace term_io { 
    enum cmd_t { 
        update_header, 
        update_body, 
        update_bottom, 
        update_cursor,
    };

    /**
     * Flags
     */
    constexpr const uint8_t _FLAG_ERROR = 0xFF;

    /**
     * @brief Максимальная длина заголовка.
     * 
     * Эта константа определяет максимальное количество символов, которое может содержать заголовок.
     */
    constexpr const uint16_t MAX_TITLE_LENGTH = 120;

    /**
     * @brief Сообщение для слишком длинного заголовка.
     * 
     * Эта константа содержит сообщение, которое будет отображаться, если заголовок превышает допустимую длину.
     */
    constexpr const char* HUGE_TITLE_MESSAGE = "Huge title. Sorry :(";

    /**
     * @brief Доступ только для чтения (Read Only).
     * 
     * Эта константа используется для обозначения режима доступа, при котором разрешено только чтение данных.
     */
    constexpr const uint8_t R_ONLY = 0x03; // 0b00000011

    /**
     * @brief Доступ только для записи (Write Only).
     * 
     * Эта константа используется для обозначения режима доступа, при котором разрешена только запись данных.
     */
    constexpr const uint8_t W_ONLY = 0x0C; // 0b00001100

    /**
     * @brief Заблокированный доступ (Locked).
     * 
     * Эта константа используется для обозначения заблокированного режима доступа, при котором доступ к данным запрещен.
     */
    constexpr const uint8_t LOCKED = 0xF0; // 0b11110000

    /** 
     * @brief Маски элементов матрицы.
     * 
     * Эти константы используются для выделения различных частей элементов матрицы.
     */ 
    constexpr const uint32_t MASK_ID        = 0xFFFFFF00; ///< Маска для идентификатора элемента (оставляет старшие 24 бита).
    constexpr const uint32_t MASK_ACCESS    = 0x000000FF; ///< Маска для доступа к элементу (оставляет младшие 8 бит).

    /**
     * @brief ID элементов. (16 бита!)
     * 
     * Эти константы используются для обозначения различных частей элементов в системе.
     */
    constexpr const uint32_t HEADER_ID = 0x00; ///< Идентификатор заголовка.
    constexpr const uint32_t BODY_ID   = 0x20; ///< Идентификатор тела.
    constexpr const uint32_t FOOTER_ID = 0xFF; ///< Идентификатор нижней части.

    /**
     * @brief Маска для выравнивания по оси X.
     * 
     * Эта константа используется для применения маски на значения выравнивания по оси X, что позволяет выбрать определенное выравнивание.
     */
    constexpr const uint8_t MASK_DOCK_X = 0xF0;

    /**
     * @brief Центрирование по оси X.
     * 
     * Эта константа используется для центрирования элемента по оси X.
     */
    constexpr const uint8_t DOCK_X_CENTER = 0b1000 << 4;

    /**
     * @brief Выравнивание по левому краю оси X.
     * 
     * Эта константа используется для выравнивания элемента по левому краю по оси X.
     */
    constexpr const uint8_t DOCK_X_LEFT = 0b0110 << 4;

    /**
     * @brief Выравнивание по правому краю оси X.
     * 
     * Эта константа используется для выравнивания элемента по правому краю по оси X.
     */
    constexpr const uint8_t DOCK_X_RIGHT = 0b0001 << 4;

    /**
     * @brief Маска для выравнивания по оси Y.
     * 
     * Эта константа используется для применения маски на значения выравнивания по оси Y, что позволяет выбрать определенное выравнивание.
     */
    constexpr const uint8_t MASK_DOCK_Y = 0x0F;

    /**
     * @brief Выравнивание по верхнему краю оси Y.
     * 
     * Эта константа используется для выравнивания элемента по верхнему краю по оси Y.
     */
    constexpr const uint8_t DOCK_Y_TOP = 0b1000;

    /**
     * @brief Центрирование по оси Y.
     * 
     * Эта константа используется для центрирования элемента по оси Y.
     */
    constexpr const uint8_t DOCK_Y_CENTER = 0b0110;

    /**
     * @brief Выравнивание по нижнему краю оси Y.
     * 
     * Эта константа используется для выравнивания элемента по нижнему краю по оси Y.
     */
    constexpr const uint8_t DOCK_Y_BOTTOM = 0b0001;

    /**
     * @brief Маска для выравнивания по оси X.
     * 
     * Эта константа используется для применения маски на значения выравнивания по оси X при выравнивании элементов.
     */
    constexpr const uint8_t MASK_ALIGN_X = 0xF0;

    /**
     * @brief Центрирование по оси X при выравнивании.
     * 
     * Эта константа используется для центрирования элемента по оси X при выравнивании.
     */
    constexpr const uint8_t ALIGN_X_CENTER = 0b1000 << 4;

    /**
     * @brief Выравнивание по левому краю оси X при выравнивании.
     * 
     * Эта константа используется для выравнивания элемента по левому краю по оси X при выравнивании.
     */
    constexpr const uint8_t ALIGN_X_LEFT = 0b0110 << 4;

    /**
     * @brief Выравнивание по правому краю оси X при выравнивании.
     * 
     * Эта константа используется для выравнивания элемента по правому краю по оси X при выравнивании.
     */
    constexpr const uint8_t ALIGN_X_RIGHT = 0b0001 << 4;

    /**
     * @brief Маска для выравнивания по оси Y.
     * 
     * Эта константа используется для применения маски на значения выравнивания по оси Y при выравнивании элементов.
     */
    constexpr const uint8_t MASK_ALIGN_Y = 0x0F;

    /**
     * @brief Выравнивание по верхнему краю оси Y при выравнивании.
     * 
     * Эта константа используется для выравнивания элемента по верхнему краю по оси Y при выравнивании.
     */
    constexpr const uint8_t ALIGN_Y_TOP = 0b1000;

    /**
     * @brief Центрирование по оси Y при выравнивании.
     * 
     * Эта константа используется для центрирования элемента по оси Y при выравнивании.
     */
    constexpr const uint8_t ALIGN_Y_CENTER = 0b0110;

    /**
     * @brief Выравнивание по нижнему краю оси Y при выравнивании.
     * 
     * Эта константа используется для выравнивания элемента по нижнему краю по оси Y при выравнивании.
     */
    constexpr const uint8_t ALIGN_Y_BOTTOM = 0b0001;
}

/**
 * TODO: docs
 */
class Terminal { 
public:
    typedef struct Block {
        Block*              parent;
        uint32_t            id;

        painter::figure_t*  fig;

        //dock&alignment
        uint16_t            align;
        uint16_t            dock;
    } Block;
    
    typedef painter::fig_size_t block_size_t ;
    typedef painter::pos_t      block_pos_t  ;

    Terminal();
    ~Terminal();

    /**
     *  TODO: docs
     */
    int Run();

    /**
     * 
     */
    int SetTitle(const char* str); 

    /**
     * 
     */
    Block* NewBlock(uint32_t id, figure_types type); 

    void SetBlockParent(Block* block, Block* parent); 
    void SetBlockPos (Block* block, block_pos_t x, block_pos_t y); 
    void SetBlockSize(Block* block, block_size_t width, block_size_t height);
    void SetBlockAlignX(Block* b, uint8_t align_x);
    void GetBlockAlignX(Block* b, uint8_t &align_x);  
    void SetBlockAlignY(Block* b, uint8_t align_y); 
    void GetBlockAlignY(Block* b, uint8_t &align_y);  
    void BlockDraw(Block *b);

    void SetBlockDockX(Block* b, uint8_t dock_x); 
    void GetBlockDockX(Block* b);  
    void SetBlockDockY(Block* b, uint8_t dock_y); 
    void GetBlockDockY(Block* b);  

    int GetBlockByID(uint32_t id);

    int DeleteBlockByPtr(Block* block);
    int DeleteBlockByID(uint32_t id);
protected: 
    static          uint32_t    task_list; 
    static          uint32_t    max_block_counts;
    static          Block*      block_list; 
    static          window_t    window;
    volatile static int         _t_stop_flag;
    /**
     * Signal handlers
     */
    static void sigint_handler(int val) { 
        _t_stop_flag = (val == SIGINT) ? 1 : 0;
    }

    void key_handler();
    /**
     * Internal default desings
     */
    void __draw_header(); 
    void __draw_center();
    void __draw_bottom(); 
private: 

}; 

#endif 

/**
 * TODO: dynamic text box
 */