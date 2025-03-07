/*-----------------------------------------------------------------
 Copyright (C) 2015
	Matthew Scholefield

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include <gl2d.h>
#include <list>
#include <stdio.h>
#include <nds/interrupts.h>
#include "FontGraphic.h"
#include "fontHandler.h"
#include "TextEntry.h"

// GRIT auto-genrated arrays of images
#include "font_si.h"
#include "font_16x16.h"

// Texture UV coords
#include "uvcoord_font_si.h"
#include "uvcoord_font_16x16.h"
#include "TextPane.h"

using namespace std;

FontGraphic smallFont;
FontGraphic largeFont;

glImage smallFontImages[FONT_SI_NUM_IMAGES];
glImage largeFontImages[FONT_16X16_NUM_IMAGES];

list<TextEntry> topText, bottomText;
list<TextPane> panes;

void fontInit()
{
	// Set  Bank A to texture (128 kb)
	vramSetBankA(VRAM_A_TEXTURE);
	smallFont.load(smallFontImages, // pointer to glImage array
				FONT_SI_NUM_IMAGES, // Texture packer auto-generated #define
				font_si_texcoords, // Texture packer auto-generated array
				GL_RGB256, // texture type for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_64, // sizeX for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_128, // sizeY for glTexImage2D() in videoGL.h
				GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, // param for glTexImage2D() in videoGL.h
				256, // Length of the palette (256 colors)
				(u16*) font_siPal, // Palette Data
				(u8*) font_siBitmap // image data generated by GRIT
				);

	// Do the same with our bigger texture
	largeFont.load(largeFontImages,
				FONT_16X16_NUM_IMAGES,
				font_16x16_texcoords,
				GL_RGB256,
				TEXTURE_SIZE_64,
				TEXTURE_SIZE_512,
				GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
				256,
				(u16*) font_siPal,
				(u8*) font_16x16Bitmap
				);
}

TextPane &createTextPane(int startX, int startY, int shownElements)
{
	if (panes.size() > 2)
		panes.pop_front();
	panes.emplace_back(startX, startY, shownElements);
	return panes.back();
}

static list<TextEntry> &getTextQueue(bool top)
{
	return top ? topText : bottomText;
}

FontGraphic &getFont(bool large)
{
	return large ? largeFont : smallFont;
}

void updateText(bool top)
{
	auto &text = getTextQueue(top);
	for (auto it = text.begin(); it != text.end(); ++it) {
		if (it->update()) {
			it = text.erase(it);
			--it;
			continue;
		}
		int alpha = it->calcAlpha();
		if (alpha > 0) {
			glPolyFmt(POLY_ALPHA(alpha) | POLY_CULL_NONE | POLY_ID(1));
			if (top)
				glColor(RGB15(0, 0, 0));
			getFont(it->large).print(it->x / TextEntry::PRECISION, it->y / TextEntry::PRECISION, it->message);
		}
	}
	for (auto it = panes.begin(); it != panes.end(); ++it) {
		if (it->update(top)) {
			it = panes.erase(it);
			--it;
			continue;
		}
	}
}

void clearText(bool top)
{
	list<TextEntry> &text = getTextQueue(top);
	for (auto it = text.begin(); it != text.end(); ++it) {
		if (it->immune)
			continue;
		it = text.erase(it);
		--it;
	}
}

void clearText()
{
	clearText(true);
	clearText(false);
}

void printSmall(bool top, int x, int y, const char *message)
{
	getTextQueue(top).emplace_back(false, x, y, message);
}

void printSmallCentered(bool top, int y, const char *message)
{
	getTextQueue(top).emplace_back(false, smallFont.getCenteredX(message), y, message);
}

void printLarge(bool top, int x, int y, const char *message)
{
	getTextQueue(top).emplace_back(true, x, y, message);
}

void printLargeCentered(bool top, int y, const char *message)
{
	getTextQueue(top).emplace_back(true, largeFont.getCenteredX(message), y, message);
}

int calcSmallFontWidth(const char *text)
{
	return smallFont.calcWidth(text);
}

int calcLargeFontWidth(const char *text)
{
	return largeFont.calcWidth(text);
}

TextEntry *getPreviousTextEntry(bool top)
{
	return &getTextQueue(top).back();
}

void waitForPanesToClear()
{
	while (panes.size() > 0)
		swiWaitForVBlank();
}