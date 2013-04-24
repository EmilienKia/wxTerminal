/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * wxTerminal
 * Copyright (C) Emilien Kia 2012 <emilien.kia@free.fr>
 * 
 * wxTerminal is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * wxTerminal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>

#include <wx/dcbuffer.h>
#include <wx/caret.h>
#include <wx/event.h>

#include <cstring>
using namespace std;

#include "terminal-ctrl.hpp"

//
//
// wxTerminalCharacter
//
//

wxTerminalCharacter wxTerminalCharacter::DefaultCharacter = { 0, { 7, 0, wxTCS_Invisible} };

//
//
// wxTerminalContent
//
//

void wxTerminalContent::setChar(wxPoint pos, wxTerminalCharacter c)
{
	ensureHasChar(pos.y, pos.x);
	at(pos.y)[pos.x] = c;
}

void wxTerminalContent::insertChar(wxPoint pos, wxTerminalCharacter c)
{
	ensureHasChar(pos.y, pos.x);
	wxTerminalLine& line = at(pos.y);
	line.insert(line.begin()+pos.x, c);
}

void wxTerminalContent::addNewLine()
{
	push_back(wxTerminalLine());
}

void wxTerminalContent::ensureHasLine(size_t l)
{
	if(size()<=l)
		resize(l+1);
}

void wxTerminalContent::ensureHasChar(size_t l, size_t c)
{
	ensureHasLine(l);
	
	wxTerminalLine& line = at(l);

	if(line.size()<=c)
		line.resize(c+1, wxTerminalCharacter::DefaultCharacter);
}

//
//
// wxTerminalScreen
//
//

wxTerminalScreen::wxTerminalScreen():
_originPosition(0, 0),
_caretPosition(0,0),
_size(80, 25)
{
}

void wxTerminalScreen::setChar(wxPoint pos, wxTerminalCharacter ch)
{
	_content.setChar(pos + _originPosition, ch);
}

void wxTerminalScreen::setChar(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.attr  = attr;
	_content.setChar(pos + _originPosition, ch);
}

void wxTerminalScreen::setCharAbsolute(wxPoint pos, wxTerminalCharacter ch)
{
	_content.setChar(pos, ch);
}

void wxTerminalScreen::setCharAbsolute(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.attr  = attr;
	_content.setChar(pos, ch);
}


void wxTerminalScreen::insertChar(wxPoint pos, wxTerminalCharacter ch)
{
	_content.insertChar(pos + _originPosition, ch);
	// TODO Validate content here ? (split long lines ?)
}

void wxTerminalScreen::insertChar(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.attr  = attr;
	// TODO Validate content here ? (split long lines ?)
	_content.insertChar(pos + _originPosition, ch);
}

void wxTerminalScreen::insertCharAbsolute(wxPoint pos, wxTerminalCharacter ch)
{
	_content.insertChar(pos, ch);
	// TODO Validate content here ? (split long lines ?)
}

void wxTerminalScreen::insertCharAbsolute(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.attr  = attr;
	// TODO Validate content here ? (split long lines ?)
	_content.insertChar(pos, ch);
}

void wxTerminalScreen::setCaretPosition(wxPoint pos)
{
	_caretPosition = pos + _originPosition;
}

void wxTerminalScreen::setCaretAbsolutePosition(wxPoint pos)
{
	_caretPosition = pos;
}

void wxTerminalScreen::moveOrigin(int lines)
{
	_originPosition.y += lines;
	if(_originPosition.y<0) // Sanitize to 0 (origin cannot be before begining of history).
		_originPosition.y = 0;
}

void wxTerminalScreen::setOrigin(int lines)
{
	_originPosition.y = lines;
	if(_originPosition.y<0) // Sanitize to 0 (origin cannot be before begining of history).
		_originPosition.y = 0;
}

void wxTerminalScreen::insertChar(wxUniChar c, const wxTerminalCharacterAttributes& attr)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.attr  = attr;
	_content.insertChar(_caretPosition, ch);
	// TODO Validate content here ? (split long lines ?)
	moveCaret(0, 1);
}

void wxTerminalScreen::overwriteChar(wxUniChar c, const wxTerminalCharacterAttributes& attr)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.attr  = attr;
	_content.setChar(_caretPosition, ch);
	moveCaret(0, 1);
}

void wxTerminalScreen::moveCaret(int lines, int cols)
{
	_caretPosition.y += lines;
	_caretPosition.x += cols;

	if(cols>0)
	{
		while(_caretPosition.x >= _size.x)
		{
			_caretPosition.x -= _size.x;
			_caretPosition.y ++;
		}
	}
	else // if(cols<0)
	{
		while(_caretPosition.x < 0)
		{
			_caretPosition.x += _size.x;
			_caretPosition.y --;
		}
	}

	// TODO Ensure caret is at a valid place in history (no underflow)
}

void wxTerminalScreen::setCaretColumn(int col)
{
	wxTerminalLine& line = getLine(getCaretPosition().y);

	if(line.size() == 0)
		col = 0;
	else if(col<0 || col>= line.size())
		col = line.size() - 1;

	_caretPosition.x = col;
}


//
//
// wxTerminalCharacterDecoder
//
// Decode UTF-8 only at present day.
//
//
wxTerminalCharacterDecoder::wxTerminalCharacterDecoder():
_state(wxTCDSTATE_DEFAULT),
_buffer(0)
{
}

bool wxTerminalCharacterDecoder::add(unsigned char c, wxUniChar& ch)
{
	switch(_state)
	{
		case wxTCDSTATE_DEFAULT:
		{
			if( (c & 0x80) == 0 )  // UTF-8, first char == 0xxxxxxx
			{
				// Only one byte.
				ch = wxUniChar(_buffer = (unsigned long)c);
				return true;
			}
			else if( (c & 0xE0) == 0xC0 ) // UTF-8, first char = 110xxxxx
			{
				// Wait for two bytes.
				_buffer = (c & 0x1F) << 6;
				_state = wxTCDSTATE_UTF8_2BYTES;
				return false;
			}
			else if( (c & 0xF0) == 0xE0 ) // UTF-8, first char = 1110xxxx
			{
				// Wait for two bytes.
				_buffer = (c & 0x0F) << 12;
				_state = wxTCDSTATE_UTF8_3BYTES_1;
				return false;
			}
			else if( (c & 0xF8) == 0xF0 ) // UTF-8, first char = 11110xxx
			{
				// Wait for two bytes.
				_buffer = (c & 0x07) << 18;
				_state = wxTCDSTATE_UTF8_3BYTES_1;
				return false;
			}
			else
			{
				// Bad encoding, reset !
				ch = wxUniChar(_buffer = (unsigned long)c);
				return true;
			}
			break;
		}
		case wxTCDSTATE_UTF8_2BYTES:
		case wxTCDSTATE_UTF8_3BYTES_2:
		case wxTCDSTATE_UTF8_4BYTES_3:
		{
			if( (c & 0xC0) == 0x80 )  // UTF-8, other char == 10xxxxxx
			{
				ch = wxUniChar( _buffer |= (unsigned long) (c & 0x3F) );
				_state = wxTCDSTATE_DEFAULT;
				return true;
			}
			else
			{
				// Bad encoding, reset !
				// TODO WTF ?
				_buffer = 0;
				_state = wxTCDSTATE_DEFAULT;
				return false;
			}
			break;
		}
		case wxTCDSTATE_UTF8_3BYTES_1:
		case wxTCDSTATE_UTF8_4BYTES_2:
		{
			if( (c & 0xC0) == 0x80 )  // UTF-8, other char == 10xxxxxx
			{
				 _buffer |= (((unsigned long) (c & 0x3F)) << 6);
				if(_state == wxTCDSTATE_UTF8_3BYTES_1)
					_state = wxTCDSTATE_UTF8_3BYTES_2;
				else
					_state = wxTCDSTATE_UTF8_4BYTES_3;
				return false;
			}
			else
			{
				// Bad encoding, reset !
				// TODO WTF ?
				_buffer = 0;
				_state = wxTCDSTATE_DEFAULT;
				return false;
			}
			break;
		}
		case wxTCDSTATE_UTF8_4BYTES_1:
		{
			if( (c & 0xC0) == 0x80 )  // UTF-8, other char == 10xxxxxx
			{
				 _buffer |= (((unsigned long) (c & 0x3F)) << 12);
				_state = wxTCDSTATE_UTF8_4BYTES_2;
				return false;
			}
			else
			{
				// Bad encoding, reset !
				// TODO WTF ?
				_buffer = 0;
				_state = wxTCDSTATE_DEFAULT;
				return false;
			}
			break;
		}
		default:
		{
			// Bad encoding, reset !
			// TODO WTF ?
			_buffer = 0;
			_state = wxTCDSTATE_DEFAULT;
			return false;
		}
		
	}	
	return false;
}

