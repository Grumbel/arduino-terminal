// Copyright (c) 2018 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <LiquidCrystal.h>

// LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int BELL_PIN = 7;

// http://man7.org/linux/man-pages/man4/console_codes.4.html
// http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-048.pdf

enum class TerminalState: uint8_t
{
  kInitial,
  kEscape,
  kControlSequence
};

template<int PIN>
class Bell
{
private:
  unsigned int m_stoptime;

public:
  Bell() :
    m_stoptime(0)
  {
  }

  void begin() {
    pinMode(PIN, OUTPUT);
  }

  void ring(int msec) {
    digitalWrite(PIN, HIGH);
    m_stoptime = millis() + msec;
  }

  void update() {
    if (m_stoptime && millis() >= m_stoptime)
    {
      digitalWrite(PIN, LOW);
    }
  }
};

Bell<BELL_PIN> g_bell;

template<int WIDTH, int HEIGHT>
class Framebuffer
{
private:
  int m_cursor_x;
  int m_cursor_y;
  char m_buffer[HEIGHT][WIDTH];

public:
  Framebuffer() :
    m_cursor_x(0),
    m_cursor_y(0),
    m_buffer()
  {
  }

  void set_cursor(int x, int y)
  {
    m_cursor_x = x;
    m_cursor_y = y;
    lcd.setCursor(x, y);
  }

  void move_cursor(int dx, int dy)
  {
    m_cursor_x += dx;
    m_cursor_y += dy;

    if (m_cursor_x < 0)
      m_cursor_x = 0;
    else if (m_cursor_x >= WIDTH)
      m_cursor_x = WIDTH - 1;

    if (m_cursor_y < 0)
      m_cursor_y = 0;
    else if (m_cursor_y >= HEIGHT)
      m_cursor_y = HEIGHT - 1;

    lcd.setCursor(m_cursor_x, m_cursor_y);
  }

  void insert(char c)
  {
    if (m_cursor_x >= WIDTH)
    {
      next_row();
    }

    lcd.write(c);
    m_buffer[m_cursor_y][m_cursor_x] = c;
    m_cursor_x += 1;
  }

  void newline()
  {
    next_row();
  }

  void carriage_return()
  {
    m_cursor_x = 0;
    lcd.setCursor(m_cursor_x, m_cursor_y);
  }

  void clear()
  {
    lcd.clear();
    lcd.setCursor(m_cursor_x, m_cursor_y);

    //m_cursor_x = 0;
    //m_cursor_y = 0;

    for(int y = 0; y < HEIGHT; ++y)
    {
      memset(m_buffer[y], ' ', WIDTH);
    }
  }

private:
  void next_row()
  {
    m_cursor_x = 0;
    m_cursor_y += 1;

    if (m_cursor_y >= HEIGHT)
    {
      scroll_up();
      m_cursor_y = HEIGHT - 1;
    }
    lcd.setCursor(m_cursor_x, m_cursor_y);
  }

  /** Scroll up one line */
  void scroll_up()
  {
    for(int y = 0; y < HEIGHT - 1; ++y)
    {
      memcpy(m_buffer[y], m_buffer[y + 1], WIDTH);
    }
    memset(m_buffer[HEIGHT - 1], ' ', WIDTH);
    sync_buffer();
  }

  /** Copy the framebuffer to the LCD */
  void sync_buffer()
  {
    for(int y = 0; y < HEIGHT; ++y)
    {
      lcd.setCursor(0, y);
      for(int x = 0; x < WIDTH; ++x)
      {
        lcd.print(m_buffer[y][x]);
      }
    }
  }
};

Framebuffer<20, 4> framebuffer;

class Terminal
{
private:
  const int m_width;
  const int m_height;
  int m_ctrl_seq[16];
  int m_ctrl_idx;
  TerminalState m_state;

public:
  Terminal(int w, int h) :
    m_width(w),
    m_height(h),
    m_ctrl_seq(),
    m_ctrl_idx(0),
    m_state(TerminalState::kInitial)
  {}

  void print(const char* text)
  {
    while(*text)
    {
      put_char(*text++);
    }
  }

  void put_char(char c)
  {
    switch (m_state)
    {
      case TerminalState::kInitial:
        put_initial(c);
        break;

      case TerminalState::kEscape:
        put_escape(c);
        break;

      case TerminalState::kControlSequence:
        put_control_sequence(c);
        break;
    }
  }

  void put_initial(char c)
  {
    switch (c)
    {
      case 27:
        m_state = TerminalState::kEscape;
        break;

      case '\n':
        framebuffer.newline();
        break;

      case '\r':
        framebuffer.carriage_return();
        break;

      case '\a':
        g_bell.ring(250);
        break;

      case '\t':
        for(int i = 0; i < 8; ++i)
        {
          framebuffer.insert(' ');
        }
        break;

      default:
        framebuffer.insert(c);
        break;
    }
  }

