#include <menu.h>
#include <stdio.h>
#include <rendering.h>
#include <windows.h>
#include <window.h>
#include <voxlap.h>
#include <aos_config.h>

#define MAX(x,y) (x>y) ? x : y
#define MIN(x,y) (x<y) ? x : y

struct Menu* menus[MAX_MENU_ENTRIES];

int mouseXPos = 0;
int mouseYPos = 0;
int showCursor = 0;

// only one can be active per time
struct ItemTextInput* activeInputItem = 0;

int getNextAvailableMenuId() {
	int i = 0;
	for (; i <= MAX_MENU_ENTRIES; i++) {
		if (menus[i] == NULL)
			break;
	}

	return i;
}

int getNextAvailableItemId(struct Menu* menu) {
	int i = 0;
	for (; i <= MAX_MENU_ITEMS; i++) {
		if (menu->items[i] == NULL)
			break;
	}

	return i;
}

void createText(struct Menu* menu, int fontid, int color, char* text) {
	int id = getNextAvailableItemId(menu);

	struct ItemText* txtItem = malloc(sizeof(struct ItemText)+32);
	txtItem->type = 0;
	txtItem->id = id;
	txtItem->color = color;
	txtItem->fontId = fontid;
	strncpy(txtItem->text, text, 32);

	menu->items[id] = txtItem;
	menus[menu->id] = menu;
}

void addNewText(struct ItemMultitext* multitext, char* text) {
	int strLen = strlen(text);
	struct MultitextNode* node = malloc(sizeof(struct MultitextNode) + (strLen < 128 ? strLen : 128));

	node->next = 0;
	strncpy(node->text, text, 128);

	if (multitext->lastNode == 0) {
		node->previous = 0;

		multitext->firstNode = node;
		multitext->lastNode = node;
	} else {
		node->previous = multitext->lastNode;

		multitext->lastNode->next = node;
		multitext->lastNode = node;
	}
}

struct ItemMultitext* createMultitext(struct Menu* menu, int color) {
	int id = getNextAvailableItemId(menu);

	struct ItemMultitext* multitextItem = malloc(sizeof(struct ItemMultitext));
	multitextItem->type = 2;
	multitextItem->id = id;
	multitextItem->color = color;
	multitextItem->currentPos = 0;
	multitextItem->firstNode = 0;
	multitextItem->lastNode = 0;

	menu->items[id] = multitextItem;
	menus[menu->id] = menu;

	return multitextItem;
}

void createClickableButton(struct Menu* menu, char* text, void (*func)()) {
	int id = getNextAvailableItemId(menu);

	struct ItemClickableButton* btn = malloc(sizeof(struct ItemClickableButton) + 32);
	btn->type = 1;
	btn->id = id;
	btn->color = 0xffff0000;
	btn->holdColor = 0xff00ff00;
	btn->xSize = 60;
	btn->ySize = 20;
	btn->event = func;
	strncpy(btn->text, text, 32);

	menu->items[id] = btn;
	menus[menu->id] = menu;
}

void createTextInput(struct Menu* menu, int xSize, int ySize, long backgroundColor, char* placeholder) {
	int id = getNextAvailableItemId(menu);

	struct ItemTextInput* input = malloc(sizeof(struct ItemTextInput) + 256);
	input->type = 3;
	input->id = id;
	input->xSize = xSize;
	input->ySize = ySize;
	input->backgroundColor = backgroundColor;
	input->isActive = 0;
	strncpy(input->placeholder, placeholder, 128);

	menu->items[id] = input;
	menus[menu->id] = menu;
}

struct Menu* createMenu(int x, int y, int outline, char* title) {
	int menuId = getNextAvailableMenuId();
	struct Menu* menu = malloc(sizeof(struct Menu)+32);	

	menu->id = menuId;
	menu->outlineColor = outline;
	menu->x = x;
	menu->y = y;
	menu->xSize = 150;
	menu->ySize = 100;
	menu->fixedSize = 1;
	menu->hidden = 1;
	strncpy(menu->title, title, 32);

	menus[menuId] = menu;
	return menu;
}