//
//
// wxTerminalCharacterMap
//
// Extracted from Chromium OS project
// http://git.chromium.org/gitweb/?p=chromiumos/platform/assets.git;a=blob;f=chromeapps/hterm/js/hterm_vt_character_map.js
//

wxTerminalCharacterMap::wxTerminalCharacterMap()
{
	clear();
}

wxTerminalCharacterMap::wxTerminalCharacterMap(const wxTerminalCharacterMap& map)
{
	for(size_t n=0; n<256; ++n)
		_chars[n] = map._chars[n];
}

wxTerminalCharacterMap::wxTerminalCharacterMap(const wxTerminalCharacterMappping* mapping)
{
	clear();
	map(mapping);
}

void wxTerminalCharacterMap::clear()
{
	for(size_t n=0; n<256; ++n)
		_chars[n] = n;
}

void wxTerminalCharacterMap::map(const wxTerminalCharacterMappping* mapping)
{
	while(mapping->c!=0)
	{
		_chars[mapping->c] = mapping->u;
		++mapping;
	}
}

/**
 * VT100 Graphic character map.
 * http://vt100.net/docs/vt220-rm/table2-4.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_graphic[] = {
      {0x60, 0x25c6},  // ` -> diamond
      {0x61, 0x2592},  // a -> grey-box
      {0x62, 0x2409},  // b -> h/t
      {0x63, 0x240c},  // c -> f/f
      {0x64, 0x240d},  // d -> c/r
      {0x65, 0x240a},  // e -> l/f
      {0x66, 0x00b0},  // f -> degree
      {0x67, 0x00b1},  // g -> +/-
      {0x68, 0x2424},  // h -> n/l
      {0x69, 0x240b},  // i -> v/t
      {0x6a, 0x2518},  // j -> bottom-right
      {0x6b, 0x2510},  // k -> top-right
      {0x6c, 0x250c},  // l -> top-left
      {0x6d, 0x2514},  // m -> bottom-left
      {0x6e, 0x253c},  // n -> line-cross
      {0x6f, 0x23ba},  // o -> scan1
      {0x70, 0x23bb},  // p -> scan3
      {0x71, 0x2500},  // q -> scan5
      {0x72, 0x23bc},  // r -> scan7
      {0x73, 0x23bd},  // s -> scan9
      {0x74, 0x251c},  // t -> left-tee
      {0x75, 0x2524},  // u -> right-tee
      {0x76, 0x2534},  // v -> bottom-tee
      {0x77, 0x252c},  // w -> top-tee
      {0x78, 0x2502},  // x -> vertical-line
      {0x79, 0x2264},  // y -> less-equal
      {0x7a, 0x2265},  // z -> greater-equal
      {0x7b, 0x03c0},  // { -> pi
      {0x7c, 0x2260},  // | -> not-equal
      {0x7d, 0x00a3},  // } -> british-pound
      {0x7e, 0x00b7},  // ~ -> dot
	  {0,0}
};
wxTerminalCharacterMap wxTerminalCharacterMap::graphic(s_graphic);

/**
 * British character map.
 * http://vt100.net/docs/vt220-rm/table2-5.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_british[] = {
	  {0x23, 0x00a3},  // # -> british-pound
	  {0,0}
};
wxTerminalCharacterMap wxTerminalCharacterMap::british(s_british);

/**
 * US ASCII map, no changes.
 */
wxTerminalCharacterMap wxTerminalCharacterMap::us;

/**
 * Dutch character map.
 * http://vt100.net/docs/vt220-rm/table2-6.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_dutch[] = {
      {0x23, 0x00a3},  // # -> british-pound

      {0x40, 0x00be},  // @ -> 3/4

      {0x5b, 0x0132},  // [ -> 'ij' ligature (xterm goes with \u00ff?)
      {0x5c, 0x00bd},  // \ -> 1/2
      {0x5d, 0x007c},  // ] -> vertical bar

      {0x7b, 0x00a8},  // { -> two dots
      {0x7c, 0x0066},  // | -> f
      {0x7d, 0x00bc},  // } -> 1/4
      {0x7e, 0x00b4},  // ~ -> acute
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::dutch(s_dutch);

/**
 * Finnish character map.
 * http://vt100.net/docs/vt220-rm/table2-7.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_finnish[] = {
      {0x5b, 0x00c4},  // [ -> 'A' umlaut
      {0x5c, 0x00d6},  // \ -> 'O' umlaut
      {0x5d, 0x00c5},  // ] -> 'A' ring
      {0x5e, 0x00dc},  // ~ -> 'u' umlaut

      {0x60, 0x00e9},  // ` -> 'e' acute

      {0x7b, 0x00e4},  // { -> 'a' umlaut
      {0x7c, 0x00f6},  // | -> 'o' umlaut
      {0x7d, 0x00e5},  // } -> 'a' ring
      {0x7e, 0x00fc},  // ~ -> 'u' umlaut
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::finnish(s_finnish);

/**
 * French character map.
 * http://vt100.net/docs/vt220-rm/table2-8.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_french[] = {
      {0x23, 0x00a3},  // # -> british-pound

      {0x40, 0x00e0},  // @ -> 'a' grave

      {0x5b, 0x00b0},  // [ -> ring
      {0x5c, 0x00e7},  // \ -> 'c' cedilla
      {0x5d, 0x00a7},  // ] -> section symbol (double s)

      {0x7b, 0x00e9},  // { -> 'e' acute
      {0x7c, 0x00f9},  // | -> 'u' grave
      {0x7d, 0x00e8},  // } -> 'e' grave
      {0x7e, 0x00a8},  // ~ -> umlaut
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::french(s_french);

/**
 * French Canadian character map.
 * http://vt100.net/docs/vt220-rm/table2-9.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_french_canadian[] = {
      {0x40, 0x00e0},  // @ -> 'a' grave

      {0x5b, 0x00e2},  // [ -> 'a' circumflex
      {0x5c, 0x00e7},  // \ -> 'c' cedilla
      {0x5d, 0x00ea},  // ] -> 'e' circumflex
      {0x5e, 0x00ee},  // ^ -> 'i' circumflex

      {0x60, 0x00f4},  // ` -> 'o' circumflex

      {0x7b, 0x00e9},  // { -> 'e' acute
      {0x7c, 0x00f9},  // | -> 'u' grave
      {0x7d, 0x00e8},  // } -> 'e' grave
      {0x7e, 0x00fb},  // ~ -> 'u' circumflex
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::french_canadian(s_french_canadian);

/**
 * German character map.
 * http://vt100.net/docs/vt220-rm/table2-10.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_german[] = {
      {0x40, 0x00a7},  // @ -> section symbol (double s)

      {0x5b, 0x00c4},  // [ -> 'A' umlaut
      {0x5c, 0x00d6},  // \ -> 'O' umlaut
      {0x5d, 0x00dc},  // ] -> 'U' umlaut

      {0x7b, 0x00e4},  // { -> 'a' umlaut
      {0x7c, 0x00f6},  // | -> 'o' umlaut
      {0x7d, 0x00fc},  // } -> 'u' umlaut
      {0x7e, 0x00df},  // ~ -> eszett
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::german(s_german);

/**
 * Italian character map.
 * http://vt100.net/docs/vt220-rm/table2-11.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_italian[] = {
      {0x23, 0x00a3},  // # -> british-pound

      {0x40, 0x00a7},  // @ -> section symbol (double s)

      {0x5b, 0x00b0},  // [ -> ring
      {0x5c, 0x00e7},  // \ -> 'c' cedilla
      {0x5d, 0x00e9},  // ] -> 'e' acute

      {0x60, 0x00f9},  // ` -> 'u' grave

      {0x7b, 0x00e0},  // { -> 'a' grave
      {0x7c, 0x00f2},  // | -> 'o' grave
      {0x7d, 0x00e8},  // } -> 'e' grave
      {0x7e, 0x00ec},  // ~ -> 'i' grave
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::italian(s_italian);

/**
 * Norwegian/Danish character map.
 * http://vt100.net/docs/vt220-rm/table2-12.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_norwegian[] = {
      {0x40, 0x00c4},  // @ -> 'A' umlaut

      {0x5b, 0x00c6},  // [ -> 'AE' ligature
      {0x5c, 0x00d8},  // \ -> 'O' stroke
      {0x5d, 0x00c5},  // ] -> 'A' ring
      {0x5e, 0x00dc},  // ^ -> 'U' umlaut

      {0x60, 0x00e4},  // ` -> 'a' umlaut

      {0x7b, 0x00e6},  // { -> 'ae' ligature
      {0x7c, 0x00f8},  // | -> 'o' stroke
      {0x7d, 0x00e5},  // } -> 'a' ring
      {0x7e, 0x00fc},  // ~ -> 'u' umlaut
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::norwegian(s_norwegian);

/**
 * Spanish character map.
 * http://vt100.net/docs/vt220-rm/table2-13.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_spanish[] = {
      {0x23, 0x00a3},  // # -> british-pound

      {0x40, 0x00a7},  // @ -> section symbol (double s)

      {0x5b, 0x00a1},  // [ -> '!' inverted
      {0x5c, 0x00d1},  // \ -> 'N' tilde
      {0x5d, 0x00bf},  // ] -> '?' inverted

      {0x7b, 0x00b0},  // { -> ring
      {0x7c, 0x00f1},  // | -> 'n' tilde
      {0x7d, 0x00e7},  // } -> 'c' cedilla
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::spanish(s_spanish);

/**
 * Swedish character map.
 * http://vt100.net/docs/vt220-rm/table2-14.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_swedish[] = {
      {0x40, 0x00c9},  // @ -> 'E' acute

      {0x5b, 0x00c4},  // [ -> 'A' umlaut
      {0x5c, 0x00d6},  // \ -> 'O' umlaut
      {0x5d, 0x00c5},  // ] -> 'A' ring
      {0x5e, 0x00dc},  // ^ -> 'U' umlaut

      {0x60, 0x00e9},  // ` -> 'e' acute

      {0x7b, 0x00e4},  // { -> 'a' umlaut
      {0x7c, 0x00f6},  // | -> 'o' umlaut
      {0x7d, 0x00e5},  // } -> 'a' ring
      {0x7e, 0x00fc},  // ~ -> 'u' umlaut
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::swedish(s_swedish);

/**
 * Swiss character map.
 * http://vt100.net/docs/vt220-rm/table2-15.html
 */
