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
// Debug features:
//
//
#if 1 // Debuging feature activated

#define DBGOUT (std::cout)
#define NOT_IMPLEMENTED(content) {DBGOUT << "Not implemented: " << content << std::endl;}

#else  // Debuging feature activated

#define NOT_IMPLEMENTED(content);

#endif // Debuging feature activated



#if 0 // Tracing  feature activated

#define TRACEOUT (std::cout)
#define TRACE(content) {TRACEOUT << content << std::endl;}

#else // Tracing  feature activated

#define TRACE(content);

#endif  // Tracing  feature activated


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

void wxTerminalScreen::insertLines(int pos, unsigned int count)
{
	_content.insert(_content.begin()+pos+_originPosition.y, count, wxTerminalLine());
}

void wxTerminalScreen::insertLinesAbsolute(int pos, unsigned int count)
{
	_content.insert(_content.begin()+pos, count, wxTerminalLine());
}

void wxTerminalScreen::insertLinesAtCarret(unsigned int count)
{
	insertLinesAbsolute(getCaretAbsolutePosition().y, count);
}

void wxTerminalScreen::deleteLines(int pos, unsigned int count)
{
	deleteLinesAbsolute(pos + _originPosition.y, count);
}

void wxTerminalScreen::deleteLinesAbsolute(int pos, unsigned int count)
{
	int end = pos + count;
	if(end<_content.size())
	   _content.erase(_content.begin()+pos, _content.begin()+end);
	else
	   _content.erase(_content.begin()+pos, _content.end());
}

