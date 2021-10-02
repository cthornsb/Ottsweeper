#ifndef Ottsweeper_HPP
#define Ottsweeper_HPP

#include <vector>
#include <string>
#include <map>

#include "OTTApplication.hpp"
#include "OTTSpriteSet.hpp"
#include "OTTRandom.hpp"
#include "ColorRGB.hpp"

enum class TileTypes {
	NONE,
	ZERO,
	ONE,
	TWO,
	THREE,
	FOUR,
	FIVE,
	SIX,
	SEVEN,
	EIGHT,
	NORMAL,
	FLAGGED,
	UNKNOWN,
	BOMB,
	EXPLOSION,
	MISTAKE
};

enum class GameStates {
	NORMAL,
	PAUSED,
	WIN,
	LOSS
};

class Ottsweeper : public OTTApplication {
public:
	Ottsweeper() :
		OTTApplication(160, 186),
		rng(OTTRandom::Generator::XORSHIFT),
		bFirstCell(true),
		bLeftClickHeld(false),
		nSizeX(10),
		nSizeY(10),
		nBombs(10),
		nRemainingCells(nSizeX* nSizeY - nBombs),
		nMinefieldOffsetX(12),
		nMinefieldOffsetY(54),
		nCurrentCellX(0),
		nCurrentCellY(0),
		nCurrentCell(0),
		dFinalGameTime(0),
		dWindowScaleX(1),
		dWindowScaleY(1),
		nBackgroundContext(0),
		digits(),
		tiles(),
		smilies(),
		gameState(GameStates::NORMAL),
		minefield(),
		playfield(),
		gridMap()
	{
	}

	~Ottsweeper() override {
		// Nothing to clean up, window will be closed by OTTWindow class
	}

	void setCurrentWindowScale(const double& x, const double& y) {
		dWindowScaleX = x;
		dWindowScaleY = y;
	}

protected:
	void rollDice();

	void computeScores();

	void computeProbabilities();

	void printScores() const ;

	void drawDie(const unsigned short& px, const unsigned short& py, const int& value);
	
	void drawString(const std::string& str, const int& value, const ColorRGB& color=Colors::WHITE);

	bool onUserCreateWindow() override;

	bool onUserLoop() override;

private:
	OTTRandom rng;

	bool bFirstCell;

	bool bLeftClickHeld;

	int nSizeX;

	int nSizeY;

	int nBombs;

	int nRemainingCells;

	int nMinefieldOffsetX;

	int nMinefieldOffsetY;
	
	int nCurrentCellX;

	int nCurrentCellY;

	int nCurrentCell;

	double dFinalGameTime;

	double dWindowScaleX;

	double dWindowScaleY;

	unsigned int nBackgroundContext;

	OTTSpriteSet digits;

	OTTSpriteSet tiles;

	OTTSpriteSet smilies;

	GameStates gameState;

	std::vector<unsigned char> minefield;

	std::vector<unsigned char> playfield;

	std::map<TileTypes, unsigned char> gridMap;

	void setTileType(const int& x, const int& y, const TileTypes& type);

	void setTileType(const int& index, const TileTypes& type);

	TileTypes getTileType(const int& x, const int& y) const ;

	TileTypes getTileType(const int& index) const ;

	void placeBombs(const int& safeCell);

	void resetField();

	void endGame(bool bWin);

	void fillArea(const int& startX, const int& startY);

	void fillArea(const int& index);

	void getNeighbors(std::vector<int>& vec, const int& x, const int& y) const ;

	bool getNeighbors(std::vector<int>& vec, const TileTypes& type, const int& x, const int& y) const ;

	void uncoverCell(const int& x, const int& y);

	void uncoverCell(const int& cell);

	void decrement();

	void drawNumber(const int& x, const int& y, const int& value);	

	void drawTile(const int& x, const int& y, const TileTypes& type);

	void drawTile(const int& x, const int& y, const unsigned char& type);

	void generateBackground();
};

#endif // ifndef Ottsweeper_HPP