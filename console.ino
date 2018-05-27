#include <LiquidCrystal.h>

// LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// http://man7.org/linux/man-pages/man4/console_codes.4.html
// http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-048.pdf

int lcd_x = 0;
int lcd_y = 0;

enum class TerminalState: uint8_t
{
  kInitial,
  kEscape,
  kControlSequence
};

class Terminal
{
private:
  const int m_width;
  const int m_height;
  int m_cursor_x;
  int m_cursor_y;
  int m_ctrl_seq[16];
  int m_ctrl_idx;
  TerminalState m_state;

public:
  Terminal(int w, int h) :
    m_width(w),
    m_height(h),
    m_cursor_x(0),
    m_cursor_y(0),
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
        lcd_x = 0;
        lcd_y += 1;
        if (lcd_y >= m_height)
        {
          lcd_y = 0;
        }
        lcd.setCursor(lcd_x, lcd_y);
        break;

      case '\r':
        lcd_x = 0;
        lcd.setCursor(lcd_x, lcd_y);
        break;

      default:
        lcd.write(c);
        lcd_x += 1;
        if (lcd_x >= m_width)
        {
          lcd_x = 0;
          lcd_y += 1;
          if (lcd_y >= m_height)
          {
            lcd_y = 0;
          }
          lcd.setCursor(lcd_x, lcd_y);
        }
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
          break;

        case 'B':
          // Move cursor down the indicated # of rows.
          break;

        case 'C':
          // Move cursor right the indicated # of columns.
          break;

        case 'D':
          // Move cursor left the indicated # of columns.
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
          m_cursor_x = m_ctrl_seq[0];
          m_cursor_y = m_ctrl_seq[1];
          lcd.setCursor(m_cursor_x, m_cursor_y);
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
            lcd.clear();
          }
          else if (m_ctrl_seq[0] == 3)
          {
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
  lcd.begin(20, 4);
  Serial.begin(115200);
  terminal.print("--Console V0.2--");
}

void loop()
{
  while (Serial.available())
  {
    char c = Serial.read();
    terminal.put_char(c);
  }
}

/* EOF */
