#include "dialog.h"
#include "screen.h"
#include "../specKeyboard.h"

//---------------------------------------------------------------------------------------
void Shell_Window(int x, int y, int w, int h, const char *title, byte attr)
{
	int titleSize = strlen(title);
	int titlePos = (w - titleSize) / 2;

	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			char current = ' ';

			if (i == 0 && j == 0)
				current = 0xC9;
			else if (i == 0 && j == w - 1)
				current = 0xBB;
			else if (i == h - 1 && j == 0)
				current = 0xC8;
			else if (i == h - 1 && j == w - 1)
				current = 0xBC;
			else if (i == 0 || i == h - 1)
				current = 0xCD;
			else if (j == 0 || j == w - 1)
				current = 0xBA;

			if (i == 0) {
				if (j >= titlePos && j < (titlePos + titleSize)) {
					current = title[j - titlePos];
				}
				else if (j == (titlePos - 1) || j == (titlePos + titleSize)) {
					current = ' ';
				}
			}

			WriteAttr(x + j, y + i, attr);
			WriteChar(x + j, y + i, current);
		}
}
//---------------------------------------------------------------------------------------
bool Shell_MessageBox(const char *title, const char *str, const char *str2, const char *str3, int type, byte attr, byte attrSel)
{
	int w = 12;
	int h = 3;

	if (w < (int) strlen(title) + 6)
		w = strlen(title) + 6;
	if (w < (int) strlen(str) + 2)
		w = strlen(str) + 2;

	if (str2[0] != 0) {
		if (w < (int) strlen(str2) + 2)
			w = strlen(str2) + 2;
		h++;

		if (str3[0] != 0) {
			if (w < (int) strlen(str3) + 2)
				w = strlen(str3) + 2;
			h++;
		}
	}

	if (type != MB_NO)
		h++;

	int x = (32 - w) / 2;
	int y = (22 - h) / 2;

	Shell_Window(x, y, w, h, title, attr);
	x++;
	y++;

	WriteStr(x, y++, str);
	if (str2[0] != 0) {
		WriteStr(x, y++, str2);
		if (str3[0] != 0)
			WriteStr(x, y++, str3);
	}

	bool result = true;
	if (type == MB_NO)
		return true;

	while (true) {
		if (type == MB_OK) {
			WriteStrAttr(16 - 2, y, " OK ", attrSel);
		}
		else {
			WriteStrAttr(16 - 5, y, " Yes ", result ? attrSel : attr);
			WriteStrAttr(16, y, " No ", result ? attr : attrSel);
		}

		byte key = GetKey();

		if (key == K_ESC)
			result = false;
		if (key == K_ESC || key == K_RETURN)
			break;

		if (type == MB_YESNO)
			result = !result;
	}

	return result;
}
//---------------------------------------------------------------------------------------
bool Shell_InputBox(const char *title, const char *str, CString &buff)
{
	int w = 22;
	int h = 4;

	if (w < (int) strlen(title) + 6)
		w = strlen(title) + 6;
	if (w < (int) strlen(str) + 2)
		w = strlen(str) + 2;

	int x = (32 - w) / 2;
	int y = (22 - h) / 2;

	Shell_Window(x, y, w, h, title, 0050);

	x++;
	y++;
	w -= 2;

	WriteStr(x, y++, str);
	WriteAttr(x, y, 0150, w);

	int p = strlen(buff);
	int s = 0;

	while (true) {
		while ((p - s) >= w)
			s++;
		while ((p - s) < 0)
			s--;
		while (s > 0 && (s + w - 1) > buff.Length())
			s--;

		for (int i = 0; i < w; i++) {
			WriteChar(x + i, y, buff.GetSymbol(s + i));
			WriteAttr(x + i, y, i == (p - s) ? 0106 : 0150);
		}

		char key = GetKey();

		if (key == K_RETURN)
			return true;
		else if (key == K_ESC)
			return false;
		else if (key == K_LEFT && p > 0)
			p--;
		else if (key == K_RIGHT && p < buff.Length())
			p++;
		else if (key == K_HOME)
			p = 0;
		else if (key == K_END)
			p = buff.Length();
		else if (key == K_BACKSPACE && p > 0)
			buff.Delete(--p, 1);
		else if (key == K_DELETE && p < buff.Length())
			buff.Delete(p, 1);
		else if ((byte) key >= 0x20)
			buff.Insert(p++, key);
	}
}
//---------------------------------------------------------------------------------------