static wxTerminalCharacterMap::wxTerminalCharacterMappping s_swiss[] = {
      {0x23, 0x00f9},  // # -> 'u' grave

      {0x40, 0x00e0},  // @ -> 'a' grave

      {0x5b, 0x00e9},  // [ -> 'e' acute
      {0x5c, 0x00e7},  // \ -> 'c' cedilla
      {0x5d, 0x00ea},  // ] -> 'e' circumflex
      {0x5e, 0x00ee},  // ^ -> 'i' circumflex
      {0x5f, 0x00e8},  // _ -> 'e' grave

      {0x60, 0x00f4},  // ` -> 'o' circumflex

      {0x7b, 0x00e4},  // { -> 'a' umlaut
      {0x7c, 0x00f6},  // | -> 'o' umlaut
      {0x7d, 0x00fc},  // } -> 'u' umlaut
      {0x7e, 0x00fb},  // ~ -> 'u' circumflex
	  {0,0}
    };
wxTerminalCharacterMap wxTerminalCharacterMap::swiss(s_swiss);

const wxTerminalCharacterMap* wxTerminalCharacterMap::getMap(char id)
{
	switch(id)
	{
	case '0':
		return &graphic;
	case 'A':
		return &british;
	case 'B':
		return &us;
	case '4':
		return &dutch;
	case 'C':
	case '5':
		return &finnish;
	case 'R':
		return &french;
	case 'Q':
		return &french_canadian;
	case 'K':
		return &german;
	case 'Y':
		return &italian;
	case 'E':
	case '6':
		return &norwegian;
	case 'Z':
		return &spanish;
	case 'H':
	case '7':
		return &swedish;
	case '=':
		return &swiss;
	default:
		return NULL;
	}
}

//
//
// wxTerminalCtrl
//
//


wxString wxTerminalCtrlNameStr(wxT("wxTerminalCtrl"));

wxBEGIN_EVENT_TABLE(wxTerminalCtrl, wxWindow)
	EVT_PAINT(wxTerminalCtrl::OnPaint)
	EVT_SIZE(wxTerminalCtrl::OnSize)
	EVT_SCROLLWIN(wxTerminalCtrl::OnScroll)
	EVT_CHAR(wxTerminalCtrl::OnChar)
	EVT_TIMER(wxID_ANY, wxTerminalCtrl::OnTimer)
wxEND_EVENT_TABLE()

wxTerminalCtrl::wxTerminalCtrl(wxWindow *parent, wxWindowID id, const wxPoint &pos,
    const wxSize &size, long style, const wxString &name):
wxWindow(parent, id, pos, size, style|wxVSCROLL/*|wxHSCROLL*/, name),
m_inputStream(NULL),
m_outputStream(NULL),
m_primaryScreen(NULL),
m_alternateScreen(NULL),
m_currentScreen(NULL)
{
	CommonInit();
}

wxTerminalCtrl::wxTerminalCtrl():
m_inputStream(NULL),
m_outputStream(NULL),
m_primaryScreen(NULL),
m_alternateScreen(NULL),
m_currentScreen(NULL)
{
}

wxTerminalCtrl::~wxTerminalCtrl()
{
}

bool wxTerminalCtrl::Create(wxWindow *parent, wxWindowID id, const wxPoint &pos,
    const wxSize &size, long style, const wxString &name)
{
	if(!wxWindow::Create(parent, id, pos, size, style|wxVSCROLL/*|wxHSCROLL*/, name))
		return false;
	CommonInit();
	return true;
}

void wxTerminalCtrl::CommonInit()
{
	// Initialize screens
	m_primaryScreen = new wxTerminalScreen;
	m_alternateScreen = new wxTerminalScreen;
	m_currentScreen = m_primaryScreen; //  Default is primary ;)

	// Default character set
	m_charset = wxTCSET_UTF_8;
	
	// Default character maps
	m_Gx[0] = wxTerminalCharacterMap::getMap('B');
	m_Gx[1] = wxTerminalCharacterMap::getMap('0');
	m_Gx[2] = wxTerminalCharacterMap::getMap('B');
	m_Gx[3] = wxTerminalCharacterMap::getMap('B');
	m_GL = m_GR = 0;
	
	m_colours[0]  = wxColour(0, 0, 0); // Normal black
	m_colours[1]  = wxColour(255, 85, 85); // Bright red
	m_colours[2] = wxColour(85, 255, 85); // Bright green
	m_colours[3] = wxColour(255, 255, 85); // Bright yellow
	m_colours[4] = wxColour(85, 85, 255); // Bright blue
	m_colours[5] = wxColour(255, 85, 255); // Bright magenta
	m_colours[6] = wxColour(85, 255, 255); // Bright cyan
	m_colours[7] = wxColour(255, 255, 255); // Bright grey (white)
/*	m_colours[0]  = wxColour(0, 0, 0); // Normal black
	m_colours[1]  = wxColour(170, 0, 0); // Normal red
	m_colours[2]  = wxColour(0, 170, 0); // Normal green
	m_colours[3]  = wxColour(170, 85, 0); // Normal yellow (brown)
	m_colours[4]  = wxColour(0, 0, 170); // Normal blue
	m_colours[5]  = wxColour(170, 0, 170); // Normal magenta
	m_colours[6]  = wxColour(0, 170, 170); // Normal cyan
	m_colours[7]  = wxColour(170, 170, 170); // Normal gray
	m_colours[8]  = wxColour(85, 85, 85); // Bright black (dark grey)
	m_colours[9]  = wxColour(255, 85, 85); // Bright red
	m_colours[10] = wxColour(85, 255, 85); // Bright green
	m_colours[11] = wxColour(255, 255, 85); // Bright yellow
	m_colours[12] = wxColour(85, 85, 255); // Bright blue
	m_colours[13] = wxColour(255, 85, 255); // Bright magenta
	m_colours[14] = wxColour(85, 255, 255); // Bright cyan
	m_colours[15] = wxColour(255, 255, 255); // Bright grey (white)
*/

	m_consoleSize = wxSize(80, 25);

	GenerateFonts(wxFont(10, wxFONTFAMILY_TELETYPE));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(m_colours[0]);

	m_currentAttributes = {7, 0, wxTCS_Normal};

	m_caret = new wxCaret(this, GetCharSize());
	m_caret->Show();

	UpdateScrollBars();
	UpdateCaret();
	
	m_timer = new wxTimer(this);
	m_timer->Start(10);
}

