// Seminarski rad
// Predmet: Programski jezici
// Autor: Đorđe Đukić, RT-8/16
// Visoka škola elektrotehnike i računarstva strukovnih studija
// Beograd
// 14. januar 2018.
// verzija 2

// 17. mart 2018.
// revizija 1: sređivanje koda

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <map>

#define WINDOW_WIDTH 72
#define WINDOW_HEIGHT 36

using namespace std;

CHAR_INFO con_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
COORD char_buf_size = {WINDOW_WIDTH, WINDOW_HEIGHT};
COORD char_pos = {0, 0};
SMALL_RECT con_write_area = {0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1};
CONSOLE_CURSOR_INFO cursor_info;

HANDLE out_handle;
HANDLE in_handle;

int points;
time_t timer;
bool exit_flag = false;

string scores_filename = "scores.bin";
fstream fScores;

struct Score
{
	int score;
	char name[16];
};

Score scores[10];

struct Borders {
	unsigned char
		Vertical,
		Horizontal,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
		UpT,
		DownT,
		LeftT,
		RightT,
		Cross;
};

const Borders SingleBorders = {0xB3, 0xC4, 0xDA, 0xBF, 0xC0, 0xD9, 0xC1, 0xC2, 0xB4, 0xC3, 0xC5};
const Borders DoubleBorders = {0xBA, 0xCD, 0xC9, 0xBB, 0xC8, 0xBC, 0xCA, 0xCB, 0xB9, 0xCC, 0xCE};

const unsigned char ShadowCharacter = 0xB1;
const unsigned char TransparentCharacter = 0xFF;

struct Style
{
	WORD color;

	unsigned int padding;

	const Borders *borders;
	WORD bordercolor;

	bool shadow;

	Style(WORD color, unsigned int padding, const Borders *borders, WORD bordercolor, bool shadow=false) :
	color(color), padding(padding), borders(borders), bordercolor(bordercolor), shadow(shadow) {};

	Style(WORD color, unsigned int padding, const Borders *borders, bool shadow=false) :
	color(color), padding(padding), borders(borders), bordercolor(color), shadow(shadow) {};

	Style(WORD color) :
	color(color), padding(0), borders(nullptr), bordercolor(color), shadow(false) {};
};

const Style RedWindow(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_RED | BACKGROUND_INTENSITY,
	1, &DoubleBorders, true);

const Style GreenWindow(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	1, &DoubleBorders, true);

const Style BlueWindow(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	1, &DoubleBorders, true);

const Style BlueFrame(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	7, &SingleBorders);

const Style WhiteFrame(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	1, &SingleBorders);

const Style WhiteShadedModal(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	2, nullptr, true);