void wxTerminalScreen::deleteLinesAtCarret(unsigned int count)
{
	deleteLinesAbsolute(getCaretAbsolutePosition().y, count);
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

	m_tabWidth = 8;
	setDefaultTabStops();

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

void wxTerminalCtrl::insertLines(unsigned int count)
{
	m_currentScreen->insertLinesAtCarret(count);
}

void wxTerminalCtrl::deleteLines(unsigned int count)
{
	m_currentScreen->deleteLinesAtCarret(count);
}



void wxTerminalCtrl::forwardTabStops()
{
	size_t col = m_currentScreen->getCaretAbsolutePosition().x;
	for(std::set<unsigned int>::const_iterator it=m_tabstops.begin(); it!=m_tabstops.end(); ++it)
	{
		if(*it>col)
		{
			setCursorColumn(*it);
			return;
		}
	}

	setCursorColumn(m_consoleSize.x-1);
}

void wxTerminalCtrl::backwardTabStops()
{
	size_t col = m_currentScreen->getCaretAbsolutePosition().x;
	for(std::set<unsigned int>::const_reverse_iterator it=m_tabstops.rbegin(); it!=m_tabstops.rend(); ++it)
	{
		if(*it<col)
		{
			setCursorColumn(*it);
			return;
		}
	}

	setCursorColumn(0);
}

void wxTerminalCtrl::setTabStop(int col)
{
	m_tabstops.insert(col);
}

void wxTerminalCtrl::setTabStop()
{
	m_tabstops.insert(m_currentScreen->getCaretAbsolutePosition().x);
}

void wxTerminalCtrl::clearTabStop(int col)
{
	m_tabstops.erase(col);
}

void wxTerminalCtrl::clearTabStop()
{
	m_tabstops.erase(m_currentScreen->getCaretAbsolutePosition().x);
}

void wxTerminalCtrl::clearAllTabStops()
{
	m_tabstops.clear();
}

void wxTerminalCtrl::setDefaultTabStops(int col)
{
	for(size_t c = 0; c < m_consoleSize.x; c += m_tabWidth)
	{
		if(c >= col)
			m_tabstops.insert(c);
	}
}




void wxTerminalCtrl::setANSIMode(unsigned int mode, bool state)
{
	TRACE("setANSIMode mode=" << mode << " state=" << state);
	switch(mode)
	{
	case 2: // Keyboard Action Mode (AM).
		NOT_IMPLEMENTED("Keyboard Action Mode - KBD LOCKED");
		break;
	case 4: // Insert Mode (IRM).
		insertMode(state);
		break;
	case 12: // Send/receive (SRM).
		NOT_IMPLEMENTED("KeSend/receive (SRM)");
		break;
	case 20: // Automatic Newline (LNM).
		autoCarriageReturn(state);
		break;
	default: // nrecognizedd ANSI mode.
		break;
	}
}

void wxTerminalCtrl::setDECMode(unsigned int mode, bool state)
{
	NOT_IMPLEMENTED("setDECMode mode=" << mode << " state=" << state);
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
	TRACE("S7C1T");
	// Note: no trapping 7 and 8 bits modes as they are always recognized for now
}

void wxTerminalCtrl::onS8C1T() // 8-bit controls
{
	TRACE("S8C1T");
	// Note: no trapping 7 and 8 bits modes as they are always recognized for now
}

void wxTerminalCtrl::onANSIconf1() // Set ANSI conformance level 1  (vt100, 7-bit controls).
{
	NOT_IMPLEMENTED("ANSIconf1");
}

void wxTerminalCtrl::onANSIconf2() // Set ANSI conformance level 2  (vt200).
{
	NOT_IMPLEMENTED("ANSIconf2");
}

void wxTerminalCtrl::onANSIconf3() // Set ANSI conformance level 3  (vt300).
{
	NOT_IMPLEMENTED("ANSIconf3");
}

void wxTerminalCtrl::onDECDHLth() // DEC double-height line, top half
{
	NOT_IMPLEMENTED("DECDHLth");
}

void wxTerminalCtrl::onDECDHLbh() // DEC double-height line, bottom half
{
	NOT_IMPLEMENTED("DECDHLbh");
}

void wxTerminalCtrl::onDECSWL() // DEC single-width line
{
	NOT_IMPLEMENTED("DECSWL");
}

void wxTerminalCtrl::onDECDWL() // DEC double-width line
{
	NOT_IMPLEMENTED("DECDWL");
}

void wxTerminalCtrl::onDECALN() // DEC Screen Alignment Test
{
	NOT_IMPLEMENTED("DECALN");
}

void wxTerminalCtrl::onISO8859_1() // Select default character set. That is ISO 8859-1 (ISO 2022).
{
	NOT_IMPLEMENTED("ISO8859_1");
}

void wxTerminalCtrl::onUTF_8() // Select UTF-8 character set (ISO 2022).
{
	NOT_IMPLEMENTED("UTF_8");
}

// Character Set Selection (SCS). Designate G(id) (G0...G3) Character Set (ISO 2022)
void wxTerminalCtrl::onSCS(unsigned char id, unsigned char charset)
{
	TRACE("SCS id=" << id << " charset=" << charset);
	if(id<4)
		m_Gx[id] = wxTerminalCharacterMap::getMap(charset);
	if(m_Gx[id]==NULL)
		m_Gx[id] = &wxTerminalCharacterMap::us;
}

void wxTerminalCtrl::onDECBI() // Back Index, VT420 and up.
{
	NOT_IMPLEMENTED("DECBI");
}

void wxTerminalCtrl::onDECSC() // Save cursor
{
	NOT_IMPLEMENTED("DECSC");
}

void wxTerminalCtrl::onDECRC() // Restore cursor
{
	NOT_IMPLEMENTED("DECRC");
}

void wxTerminalCtrl::onDECFI() // Forward Index, VT420 and up.
{
	NOT_IMPLEMENTED("DECFI");
}

void wxTerminalCtrl::onDECKPAM() // Application Keypad
{
	NOT_IMPLEMENTED("DECKPAM");
}

void wxTerminalCtrl::onDECKPNM() // Normal Keypad
{
	NOT_IMPLEMENTED("DECKPNM");
}

void wxTerminalCtrl::onRIS() // Full Reset
{
	NOT_IMPLEMENTED("RIS");
}

void wxTerminalCtrl::onLS2() // Invoke the G2 Character Set as GL.
{
	TRACE("LS2");
	m_GL = 2;
}

void wxTerminalCtrl::onLS3() // Invoke the G3 Character Set as GL.
{
	TRACE("LS3");
	m_GL = 3;
}

void wxTerminalCtrl::onLS1R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	TRACE("LS1R");
	m_GR = 1;
}

void wxTerminalCtrl::onLS2R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	TRACE("LS2R");
	m_GR = 2;
}

void wxTerminalCtrl::onLS3R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	TRACE("LS3R");
	m_GR = 3;
}