void wxTerminalCtrl::GenerateFonts(const wxFont& font)
{
	m_defaultFont   = font;
	m_boldFont      = font.Bold();
	m_underlineFont = font.Underlined();
	m_boldUnderlineFont = m_boldFont.Underlined();
}


void wxTerminalCtrl::wrapAround(bool val)
{
	m_options = (m_options & ~wxTOF_WRAPAROUND);
	if(val)
		m_options |=  wxTOF_WRAPAROUND;
}

void wxTerminalCtrl::reverseWrapAround(bool val)
{
	m_options = (m_options & ~wxTOF_REVERSE_WRAPAROUND);
	if(val)
		m_options |=  wxTOF_REVERSE_WRAPAROUND;
}

void wxTerminalCtrl::originMode(bool val)
{
	m_options = (m_options & ~wxTOF_ORIGINMODE);
	if(val)
		m_options |=  wxTOF_ORIGINMODE;

	// TODO
	// setCursorPosition(0,0);
}

void wxTerminalCtrl::autoCarriageReturn(bool val)
{
	m_options = (m_options & ~wxTOF_AUTO_CARRIAGE_RETURN);
	if(val)
		m_options |=  wxTOF_AUTO_CARRIAGE_RETURN;
}

void wxTerminalCtrl::cursorVisible(bool val)
{
	m_options = (m_options & ~wxTOF_CURSOR_VISIBLE);
	if(val)
		m_options |=  wxTOF_CURSOR_VISIBLE;

	// TODO
}

void wxTerminalCtrl::cursorBlink(bool val)
{
	m_options = (m_options & ~wxTOF_CURSOR_BLINK);
	if(val)
		m_options |=  wxTOF_CURSOR_BLINK;

	// TODO
}

void wxTerminalCtrl::insertMode(bool val)
{
	m_options = (m_options & ~wxTOF_INSERT_MODE);
	if(val)
		m_options |=  wxTOF_INSERT_MODE;
}

void wxTerminalCtrl::reverseVideo(bool val)
{
	m_options = (m_options & ~wxTOF_REVERSE_VIDEO);
	if(val)
		m_options |=  wxTOF_REVERSE_VIDEO;

	// TODO
}

wxSize wxTerminalCtrl::GetCharSize(wxChar c)const
{
	wxSize sz;
	GetTextExtent(c, &sz.x, &sz.y, NULL, NULL, &m_defaultFont);
	return sz;
}

void wxTerminalCtrl::UpdateCaret()
{
	wxPoint pos = m_currentScreen->getCaretPosition();
	wxSize sz = GetCharSize();
	m_caret->Move(sz.x*pos.x, sz.y*pos.y);
}

void wxTerminalCtrl::SetChar(wxUniChar c)
{
	if(insertMode())
		m_currentScreen->insertChar(c, m_currentAttributes);
	else
		m_currentScreen->overwriteChar(c, m_currentAttributes);

	// TODO add automatic scroll
}

void wxTerminalCtrl::newLine()
{
	m_currentScreen->setCaretColumn(0);
	m_currentScreen->moveCaret(1, 0);
}

void wxTerminalCtrl::lineFeed()
{
	int col = m_currentScreen->getCaretPosition().x;
	newLine();
	m_currentScreen->setCaretColumn(col);
}

void wxTerminalCtrl::formFeed()
{
	if(autoCarriageReturn())
		newLine();
	else
		lineFeed();
}

void wxTerminalCtrl::carriageReturn()
{
	m_currentScreen->setCaretColumn(0);
}


void wxTerminalCtrl::cursorUp(int count)
{
	if(count<0)
		cursorDown(-count);
	else if(count>0)
	{
		m_currentScreen->moveCaret(-count, 0);
	}
}

void wxTerminalCtrl::cursorDown(int count)
{
	if(count<0)
		cursorUp(-count);
	else if(count>0)
	{
		m_currentScreen->moveCaret(count, 0);
	}
}

void wxTerminalCtrl::cursorLeft(int count)
{
	if(count<0)
		cursorRight(-count);
	else if(count>0)
	{
		m_currentScreen->moveCaret(0, -count);
	}
}

void wxTerminalCtrl::cursorRight(int count)
{
	if(count<0)
		cursorLeft(-count);
	else if(count>0)
	{
		m_currentScreen->moveCaret(0, count);
	}
}

void wxTerminalCtrl::setCursorColumn(int col)
{
	if(col<0)
		col = 0;
	m_currentScreen->setCaretColumn(col);
}

void wxTerminalCtrl::setCursorPosition(int row, int col)
{
	m_currentScreen->setCaretPosition(wxPoint(col, row));
}




void wxTerminalCtrl::eraseLeft()
{
	// NOTE probably remove characters instead of erasing them if line remaining is empty.
	wxTerminalLine& line = m_currentScreen->getCurrentLine();
	for(size_t col=0; col < m_currentScreen->getCaretAbsolutePosition().x; ++col)
		line[col] = wxTerminalCharacter::DefaultCharacter;
}

void wxTerminalCtrl::eraseRight()
{
	m_currentScreen->getCurrentLine().resize(m_currentScreen->getCaretAbsolutePosition().x, wxTerminalCharacter::DefaultCharacter);
}

void wxTerminalCtrl::eraseLine()
{
	m_currentScreen->getCurrentLine().clear();
}

void wxTerminalCtrl::eraseAbove()
{
	for(size_t row=0; row<m_currentScreen->getCaretPosition().y; ++row)
		m_currentScreen->getLine(row).clear();
	eraseLeft();
}

void wxTerminalCtrl::eraseBelow()
{
	for(size_t row=m_currentScreen->getCaretPosition().y; row<m_consoleSize.y; ++row)
		m_currentScreen->getLine(row).clear();
	eraseRight();
}

void wxTerminalCtrl::eraseScreen()
{
	// TODO Should I scroll down instead of clear screen to keep screen in buffer ??
	for(size_t row=0; row<m_consoleSize.y; ++row)
		m_currentScreen->getLine(row).clear();
}