const Style YellowTitle(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
const Style TealTitle(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY);

const Style BlueBorderedButton(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_BLUE,
	0, &SingleBorders);

const Style YellowBorderedButton(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	0, &SingleBorders);

const Style WhiteBorderedButton(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	0, &SingleBorders);

const Style GreyBorderlessButton(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	0, nullptr);

const Style RedBorderlessButton(
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |
	BACKGROUND_RED | BACKGROUND_INTENSITY,
	0, nullptr);

const Style WhiteInputBox(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	0, &DoubleBorders);

const Style YellowBox(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	0, nullptr);

const Style WhiteInputLine(
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	0, nullptr);

const Style BlueLabel(
	FOREGROUND_INTENSITY | 
	BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	0, nullptr);

string tables_filename = "asocijacije.txt";
ifstream fTables;

map<string,string> table;

void load_random_table()
{
	fTables.open(tables_filename);
	int num_of_lines;
	fTables >> num_of_lines;

	srand((unsigned int)time(nullptr));
	int chosen_table = rand() % num_of_lines;

	fTables.ignore();
	string line;
	for(int i = 0; i < chosen_table; i++)
	{
		getline(fTables, table["line"]);
	}

	for(char c = 'A'; c <= 'D'; c++)
	{
		string index;
		index = c;

		for (char i = '1'; i <= '4'; i++)
			getline(fTables, table[index + i], ',');
		getline(fTables, table[index], ',');
	}

	getline(fTables, table["???"]);

	fTables.close();
}

class Widget
{
protected:
	unsigned int width;
	unsigned int height;

	string text;

	void (*action)(Widget&);

	CHAR_INFO *buffer;

	// pointer to the last screen buffer the widget was written to (we need this for replace())
	CHAR_INFO *prev_scr_buffer;
	unsigned int prev_scr_width, prev_scr_height, prev_pos_x, prev_pos_y;

	friend void take_guess(Widget&);
	friend string scan_string(Widget&);

public:
	virtual void select() {}
	virtual void deselect() {}
	static void nop_callback(Widget& calling_widget) {}

	Widget(unsigned int width, unsigned int height, string text): width(width), height(height), text(text), action(nop_callback)
	{
		buffer = new CHAR_INFO[width * height];
	}

	~Widget()
	{
		delete buffer;
	}

	virtual void draw() {}

	void act()
	{
		action(*this);
	}

	unsigned int getWidth()
	{
		return width;
	}
	unsigned int getHeight()
	{
		return height;
	}

	const CHAR_INFO *getBuffer()
	{
		return buffer;
	}

	string getText()
	{
		return text;
	}

	void setText(string newText)
	{
		text = newText;
	}

	inline unsigned int coords(unsigned int x, unsigned int y)
	{
		return x + width * y;
	}

	bool place(CHAR_INFO *screen_buffer, unsigned int width, unsigned int height, unsigned int x, unsigned int y)
	{
		if(this->width + x > width || this->height + y > height)
			return false;

		prev_scr_buffer = screen_buffer;
		prev_scr_width = width;
		prev_scr_height = height;
		prev_pos_x = x;
		prev_pos_y = y;

		int buffer_index = 0;
		for (unsigned int j = y; j < y + this->height; j++)
		{
			for (unsigned int i = x; i < x + this->width; i++)
			{
				if(buffer[buffer_index].Char.AsciiChar != (char) TransparentCharacter)
				{
					if(buffer[buffer_index].Char.AsciiChar == (char) ShadowCharacter)
					{
						if(screen_buffer[i + width * j].Attributes & BACKGROUND_INTENSITY)
							screen_buffer[i + width * j].Attributes &= ~(BACKGROUND_INTENSITY | FOREGROUND_INTENSITY);
						else
							screen_buffer[i + width * j].Attributes &= ~(BACKGROUND_INTENSITY | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
					}
					else
					{
						screen_buffer[i + width * j].Char.AsciiChar = buffer[buffer_index].Char.AsciiChar;
						screen_buffer[i + width * j].Attributes = buffer[buffer_index].Attributes;
					}
				}
				buffer_index++;
			}
		}
		return true;
	}

	bool replace()
	{
		if(prev_scr_buffer)
		{
			place(prev_scr_buffer, prev_scr_width, prev_scr_height, prev_pos_x, prev_pos_y);
			return true;
		}
		else
			return false;
	}

	void bind_action(void (*callback_function)(Widget&))
	{
		action = callback_function;
	}
};

class Frame: public Widget
{
protected:
	Style style;

	unsigned int content_start_x, content_start_y, content_end_x, content_end_y;
	friend string scan_string(Widget&);

	void draw()
	{
		unsigned int border_thickness, shadow_thickness;

		if(style.shadow)
		{
			shadow_thickness = 1;

			buffer[coords(0, height - 1)].Char.AsciiChar = TransparentCharacter;
			buffer[coords(0, height - 1)].Attributes = 0;
			buffer[coords(width - 1, 0)].Char.AsciiChar = TransparentCharacter;
			buffer[coords(width - 1, 0)].Attributes = 0;

			for(unsigned int i = 1; i < width; i++)
			{
				buffer[coords(i, height - 1)].Char.AsciiChar = ShadowCharacter;
				buffer[coords(i, height - 1)].Attributes = 0;
			}
			for(unsigned int j = 1; j < height; j++)
			{
				buffer[coords(width - 1, j)].Char.AsciiChar = ShadowCharacter;
				buffer[coords(width - 1, j)].Attributes = 0;
			}
		}
		else
			shadow_thickness = 0;

		if(style.borders)
		{
			border_thickness = 1;

			// corners
			buffer[0].Char.AsciiChar = style.borders->TopLeft;
			buffer[0].Attributes = style.bordercolor;
			buffer[coords(width - 1 - shadow_thickness, 0)].Char.AsciiChar = style.borders->TopRight;
			buffer[coords(width - 1 - shadow_thickness, 0)].Attributes = style.bordercolor;
			buffer[coords(0, height - 1 - shadow_thickness)].Char.AsciiChar = style.borders->BottomLeft;
			buffer[coords(0, height - 1 - shadow_thickness)].Attributes = style.bordercolor;
			buffer[coords(width - 1 - shadow_thickness, height - 1 - shadow_thickness)].Char.AsciiChar = style.borders->BottomRight;
			buffer[coords(width - 1 - shadow_thickness, height - 1 - shadow_thickness)].Attributes = style.bordercolor;

			// edges
			for(unsigned int i = 1; i < width - 1 - shadow_thickness; i++)
			{
				buffer[coords(i, 0)].Char.AsciiChar = style.borders->Horizontal;
				buffer[coords(i, 0)].Attributes = style.bordercolor;
				buffer[coords(i, height - 1 - shadow_thickness)].Char.AsciiChar = style.borders->Horizontal;
				buffer[coords(i, height - 1 - shadow_thickness)].Attributes = style.bordercolor;
			}
			for(unsigned int j = 1; j < height - 1 - shadow_thickness; j++)
			{
				buffer[coords(0, j)].Char.AsciiChar = style.borders->Vertical;
				buffer[coords(0, j)].Attributes = style.bordercolor;
				buffer[coords(width - 1 - shadow_thickness, j)].Char.AsciiChar = style.borders->Vertical;
				buffer[coords(width - 1 - shadow_thickness, j)].Attributes = style.bordercolor;
			}
		}
		else
			border_thickness = 0;

		unsigned int pad_start_x = 0 + border_thickness;
		unsigned int pad_start_y = 0 + border_thickness;
		unsigned int pad_end_x = width - border_thickness - shadow_thickness;
		unsigned int pad_end_y = height - border_thickness - shadow_thickness;

		content_start_x = pad_start_x + style.padding;
		content_start_y = pad_start_y + style.padding;
		content_end_x = pad_end_x - style.padding;
		content_end_y = pad_end_y - style.padding;

		for(unsigned int j = pad_start_y; j < pad_end_y; j++)
		{
			for(unsigned int i = pad_start_x; i < pad_end_x; i++)
			{
				buffer[coords(i, j)].Char.AsciiChar = ' ';
				buffer[coords(i, j)].Attributes = style.color;
			}
		}
	}

public:
	Frame(unsigned int width, unsigned int height, Style style): Widget(width, height, ""), style(style)
	{
		draw();
	}

	Frame(unsigned int width, unsigned int height, string text, Style style): Widget(width, height, text), style(style)
	{
		draw();
	}

	void restyle(Style style)
	{
		this->style = style;
		draw();
	}
};

class Button: public Frame
{
	Style regular_style;
	Style selected_style;

public:
	void draw()
	{
		Frame::draw();

		unsigned int content_width = content_end_x - content_start_x;
		unsigned int content_height = content_end_y - content_start_y;

		unsigned int center_x = content_start_x + content_width / 2;
		unsigned int center_y = content_start_y + content_height / 2;

		unsigned int buffer_index = 0;
		unsigned int i;

		if(text.length() < content_width)
		{
			for(i = center_x - text.length() / 2; buffer_index < text.length(); i++, buffer_index++)
			{
				buffer[coords(i, center_y)].Char.AsciiChar = text[buffer_index];
				buffer[coords(i, center_y)].Attributes = style.color;
			}
		}
		else
		{
			for(i = content_start_x; i < content_end_x - 3; i++, buffer_index++)
			{
				buffer[coords(i, center_y)].Char.AsciiChar = text[buffer_index];
				buffer[coords(i, center_y)].Attributes = style.color;				
			}
			for(; i < content_end_x; i++)
			{
				buffer[coords(i, center_y)].Char.AsciiChar = '.';
				buffer[coords(i, center_y)].Attributes = style.color;
			}
		}
	}

	Button(unsigned int width, unsigned int height, Style style, Style selected_style, string text = ""): Frame(width, height, text, style), regular_style(style), selected_style(selected_style)
	{
		draw();
	}

	void restyle(Style style)
	{
		this->style = style;
		draw();
	}

	void select()
	{
		restyle(selected_style);
	}

	void deselect()
	{
		restyle(regular_style);
	}
};

class TextBox: public Frame
{
public:
	void draw()
	{
		Frame::draw();

		bool crlf;
		unsigned int buffer_index = 0;
		for(unsigned int j = content_start_y; j < content_end_y; j++)
		{
			crlf = false;
			for(unsigned int i = content_start_x; i < content_end_x; i++)
			{
				if(crlf)
				{
					buffer[coords(i, j)].Char.AsciiChar = ' ';
					buffer[coords(i, j)].Attributes = style.color;
				}
				else if(buffer_index < text.length())
				{
					buffer[coords(i, j)].Char.AsciiChar = text[buffer_index];
					buffer[coords(i, j)].Attributes = style.color;

					buffer_index++;

					if(buffer_index < text.length() && text[buffer_index] == '\n')
					{
						crlf = true;
						buffer_index++;
					}
				}
				// if we printed out all the text
				else
					break;
			}
		}
	}

	TextBox(unsigned int width, unsigned int height, Style style, string text = ""): Frame(width, height, text, style)
	{
		draw();
	}
};

class NavigationMap
{
	int width, height, x, y;
	Widget ***widgets;

public:
	NavigationMap(int width, int height): width(width), height(height), x(0), y(0)
	{
		widgets = new Widget**[width];
		for(int i = 0; i < width; i++)
		{
			widgets[i] = new Widget*[height];
			for(int j = 0; j < height; j++)
				widgets[i][j] = nullptr;
		}
	}
	~NavigationMap()
	{
		for(int i = 0; i < width; i++)
			delete widgets[i];
		delete widgets;
	}

	Widget **operator[](int index)
	{
		return widgets[index];
	}

	int getX()
	{
		return x;
	}

	int getY()
	{
		return y;
	}

	Widget *current()
	{
		return widgets[x][y];
	}

	void move(int newX, int newY)
	{
		widgets[x][y]->deselect();
		widgets[x][y]->replace();
		widgets[newX][newY]->select();
		widgets[newX][newY]->replace();
		x = newX;
		y = newY;
	}

	bool up()
	{
		if(widgets[x][y] && y-1 >= 0 && widgets[x][y-1])
		{
			if(widgets[x][y] == widgets[x][y-1])
			{
				y--;
				up();
			}
			else
				move(x, y-1);
			return true;
		}
		else
			return false;
	}
	bool down()
	{
		if(widgets[x][y] && y+1 < height && widgets[x][y+1])
		{
			if(widgets[x][y] == widgets[x][y+1])
			{
				y++;
				down();
			}
			else
				move(x, y+1);
			return true;
		}
		else
			return false;
	}
	bool left()
	{
		if(widgets[x][y] && x-1 >= 0 && widgets[x-1][y])
		{
			if(widgets[x][y] == widgets[x-1][y])
			{
				x--;
				left();
			}
			else
				move(x-1, y);
			return true;
		}
		else
			return false;
	}
	bool right()
	{
		if(widgets[x][y] && x+1 < width && widgets[x+1][y])
		{
			if(widgets[x][y] == widgets[x+1][y])
			{
				x++;
				right();
			}
			else
				move(x+1, y);
			return true;
		}
		else
			return false;
	}
};

NavigationMap asocijacije(2, 11);

// read console input buffer and return malloc'd INPUT_RECORD array
DWORD getInput(INPUT_RECORD **eventBuffer)
{
	DWORD numEvents = 0;
	DWORD numEventsRead = 0;

	GetNumberOfConsoleInputEvents(in_handle, &numEvents);

	if (numEvents)
	{
		*eventBuffer = (INPUT_RECORD*)malloc(sizeof(INPUT_RECORD) * numEvents);
		ReadConsoleInput(in_handle, *eventBuffer, numEvents, &numEventsRead);
	}

	return numEventsRead;
}

void set_exit_flag(Widget &calling_widget)
{
	exit_flag = true;
}

void exit_dialog()
{
	CHAR_INFO old_con_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
	memcpy(old_con_buffer, con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));

	NavigationMap dialog_NavigationMap(2, 1);

	TextBox dialog(40, 8, WhiteShadedModal, "Da li zelite da izadjete iz igre?");
	dialog.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 16, 14);

	Button yes(8, 1, GreyBorderlessButton, RedBorderlessButton, "Da");
	yes.select();
	yes.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 26, 19);
	yes.bind_action(set_exit_flag);
	dialog_NavigationMap[0][0] = &yes;

	Button no(8, 1, GreyBorderlessButton, RedBorderlessButton, "Ne");
	no.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 36, 19);
	dialog_NavigationMap[1][0] = &no;

	// put dialog on the screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);

	DWORD numEventsRead = 0;
	INPUT_RECORD *eventBuffer;

	bool write = false;
	bool loop_flag = true;
	while(loop_flag)
	{
		WaitForSingleObject(in_handle, INFINITE);
		numEventsRead = getInput(&eventBuffer);

		if (numEventsRead)
		{
			for (unsigned int i = 0; i < numEventsRead; i++)
			{
				switch (eventBuffer[i].EventType)
				{
				case KEY_EVENT:
					if (eventBuffer[i].Event.KeyEvent.bKeyDown)
					{
						switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_ESCAPE:
							loop_flag = false;
							break;
						case VK_LEFT:
							write = dialog_NavigationMap.left();
							break;
						case VK_RIGHT:
							write = dialog_NavigationMap.right();
							break;
						case VK_RETURN:
							dialog_NavigationMap.current()->act();
							loop_flag = false;
							break;
						}
						break;
					}
				}
			}
			free(eventBuffer);
		}

		// screen update
		if (write)
		{
			WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
			write = false;
		}
	}
	// restore previous screen contents
	memcpy(con_buffer, old_con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));
	// redraw screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
}