// C0:
//-----	
void wxTerminalCtrl::onNUL()  // 0x00
{
	NOT_IMPLEMENTED("NULL");
}

void wxTerminalCtrl::onSOH()  // 0x01
{
	NOT_IMPLEMENTED("SOH");
}

void wxTerminalCtrl::onSTX()  // 0x02
{
	NOT_IMPLEMENTED("STX");
}

void wxTerminalCtrl::onETX()  // 0x03
{
	NOT_IMPLEMENTED("ETX");
}

void wxTerminalCtrl::onEOT()  // 0x04
{
	NOT_IMPLEMENTED("EOT");
}

void wxTerminalCtrl::onENQ()  // 0x05
{
	NOT_IMPLEMENTED("ENQ");
}

void wxTerminalCtrl::onACK()  // 0x06
{
	NOT_IMPLEMENTED("ACK");
}

void wxTerminalCtrl::onBEL()  // 0x07
{
	TRACE("BEL");
	wxBell();
}

void wxTerminalCtrl::onBS()   // 0x08 - BACK SPACE
{
	TRACE("BS");
	// TODO Remove current char
	cursorLeft();
}

void wxTerminalCtrl::onHT()   // 0x09
{
	//  Move the cursor to the next tab stop, or to the right margin if no further tab stops are present on the line.
	TRACE("HT");
	forwardTabStops();
}

void wxTerminalCtrl::onLF()   // 0x0A - LINE FEED
{
	TRACE("LF");
	SetChar(0x0A);
	formFeed(); // Do form feed (instead of line feed) processing accordingly with autoCarriageReturn
}

void wxTerminalCtrl::onVT()   // 0x0B - VERTICAL TAB
{
	TRACE("VT");
	formFeed(); // Processed as LINE FEED
}

void wxTerminalCtrl::onFF()   // 0x0C
{
	TRACE("FF");
	formFeed();
}

void wxTerminalCtrl::onCR()   // 0x0D - CARRIAGE RETURN -- Done
{
	TRACE("CR");
	carriageReturn();
}

void wxTerminalCtrl::onSO()   // 0x0E
{
	TRACE("SO");
	// Shift Out (SO), aka Lock Shift 0 (LS1).
	// Invoke G1 character set in GL.
	m_GL = 1;
}

void wxTerminalCtrl::onSI()   // 0x0F
{
	TRACE("SI");
    // Shift In (SI), aka Lock Shift 0 (LS0).
    // Invoke G0 character set in GL.
	m_GL = 0;
}

void wxTerminalCtrl::onDLE()  // 0x10
{
	NOT_IMPLEMENTED("DLE");
}

void wxTerminalCtrl::onDC1()  // 0x11
{
	NOT_IMPLEMENTED("DC1");
}

void wxTerminalCtrl::onDC2()  // 0x12
{
	NOT_IMPLEMENTED("DC2");
}

void wxTerminalCtrl::onDC3()  // 0x13
{
	NOT_IMPLEMENTED("DC3");
}

void wxTerminalCtrl::onDC4()  // 0x14
{
	NOT_IMPLEMENTED("DC4");
}

void wxTerminalCtrl::onNAK()  // 0x15
{
	NOT_IMPLEMENTED("NAK");
}

void wxTerminalCtrl::onSYN()  // 0x16
{
	NOT_IMPLEMENTED("SYN");
}

void wxTerminalCtrl::onETB()  // 0x17
{
	NOT_IMPLEMENTED("ETB");
}

void wxTerminalCtrl::onEM()   // 0x19
{
	NOT_IMPLEMENTED("EM");
}

void wxTerminalCtrl::onFS()   // 0x1C
{
	NOT_IMPLEMENTED("FS");
}

void wxTerminalCtrl::onGS()   // 0x1D
{
	NOT_IMPLEMENTED("GS");
}

void wxTerminalCtrl::onRS()   // 0x1E
{
	NOT_IMPLEMENTED("RS");
}

void wxTerminalCtrl::onUS()   // 0x1F
{
	NOT_IMPLEMENTED("US");
}


// C1:
//-----	
void wxTerminalCtrl::onPAD()  // 0x80
{
	NOT_IMPLEMENTED("PAD");
}

void wxTerminalCtrl::onHOP()  // 0x81
{
	NOT_IMPLEMENTED("HOP");
}