void wxTerminalCtrl::OnPaint(wxPaintEvent& event)
{
	wxCaretSuspend caretSuspend(this);
		
	wxSize charSz = GetCharSize();
	wxSize clientSz = GetClientSize();
	wxSize clchSz = GetClientSizeInChars();

	wxAutoBufferedPaintDC dc(this);
	dc.SetBrush(wxBrush(*wxBLACK));
	dc.SetPen(wxNullPen);
	dc.DrawRectangle(0, 0, clientSz.x, clientSz.y);

	for(size_t row=0; row<clchSz.y && row<m_currentScreen->getScreenRowCount(); row++)
	{
		const wxTerminalLine& line = m_currentScreen->getLine(row);
		for(size_t col=0; col<line.size(); col++)
		{
			const wxTerminalCharacter &ch = line[col];

			// Invisible or not shown so skip
			if(ch.c < 32 || ch.attr.style & wxTCS_Invisible)
				continue;

			// Choose font
			if(ch.attr.style & wxTCS_Bold)
			{
				if(ch.attr.style & wxTCS_Underlined)
					dc.SetFont(m_boldUnderlineFont);
				else
					dc.SetFont(m_boldFont);
			}
			else
			{
				if(ch.attr.style & wxTCS_Underlined)
					dc.SetFont(m_underlineFont);
				else
					dc.SetFont(m_defaultFont);
			}

			// Choose colors
			if(ch.attr.style & wxTCS_Inverse)
			{
				dc.SetBrush(wxBrush(m_colours[ch.attr.fore]));
				dc.SetTextBackground(m_colours[ch.attr.fore]);
				dc.SetTextForeground(m_colours[ch.attr.back]);
			}
			else
			{
				dc.SetBrush(wxBrush(m_colours[ch.attr.back]));
				dc.SetTextBackground(m_colours[ch.attr.back]);
				dc.SetTextForeground(m_colours[ch.attr.fore]);
			}

			// Draw
			dc.DrawRectangle(col*charSz.x, row*charSz.y, charSz.x, charSz.y);
			dc.DrawText(ch.c, col*charSz.x, row*charSz.y);
		}
	}
}

void wxTerminalCtrl::OnScroll(wxScrollWinEvent& event)
{
	if(event.GetOrientation() == wxVERTICAL)
	{
		m_currentScreen->setOrigin(event.GetPosition());
	}

	// Apply caret position (after scrolling)
	UpdateCaret();

	Refresh();
	event.Skip();
}

void wxTerminalCtrl::OnSize(wxSizeEvent& event)
{
	wxSize sz = GetClientSize();
	wxSize ch = GetCharSize();
	m_consoleSize = wxSize(sz.x/ch.x, sz.y/ch.y);
	
	m_primaryScreen->setScreenSize(m_consoleSize);
	m_alternateScreen->setScreenSize(m_consoleSize);
	
	UpdateScrollBars();
}

void wxTerminalCtrl::UpdateScrollBars()
{
	SetScrollbar(wxVERTICAL, GetScrollPos(wxVERTICAL), m_consoleSize.y, m_currentScreen->getHistoryRowCount());
	UpdateCaret();
}

void wxTerminalCtrl::OnChar(wxKeyEvent& event)
{
	int c = event.GetUnicodeKey();
	if( c != WXK_NONE)
	{
		// Process special keys
		if(c==WXK_RETURN)
			c = wxT('\n');

		if(m_outputStream!=NULL)
		{
			if(c<32)
			{
				if(c!=0x08 && c!=0x0A && c!=0x0D) // Backspace or Line feed or Cariage return
					printf("Enter C0 code : %02x\n", c);
			}
			m_outputStream->PutC(c);
		}
		
	}
	else
	{
		int key = event.GetKeyCode();
		switch(key)
		{
			case WXK_UP:
			{
				static unsigned char cuu[] = {0x1B, 0x5B, 0x41};// CUrsor Up
				m_outputStream->Write(cuu, 3);
				break;
			}
			case WXK_DOWN:
			{
				static unsigned char cud[] = {0x1B, 0x5B, 0x42};// CUrsor Down
				m_outputStream->Write(cud, 3);
				break;
			}
			case WXK_LEFT:
			{
				static unsigned char cub[] = {0x1B, 0x5B, 0x44};// CUrsor Backward
				m_outputStream->Write(cub, 3);
				break;
			}
			case WXK_RIGHT:
			{
				static unsigned char cuf[] = {0x1B, 0x5B, 0x43};// CUrsor Foreward
				m_outputStream->Write(cuf, 3);
				break;
			}			
			case WXK_PAGEUP:
			{
				static unsigned char su[] = {0x1B, 0x5B, 0x53};// Scroll Up
				m_outputStream->Write(su, 3);
				break;
			}
			case WXK_PAGEDOWN:
			{
				static unsigned char sd[] = {0x1B, 0x5B, 0x54};// Scroll Down
				m_outputStream->Write(sd, 3);
				break;
			}
			default:
			{
				std::cout << "Key : " << key << std::endl;
				break;
			}
		}
	}
}


//
// Treatment of inputs and outputs 
//

void wxTerminalCtrl::OnTimer(wxTimerEvent& event)
{
	if(m_inputStream)
	{
		if(m_inputStream->CanRead())
		{
			unsigned char buffer[256];
			while(true)
			{
				m_inputStream->Read(buffer, 256);
				size_t sz = m_inputStream->LastRead();
				Append(buffer, sz);
				if(sz<256)
					break;
			}

			UpdateScrollBars();
			Refresh();
		}
	}
	if(m_errorStream)
	{
		if(m_errorStream->CanRead())
		{
			unsigned char buffer[256];
			while(true)
			{
				m_errorStream->Read(buffer, 256);
				size_t sz = m_errorStream->LastRead();
				Append(buffer, sz);
				if(sz<256)
					break;
			}

			UpdateScrollBars();
			Refresh();
		}
	}
}

void wxTerminalCtrl::Append(const unsigned char* buff, size_t sz)
{
	if(buff && sz>0)
	{
		for(size_t n=0; n<sz; n++)
		{
			Process(buff[n]);
		}
	}
}


//
// Definition of TerminalParser interface abstract functions:
//


void wxTerminalCtrl::onPrintableChar(unsigned char c)
{
	switch(m_charset)
	{
		case wxTCSET_ISO_8859_1:
			SetChar(c);
			break;
		case wxTCSET_UTF_8:
		{
			wxUniChar ch;
			if(m_mbdecoder.add(c, ch))
			{
				SetChar(ch);
			}
			break;
		}
	}
}

void wxTerminalCtrl::onS7C1T() // 7-bit controls
{
	std::cout << "onS7C1T" << std::endl;
	// Note: no trapping 7 and 8 bits modes as they are always recognized for now
}

void wxTerminalCtrl::onS8C1T() // 8-bit controls
{
	std::cout << "onS8C1T" << std::endl;
	// Note: no trapping 7 and 8 bits modes as they are always recognized for now
}

void wxTerminalCtrl::onANSIconf1() // Set ANSI conformance level 1  (vt100, 7-bit controls).
{
	std::cout << "onANSIconf1" << std::endl;
}

void wxTerminalCtrl::onANSIconf2() // Set ANSI conformance level 2  (vt200).
{
	std::cout << "onANSIconf2" << std::endl;
}

void wxTerminalCtrl::onANSIconf3() // Set ANSI conformance level 3  (vt300).
{
	std::cout << "onANSIconf3" << std::endl;
}

void wxTerminalCtrl::onDECDHLth() // DEC double-height line, top half
{
	std::cout << "onDECDHLth" << std::endl;
}

void wxTerminalCtrl::onDECDHLbh() // DEC double-height line, bottom half
{
	std::cout << "onDECDHLbh" << std::endl;
}

void wxTerminalCtrl::onDECSWL() // DEC single-width line
{
	std::cout << "onDECSWL" << std::endl;
}

void wxTerminalCtrl::onDECDWL() // DEC double-width line
{
	std::cout << "onDECDWL" << std::endl;
}

void wxTerminalCtrl::onDECALN() // DEC Screen Alignment Test
{
	std::cout << "onDECALN" << std::endl;
}

void wxTerminalCtrl::onISO8859_1() // Select default character set. That is ISO 8859-1 (ISO 2022).
{
	std::cout << "onISO8859_1" << std::endl;
}

void wxTerminalCtrl::onUTF_8() // Select UTF-8 character set (ISO 2022).
{
	std::cout << "onUTF_8" << std::endl;
}

// Character Set Selection (SCS). Designate G(id) (G0...G3) Character Set (ISO 2022)
void wxTerminalCtrl::onSCS(unsigned char id, unsigned char charset)
{
	if(id<4)
		m_Gx[id] = wxTerminalCharacterMap::getMap(charset);
	if(m_Gx[id]==NULL)
		m_Gx[id] = &wxTerminalCharacterMap::us;
}