void reveal_answer(Widget &calling_widget)
{
	string current_text = calling_widget.getText();
	auto iter = table.find(current_text);
	if(iter != table.end())
	{
		calling_widget.setText(table[current_text]);
		calling_widget.draw();
		calling_widget.replace();
		calling_widget.bind_action(Widget::nop_callback);
	}
}

string scan_string(Widget &calling_widget)
{
	TextBox input_box(calling_widget.getWidth(), calling_widget.getHeight(), WhiteInputBox);
	input_box.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, calling_widget.prev_pos_x, calling_widget.prev_pos_y);
	unsigned int content_width = input_box.content_end_x - input_box.content_start_x;

	GetConsoleCursorInfo(out_handle, &cursor_info);
	cursor_info.bVisible = true;
	SetConsoleCursorInfo(out_handle, &cursor_info);

	COORD cursor_pos = {input_box.prev_pos_x + input_box.content_start_x, input_box.prev_pos_y + input_box.content_start_y};
	SetConsoleCursorPosition(out_handle, cursor_pos);

	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);

	DWORD numEventsRead = 0;
	INPUT_RECORD *eventBuffer;
	string buffer;
	unsigned int insertion_point = 0;

	bool cancelled = false;
	bool write = false;
	bool loop_flag = true;
	while(loop_flag)
	{
		WaitForSingleObject(in_handle, INFINITE);
		numEventsRead = getInput(&eventBuffer);

		if (numEventsRead)
		{
			for (unsigned int i = 0; i < numEventsRead; i++)
			{
				switch (eventBuffer[i].EventType)
				{
				case KEY_EVENT:
					if (eventBuffer[i].Event.KeyEvent.bKeyDown)
					{
						if (isalpha(eventBuffer[i].Event.KeyEvent.uChar.AsciiChar) && buffer.length() < content_width)
						{
							buffer.insert(insertion_point, 1, eventBuffer[i].Event.KeyEvent.uChar.AsciiChar);
							input_box.setText(buffer);
							insertion_point++;
							write = true;
						}
						else
						{
							switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode)
							{
							case VK_ESCAPE:
								loop_flag = false;
								cancelled = true;
								write = true;
								break;
							case VK_LEFT:
								if(insertion_point > 0)
									insertion_point--;
								write = true;
								break;
							case VK_RIGHT:
								if(insertion_point < buffer.length()-1)
									insertion_point++;
								write = true;
								break;
							case VK_RETURN:
								loop_flag = false;
								write = true;
								break;
							case VK_DELETE:
								if(insertion_point < buffer.length())
								{
									buffer.erase(insertion_point, 1);
									input_box.setText(buffer);
									write = true;
								}						
								break;
							case VK_BACK:
								if(insertion_point > 0)
								{
									insertion_point--;
									buffer.erase(insertion_point, 1);
									input_box.setText(buffer);
									write = true;
								}
								break;
							}
						}
						break;
					}
				}
			}
			free(eventBuffer);
		}

		// screen update
		if (write)
		{
			input_box.draw();
			input_box.replace();
			cursor_pos.X = input_box.prev_pos_x + input_box.content_start_x + insertion_point;
			SetConsoleCursorPosition(out_handle, cursor_pos);
			WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
			write = false;
		}
	}
	cursor_info.bVisible = false;
	SetConsoleCursorInfo(out_handle, &cursor_info);

	if(cancelled)
		return "";
	else
		return buffer;
}