void wxTerminalCtrl::onBPH()  // 0x82
{
	NOT_IMPLEMENTED("BPH");
}

void wxTerminalCtrl::onNBH()  // 0x83
{
	NOT_IMPLEMENTED("NBH");
}

void wxTerminalCtrl::onIND()  // 0x84
{
	NOT_IMPLEMENTED("IND");
}

void wxTerminalCtrl::onNEL()  // 0x85
{
	NOT_IMPLEMENTED("NEL");
}

void wxTerminalCtrl::onSSA()  // 0x86
{
	NOT_IMPLEMENTED("SSA");
}

void wxTerminalCtrl::onESA()  // 0x87
{
	NOT_IMPLEMENTED("ESA");
}

void wxTerminalCtrl::onHTS()  // 0x88
{
	TRACE("HTS");
	setTabStop();
}

void wxTerminalCtrl::onHTJ()  // 0x89
{
	NOT_IMPLEMENTED("HTJ");
}

void wxTerminalCtrl::onVTS()  // 0x8A
{
	NOT_IMPLEMENTED("VTS");
}

void wxTerminalCtrl::onPLD()  // 0x8B
{
	NOT_IMPLEMENTED("PLD");
}

void wxTerminalCtrl::onPLU()  // 0x8C
{
	NOT_IMPLEMENTED("PLU");
}

void wxTerminalCtrl::onRI()   // 0x8D
{
	NOT_IMPLEMENTED("RI");
}

void wxTerminalCtrl::onSS2()  // 0x8E
{
	NOT_IMPLEMENTED("SS2");
}

void wxTerminalCtrl::onSS3()  // 0x8F
{
	NOT_IMPLEMENTED("SS3");
}

void wxTerminalCtrl::onPU1()  // 0x91
{
	NOT_IMPLEMENTED("PU1");
}

void wxTerminalCtrl::onPU2()  // 0x92
{
	NOT_IMPLEMENTED("PU2");
}

void wxTerminalCtrl::onSTS()  // 0x93
{
	NOT_IMPLEMENTED("STS");
}

void wxTerminalCtrl::onCCH()  // 0x94
{
	NOT_IMPLEMENTED("CCH");
}

void wxTerminalCtrl::onMW()   // 0x95
{
	NOT_IMPLEMENTED("MW");
}

void wxTerminalCtrl::onSPA()  // 0x96
{
	NOT_IMPLEMENTED("SPA");
}

void wxTerminalCtrl::onEPA()  // 0x97
{
	NOT_IMPLEMENTED("EPA");
}

void wxTerminalCtrl::onSGCI() // 0x99
{
	NOT_IMPLEMENTED("SGCI");
}

void wxTerminalCtrl::onSCI()  // 0x9A
{
	NOT_IMPLEMENTED("SCI");
}


// CSI:
//-----	
void wxTerminalCtrl::onICH(unsigned short nb) // Insert P s (Blank) Character(s) (default = 1)
{
	NOT_IMPLEMENTED("ICH " << nb);
}

void wxTerminalCtrl::onCUU(unsigned short nb) // Cursor Up P s Times (default = 1)
{
	TRACE("CUU " << nb);
	cursorUp(nb);
}

void wxTerminalCtrl::onCUD(unsigned short nb) // Cursor Down P s Times (default = 1)
{
	TRACE("CUD " << nb);
	cursorDown(nb);
}

void wxTerminalCtrl::onCUF(unsigned short nb) // Cursor Forward P s Times (default = 1)
{
	TRACE("CUF " << nb);
	cursorRight(nb);
}

void wxTerminalCtrl::onCUB(unsigned short nb) // Cursor Backward P s Times (default = 1)
{
	TRACE("CUB " << nb);
	cursorLeft(nb);
}

void wxTerminalCtrl::onCNL(unsigned short nb) // Cursor Next Line P s Times (default = 1)
{
	TRACE("CNL " << nb);
	cursorDown(nb);
	setCursorColumn(0);
}

void wxTerminalCtrl::onCPL(unsigned short nb) // Cursor Preceding Line P s Times (default = 1)
{
	TRACE("CPL " << nb);
	cursorUp(nb);
	setCursorColumn(0);
}