void wxTerminalCtrl::onDECBI() // Back Index, VT420 and up.
{
	std::cout << "onDECBI" << std::endl;
}

void wxTerminalCtrl::onDECSC() // Save cursor
{
	std::cout << "onDECSC" << std::endl;
}

void wxTerminalCtrl::onDECRC() // Restore cursor
{
	std::cout << "onDECRC" << std::endl;
}

void wxTerminalCtrl::onDECFI() // Forward Index, VT420 and up.
{
	std::cout << "onDECFI" << std::endl;
}

void wxTerminalCtrl::onDECKPAM() // Application Keypad
{
	std::cout << "onDECKPAM" << std::endl;
}

void wxTerminalCtrl::onDECKPNM() // Normal Keypad
{
	std::cout << "onDECKPNM" << std::endl;
}

void wxTerminalCtrl::onRIS() // Full Reset
{
	std::cout << "onRIS" << std::endl;
}

void wxTerminalCtrl::onLS2() // Invoke the G2 Character Set as GL.
{
	m_GL = 2;
}

void wxTerminalCtrl::onLS3() // Invoke the G3 Character Set as GL.
{
	m_GL = 3;
}

void wxTerminalCtrl::onLS1R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	m_GR = 1;
}

void wxTerminalCtrl::onLS2R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	m_GR = 2;
}

void wxTerminalCtrl::onLS3R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	m_GR = 3;
}


// C0:
//-----	
void wxTerminalCtrl::onNUL()  // 0x00
{
	std::cout << "onNUL" << std::endl;
}

void wxTerminalCtrl::onSOH()  // 0x01
{
	std::cout << "onSOH" << std::endl;
}

void wxTerminalCtrl::onSTX()  // 0x02
{
	std::cout << "onSTX" << std::endl;
}

void wxTerminalCtrl::onETX()  // 0x03
{
	std::cout << "onETX" << std::endl;
}

void wxTerminalCtrl::onEOT()  // 0x04
{
	std::cout << "onEOT" << std::endl;
}

void wxTerminalCtrl::onENQ()  // 0x05
{
	std::cout << "onSonENQ" << std::endl;
}

void wxTerminalCtrl::onACK()  // 0x06
{
	std::cout << "onACK" << std::endl;
}

void wxTerminalCtrl::onBEL()  // 0x07
{
	wxBell();
	std::cout << "onBEL" << std::endl;
}

void wxTerminalCtrl::onBS()   // 0x08 - BACK SPACE
{
	// TODO Remove current char
	cursorLeft();
}

void wxTerminalCtrl::onHT()   // 0x09
{
	std::cout << "onHT" << std::endl;
}

void wxTerminalCtrl::onLF()   // 0x0A - LINE FEED
{
	SetChar(0x0A);
	formFeed(); // Do form feed (instead of line feed) processing accordingly with autoCarriageReturn
}

void wxTerminalCtrl::onVT()   // 0x0B - VERTICAL TAB
{
	// std::cout << "onVT" << std::endl;
	formFeed(); // Processed as LINE FEED
}

void wxTerminalCtrl::onFF()   // 0x0C
{
	// std::cout << "onFF" << std::endl;
	formFeed();
}

void wxTerminalCtrl::onCR()   // 0x0D - CARRIAGE RETURN -- Done
{
	// SetChar(0x0D);
	carriageReturn();
}

void wxTerminalCtrl::onSO()   // 0x0E
{
	// Shift Out (SO), aka Lock Shift 0 (LS1).
	// Invoke G1 character set in GL.
	m_GL = 1;
}

void wxTerminalCtrl::onSI()   // 0x0F
{
    // Shift In (SI), aka Lock Shift 0 (LS0).
    // Invoke G0 character set in GL.
	m_GL = 0;
}

void wxTerminalCtrl::onDLE()  // 0x10
{
	std::cout << "onDLE" << std::endl;
}

void wxTerminalCtrl::onDC1()  // 0x11
{
	std::cout << "onDC1" << std::endl;
}

void wxTerminalCtrl::onDC2()  // 0x12
{
	std::cout << "onDC2" << std::endl;
}

void wxTerminalCtrl::onDC3()  // 0x13
{
	std::cout << "onDC3" << std::endl;
}

void wxTerminalCtrl::onDC4()  // 0x14
{
	std::cout << "onDC4" << std::endl;
}

void wxTerminalCtrl::onNAK()  // 0x15
{
	std::cout << "onNAK" << std::endl;
}

void wxTerminalCtrl::onSYN()  // 0x16
{
	std::cout << "onSYN" << std::endl;
}

void wxTerminalCtrl::onETB()  // 0x17
{
	std::cout << "onETB" << std::endl;
}

void wxTerminalCtrl::onEM()   // 0x19
{
	std::cout << "onEM" << std::endl;
}

void wxTerminalCtrl::onFS()   // 0x1C
{
	std::cout << "onFS" << std::endl;
}

void wxTerminalCtrl::onGS()   // 0x1D
{
	std::cout << "onGS" << std::endl;
}

void wxTerminalCtrl::onRS()   // 0x1E
{
	std::cout << "nRS" << std::endl;
}

void wxTerminalCtrl::onUS()   // 0x1F
{
	std::cout << "onUS" << std::endl;
}


// C1:
//-----	
void wxTerminalCtrl::onPAD()  // 0x80
{
	std::cout << "onPAD" << std::endl;
}

void wxTerminalCtrl::onHOP()  // 0x81
{
	std::cout << "onHOP" << std::endl;
}

void wxTerminalCtrl::onBPH()  // 0x82
{
	std::cout << "onBPH" << std::endl;
}

void wxTerminalCtrl::onNBH()  // 0x83
{
	std::cout << "onNBH" << std::endl;
}

void wxTerminalCtrl::onIND()  // 0x84
{
	std::cout << "onIND" << std::endl;
}

void wxTerminalCtrl::onNEL()  // 0x85
{
	std::cout << "onNEL" << std::endl;
}

void wxTerminalCtrl::onSSA()  // 0x86
{
	std::cout << "onSSA" << std::endl;
}

void wxTerminalCtrl::onESA()  // 0x87
{
	std::cout << "onESA" << std::endl;
}

void wxTerminalCtrl::onHTS()  // 0x88
{
	std::cout << "onHTS" << std::endl;
}

void wxTerminalCtrl::onHTJ()  // 0x89
{
	std::cout << "onHTJ" << std::endl;
}

void wxTerminalCtrl::onVTS()  // 0x8A
{
	std::cout << "onVTS" << std::endl;
}

void wxTerminalCtrl::onPLD()  // 0x8B
{
	std::cout << "onPLD" << std::endl;
}

void wxTerminalCtrl::onPLU()  // 0x8C
{
	std::cout << "onPLU" << std::endl;
}

void wxTerminalCtrl::onRI()   // 0x8D
{
	std::cout << "onRI" << std::endl;
}

void wxTerminalCtrl::onSS2()  // 0x8E
{
	std::cout << "onSS2" << std::endl;
}

void wxTerminalCtrl::onSS3()  // 0x8F
{
	std::cout << "onSS3" << std::endl;
}

void wxTerminalCtrl::onPU1()  // 0x91
{
	std::cout << "onPU1" << std::endl;
}

void wxTerminalCtrl::onPU2()  // 0x92
{
	std::cout << "onPU2" << std::endl;
}

void wxTerminalCtrl::onSTS()  // 0x93
{
	std::cout << "onSTS" << std::endl;
}

void wxTerminalCtrl::onCCH()  // 0x94
{
	std::cout << "onCCH" << std::endl;
}

void wxTerminalCtrl::onMW()   // 0x95
{
	std::cout << "onMW" << std::endl;
}

void wxTerminalCtrl::onSPA()  // 0x96
{
	std::cout << "onSPA" << std::endl;
}

void wxTerminalCtrl::onEPA()  // 0x97
{
	std::cout << "onEPA" << std::endl;
}

void wxTerminalCtrl::onSGCI() // 0x99
{
	std::cout << "onSGCI" << std::endl;
}