  void put_escape(char c)
  {
    switch (c)
    {
      case '[':
        m_state = TerminalState::kControlSequence;
        m_ctrl_idx = 0;
        // FIXME: clear ctrl_seq
        m_ctrl_seq[m_ctrl_idx] = 0;
        break;

      case 'c':
        reset();
        break;

      default:
        break;
    }
  }

  void put_control_sequence(char c)
  {
    if (c == ';')
    {
      if (m_ctrl_idx < 15)
      {
        m_ctrl_idx += 1;
        m_ctrl_seq[m_ctrl_idx] = 0;
      }
      else
      {
        m_state = TerminalState::kInitial;
      }
    }
    else if ('0' <= c && c <= '9')
    {
      m_ctrl_seq[m_ctrl_idx] *= 10;
      m_ctrl_seq[m_ctrl_idx] = c - '0';
    }
    else
    {
      switch(c)
      {
        case '@':
          // Insert the indicated # of blank characters.
          break;

        case 'A':
          // Move cursor up the indicated # of rows.
          framebuffer.move_cursor(0, -m_ctrl_seq[0]);
          break;

        case 'B':
          // Move cursor down the indicated # of rows.
          framebuffer.move_cursor(0, m_ctrl_seq[0]);
          break;

        case 'C':
          // Move cursor right the indicated # of columns.
          framebuffer.move_cursor(m_ctrl_seq[0], 0);
          break;

        case 'D':
          // Move cursor left the indicated # of columns.
          framebuffer.move_cursor(-m_ctrl_seq[0], 0);
          break;

        case 'E':
          // Move cursor down the indicated # of rows, to column 1.
          break;

        case 'F':
          // Move cursor up the indicated # of rows, to column 1.
          break;

        case 'G':
          // Move cursor to indicated column in current row.
          break;

        case 'H':
          // Move cursor to the indicated row, column (origin at 1,1).
          framebuffer.set_cursor(m_ctrl_seq[0] - 1, m_ctrl_seq[1] - 1);
          break;

        case 'J':
          // Erase display (default: from cursor to end of display).
          // ESC [ 1 J: erase from start to cursor.
          // ESC [ 2 J: erase whole display.
          // ESC [ 3 J: erase whole display including scroll-back buffer (since Linux 3.0).
          if (m_ctrl_seq[0] == 1)
          {
          }
          else if (m_ctrl_seq[0] == 2)
          {
            framebuffer.clear();
          }
          else if (m_ctrl_seq[0] == 3)
          {
            framebuffer.clear();
          }
          break;

        case 'K':
          // Erase line (default: from cursor to end of line).
          // ESC [ 1 K: erase from start of line to cursor.
          // ESC [ 2 K: erase whole line.
          break;

        case 'L': // Insert the indicated # of blank lines.
          break;

        case 'M':
          // Delete the indicated # of lines.
          break;

        case 'P':
          // Delete the indicated # of characters on current line.
          break;

        case 'X':
          // Erase the indicated # of characters on current line.
          break;

        case 'a':
          // Move cursor right the indicated # of columns.
          break;

        case 'c':
          // Answer ESC [ ? 6 c: "I am a VT102".
          break;

        case 'd':
          // Move cursor to the indicated row, current column.
          break;

        case 'e':
          // Move cursor down the indicated # of rows.
          break;

        case 'f':
          // Move cursor to the indicated row, column.
          break;

        case 'g':
          // Without parameter: clear tab stop at current position.
          //ESC [ 3 g: delete all tab stops.
          break;

        case 'h':
          // Set Mode (see below).
          break;

        case 'l':
          // Reset Mode (see below).
          break;

        case 'm':
          // Set attributes (see below).
          break;

        case 'n':
          // Status report (see below).
          break;

        case 'q': // Set keyboard LEDs.
          break;

        case 'r':
          // DECSTBM   Set scrolling region; parameters are top and bottom row.
          break;

        case 's':
          // Save cursor location.
          break;

        case 'u':
          // Restore cursor location.
          break;

        case '`':
          // Move cursor to indicated column in current row.
          break;

        default:
          break;
      }

      m_state = TerminalState::kInitial;
    }
  }

  void reset()
  {
  }
};

Terminal terminal(20, 4);

void setup()
{
  g_bell.begin();
  lcd.begin(20, 4);
  Serial.begin(115200);
  terminal.print("--Console V0.3.2--");
}

void loop()
{
  while (Serial.available())
  {
    char c = Serial.read();
    terminal.put_char(c);
  }

  g_bell.update();
}

/* EOF */
