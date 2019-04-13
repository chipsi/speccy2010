#include "debugger.h"
#include "screen.h"
#include "dialog.h"
#include "../system.h"
#include "../specKeyboard.h"

#define _W(x) ((x) & 0xFFFF)

unsigned dumpTop = 0;
unsigned dumpPos = 0;
unsigned lastDumpTop = -1U;
unsigned lastDumpPos = -1U;

bool dumpWindowAsciiMode = false;
bool dumpWindowRightPane = false;
bool lowNibble = false;

extern byte activeWindow;
extern bool activeRegPanel;
extern byte watcherMode;
extern bool allowROMAccess;
//---------------------------------------------------------------------------------------
void Debugger_UpdateDump(bool forceRedraw)
{
	if (lastDumpTop != dumpTop || forceRedraw) {
		byte i, j, y, c;
		word inc = dumpWindowAsciiMode ? 0x20 : 0x08;
		word pc = _W(dumpTop & (-inc));

		for (i = 0, y = WINDOW_OFF_Y; i < WINDOW_HEIGHT; i++, y++) {
			DrawHexNum(0, y, pc, 4, 'A');

			for (j = 0; j < inc; j++, pc++) {
				c = ReadByteAtCursor(pc);

				if (dumpWindowAsciiMode)
					DrawSaveChar(35 + (j * 6), y, c, false, (pc == _W(dumpPos)));
				else {
					DrawHexNum(35 + (j * 16), y, c, 2, 'A');
					DrawSaveChar(169 + (j * 8), y, c);
				}
			}

			if (dumpWindowAsciiMode)
				DrawChar(227, y, ' ');

			DrawChar(238, y, '\x1C');
			DrawHexNum(244, y, (pc - 1) & 0xFF, 2, 'A');
		}

		lastDumpTop = dumpTop;
	}
	else if (dumpWindowAsciiMode) {
		int offset = lastDumpPos - lastDumpTop;
		byte x = offset % 0x20;
		byte y = offset / 0x20;
		byte c = ReadByteAtCursor(lastDumpPos);

		DrawSaveChar(35 + (x * 6), WINDOW_OFF_Y + y, c);

		offset = dumpPos - dumpTop;
		x = offset % 0x20;
		y = offset / 0x20;
		c = ReadByteAtCursor(dumpPos);

		DrawSaveChar(35 + (x * 6), WINDOW_OFF_Y + y, c, false, true);
	}

	lastDumpPos = dumpPos;

	Debugger_UpdateDumpAttrs();
}
//---------------------------------------------------------------------------------------
void Debugger_UpdateDumpAttrs()
{
	byte atr0 = activeRegPanel ? COL_NORMAL : COL_ACTIVE;
	byte atr1 = atr0 & ~3;

	int offset = dumpPos - dumpTop;
	byte offX = offset % 8;
	byte offY = offset / 8;

	for (byte i = 0, y = WINDOW_OFF_Y; i < WINDOW_HEIGHT; i++, y++) {
		if (dumpWindowAsciiMode)
			DrawAttr8( 0, y, atr0, 29);
		else {
			DrawAttr8( 0, y, atr0, 21);
			DrawAttr8(21, y, atr1 + 1, 8);

			if (!activeRegPanel && i == offY) {
				if (dumpWindowRightPane)
					DrawAttr8(21 + offX, y, COL_CURSOR);
				else
					DrawAttr8(4 + (offX << 1), y, COL_CURSOR, 2);
			}
		}

		DrawAttr8(29, y, atr1, 3);
	}
}
//---------------------------------------------------------------------------------------
bool Debugger_TestKeyDump(byte key, bool *updateAll)
{
	bool isDump = (activeWindow == WIN_MEMDUMP);
	int reg = -1;

	if ((ReadKeyFlags() & fKeyCtrl) != 0) {
		switch (key) {
			case 'h':
				reg = REG_HL;
				break;
			case 'H':
				reg = REG_HL_;
				break;
			case 'd':
				reg = REG_DE;
				break;
			case 'D':
				reg = REG_DE_;
				break;
			case 'b':
				reg = REG_BC;
				break;
			case 'B':
				reg = REG_BC_;
				break;
			case 'x':
				reg = REG_IX;
				break;
			case 'y':
				reg = REG_IY;
				break;
			case 's':
				if (!isDump)
					break;

				reg = REG_SP;
				break;
			case 'p':
				// PC or switch watcher to ports
				reg = isDump ? REG_PC : 0;
				break;
			default:
				reg = -1;
		}
	}

	if (isDump) {
		unsigned inc = dumpWindowAsciiMode ? 0x20 : 0x08;
		unsigned page = (WINDOW_HEIGHT * inc);

		unsigned top = (dumpTop < (0x10000 - page)) ? (dumpTop + 0x10000) : dumpTop;
		unsigned pos = (dumpPos < (0x10000 - page)) ? (dumpPos + 0x10000) : dumpPos;

		if (reg > 0)
			top = pos = GetCPUState(reg & 0xFF);
		else if ((key == 'g' && (ReadKeyFlags() & fKeyCtrl) != 0) ||
				(key == 'm' && (ReadKeyFlags() & fKeyCtrl) == 0)) {

			if ((reg = Debugger_GetMemAddress(dumpTop)) >= 0)
				top = pos = (unsigned) reg;
			reg = -1;
		}
		else if (key == K_UP)
			pos -= inc;
		else if (key == K_DOWN)
			pos += inc;
		else if (key == K_LEFT)
			pos--;
		else if (key == K_RIGHT)
			pos++;
		else if (key == K_PAGEUP) {
			pos -= page;
			top -= page;
		}
		else if (key == K_PAGEDOWN) {
			pos += page;
			top += page;
		}
		else if (key == K_HOME)
			pos &= ~(inc - 1);
		else if (key == K_END)
			pos |= (inc - 1);

		else if (key >= ' ' && (ReadKeyFlags() & (fKeyCtrl | fKeyAlt)) == 0) {
			if (dumpPos < 0x4000 && !allowROMAccess) {
				allowROMAccess = Shell_MessageBox("ROM Access",
					"You are about to modify contents",
					"of (EP)ROM directly in SD-RAM.",
					"You have been warned! Sure?", MB_YESNO);

				if (!allowROMAccess)
					return true;
			}

			if (dumpWindowAsciiMode || dumpWindowRightPane) {
				WriteByteAtCursor(pos++, key);
				*updateAll = true;
			}
			else {
				if (key >= '0' && key <= '9')
					key -= '0';
				else if (key >= 'A' && key <= 'F')
					key -= ('A' - 10);
				else if (key >= 'a' && key <= 'f')
					key -= ('a' - 10);
				else
					return false;

				byte c = ReadByteAtCursor(pos);
				if (lowNibble)
					c = (c & 0xF0) | key;
				else
					c = (c & 0x0F) | (key << 4);

				WriteByteAtCursor(pos, c);

				if (lowNibble) {
					*updateAll = true;
					pos++;
				}
				else
					return (*updateAll = lowNibble = true);
			}
		}
		else
			return false;

		while (pos < top)
			top -= inc;
		while ((pos - top) >= page)
			top += inc;

		dumpPos = pos & 0xFFFF;
		dumpTop = top & _W(-inc);

		lowNibble = false;
		return true;
	}
	else if (reg >= 0) {
		watcherMode = reg & 0xFF;
		return (*updateAll = true);
	}

	return false;
}
//---------------------------------------------------------------------------------------