void wxTerminalCtrl::onSCI()  // 0x9A
{
	std::cout << "onSCI" << std::endl;
}


// CSI:
//-----	
void wxTerminalCtrl::onICH(unsigned short nb) // Insert P s (Blank) Character(s) (default = 1)
{
	std::cout << "onICH " << nb << std::endl;
}

void wxTerminalCtrl::onCUU(unsigned short nb) // Cursor Up P s Times (default = 1)
{
	// std::cout << "onCUU " << nb << std::endl;
	cursorUp(nb);
}

void wxTerminalCtrl::onCUD(unsigned short nb) // Cursor Down P s Times (default = 1)
{
	// std::cout << "onCUD " << nb << std::endl;
	cursorDown(nb);
}

void wxTerminalCtrl::onCUF(unsigned short nb) // Cursor Forward P s Times (default = 1)
{
	//std::cout << "onCUF " << nb << std::endl;
	cursorRight(nb);
}

void wxTerminalCtrl::onCUB(unsigned short nb) // Cursor Backward P s Times (default = 1)
{
	//std::cout << "onCUB " << nb << std::endl;
	cursorLeft(nb);
}

void wxTerminalCtrl::onCNL(unsigned short nb) // Cursor Next Line P s Times (default = 1)
{
	// std::cout << "onCNL " << nb << std::endl;
	cursorDown(nb);
	setCursorColumn(0);
}

void wxTerminalCtrl::onCPL(unsigned short nb) // Cursor Preceding Line P s Times (default = 1)
{
	// std::cout << "onCPL " << nb << std::endl;
	cursorUp(nb);
	setCursorColumn(0);
}

void wxTerminalCtrl::onCHA(unsigned short nb) // Moves the cursor to column n.
{
	// std::cout << "onCHA " << nb << std::endl;
	setCursorColumn(nb);
}

void wxTerminalCtrl::onCUP(unsigned short row, unsigned short col) // Moves the cursor to row n, column m
{
	// std::cout << "onCUP " << row << " - " << col << std::endl;
	setCursorPosition(row-1, col-1);
}

void wxTerminalCtrl::onCHT(unsigned short nb) // Cursor Forward Tabulation P s tab stops (default = 1)
{
	std::cout << "onCHT " << nb << std::endl;
}


void wxTerminalCtrl::onED(unsigned short opt) // Clears part of the screen.
{
	switch(opt)
	{
		case 0: // Erase Below
			// std::cout << "Erase Below" << std::endl;
			eraseBelow();
			break;
		case 1: // Erase Above
			// std::cout << "Erase Above" << std::endl;
			eraseAbove();
			break;
		case 2: // Erase All
			// std::cout << "Erase All" << std::endl;
			eraseScreen();
			break;
		case 3: // Erase Saved Lines (xterm)
			// std::cout << "Erase Saved Lines (xterm)" << std::endl;
			eraseScreen();
			// TODO Erase buffered lines ??
			break;
		default:
			break;
	}
}

void wxTerminalCtrl::onDECSED(unsigned short opt)  // Erase in Display. 0 → Selective Erase Below (default). 1 → Selective Erase Above. 2 → Selective Erase All. 3 → Selective Erase Saved Lines (xterm).
{
	std::cout << "onDECSED " << opt << std::endl;
}

void wxTerminalCtrl::onEL(unsigned short opt) // Erases part of the line. -- Mostly done
{
	switch(opt)
	{
		case 0: // Erase Right
			// std::cout << "Erase Right" << std::endl;
			eraseRight();
			break;
		case 1: // Erase Left
			// std::cout << "Erase Left" << std::endl;
			eraseLeft();
			break;
		case 2: // Erase Line
			// std::cout << "Erase Line" << std::endl;
			eraseLine();
			break;
		default:
			break;
	}
}

void wxTerminalCtrl::onDECSEL(unsigned short nb)  // Erase in Line. 0 → Selective Erase to Right (default). 1 → Selective Erase to Left. 2 → Selective Erase All.
{
	std::cout << "onDECSEL " << nb << std::endl;
}

void wxTerminalCtrl::onIL(unsigned short nb)  // Insert Ps Line(s) (default = 1)
{
	std::cout << "onIL " << nb << std::endl;
}

void wxTerminalCtrl::onDL(unsigned short nb)  // Delete Ps Line(s) (default = 1)
{
	std::cout << "onDL " << nb << std::endl;
}

void wxTerminalCtrl::onDCH(unsigned short nb) // Delete Ps Character(s) (default = 1)
{
	std::cout << "onDCH " << nb << std::endl;
}

void wxTerminalCtrl::onSU(unsigned short nb)  // Scroll up Ps lines (default = 1)
{
	std::cout << "onSU " << nb << std::endl;
}

void wxTerminalCtrl::onSD(unsigned short nb)  // Scroll down Ps lines (default = 1)
{
	std::cout << "onSD " << nb << std::endl;
}

void wxTerminalCtrl::onECH(unsigned short nb)  // Erase Ps Character(s) (default = 1)
{
	std::cout << "onECH " << nb << std::endl;
}

void wxTerminalCtrl::onCBT(unsigned short nb)  // Cursor Backward Tabulation Ps tab stops (default = 1)
{
	std::cout << "onCBT " << nb << std::endl;
}

