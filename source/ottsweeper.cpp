#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <queue>

#include "ottsweeper.hpp"
#include "OTTTexture.hpp"
#include "OTTSpriteSet.hpp"
#include "OTTSystem.hpp"
#include "OTTConfigFile.hpp"

Ottsweeper* winptr = 0x0;

void resizeCallback(GLFWwindow* window, int width, int height) {
	winptr->updateWindowSize(width, height);
	winptr->setCurrentWindowScale((double)width / winptr->getNativeWidth(), (double)height / winptr->getNativeHeight());
}

bool Ottsweeper::onUserCreateWindow() {
	winptr = this;

	// Enable mouse and keyboard support
	enableKeyboard();
	enableMouse();

	// Set window resize callback function
	setWindowResizeCallback(resizeCallback);

	// Setup tile map
	gridMap[TileTypes::ZERO] = 0;
	gridMap[TileTypes::ONE] = 1;
	gridMap[TileTypes::TWO] = 2;
	gridMap[TileTypes::THREE] = 3;
	gridMap[TileTypes::FOUR] = 4;
	gridMap[TileTypes::FIVE] = 5;
	gridMap[TileTypes::SIX] = 6;
	gridMap[TileTypes::SEVEN] = 7;
	gridMap[TileTypes::EIGHT] = 8;
	gridMap[TileTypes::BOMB] = 9;
	gridMap[TileTypes::EXPLOSION] = 10;
	gridMap[TileTypes::MISTAKE] = 11;
	gridMap[TileTypes::NORMAL] = 12;
	gridMap[TileTypes::FLAGGED] = 13;
	gridMap[TileTypes::UNKNOWN] = 14;

	// Read input config file
	std::string configFilePath = "default.cfg";
	std::string assetsFilePath = "tiles.png";
	ConfigFile cfgFile;
	if (cfgFile.read(configFilePath)) { // Read configuration file
		if (cfgFile.search("MINES", true))
			nBombs = (int)cfgFile.getUInt();
		if (cfgFile.search("COLS", true))
			nSizeX = (int)cfgFile.getUInt();
		if (cfgFile.search("ROWS", true))
			nSizeY = (int)cfgFile.getUInt();
		if (cfgFile.search("DIFFICULTY", true)) {
			const int diffX[9] = { 10, 9, 8, 16, 16, 15, 15, 15, 30 };
			const int diffY[9] = { 10, 9, 8, 16, 15, 15, 14, 13, 16 };
			const int diffB[9] = { 10, 10, 10, 40, 40, 40, 40, 40, 99 };
			const unsigned int difficulty = cfgFile.getUInt();
			if (difficulty < 9) {
				nSizeX = diffX[difficulty];
				nSizeY = diffY[difficulty];
				nBombs = diffB[difficulty];
				std::cout << " Set difficulty level to " << difficulty << "." << std::endl;
			}
			else {
				std::cout << " Error! Invalid difficulty specified (" << difficulty << ")." << std::endl;
			}
		}
		if (cfgFile.search("TEXTURES", true))
			assetsFilePath = cfgFile.getCurrentParameterString();
	}
	else {
		std::cout << " Warning! Failed to load input configuration file." << std::endl;
		std::cout << "  Generating new default config file." << std::endl;
		std::ofstream ofile(configFilePath.c_str());
		ofile << "# Automatically generated config file for Ottsweeper" << std::endl;
		ofile << "# Built-in difficulty levels:" << std::endl;
		ofile << "#  0 - 10 x 10 (10 mines)" << std::endl; // 10.0
		ofile << "#  1 - 09 x 09 (10 mines)" << std::endl; // 8.1
		ofile << "#  2 - 08 x 08 (10 mines)" << std::endl; // 6.4
		ofile << "#  3 - 16 x 16 (40 mines)" << std::endl; // 6.4
		ofile << "#  4 - 16 x 15 (40 mines)" << std::endl; // 6.0
		ofile << "#  5 - 15 x 15 (40 mines)" << std::endl; // 5.6
		ofile << "#  6 - 15 x 14 (40 mines)" << std::endl; // 5.3
		ofile << "#  7 - 15 x 13 (40 mines)" << std::endl; // 4.8
		ofile << "#  8 - 30 x 16 (99 mines)" << std::endl; // 4.8
		ofile << "#DIFFICULTY  0" << std::endl;
		ofile << "MINES      10" << std::endl;
		ofile << "COLS       10" << std::endl;
		ofile << "ROWS       10" << std::endl;
		ofile << "TEXTURES   tiles.png" << std::endl;
		ofile << std::endl; // Add an extra new line to make sure we keep the final variable
		ofile.close();
	}

	// Numerical digits (13x23, 10 sprites)
	OTTTexture sweeperAssets(assetsFilePath);
	sweeperAssets.increaseColorDepth(4);
	digits.addSprites(&sweeperAssets, 0, 0, 13, 23, 10, 1);

	// Field tiles (16x16, 9+6 sprites)
	tiles.addSprites(&sweeperAssets, 0, 23, 16, 16, 9, 2);

	// Smiley faces (24x24, 3 sprites)
	smilies.addSprites(&sweeperAssets, 0, 55, 24, 24, 3, 1);

	// Print minefield info
	std::cout << " Minefield size set to (" << nSizeX << " x " << nSizeY << ", " << nBombs << " mines)." << std::endl;

	// Change the size of the window
	// Vertical borders: 3 pixels of White, 6 pixels of Gray, 3 pixels of Dark Gray
	// Horizontal borders: 3 pixels of White, 5 pixels of Gray, 3 pixels of Dark Gray
	nNativeWidth = 2 * (3 + 6 + 3) + nSizeX * 16;
	nNativeHeight = 3 * (3 + 5 + 3) + nSizeY * 16 + 32; // Plus 32 pixel header
	updateWindowSize(nNativeWidth, nNativeHeight, true);

	// Generate background texture
	generateBackground();

	// Seed random number generator
	rng.seed();

	// Setup minefield
	minefield = std::vector<unsigned char>(nSizeY * nSizeX, 0);
	playfield = std::vector<unsigned char>(nSizeY * nSizeX, 1);

	// Randomize bomb placement
	resetField();

	// Success
	return true;
}