void wxTerminalCtrl::onCHA(unsigned short nb) // Moves the cursor to column n.
{
	TRACE("CHA " << nb);
	setCursorColumn(nb);
}

void wxTerminalCtrl::onCUP(unsigned short row, unsigned short col) // Moves the cursor to row n, column m
{
	TRACE("CUP row=" << row << " col=" << col);
	setCursorPosition(row-1, col-1);
}

void wxTerminalCtrl::onCHT(unsigned short nb) // Cursor Forward Tabulation P s tab stops (default = 1)
{
	TRACE("CHT nb=" << nb);
	while(nb-- > 0)
	{
		forwardTabStops();
	}
}


void wxTerminalCtrl::onED(unsigned short opt) // Clears part of the screen.
{
	TRACE("ED " << opt); 
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
	NOT_IMPLEMENTED("DECSED " << opt);
}

void wxTerminalCtrl::onEL(unsigned short opt) // Erases part of the line. -- Mostly done
{
	TRACE("EL " << opt);
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
	NOT_IMPLEMENTED("DECSEL " << nb);
}

void wxTerminalCtrl::onIL(unsigned short nb)  // Insert Ps Line(s) (default = 1)
{
	TRACE("IL " << nb);
	insertLines(nb);
}

void wxTerminalCtrl::onDL(unsigned short nb)  // Delete Ps Line(s) (default = 1)
{
	TRACE("DL " << nb);
	deleteLines(nb);
}

void wxTerminalCtrl::onDCH(unsigned short nb) // Delete Ps Character(s) (default = 1)
{
	NOT_IMPLEMENTED("DCH " << nb);
}

void wxTerminalCtrl::onSU(unsigned short nb)  // Scroll up Ps lines (default = 1)
{
	NOT_IMPLEMENTED("SU " << nb);
}

void wxTerminalCtrl::onSD(unsigned short nb)  // Scroll down Ps lines (default = 1)
{
	NOT_IMPLEMENTED("SD " << nb);
}

void wxTerminalCtrl::onECH(unsigned short nb)  // Erase Ps Character(s) (default = 1)
{
	NOT_IMPLEMENTED("ECH " << nb);
}

void wxTerminalCtrl::onCBT(unsigned short nb)  // Cursor Backward Tabulation Ps tab stops (default = 1)
{
	TRACE("CBT nb=" << nb);
	while(nb-- > 0)
	{
		backwardTabStops();
	}
}