void wxTerminalCtrl::onHPA(const std::vector<unsigned short> nbs)  // Character Position Absolute [column] (default = [row,1]) (HPA).
{
	std::cout << "onHPA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onHPR(const std::vector<unsigned short> nbs)  // Character Position Relative [columns] (default = [row,col+1]) (HPR).
{
	std::cout << "onHPR";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;}

void wxTerminalCtrl::onVPA(const std::vector<unsigned short> nbs)  // Line Position Absolute [row] (default = [1,column])
{
	std::cout << "onVPA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onVPR(const std::vector<unsigned short> nbs)  // Line Position Relative [rows] (default = [row+1,column]) (VPR)
{
	std::cout << "onVPR";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onHVP(unsigned short row, unsigned short col) // Horizontal and Vertical Position [row;column] (default = [1,1])
{
	std::cout << "onHVP " << row << " " << col << std::endl;
}

void wxTerminalCtrl::onTBC(unsigned short nb)  // Tab Clear
{
	std::cout << "onTBC " << nb << std::endl;
}

void wxTerminalCtrl::onSM(const std::vector<unsigned short> nbs)  // Set Mode
{
	std::cout << "onSM";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECSET(const std::vector<unsigned short> nbs)  // DEC Private Mode Set
{
	std::cout << "onDECSET";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onMC(const std::vector<unsigned short> nbs)  // Media Copy
{
	std::cout << "onMC";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECMC(const std::vector<unsigned short> nbs)  // DEC specific Media Copy
{
	std::cout << "onDECMC";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onRM(const std::vector<unsigned short> nbs)  // Reset Mode
{
	std::cout << "onRM";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECRST(const std::vector<unsigned short> nbs)  // DEC Private Mode Reset
{
	std::cout << "onDECRST";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDSR(unsigned short nb)  // Device Status Report
{
	std::cout << "onDSR " << nb << std::endl;
}

void wxTerminalCtrl::onSGR(const std::vector<unsigned short> nbs) // Select Graphic Renditions -- In progress
{
	for(size_t n=0; n<nbs.size(); ++n)
	{
		unsigned short sgr = nbs[n]; 
		switch(sgr)
		{
		case 0: // Default
			m_currentAttributes = {7, 0, wxTCS_Normal};
			break;
		case 1: // Bold
			m_currentAttributes.style |= wxTCS_Bold;
			break;
		case 4: // Underline
			m_currentAttributes.style |= wxTCS_Underlined;
			break;
		case 5: // Blink
			m_currentAttributes.style |= wxTCS_Blink;
			break;
		case 7: // Inverse
			m_currentAttributes.style |= wxTCS_Inverse;
			break;
		case 8: // Invisible (hidden)
			m_currentAttributes.style |= wxTCS_Invisible;
			break;
		case 22: // Normal (neither bold nor faint)
			m_currentAttributes.style &= ~wxTCS_Bold;
			break;
		case 24: // Not underlined
			m_currentAttributes.style &= ~wxTCS_Underlined;
			break;
		case 25: // Steady (not blinking)
			m_currentAttributes.style &= ~wxTCS_Blink;
			break;
		case 27: // Positive (not inverse)
			m_currentAttributes.style &= ~wxTCS_Inverse;
			break;
		case 28: // Visible (not hidden)
			m_currentAttributes.style &= ~wxTCS_Invisible;
			break;
		case 30: // Foreground black
		case 31: // Foreground red
		case 32: // Foreground green
		case 33: // Foreground yellow
		case 34: // Foreground blue
		case 35: // Foreground purple
		case 36: // Foreground cyan
		case 37: // Foreground white
			m_currentAttributes.fore = sgr - 30; 
			break;
		case 39: // Foreground default
			m_currentAttributes.fore = 7;
			break;
		case 40: // Background black
		case 41: // Background red
		case 42: // Background green
		case 43: // Background yellow
		case 44: // Background blue
		case 45: // Background purple
		case 46: // Background cyan
		case 47: // Background white
			m_currentAttributes.back = sgr - 40; 
			break;
		case 49: // ForegBackground default
			m_currentAttributes.back = 0;
			break;
		default:
			printf("ApplySGR %d\n", sgr);
			break;
		}		
	}
}

void wxTerminalCtrl::onDECDSR(unsigned short nb)  // DEC-specific Device Status Report
{
	std::cout << "onDECDSR " << nb << std::endl;
}

void wxTerminalCtrl::onDECSTR()  // Soft terminal reset
{
	std::cout << "onDECSTR" << std::endl;
}

void wxTerminalCtrl::onDECSCL(unsigned short nb1, unsigned short nb2) // Set conformance level
{
	std::cout << "onDECSCL " << nb1 << " " << nb2 << std::endl;
}

void wxTerminalCtrl::onDECRQM(unsigned short nb)  // Request DEC private mode
{
	std::cout << "onDECRQM " << nb << std::endl;
}

void wxTerminalCtrl::onDECLL(unsigned short nb)  // Load LEDs
{
	std::cout << "onDECLL " << nb << std::endl;
}

void wxTerminalCtrl::onDECSCUSR(unsigned short nb)  // Set cursor style (DECSCUSR, VT520).
{
	std::cout << "onDECSCUSR " << nb << std::endl;
}

void wxTerminalCtrl::onDECSCA(unsigned short nb)  // Select character protection attribute (DECSCA).
{
	std::cout << "onDECSCA " << nb << std::endl;
}

void wxTerminalCtrl::onDECSTBM(unsigned short top, unsigned short bottom) // Set Scrolling Region [top;bottom] (default = full size of window)
{
	std::cout << "onDECSTBM top=" << top << " bottom=" << bottom << std::endl;
}

void wxTerminalCtrl::onRDECPMV(const std::vector<unsigned short> nbs)  // Restore DEC Private Mode Values. The value of P s previously saved is restored. P s values are the same as for DECSET.
{
	std::cout << "onRDECPMV";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onSDECPMV(const std::vector<unsigned short> nbs)  // Save DEC Private Mode Values. P s values are the same as for DECSET.
{
	std::cout << "onSDECPMV";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECCARA(const std::vector<unsigned short> nbs)  // Change Attributes in Rectangular Area (DECCARA), VT400 and up.
{
	std::cout << "onDECCARA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECSLRM(unsigned short left, unsigned short right) // Set left and right margins (DECSLRM), available only when DECLRMM is enabled (VT420 and up).
{
	std::cout << "onDECSLRM left=" << left << " right=" << right << std::endl;
}

void wxTerminalCtrl::onANSISC()  // Save cursor (ANSI.SYS), available only when DECLRMM is disabled.
{
	std::cout << "onANSISC" << std::endl;
}

void wxTerminalCtrl::onANSIRC()  // Restore cursor (ANSI.SYS).
{
	std::cout << "onANSIRC" << std::endl;
}

void wxTerminalCtrl::onWindowManip(unsigned short nb1, unsigned short nb2, unsigned short nb3) // Set conformance level
{
	std::cout << "onWindowManip " << nb1 << " " << nb2 << " " << nb3 << std::endl;
}

void wxTerminalCtrl::onDECRARA(const std::vector<unsigned short> nbs)  // Reverse Attributes in Rectangular Area (DECRARA), VT400 and up.
{
	std::cout << "onDECRARA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECSWBV(unsigned short nb)  // Set warning-bell volume (DECSWBV, VT520).
{
	std::cout << "onDECSWBV " << nb << std::endl;
}

void wxTerminalCtrl::onDECSMBV(unsigned short nb)  // Set margin-bell volume (DECSMBV, VT520).
{
	std::cout << "onDECSMBV " << nb << std::endl;
}

void wxTerminalCtrl::onDECCRA(const std::vector<unsigned short> nbs)  // Copy Rectangular Area (DECCRA, VT400 and up).
{
	std::cout << "onDECCRA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECEFR(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Enable Filter Rectangle (DECEFR), VT420 and up.
{
	std::cout << "onDECEFR " << top << " " << left << " " << bottom << " " << right << std::endl;
}

void wxTerminalCtrl::onDECREQTPARM(unsigned short nb)  // Request Terminal Parameters
{
	std::cout << "onDECREQTPARM " << nb << std::endl;
}

void wxTerminalCtrl::onDECSACE(unsigned short nb)  // Select Attribute Change Extent
{
	std::cout << "onDECSACE " << nb << std::endl;
}

void wxTerminalCtrl::onDECFRA(unsigned short chr, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Fill Rectangular Area (DECFRA), VT420 and up.
{
	std::cout << "onDECFRA " << chr << " " << top << " " << left << " " << bottom << " " << right << std::endl;
}

void wxTerminalCtrl::onDECRQCRA(unsigned short id, unsigned short page, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Request Checksum of Rectangular Area (DECRQCRA), VT420 and up.
{
	std::cout << "onDECRQCRA " << id << " " << page << " " << top << " " << left << " " << bottom << " " << right << std::endl;
}

void wxTerminalCtrl::onDECELR(unsigned short nb1, unsigned short nb2) // Enable Locator Reporting (DECELR).
{
	std::cout << "onDECELR " << nb1 << " " << nb2 << std::endl;
}

void wxTerminalCtrl::onDECERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Erase Rectangular Area (DECERA), VT400 and up.
{
	std::cout << "onDECERA " << top << " " << left << " " << bottom << " " << right  << std::endl;
}

void wxTerminalCtrl::onDECSLE(const std::vector<unsigned short> nbs)  // Select Locator Events (DECSLE).
{
	std::cout << "onDECSLE";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;
}

void wxTerminalCtrl::onDECSERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Selective Erase Rectangular Area (), VT400 and up.
{
	std::cout << "onDECSERA " << top << " " << left << " " << bottom << " " << right << std::endl;
}

void wxTerminalCtrl::onDECRQLP(unsigned short nb)  // Request Locator Position (DECRQLP).
{
	std::cout << "onDECRQLP " << nb << std::endl;
}

void wxTerminalCtrl::onDECIC(unsigned short nb)  // Insert P s Column(s) (default = 1) (DECIC), VT420 and up.
{
	std::cout << "onDECIC " << nb << std::endl;
}

void wxTerminalCtrl::onDECDC(unsigned short nb)  // InsDelete P s Column(s) (default = 1) (DECIC), VT420 and up.
{
	std::cout << "onDECDC " << nb << std::endl;
}

void wxTerminalCtrl::onOSC(unsigned short command, const std::vector<unsigned char>& params) // Receive an OSC (Operating System Command) command. -- In progress
{
	switch(command)
	{
		case 0: // Change Icon Name and Window Title to Pt.
		case 2: // Change Window Title to Pt.
		{
			wxString str((const char*)&params.front(), params.size());
			printf("Change title: \"%s\"\n", (const char*)str.c_str());
			break;
		}
		default:
			// TODO Add others
			printf("Unknown OSC 0x%04X\n", (int)command);
			break;
	}
}