bool Ottsweeper::onUserLoop() {
	static double dDisplayTime = 0;

	//clear(); // Clear the screen

	// Check for key presses
	if (keys.poll('r')) { // Reset
		resetField();
	}

	// Check for mouse events
	std::vector<int> neighbors;
	nCurrentCellX = ((int)(mouse.getX() / dWindowScaleX) - nMinefieldOffsetX) / 16;
	nCurrentCellY = ((int)(mouse.getY() / dWindowScaleY) - nMinefieldOffsetY) / 16;
	nCurrentCell = nCurrentCellY * nSizeX + nCurrentCellX;
	bLeftClickHeld = false;
	if (nCurrentCellX >= 0 && nCurrentCellY >= 0) {
		if (mouse.check(0)) { // Left mouse button held
			bLeftClickHeld = true;
		}
		else if (mouse.released(0)) { // Left click
			if (playfield[nCurrentCell] == 0) { // Uncovered cell
				if (minefield[nCurrentCell] > 0 && mouse.check(1)) { // Mouse is hovering over a numbered cell and right mouse button is being held 
					// If the mouse is currently over an uncovered and numbered cell, left clicking will uncover
					// all surrounding cells if a matching number of flags exist around the cell.
					std::vector<int> tempNeighbors;
					getNeighbors(tempNeighbors, nCurrentCellX, nCurrentCellY);
					unsigned char nFlags = 0;
					for (auto neighbor = tempNeighbors.begin(); neighbor != tempNeighbors.end(); neighbor++) {
						if (playfield[*neighbor] == 2) // Flagged cell
							nFlags++;
					}
					if (nFlags == minefield[nCurrentCell]) { // Uncover all neighboring cells
						for (auto neighbor = tempNeighbors.begin(); neighbor != tempNeighbors.end(); neighbor++) {
							if (playfield[*neighbor] != 1)
								continue;
							uncoverCell(*neighbor);
						}
					}
				}
			}
			else if (playfield[nCurrentCell] != 2) { // Cell currently hidden (but not flagged)
				uncoverCell(nCurrentCell);
			}
		}
		if (mouse.check(1)) { // Right mouse button held
			if (playfield[nCurrentCell] == 0) { // Cell is uncovered
				// If the mouse is currently over an uncovered cell, all surrounding uncovered cells will appear
				// uncovered and blank (but will remain covered).
				getNeighbors(neighbors, nCurrentCellX, nCurrentCellY);
			}
		}
		else if (mouse.released(1)) { // Right mouse button released
			switch (playfield[nCurrentCell]) {
			case 1: // Flag cell
				playfield[nCurrentCell] = 2;
				break;
			case 2: // Mark cell as unknown (?)
				playfield[nCurrentCell] = 3;
				break;
			case 3: // Un-flag cell
				playfield[nCurrentCell] = 1;
				break;
			default:
				break;
			}
		}
	}
	
	// Draw background
	drawTexture(nBackgroundContext);

	// Draw remaining mines indicator
	drawNumber(16, 15, nRemainingCells);

	// Draw the current time
	if (gameState == GameStates::NORMAL) {
		drawNumber(nNativeWidth - 55, 15, (int)dTotalTime);
	}
	else {
		drawNumber(nNativeWidth - 55, 15, (int)dFinalGameTime);
	}

	// Draw the smiley
	if (gameState == GameStates::NORMAL) {
		smilies[0].draw(nNativeWidth / 2.f, 27.f);
	}
	else if (gameState == GameStates::LOSS) {
		smilies[1].draw(nNativeWidth / 2.f, 27.f);
	}
	else if (gameState == GameStates::WIN) {
		smilies[2].draw(nNativeWidth / 2.f, 27.f);
	}

	for (int y = 0; y < nSizeY; y++) { // Over all rows
		for (int x = 0; x < nSizeX; x++) { // Over all columns
			int index = y * nSizeX + x;
			if (playfield[index] == 2) { // Flagged cell
				drawTile(x, y, TileTypes::FLAGGED);
			}
			else if (playfield[index] > 0) { // Tile is hidden
				if (bLeftClickHeld && index == nCurrentCell) {
					drawTile(x, y, TileTypes::ZERO);
					continue;
				}
				else if (!neighbors.empty()) { // Right mouse button is being held
					if (std::find(neighbors.begin(), neighbors.end(), index) != neighbors.end()) {
						drawTile(x, y, TileTypes::ZERO);
						continue;
					}
				}
				if (playfield[index] == 1) // Normal cell
					drawTile(x, y, TileTypes::NORMAL);
				else if (playfield[index] == 3) // Question mark cell
					drawTile(x, y, TileTypes::UNKNOWN);
			}
			else { // Tile is revealed
				drawTile(x, y, minefield[index]);
			}
		}
	}

	// Print framerate
	if (gameState == GameStates::NORMAL && (dTotalTime >= dDisplayTime + 2)) {
		dDisplayTime = dTotalTime;
		std::stringstream stream;
		stream << "Ottsweeper (" << dFramerate << " fps)";
		setWindowTitle(stream.str());
	}

	// Draw the screen
	render();

	return true;
}