bool check_if_high_score();
void add_score();
void display_scores();
void write_scores();

void take_guess(Widget &calling_widget)
{
	if(scan_string(calling_widget) == table[calling_widget.getText()])
	{
		if(calling_widget.getText() == "???")
		{
			points += 1000;
			points -= (int)(time(nullptr) - timer);
			for(int i = 0; i < 2; i++)
			{
				reveal_answer(*asocijacije[0][4]);
				reveal_answer(*asocijacije[0][6]);
				reveal_answer(*asocijacije[1][4]);
				reveal_answer(*asocijacije[1][6]);
				for(int j = 0; j < 4; j++)
					asocijacije[i][j]->act();
				for(int j = 7; j < 11; j++)
					asocijacije[i][j]->act();
			}
			if(check_if_high_score())
				add_score();
		}
		else
		{
			points += 200;
			int curX = asocijacije.getX();
			int curY = asocijacije.getY();
			if(curY < 5)
				for(int i = 0; i < 4; i++)
					asocijacije[curX][i]->act();
			else
				for(int i = 7; i < 11; i++)
					asocijacije[curX][i]->act();
		}
		reveal_answer(calling_widget);
	}
	else
		calling_widget.replace();
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
}

void warning_dialog(string text)
{
	CHAR_INFO old_con_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
	memcpy(old_con_buffer, con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));

	TextBox dialog(40, 8, WhiteShadedModal, "Da li zelite da izadjete iz igre?");
	dialog.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 16, 14);

	Button ok(8, 1, GreyBorderlessButton, RedBorderlessButton, "OK");
	ok.select();
	ok.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 19);

	// put dialog on the screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);

	DWORD numEventsRead = 0;
	INPUT_RECORD *eventBuffer;

	bool write = false;
	bool loop_flag = true;
	while(loop_flag)
	{
		WaitForSingleObject(in_handle, INFINITE);
		numEventsRead = getInput(&eventBuffer);

		if (numEventsRead)
		{
			for (unsigned int i = 0; i < numEventsRead; i++)
			{
				switch (eventBuffer[i].EventType)
				{
				case KEY_EVENT:
					if (eventBuffer[i].Event.KeyEvent.bKeyDown)
					{
						switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_ESCAPE:
						case VK_RETURN:
							ok.act();
							loop_flag = false;
						}
						break;
					}
					break;
				}
			}
			free(eventBuffer);
		}

		// screen update
		if (write)
		{
			WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
			write = false;
		}
	}
	// restore previous screen contents
	memcpy(con_buffer, old_con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));
	// redraw screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
}