void showAllMenus() {
	showCursor = 1;
	int menusLen = getNextAvailableMenuId();

	for (int menuId = 0; menuId < menusLen; menuId++) {
		struct Menu* menu = (struct Menu*)menus[menuId];
		menu->hidden = 0;
	}
}

void hideAllMenus() {
	showCursor = 0;
	int menusLen = getNextAvailableMenuId();

	for (int menuId = 0; menuId < menusLen; menuId++) {
		struct Menu* menu = (struct Menu*)menus[menuId];
		menu->hidden = 1;
	}
}

void renderMenuText(struct Menu* menu, struct ItemText* item, int y) {
	drawCustomFontText(menu->x, menu->y+y, item->color, item->fontId, item->text);
}

//returns 1 if an interaction happened
int handleCursor() {
	int mx;
	int my;
	int status;

	struct WindowSize winsize = getConfigWindowSize();
	getmousechange(&mx, &my, &status);

	mouseXPos += mx;
	mouseYPos += my;

	if (mouseXPos < 0) {
		mouseXPos = 0;
	} else {
		mouseXPos = MIN(mouseXPos, winsize.width);
	}

	if (mouseYPos < 0) {
		mouseYPos = 0;
	} else {
		mouseYPos = MIN(mouseYPos, winsize.height);
	}

	return status;
}

void handleKeyboard() {
	if (!activeInputItem)
		return;

	long key = keyread();
	if(!key&255)
		return;

	int currentLen = strlen(activeInputItem->input);
	if (currentLen > 127)
		return;

	activeInputItem->input[currentLen] = key&255;

}

int checkCursorOver(int areaX1, int areaY1, int areaX2, int areaY2) {
	return (mouseXPos >= areaX1 && mouseXPos <= areaX2 && mouseYPos >= areaY1 && mouseYPos <= areaY2);
}

void drawCursor() {
	// todo load mouse cursor image
	long color[] = {0xffffffff};
	drawtile(color, 1, 1, 1, 0x0, 0x0, mouseXPos, mouseYPos, 15, 15, -1);
}