void Ottsweeper::setTileType(const int& x, const int& y, const TileTypes& type) {
	minefield[y * nSizeX + x] = gridMap[type];
}

void Ottsweeper::setTileType(const int& index, const TileTypes& type) {
	minefield[index] = gridMap[type];
}

TileTypes Ottsweeper::getTileType(const int& x, const int& y) const {
	return getTileType(y * nSizeX + x);
}

TileTypes Ottsweeper::getTileType(const int& index) const {
	for (auto type = gridMap.begin(); type != gridMap.end(); type++) {
		if (type->second == minefield[index])
			return type->first;
	}
	return TileTypes::NONE;
}

void Ottsweeper::placeBombs(const int& safeCell) {
	// Clear minefield
	std::fill(minefield.begin(), minefield.end(), gridMap[TileTypes::ZERO]);

	int maxBombs = nSizeX * nSizeY;
	std::vector<int> cellIDs;
	cellIDs.reserve(maxBombs - 1);
	for (int i = 0; i < maxBombs; i++) {
		if(i != safeCell)
			cellIDs.push_back(i);
	}

	// Randomly place bombs
	for (int i = 0; i < nBombs; i++) {
		int randIndex = rng.rand32() % (maxBombs - i - 1);
		setTileType(cellIDs.at(randIndex) % nSizeX, cellIDs.at(randIndex) / nSizeX, TileTypes::BOMB);
		cellIDs.erase(cellIDs.begin() + randIndex);
	}

	// Count all cell neighbors
	for (int y = 0; y < nSizeY; y++) { // Over all rows
		for (int x = 0; x < nSizeX; x++) { // Over all columns
			if (getTileType(x, y) == TileTypes::BOMB) // Skip bombs
				continue;
			unsigned char count = 0;
			int xlow = std::max(0, x - 1);
			int xhigh = std::min(nSizeX - 1, x + 1);
			int ylow = std::max(0, y - 1);
			int yhigh = std::min(nSizeY - 1, y + 1);
			for (int yp = ylow; yp <= yhigh; yp++) {
				for (int xp = xlow; xp <= xhigh; xp++) {
					if (getTileType(xp, yp) == TileTypes::BOMB)
						count++;
				}
			}
			minefield[y * nSizeX + x] = count;
		}
	}
}