bool check_if_high_score()
{
	return points > scores[9].score;
}

void add_score()
{
	CHAR_INFO old_con_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
	memcpy(old_con_buffer, con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));

	TextBox dialog(40, 10, WhiteShadedModal, "Cestitam! Vas rezultat je medju top 10! Unesite svoje ime:");
	dialog.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 16, 12);

	Button bName(17, 3, WhiteBorderedButton, YellowBorderedButton, "Ime");
	bName.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 25, 17);

	// put dialog on the screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);

	string name = scan_string(bName);

	int i, j;
	for (i = 0; i < 10, points <= scores[i].score; i++);

	for(j = 9; j > i; j--)
	{
		scores[j] = scores[j-1];
	}

	scores[i].score = points;
	strcpy_s(scores[i].name, name.c_str());

	// restore previous screen contents
	memcpy(con_buffer, old_con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));
	// redraw screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
	display_scores();
	write_scores();
}

void display_scores()
{
	CHAR_INFO old_con_buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
	memcpy(old_con_buffer, con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));

	TextBox dialog(40, 18, WhiteShadedModal, "Najbolji rezultati:");
	dialog.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 16, 9);

	Button ok(8, 1, GreyBorderlessButton, RedBorderlessButton, "OK");
	ok.select();
	ok.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 24);

	stringstream line;

	for (int i = 0; i < 10; i++)
		line << right << setw(2) << i+1 << ". " << left << setw(15) << scores[i].name << right << setw(7) << scores[i].score << endl;

	TextBox results(26, 10, YellowBox, line.str());
	results.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 22, 13);

	// put dialog on the screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);

	DWORD numEventsRead = 0;
	INPUT_RECORD *eventBuffer;

	bool write = false;
	bool loop_flag = true;
	while(loop_flag)
	{
		WaitForSingleObject(in_handle, INFINITE);
		numEventsRead = getInput(&eventBuffer);

		if (numEventsRead)
		{
			for (unsigned int i = 0; i < numEventsRead; i++)
			{
				switch (eventBuffer[i].EventType)
				{
				case KEY_EVENT:
					if (eventBuffer[i].Event.KeyEvent.bKeyDown)
					{
						switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_ESCAPE:
						case VK_RETURN:
							ok.act();
							loop_flag = false;
							break;
						}
						break;
					}
				}
			}
			free(eventBuffer);
		}

		// screen update
		if (write)
		{
			WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
			write = false;
		}
	}
	// restore previous screen contents
	memcpy(con_buffer, old_con_buffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(CHAR_INFO));
	// redraw screen
	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
}