void wxTerminalCtrl::onHPA(const std::vector<unsigned short> nbs)  // Character Position Absolute [column] (default = [row,1]) (HPA).
{
	NOT_IMPLEMENTED("HPA");
/*	std::cout << "onHPA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onHPR(const std::vector<unsigned short> nbs)  // Character Position Relative [columns] (default = [row,col+1]) (HPR).
{
	NOT_IMPLEMENTED("HPR");
/*	std::cout << "onHPR";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onVPA(const std::vector<unsigned short> nbs)  // Line Position Absolute [row] (default = [1,column])
{
	NOT_IMPLEMENTED("VPA");
/*	std::cout << "onVPA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onVPR(const std::vector<unsigned short> nbs)  // Line Position Relative [rows] (default = [row+1,column]) (VPR)
{
	NOT_IMPLEMENTED("VPR");
/*	std::cout << "onVPR";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onHVP(unsigned short row, unsigned short col) // Horizontal and Vertical Position [row;column] (default = [1,1])
{
	NOT_IMPLEMENTED("HVP row=" << row << " col=" << col);
}

void wxTerminalCtrl::onTBC(unsigned short nb)  // Tab Clear
{
	TRACE("TBC " << nb);
	if(nb==0)
		clearTabStop();
	else if(nb==3)
		clearAllTabStops();
}

void wxTerminalCtrl::onSM(const std::vector<unsigned short> nbs)  // Set Mode
{
	TRACE("SM");
	for(size_t n=0; n<nbs.size(); ++n)
		setANSIMode(nbs[n], true);
}

void wxTerminalCtrl::onDECSET(const std::vector<unsigned short> nbs)  // DEC Private Mode Set
{
	TRACE("DECSET");
	for(size_t n=0; n<nbs.size(); ++n)
		setDECMode(nbs[n], true);
}

void wxTerminalCtrl::onMC(const std::vector<unsigned short> nbs)  // Media Copy
{
	NOT_IMPLEMENTED("MC");
/*	std::cout << "onMC";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onDECMC(const std::vector<unsigned short> nbs)  // DEC specific Media Copy
{
	NOT_IMPLEMENTED("DECMC");
/*	std::cout << "onDECMC";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onRM(const std::vector<unsigned short> nbs)  // Reset Mode
{
	TRACE("RM");
	for(size_t n=0; n<nbs.size(); ++n)
		setANSIMode(nbs[n], false);
}

void wxTerminalCtrl::onDECRST(const std::vector<unsigned short> nbs)  // DEC Private Mode Reset
{
	TRACE("DECRST");
	for(size_t n=0; n<nbs.size(); ++n)
		setDECMode(nbs[n], false);
}

void wxTerminalCtrl::onDSR(unsigned short nb)  // Device Status Report
{
	NOT_IMPLEMENTED("DSR " << nb);
}

void wxTerminalCtrl::onSGR(const std::vector<unsigned short> nbs) // Select Graphic Renditions -- In progress
{
	TRACE("SGR");
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
	NOT_IMPLEMENTED("DECDSR " << nb);
}

void wxTerminalCtrl::onDECSTR()  // Soft terminal reset
{
	NOT_IMPLEMENTED("DECSTR");
}

void wxTerminalCtrl::onDECSCL(unsigned short nb1, unsigned short nb2) // Set conformance level
{
	NOT_IMPLEMENTED("DECSCL " << nb1 << " " << nb2);
}

void wxTerminalCtrl::onDECRQM(unsigned short nb)  // Request DEC private mode
{
	NOT_IMPLEMENTED("DECRQM " << nb);
}

void wxTerminalCtrl::onDECLL(unsigned short nb)  // Load LEDs
{
	NOT_IMPLEMENTED("DECLL " << nb);
}

void wxTerminalCtrl::onDECSCUSR(unsigned short nb)  // Set cursor style (DECSCUSR, VT520).
{
	NOT_IMPLEMENTED("DECSCUSR " << nb);
}

void wxTerminalCtrl::onDECSCA(unsigned short nb)  // Select character protection attribute (DECSCA).
{
	NOT_IMPLEMENTED("DECSCA " << nb);
}

void wxTerminalCtrl::onDECSTBM(unsigned short top, unsigned short bottom) // Set Scrolling Region [top;bottom] (default = full size of window)
{
	NOT_IMPLEMENTED("DECSTBM top=" << top << " bottom=" << bottom);
}

void wxTerminalCtrl::onRDECPMV(const std::vector<unsigned short> nbs)  // Restore DEC Private Mode Values. The value of P s previously saved is restored. P s values are the same as for DECSET.
{
	NOT_IMPLEMENTED("RDECPMV");
/*	std::cout << "onRDECPMV";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onSDECPMV(const std::vector<unsigned short> nbs)  // Save DEC Private Mode Values. P s values are the same as for DECSET.
{
	NOT_IMPLEMENTED("SDECPMV");
/*	std::cout << "onSDECPMV";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onDECCARA(const std::vector<unsigned short> nbs)  // Change Attributes in Rectangular Area (DECCARA), VT400 and up.
{
	NOT_IMPLEMENTED("DECCARA");
/*	std::cout << "onDECCARA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onDECSLRM(unsigned short left, unsigned short right) // Set left and right margins (DECSLRM), available only when DECLRMM is enabled (VT420 and up).
{
	NOT_IMPLEMENTED("DECSLRM left=" << left << " right=" << right);
}

void wxTerminalCtrl::onANSISC()  // Save cursor (ANSI.SYS), available only when DECLRMM is disabled.
{
	NOT_IMPLEMENTED("ANSISC");
}

void wxTerminalCtrl::onANSIRC()  // Restore cursor (ANSI.SYS).
{
	NOT_IMPLEMENTED("ANSIRC");
}

void wxTerminalCtrl::onWindowManip(unsigned short nb1, unsigned short nb2, unsigned short nb3) // Set conformance level
{
	NOT_IMPLEMENTED("WindowManip " << nb1 << " " << nb2 << " " << nb3);
}

void wxTerminalCtrl::onDECRARA(const std::vector<unsigned short> nbs)  // Reverse Attributes in Rectangular Area (DECRARA), VT400 and up.
{
	NOT_IMPLEMENTED("DECRARA");
/*	std::cout << "onDECRARA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onDECSWBV(unsigned short nb)  // Set warning-bell volume (DECSWBV, VT520).
{
	NOT_IMPLEMENTED("DECSWBV " << nb);
}

void wxTerminalCtrl::onDECSMBV(unsigned short nb)  // Set margin-bell volume (DECSMBV, VT520).
{
	NOT_IMPLEMENTED("DECSMBV " << nb);
}

void wxTerminalCtrl::onDECCRA(const std::vector<unsigned short> nbs)  // Copy Rectangular Area (DECCRA, VT400 and up).
{
	NOT_IMPLEMENTED("DECCRA");
/*	std::cout << "onDECCRA";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onDECEFR(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Enable Filter Rectangle (DECEFR), VT420 and up.
{
	NOT_IMPLEMENTED("DECEFR " << top << " " << left << " " << bottom << " " << right);
}

void wxTerminalCtrl::onDECREQTPARM(unsigned short nb)  // Request Terminal Parameters
{
	NOT_IMPLEMENTED("DECREQTPARM " << nb);
}

void wxTerminalCtrl::onDECSACE(unsigned short nb)  // Select Attribute Change Extent
{
	NOT_IMPLEMENTED("DECSACE " << nb);
}

void wxTerminalCtrl::onDECFRA(unsigned short chr, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Fill Rectangular Area (DECFRA), VT420 and up.
{
	NOT_IMPLEMENTED("DECFRA " << chr << " " << top << " " << left << " " << bottom << " " << right);
}

void wxTerminalCtrl::onDECRQCRA(unsigned short id, unsigned short page, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Request Checksum of Rectangular Area (DECRQCRA), VT420 and up.
{
	NOT_IMPLEMENTED("DECRQCRA " << id << " " << page << " " << top << " " << left << " " << bottom << " " << right);
}

void wxTerminalCtrl::onDECELR(unsigned short nb1, unsigned short nb2) // Enable Locator Reporting (DECELR).
{
	NOT_IMPLEMENTED("DECELR " << nb1 << " " << nb2);
}

void wxTerminalCtrl::onDECERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Erase Rectangular Area (DECERA), VT400 and up.
{
	NOT_IMPLEMENTED("DECERA " << top << " " << left << " " << bottom << " " << right);
}

void wxTerminalCtrl::onDECSLE(const std::vector<unsigned short> nbs)  // Select Locator Events (DECSLE).
{
	NOT_IMPLEMENTED("DECSLE");
/*	std::cout << "onDECSLE";
	for(size_t n=0; n<nbs.size(); ++n)
		std::cout << " " << nbs[n];
	std::cout << std::endl;*/
}