void drawMenus() {
	int menusLen = getNextAvailableMenuId();

	int interaction = 0;
	if (showCursor)
		interaction = handleCursor();

	for (int menuId = 0; menuId < menusLen; menuId++) {
		struct Menu* menu = (struct Menu*)menus[menuId];
		if (menu->hidden)
			continue;

		int itemsLen = getNextAvailableItemId(menu);

		int largestX = MAX(menu->xSize, strlen(menu->title)*8);
		int largestY = 8;

		// title background
		long color[] = {0xe0000000};
		drawtile(color, 1, 1, 1, 0x0, 0x0, menu->x, menu->y, menu->xSize, largestY, -1);

		// content background
		color[0] = 0xc0000000;
		drawtile(color, 1, 1, 1, 0x0, 0x0, menu->x, menu->y, menu->xSize, menu->ySize, -1);

		drawText(menu->x, menu->y, 0xffffff, menu->title);

		for (int itemId = 0; itemId < itemsLen; itemId++) {
			void* item = menu->items[itemId];
			int itemType = ((struct Item*)item)->type;

			switch(itemType) {
				case TEXT_ITEM:
					struct ItemText* txtItem = (struct ItemText*)item;
					int txtSizeX = getCustomFontSize(txtItem->fontId, txtItem->text);
					if (menu->fixedSize && txtSizeX > menu->xSize) {
						int singleCharSize = txtSizeX/strlen(txtItem->text);
						for (int i = txtSizeX; i > menu->xSize; i--) {
							txtItem->text[i/singleCharSize] = '\0';
						}
					}

					renderMenuText(menu, item, largestY);

					largestX = MAX(largestX, txtSizeX);
					largestY += MAX(largestY, ((txtItem->fontId+1)*8)+8);
					break;
				case CLICKABLE_BUTTON_ITEM:
					struct ItemClickableButton* clickBtn = (struct ItemClickableButton*)item;

					if (interaction && checkCursorOver(menu->x, menu->y+largestY,
													   menu->x+clickBtn->xSize,
													   menu->y+largestY+clickBtn->ySize))
					{
						color[0] = clickBtn->holdColor;
						clickBtn->isClicking = 1;
						clickBtn->event(menu, clickBtn);
					} else {
						// i prefer this than setting value two times
						// probably later i cna find something better
						int clicking = clickBtn->isClicking;
						clickBtn->isClicking = 0;
						if (clicking) {
							clickBtn->event(menu, clickBtn);
						}

						color[0] = clickBtn->color;
					}

					drawtile(color, 1, 1, 1, 0x0, 0x0, menu->x, menu->y+largestY, clickBtn->xSize, clickBtn->ySize, -1);

					int textlen = strlen(clickBtn->text);
					drawText(menu->x+clickBtn->xSize/2-textlen*3, menu->y+largestY+clickBtn->ySize/2-4, 0x0, clickBtn->text);

					drawSquare(menu->x, menu->y+largestY, menu->x+clickBtn->xSize, menu->y+largestY+clickBtn->ySize, 0x0);

					largestX = MAX(largestX, clickBtn->xSize);
					largestY += clickBtn->ySize+2;
					break;
				case MULTITEXT_ITEM:
					struct ItemMultitext* multitext = (struct ItemMultitext*)item;
					struct MultitextNode* lastNode = multitext->lastNode;

					for (int i = 0; i < 5; i++) {
						int txtSizeX = strlen(lastNode->text)*6; // drawText uses 6x8

						if (menu->fixedSize && txtSizeX > menu->xSize) {
							char copyTxtNode[128];
							int characterCopy = 0;

							for (int characterNode = 0; characterNode < txtSizeX/6; characterNode++) {
								int copyTxtNodeLen = strlen(copyTxtNode)*6;
								if (copyTxtNodeLen+2 > menu->xSize) {
									drawText(menu->x, menu->y+largestY, multitext->color, copyTxtNode);
									largestY += 8;
									memset(copyTxtNode, 0, sizeof copyTxtNode);
									characterCopy = 0;
								}

								copyTxtNode[characterCopy] = lastNode->text[characterNode];
								characterCopy+=1;
							}

							drawText(menu->x, menu->y+largestY, multitext->color, copyTxtNode);
						} else {
							drawText(menu->x, menu->y+largestY, multitext->color, lastNode->text);
						}

						largestX = MAX(largestX, txtSizeX);
						largestY += 10;

						if (lastNode->previous == 0)
							break;

						lastNode = lastNode->previous;
					}
					break;
				case TEXTINPUT_ITEM:
					struct ItemTextInput* input = (struct ItemTextInput*)item;

					color[0] = input->backgroundColor;
					drawtile(color, 1, 1, 1, 0x0, 0x0, menu->x, menu->y+largestY, input->xSize, input->ySize, -1);
					int displayLen = input->xSize/6;
					char textDisplay[displayLen];

					if (interaction && checkCursorOver(menu->x, menu->y+largestY,
													   menu->x+input->xSize,
													   menu->y+largestY+input->ySize))
					{
						input->isActive = 1;
						activeInputItem = input;
					}

					if (!input->isActive) {
						if (input->input[0]) {
							strncpy(textDisplay, input->input, displayLen);
						} else {
							strncpy(textDisplay, input->placeholder, displayLen);
						}
					} else {
						char result[129];
						snprintf(result, displayLen, "%s_", input->input);
						strncpy(textDisplay, result, displayLen);
					}

					drawText(menu->x, menu->y+largestY+input->ySize/2-4, 0x505050, textDisplay);
			}
		}

		largestY = MAX(menu->ySize, largestY);
		if (menu->fixedSize) {
			largestX = menu->xSize;
			largestY = menu->ySize;
		} else {
			menu->xSize = largestX;
			menu->ySize = largestY;
		}

		// separator title | content
		drawline2d(menu->x, menu->y+8, menu->x+largestX, menu->y+8, menu->outlineColor);
		drawSquare(menu->x, menu->y, menu->x+largestX, menu->y+largestY, menu->outlineColor);

		if (interaction && checkCursorOver(menu->x, menu->y, menu->x+largestX, menu->y+8)) {
			int xoffset, yoffset, bst;
			getmousechange(&xoffset, &yoffset, &bst);
			menu->x += xoffset;
			menu->y += yoffset;
		}
	}

	if (showCursor)
		drawCursor();
}