void write_scores()
{
	fScores.open(scores_filename, ios::binary | ios::out);
	if(fScores.is_open())
	{
		fScores.write((char *)scores, 10*sizeof(Score));
		fScores.close();
	}
	else
		warning_dialog("Greska pri pisanju datoteke sa rezultatima.");
}

void read_scores()
{
	fScores.open(scores_filename, ios::binary | ios::in);
	if(fScores.is_open())
	{
		fScores.read((char *)scores, 10*sizeof(Score));
		fScores.close();
	}
	else
	{
		for (int i = 0; i < 10; i++)
		{
			scores[i].score = 0;
			strcpy_s(scores[i].name, "-");
		}
		write_scores();
	}
}

int main(void)
{
	unsigned int x, y;

	srand((unsigned int) time(0));

	SMALL_RECT window_size = {0, 0, WINDOW_WIDTH-1, WINDOW_HEIGHT-1};
	COORD buffer_size = {WINDOW_WIDTH, WINDOW_HEIGHT};

	COORD char_buf_size = {WINDOW_WIDTH, WINDOW_HEIGHT};
	COORD char_pos = {0, 0};
	SMALL_RECT con_write_area = {0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1};

	// initialize handles
	out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	in_handle = GetStdHandle(STD_INPUT_HANDLE);

	// pointer to type INPUT_RECORD, used to point to a buffer of INPUT_RECORD structs
	INPUT_RECORD *eventBuffer;

	DWORD numEventsRead = 0;

	GetConsoleCursorInfo(out_handle, &cursor_info);
	cursor_info.bVisible = false;
	SetConsoleCursorInfo(out_handle, &cursor_info);

	SetConsoleTitle(L"Asocijacije");

	SetConsoleWindowInfo(out_handle, TRUE, &window_size);
	SetConsoleScreenBufferSize(out_handle, buffer_size);

	char c = 0;

	// blank buffer
	for (y = 0; y < WINDOW_HEIGHT; ++y)
	{
		for (x = 0; x < WINDOW_WIDTH; ++x)
		{
			con_buffer[x + WINDOW_WIDTH * y].Char.AsciiChar = ' ';
			con_buffer[x + WINDOW_WIDTH * y].Attributes = BACKGROUND_BLUE;
		}
	}

	TextBox frame(72, 36, BlueFrame);
	frame.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0);

	TextBox title(13, 1, TealTitle, " Asocijacije ");
	title.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0);

	TextBox instructions(52, 1, BlueLabel, " Esc - izlazak, H - tabela rezultata, R - nova igra ");
	instructions.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 35);

	read_scores();
	points = 0;
	timer = time(nullptr);
	load_random_table();

	Button a1(18, 3, BlueBorderedButton, YellowBorderedButton, "A1");
	Button a2(18, 3, BlueBorderedButton, YellowBorderedButton, "A2");
	Button a3(18, 3, BlueBorderedButton, YellowBorderedButton, "A3");
	Button a4(18, 3, BlueBorderedButton, YellowBorderedButton, "A4");
	Button col_a(18, 3, BlueBorderedButton, YellowBorderedButton, "A");
	a1.select();
	a1.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 5, 2);
	a2.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 8, 5);
	a3.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 11, 8);
	a4.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 14, 11);
	col_a.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 17, 14);
	asocijacije[0][0] = &a1;
	asocijacije[0][1] = &a2;
	asocijacije[0][2] = &a3;
	asocijacije[0][3] = &a4;
	asocijacije[0][4] = &col_a;
	a1.bind_action(reveal_answer);
	a2.bind_action(reveal_answer);
	a3.bind_action(reveal_answer);
	a4.bind_action(reveal_answer);
	col_a.bind_action(take_guess);

	Button b1(18, 3, BlueBorderedButton, YellowBorderedButton, "B1");
	Button b2(18, 3, BlueBorderedButton, YellowBorderedButton, "B2");
	Button b3(18, 3, BlueBorderedButton, YellowBorderedButton, "B3");
	Button b4(18, 3, BlueBorderedButton, YellowBorderedButton, "B4");
	Button col_b(18, 3, BlueBorderedButton, YellowBorderedButton, "B");
	b1.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 49, 2);
	b2.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 46, 5);
	b3.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 43, 8);
	b4.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 40, 11);
	col_b.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 37, 14);
	asocijacije[1][0] = &b1;
	asocijacije[1][1] = &b2;
	asocijacije[1][2] = &b3;
	asocijacije[1][3] = &b4;
	asocijacije[1][4] = &col_b;
	b1.bind_action(reveal_answer);
	b2.bind_action(reveal_answer);
	b3.bind_action(reveal_answer);
	b4.bind_action(reveal_answer);
	col_b.bind_action(take_guess);

	Button c1(18, 3, BlueBorderedButton, YellowBorderedButton, "C1");
	Button c2(18, 3, BlueBorderedButton, YellowBorderedButton, "C2");
	Button c3(18, 3, BlueBorderedButton, YellowBorderedButton, "C3");
	Button c4(18, 3, BlueBorderedButton, YellowBorderedButton, "C4");
	Button col_c(18, 3, BlueBorderedButton, YellowBorderedButton, "C");
	c1.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 5, 32);
	c2.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 8, 29);
	c3.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 11, 26);
	c4.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 14, 23);
	col_c.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 17, 20);
	asocijacije[0][6] = &col_c;
	asocijacije[0][7] = &c4;
	asocijacije[0][8] = &c3;
	asocijacije[0][9] = &c2;
	asocijacije[0][10] = &c1;
	c1.bind_action(reveal_answer);
	c2.bind_action(reveal_answer);
	c3.bind_action(reveal_answer);
	c4.bind_action(reveal_answer);
	col_c.bind_action(take_guess);

	Button d1(18, 3, BlueBorderedButton, YellowBorderedButton, "D1");
	Button d2(18, 3, BlueBorderedButton, YellowBorderedButton, "D2");
	Button d3(18, 3, BlueBorderedButton, YellowBorderedButton, "D3");
	Button d4(18, 3, BlueBorderedButton, YellowBorderedButton, "D4");
	Button col_d(18, 3, BlueBorderedButton, YellowBorderedButton, "D");
	d1.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 49, 32);
	d2.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 46, 29);
	d3.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 43, 26);
	d4.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 40, 23);
	col_d.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 37, 20);
	asocijacije[1][6] = &col_d;
	asocijacije[1][7] = &d4;
	asocijacije[1][8] = &d3;
	asocijacije[1][9] = &d2;
	asocijacije[1][10] = &d1;
	d1.bind_action(reveal_answer);
	d2.bind_action(reveal_answer);
	d3.bind_action(reveal_answer);
	d4.bind_action(reveal_answer);
	col_d.bind_action(take_guess);

	Button solution(30, 3, BlueBorderedButton, YellowBorderedButton, "???");
	solution.place(con_buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 21, 17);
	asocijacije[0][5] = &solution;
	asocijacije[1][5] = &solution;
	solution.bind_action(take_guess);

	WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);

	bool write = false;
	bool loop_flag = true;
	while(loop_flag)
	{
		WaitForSingleObject(in_handle, INFINITE);
		numEventsRead = getInput(&eventBuffer);

		if (numEventsRead)
		{
			for (unsigned int i = 0; i < numEventsRead; i++)
			{
				switch (eventBuffer[i].EventType)
				{
				case KEY_EVENT:
					// we only react on keydowns, not keyups
					if (eventBuffer[i].Event.KeyEvent.bKeyDown)
					{
						switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_ESCAPE:
							exit_dialog();
							if(exit_flag)
								loop_flag = false;
							break;
						case VK_UP:
							// only write to screen if move was successful
							write = asocijacije.up();
							break;
						case VK_DOWN:
							write = asocijacije.down();
							break;
						case VK_LEFT:
							write = asocijacije.left();
							break;
						case VK_RIGHT:
							write = asocijacije.right();
							break;
						case VK_RETURN:
							asocijacije.current()->act();
							write = true;
							break;
						case 0x48:	// H
							display_scores();
							break;
						case 0x52:	// R
							points = 0;
							timer = time(nullptr);
							load_random_table();

							a1.setText("A1");
							a2.setText("A2");
							a3.setText("A3");
							a4.setText("A4");
							col_a.setText("A");
							b1.setText("B1");
							b2.setText("B2");
							b3.setText("B3");
							b4.setText("B4");
							col_b.setText("B");
							c1.setText("C1");
							c2.setText("C2");
							c3.setText("C3");
							c4.setText("C4");
							col_c.setText("C");
							d1.setText("D1");
							d2.setText("D2");
							d3.setText("D3");
							d4.setText("D4");
							col_d.setText("D");
							solution.setText("???");
							a1.draw();
							a1.replace();
							a2.draw();
							a2.replace();
							a3.draw();
							a3.replace();
							a4.draw();
							a4.replace();
							col_a.draw();
							col_a.replace();
							b1.draw();
							b1.replace();
							b2.draw();
							b2.replace();
							b3.draw();
							b3.replace();
							b4.draw();
							b4.replace();
							col_b.draw();
							col_b.replace();
							c1.draw();
							c1.replace();
							c2.draw();
							c2.replace();
							c3.draw();
							c3.replace();
							c4.draw();
							c4.replace();
							col_c.draw();
							col_c.replace();
							d1.draw();
							d1.replace();
							d2.draw();
							d2.replace();
							d3.draw();
							d3.replace();
							d4.draw();
							d4.replace();
							col_d.draw();
							col_d.replace();
							solution.draw();
							solution.replace();
							a1.bind_action(reveal_answer);
							a2.bind_action(reveal_answer);
							a3.bind_action(reveal_answer);
							a4.bind_action(reveal_answer);
							col_a.bind_action(take_guess);
							b1.bind_action(reveal_answer);
							b2.bind_action(reveal_answer);
							b3.bind_action(reveal_answer);
							b4.bind_action(reveal_answer);
							col_b.bind_action(take_guess);
							c1.bind_action(reveal_answer);
							c2.bind_action(reveal_answer);
							c3.bind_action(reveal_answer);
							c4.bind_action(reveal_answer);
							col_c.bind_action(take_guess);
							d1.bind_action(reveal_answer);
							d2.bind_action(reveal_answer);
							d3.bind_action(reveal_answer);
							d4.bind_action(reveal_answer);
							col_d.bind_action(take_guess);
							solution.bind_action(take_guess);

							write = true;
							break;
						}
					}
					break;
				}
			}
			free(eventBuffer);
		}

		// screen update
		if (write)
		{
			WriteConsoleOutputA(out_handle, con_buffer, char_buf_size, char_pos, &con_write_area);
			write = false;
		}
	}

	return 0;
}