void Ottsweeper::resetField() {
	std::fill(playfield.begin(), playfield.end(), 1);
	std::fill(minefield.begin(), minefield.end(), 0);
	dTotalTime = 0; // Reset game timer
	nRemainingCells = nSizeX * nSizeY - nBombs;
	gameState = GameStates::NORMAL;
	bFirstCell = true;
}

void Ottsweeper::endGame(bool bWin) {
	if (bWin) { // Win
		std::stringstream stream;
		stream << " You Won! Time: " << dTotalTime << " s";
		setWindowTitle(stream.str());
		for (int y = 0; y < nSizeY; y++) { // Over all rows
			for (int x = 0; x < nSizeX; x++) { // Over all columns
				switch (getTileType(x, y)) {
				case TileTypes::BOMB: // Bomb
					setTileType(x, y, TileTypes::FLAGGED);
					break;
				default:
					break;
				}
				playfield[y * nSizeX + x] = 0;
			}
		}
		gameState = GameStates::WIN;
	}
	else { // Loss
		std::stringstream stream;
		stream << " You Lose! Try again :)" << std::endl;
		setWindowTitle(stream.str());
		for (int y = 0; y < nSizeY; y++) { // Over all rows
			for (int x = 0; x < nSizeX; x++) { // Over all columns
				switch (getTileType(x, y)) {
				case TileTypes::BOMB:
					break;
				default: // Not a bomb
					if (playfield[y * nSizeX + x] == 2) { // Mis - labeled bomb
						setTileType(x, y, TileTypes::MISTAKE);
					}
					break;
				}
				playfield[y * nSizeX + x] = 0;
			}
		}
		gameState = GameStates::LOSS;
	}
	dFinalGameTime = dTotalTime;
}

void Ottsweeper::fillArea(const int& startX, const int& startY) {
	std::queue<std::pair<int, int> > vec;
	vec.push(std::make_pair(startX, startY));
	int x, y;
	while (!vec.empty()) {
		x = vec.front().first;
		y = vec.front().second;
		vec.pop();
		if (getTileType(x, y) != TileTypes::ZERO)
			continue;
		playfield[y * nSizeX + x] = 0;
		if (y > 0 && playfield[(y - 1) * nSizeX + x] > 0) { // North
			if (getTileType(x, y - 1) == TileTypes::ZERO)
				vec.push(std::make_pair(x, y - 1));
			playfield[(y - 1) * nSizeX + x] = 0;
			decrement();
		}
		if (x + 1 < nSizeX && playfield[y * nSizeX + x + 1] > 0) { // East
			if (getTileType(x + 1, y) == TileTypes::ZERO)
				vec.push(std::make_pair(x + 1, y));
			playfield[y * nSizeX + x + 1] = 0;
			decrement();
		}
		if (y + 1 < nSizeY && playfield[(y + 1) * nSizeX + x] > 0) { // South
			if (getTileType(x, y + 1) == TileTypes::ZERO)
				vec.push(std::make_pair(x, y + 1));
			playfield[(y + 1) * nSizeX + x] = 0;
			decrement();
		}
		if (x > 0 && playfield[y * nSizeX + x - 1] > 0) { // West
			if (getTileType(x - 1, y) == TileTypes::ZERO)
				vec.push(std::make_pair(x - 1, y));
			playfield[y * nSizeX + x - 1] = 0;
			decrement();
		}
	}
}

void Ottsweeper::fillArea(const int& index) {
	fillArea(index % nSizeX, index / nSizeX);
}

