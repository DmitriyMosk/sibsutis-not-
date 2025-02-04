#include "terminal_api.hpp"

void term_winapi::terminal_api_instance() { 
    h_console       = GetStdHandle(STD_OUTPUT_HANDLE); 
    h_console_mode  = 0; 
}

int term_winapi::get_window_size(int &w, int &h) { 
    CONSOLE_SCREEN_BUFFER_INFO csbi; 

    if (GetConsoleScreenBufferInfo(h_console, &csbi)) { 
        w = csbi.srWindow.Right - csbi.srWindow.Left + 1; 
        h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1; 

        return 0;

    } else { 
        w = -1; 
        h = -1; 

        return -1;
    }
}

int term_winapi::get_cursor_pos(int &x, int &y) { 
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(h_console, &csbi)) {
        x = csbi.dwCursorPosition.X; 
        y = csbi.dwCursorPosition.Y;

        return 0; 
    } else {
        x = -1; 
        y = -1;

        return -1; 
    }
}

int term_winapi::set_cursor_pos(int x, int y) { 
    COORD coord = { 
        (SHORT)x, 
        (SHORT)y 
    };

    if (!SetConsoleCursorPosition(h_console, coord)) {
        return -1;
    }

    return 0; 
}

int term_winapi::set_cursor_type(bool visible, DWORD size) {
    CONSOLE_CURSOR_INFO cursorInfo;

    cursorInfo.bVisible = visible;
    cursorInfo.dwSize = size;     

    if (!SetConsoleCursorInfo(h_console, &cursorInfo)) {
        return -1;
    }
    
    return 0;
}

int term_winapi::get_terminal_mode(DWORD &mode) { 
    if (h_console_mode == 0) { 
        if (!GetConsoleMode(h_console, &mode)) {
            return -1;
        }
        h_console_mode = mode; 
    } else { 
        mode = h_console_mode;
    }

    return 0;
}

int term_winapi::set_terminal_mode(DWORD mode, bool operation) { 
    DWORD cur_mode;

    if (h_console_mode != 0) { 
        cur_mode = h_console_mode; 
    } else { 
        if (get_terminal_mode(cur_mode) != 0) { 
            return -1; 
        }
    }

    if (operation) { 
        cur_mode |=     (DWORD) mode; 
    } else { 
        cur_mode &= ~   (DWORD) mode; 
    }

    if (!SetConsoleMode(h_console, cur_mode)) {
        return -1; 
    }

    return 0; 
}

int term_winapi::out_symbol(const char *symbol, scr_color fclr, scr_color bclr) {
    set_csbi_color(fclr, bclr);

    DWORD written = 0;
    if (!WriteConsole(h_console, symbol, 1, &written, nullptr)) {
        return -1;
    }

    return 0;
}

int term_winapi::out_line(const char *symbols, unsigned long len, scr_color fclr, scr_color bclr) { 
    set_csbi_color(fclr, bclr);

    DWORD written = 0;
    if (!WriteConsole(h_console, symbols, len, &written, nullptr)) {
        return -1;
    }

    return 0; 
}

int term_winapi::set_csbi_color(scr_color fclr, scr_color bclr) { 
    WORD wAttr = (( (unsigned int) bclr << 4 ) | (unsigned int) fclr);

    if (!SetConsoleTextAttribute(h_console, wAttr)) { 
        return -1; 
    } 

    return 0; 
}

int term_winapi::get_csbi_color(scr_color &fclr, scr_color &bclr) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!GetConsoleScreenBufferInfo(h_console, &csbi)) {
        return -1; 
    }   

    fclr = (scr_color) (csbi.wAttributes & 0x0F);
    bclr = (scr_color) ((csbi.wAttributes & 0xF0) >> 4);

    return 0; 
}