void wxTerminalCtrl::onDECSERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Selective Erase Rectangular Area (), VT400 and up.
{
	NOT_IMPLEMENTED("DECSERA " << top << " " << left << " " << bottom << " " << right);
}

void wxTerminalCtrl::onDECRQLP(unsigned short nb)  // Request Locator Position (DECRQLP).
{
	NOT_IMPLEMENTED("DECRQLP " << nb);
}

void wxTerminalCtrl::onDECIC(unsigned short nb)  // Insert P s Column(s) (default = 1) (DECIC), VT420 and up.
{
	NOT_IMPLEMENTED("DECIC " << nb);
}

void wxTerminalCtrl::onDECDC(unsigned short nb)  // InsDelete P s Column(s) (default = 1) (DECIC), VT420 and up.
{
	NOT_IMPLEMENTED("onDECDC " << nb);
}

void wxTerminalCtrl::onOSC(unsigned short command, const std::vector<unsigned char>& params) // Receive an OSC (Operating System Command) command. -- In progress
{
	switch(command)
	{
		case 0: // Change Icon Name and Window Title to Pt.
		case 2: // Change Window Title to Pt.
		{
			wxString str((const char*)&params.front(), params.size());
			std::cout << "Change window title : " << str << std::endl;
			break;
		}
		default:
			// TODO Add others
			NOT_IMPLEMENTED("OSC command=" << command);
			break;
	}
}