void Ottsweeper::getNeighbors(std::vector<int>& vec, const int& x, const int& y) const {
	vec.clear();
	int xlow = std::max(0, x - 1);
	int xhigh = std::min(nSizeX - 1, x + 1);
	int ylow = std::max(0, y - 1);
	int yhigh = std::min(nSizeY - 1, y + 1);
	for (int yp = ylow; yp <= yhigh; yp++) {
		for (int xp = xlow; xp <= xhigh; xp++) {
			vec.push_back(yp * nSizeX + xp);
		}
	}
}

bool Ottsweeper::getNeighbors(std::vector<int>& vec, const TileTypes& type, const int& x, const int& y) const {
	vec.clear();
	int xlow = std::max(0, x - 1);
	int xhigh = std::min(nSizeX - 1, x + 1);
	int ylow = std::max(0, y - 1);
	int yhigh = std::min(nSizeY - 1, y + 1);
	for (int yp = ylow; yp <= yhigh; yp++) {
		for (int xp = xlow; xp <= xhigh; xp++) {
			if (getTileType(xp, yp) == type)
				vec.push_back(yp * nSizeX + xp);
		}
	}
	return !vec.empty();
}

void Ottsweeper::uncoverCell(const int& x, const int& y) {
	uncoverCell(y * nSizeX + x);
}

void Ottsweeper::uncoverCell(const int& cell) {
	if (bFirstCell) {
		placeBombs(cell);
		bFirstCell = false;
	}
	switch (getTileType(cell)) {
	case TileTypes::ZERO: // Blank space (no surrounding mines)
		fillArea(cell);
		decrement();
		break;
	case TileTypes::BOMB: // KABOOM
		setTileType(cell, TileTypes::EXPLOSION);
		endGame(false);
		break;
	default:
		decrement();
		break;
	}
	playfield[cell] = 0;
}

void Ottsweeper::decrement() {
	if (--nRemainingCells == 0) {
		endGame(true);
	}
}

void Ottsweeper::drawNumber(const int& x, const int& y, const int& value) {
	// ones = value % 10
	// tens = value / 10 (or value % 100 for value > 99)
	// hund = value / 100 (or value % 1000 for value > 999)
	if (value < 10) {
		digits[0].drawCorner(x, y);
		digits[0].drawCorner(x + 13, y);
		digits[value % 10].drawCorner(x + 26, y);
	}
	else if (value < 100) {
		digits[0].drawCorner(x, y);
		digits[value / 10].drawCorner(x + 13, y);
		digits[value % 10].drawCorner(x + 26, y);
	}
	else if (value < 1000) {
		digits[value / 100].drawCorner(x, y);
		digits[value % 100].drawCorner(x + 13, y);
		digits[value % 10].drawCorner(x + 26, y);
	}
	else { // Overflow, print 999
		for (int i = 0; i < 3; i++) {
			digits[9].drawCorner(x + i * 13, y);
		}
	}
}

void Ottsweeper::drawTile(const int& x, const int& y, const TileTypes& type) {
	tiles[gridMap[type]].drawCorner(nMinefieldOffsetX + x * 16, nMinefieldOffsetY + y * 16);
}

void Ottsweeper::drawTile(const int& x, const int& y, const unsigned char& type) {
	tiles[type].drawCorner(nMinefieldOffsetX + x * 16, nMinefieldOffsetY + y * 16);
}

void Ottsweeper::generateBackground() {
	const ColorRGB Gray1(192 / 255.f);
	const ColorRGB Gray2(128 / 255.f);
	const ColorRGB Gray3(109 / 255.f);

	// Borders:
	// 255 White
	// 192 Gray1
	// 128 Gray2
	// 109 Gray3
	// Vertical borders: 3 pixels of White, 6 pixels of Gray, 3 pixels of Dark Gray
	// Horizontal borders: 3 pixels of White, 5 pixels of Gray, 3 pixels of Dark Gray
	OTTTexture bg(nNativeWidth, nNativeHeight, "background", Gray1);
	
	// Background layer
	bg.setDrawColor(Gray1);
	bg.drawRectangle(0, 0, nNativeWidth - 1, nNativeHeight - 1, true);

	// Counter backgrounds
	bg.setDrawColor(Colors::BLACK);
	bg.drawRectangle(16, 15, 54, 37, true); // Score
	bg.drawRectangle(nNativeWidth - 55, 15, nNativeWidth - 17, 37, true); // Time

	// White borders
	bg.setDrawColor(Colors::WHITE);
	bg.drawLine(0, 0, 0, nNativeHeight - 2); // Left
	bg.drawLine(1, 0, 1, nNativeHeight - 3); // Left
	bg.drawLine(2, 0, 2, nNativeHeight - 4); // Left
	bg.drawLine(3, 0, nNativeWidth - 2, 0); // Top
	bg.drawLine(3, 1, nNativeWidth - 3, 1); // Top
	bg.drawLine(3, 2, nNativeWidth - 4, 2); // Top
	bg.drawLine(nNativeWidth - 10, 9, nNativeWidth - 10, 45); // Right of header
	bg.drawLine(nNativeWidth - 11, 10, nNativeWidth - 11, 45); // Right of header
	bg.drawLine(nNativeWidth - 12, 11, nNativeWidth - 12, 45); // Right of header
	bg.drawLine(11, 43, nNativeWidth - 13, 43); // Bottom of header
	bg.drawLine(10, 44, nNativeWidth - 13, 44); // Bottom of header
	bg.drawLine(9, 45, nNativeWidth - 13, 45); // Bottom of header
	bg.drawLine(nNativeWidth - 10, 52, nNativeWidth - 10, nNativeHeight - 9); // Right of minefield
	bg.drawLine(nNativeWidth - 11, 53, nNativeWidth - 11, nNativeHeight - 9); // Right of minefield
	bg.drawLine(nNativeWidth - 12, 54, nNativeWidth - 12, nNativeHeight - 9); // Right of minefield
	bg.drawLine(11, nNativeHeight - 11, nNativeWidth - 13, nNativeHeight - 11); // Bottom of minefield
	bg.drawLine(10, nNativeHeight - 10, nNativeWidth - 13, nNativeHeight - 10); // Bottom of minefield
	bg.drawLine(9, nNativeHeight - 9, nNativeWidth - 13, nNativeHeight - 9); // Bottom of minefield

	// Gray borders
	bg.setDrawColor(Gray2);
	bg.drawLine(nNativeWidth - 1, 1, nNativeWidth - 1, nNativeHeight - 1); // Right
	bg.drawLine(nNativeWidth - 2, 2, nNativeWidth - 2, nNativeHeight - 1); // Right
	bg.drawLine(nNativeWidth - 3, 3, nNativeWidth - 3, nNativeHeight - 1); // Right
	bg.drawLine(1, nNativeHeight - 1, nNativeWidth - 4, nNativeHeight - 1); // Bottom
	bg.drawLine(2, nNativeHeight - 2, nNativeWidth - 4, nNativeHeight - 2); // Bottom
	bg.drawLine(3, nNativeHeight - 3, nNativeWidth - 4, nNativeHeight - 3); // Bottom
	bg.drawLine(9, 8, 9, 44); // Left of header
	bg.drawLine(10, 8, 10, 43); // Left of header
	bg.drawLine(11, 8, 11, 42); // Left of header
	bg.drawLine(12, 8, nNativeWidth - 10, 8); // Top of header
	bg.drawLine(12, 9, nNativeWidth - 11, 9); // Top of header
	bg.drawLine(12, 10, nNativeWidth - 12, 10); // Top of header
	bg.drawLine(9, 51, 9, nNativeHeight - 10); // Left of minefield
	bg.drawLine(10, 51, 10, nNativeHeight - 11); // Left of minefield
	bg.drawLine(11, 51, 11, nNativeHeight - 12); // Left of minefield
	bg.drawLine(12, 51, nNativeWidth - 10, 51); // Top of minefield
	bg.drawLine(12, 52, nNativeWidth - 11, 52); // Top of minefield
	bg.drawLine(12, 53, nNativeWidth - 12, 53); // Top of minefield

	nBackgroundContext = OTTTexture::generateTextureRGBA(nNativeWidth, nNativeHeight, bg.get(), false); // Generate RGBA OpenGL texture
}

int main(int argc, char* argv[]) {
	// Declare a new 2d application
	Ottsweeper app;

	// Initialize application and open output window
	app.start(argc, argv);

	// Run the main loop
	return app.execute();
}