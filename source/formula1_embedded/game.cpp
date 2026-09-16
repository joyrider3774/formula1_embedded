#include <stdlib.h>
#include <string.h>
#include "commonvars.h"
#include "sound.h"
#include "printfuncs.h"
#include "helperfuncs.h"
#include "savestate.h"
#include "game.h"

//the cars are the same size in every skin
#define carWidth 19
#define carHeight 19
//the lives lost are an X each, going up from the bottom left
#define maxLivesShown 3
//the scores are 5 lcd font tiles on the top row
#define scoreDigits 5
#define hiScoreTileX 2
#define scoreTileX 9

//What is on the screen right now. Only what differs from the game's state is drawn again, so
//straight to the display (SCREENBUFFER 0) nothing flickers and little is sent. With a buffer
//the buffer keeps the last frame, so the same holds there. needRedraw draws everything again
static bool shownEnemyStates[3][3];
static bool shownPlayerStates[3];
//-1 is a score that is not shown
static int32_t shownHiScore, shownScore;
static uint8_t shownLivesLost;

//a car when it is on, the background under it when it is off
static void drawCar(int x, int y, bool on, const uint8_t* image)
{
	if (on)
		drawImage(x, y, carWidth, carHeight, image);
	else
		drawBackground(x, y, carWidth, carHeight);
}

//a score on the top row, value -1 shows none
static void drawScore(int32_t tileX, int32_t value)
{
	//the number is right aligned, the tiles in front of it show the background
	drawBackground(tileX * lcdFontSize, 0, scoreDigits * lcdFontSize, lcdFontSize);
	if (value >= 0)
		printNumber(imgLcdFont, tileX, 0, lcdFontSize, value, scoreDigits);
}

//Brings the screen up to the game's state: the enemies, the player, the two scores (-1 shows
//none) and an X per life lost
static void DrawGame(int32_t hiScore, int32_t score, uint8_t livesLost)
{
	int X, Y;
	if (livesLost > maxLivesShown)
		livesLost = maxLivesShown;

	if (needRedraw)
	{
		needRedraw = 0;
		//the bare background shows no car, score or X, what is on is drawn on it below
		drawBackground(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		memset(shownEnemyStates, 0, sizeof(shownEnemyStates));
		memset(shownPlayerStates, 0, sizeof(shownPlayerStates));
		shownHiScore = -1;
		shownScore = -1;
		shownLivesLost = 0;
	}

	for (X = 0; X < 3; X++)
		for (Y = 0; Y < 3; Y++)
			if (EnemyStates[X][Y] != shownEnemyStates[X][Y])
			{
				drawCar(carX(X), enemyY(Y), EnemyStates[X][Y], imgEnemy);
				shownEnemyStates[X][Y] = EnemyStates[X][Y];
			}

	for (X = 0; X < 3; X++)
	{
		if (PlayerStates[X] != shownPlayerStates[X])
		{
			drawCar(carX(X), playerY, PlayerStates[X], imgPlayer);
			shownPlayerStates[X] = PlayerStates[X];
		}
	}

	if (hiScore != shownHiScore)
	{
		drawScore(hiScoreTileX, hiScore);
		shownHiScore = hiScore;
	}

	if (score != shownScore)
	{
		drawScore(scoreTileX, score);
		shownScore = score;
	}

	if (livesLost != shownLivesLost)
	{
		for (X = 0; X < maxLivesShown; X++)
		{
			if (X < livesLost)
				printMessage(imgLcdFont, 0, 15-X, lcdFontSize, "X");
			else
				drawBackground(0, (15-X) * lcdFontSize, lcdFontSize, lcdFontSize);
		}
		shownLivesLost = livesLost;
	}
}

void MoveEnemy(void)
{
	int X, Y;
	for (X = 0; X < 3; X++)
		for (Y = 2; Y >= 1; Y--)
			EnemyStates[X][Y] = EnemyStates[X][Y - 1];
	for (X = 0; X < 3; X++)
		EnemyStates[X][0] = false;
	for (X = 0; X <= 1; X++)
		EnemyStates[rand() % 3][0] = true;
}

void MoveLeft(void)
{
	int X;
	for (X = 0; X < 2; X++)
	{
		if (PlayerStates[X + 1])
		{
			PlayerStates[X] = true;
			PlayerStates[X + 1] = false;
		}
	}
}

void MoveRight(void)
{
	int X;
	for (X = 2; X >= 1; X--)
	{
		if (PlayerStates[X - 1])
		{
			PlayerStates[X] = true;
			PlayerStates[X - 1] = false;
		}
	}
}

bool IsCollided(void)
{
	int X;
	bool Temp;
	Temp = false;
	for (X = 0; X < 3; X++)
		if (PlayerStates[X] && EnemyStates[X][2])
		{
			Temp = true;
			HitPosition = X;
		}
	return Temp;
}

void HitFlash(void)
{
	PlayerStates[HitPosition] = !PlayerStates[HitPosition];
	EnemyStates[HitPosition][2] = !EnemyStates[HitPosition][2];
}

void InitialiseStates(void)
{
	int X, Y;
	for (X = 0; X < 3; X++)
		for (Y = 0; Y < 3; Y++)
			EnemyStates[X][Y] = false;
	for (X = 0; X < 3; X++)
		PlayerStates[X] = false;
	PlayerStates[1] = true;
}

void initGame(void)
{
	srand(Platform_RandomSeed());
	Teller = 25;
	FlashesDelay = 14*FRAMERATE/60;
	Flashes = 0;
	CanMove = true;
	Score = 0;
	Delay = 60 *FRAMERATE/60;
	LivesLost = 0;
	CrashSoundPlayed = false;
	InitialiseStates();
	needRedraw = 1;
}


void game(void)
{

	int X;

    if (gameState == gsInitGame)
    {
        initGame();
        gameState -= gsInitDiff;
    }

    if (((buttons & BUTTON_RIGHT) && !(prevbuttons & BUTTON_RIGHT)) ||
		((buttons & BUTTON_B) && !(prevbuttons & BUTTON_B)))
    {
		if (CanMove)
		{
			MoveRight();
		}
    }

    if ((buttons & BUTTON_LEFT) && !(prevbuttons & BUTTON_LEFT))
    {
		if (CanMove)
		{
			MoveLeft();
		}
    }

	Teller++;
	if (Teller >= Delay)
	{
		if (!IsCollided() && CanMove)
		{
			Teller = 0;
			for (X = 0; X < 3; X++)
				if (EnemyStates[X][2])
				{
					Score += 10;
					if (Score > getHiScore())
						setHiScore(Score);
					if ((Score % 100 == 0) && (Delay > 8*FRAMERATE/60))
						Delay--;
				}
			MoveEnemy();
			playTickSound();
		}
		else
		{
			if (!CrashSoundPlayed)
			{
				SelectMusic(musNone, 0);
				SelectMusic(musCrash, 0);
				CrashSoundPlayed = true;
			}
			CanMove = false;
			FlashesDelay++;
			if (FlashesDelay == 20*FRAMERATE/60)
			{
				Flashes++;
				HitFlash();
				FlashesDelay = 0;
				if (Flashes == 6)
				{
					Flashes = 0;
					CanMove = true;
					Teller = 0;
					CrashSoundPlayed = false;
					EnemyStates[HitPosition][2] = false;
					LivesLost++;
					FlashesDelay = 14*FRAMERATE/60;
					if (LivesLost == 3)
						gameState = gsInitGameOver;
				}
			}
		}
	}
	DrawGame(getHiScore(), Score, LivesLost);
}

void initGameOver(void)
{
	Teller = 0;
	Delay = 15;
	saveSaveState();
}

void gameOver(void)
{
	if (gameState == gsInitGameOver)
	{
		initGameOver();
		gameState -= gsInitDiff;
	}
	Teller++;
	if (Teller == Delay)
	{
		SelectMusic(musGameOver, 0);
	}
	if (buttons)
	{
		gameState = gsInitGame;
	}

	DrawGame(getHiScore(), Score, LivesLost);
}

void FlashIntro(void)
{
	uint8_t X, Y;
	for (X = 0; X < 3; X++)
		for (Y = 0; Y < 3; Y++)
			EnemyStates[X][Y] = !EnemyStates[X][Y];
	for (X = 0; X < 3; X++)
		PlayerStates[X] = !PlayerStates[X];
}


void gameIntroInit(void)
{
	LivesLost = 0;
	FlashesDelay = 0;
	Score = 0;
	needRedraw = 1;
}

void gameIntro(void)
{
	if (gameState == gsInitGameIntro)
	{
		gameIntroInit();
		gameState -= gsInitDiff;
	}

	if (buttons)
	{
		gameState = gsInitGame;
	}

	FlashesDelay++;
	if (FlashesDelay == 25*FRAMERATE/60)
	{
		FlashesDelay = 0;
		FlashIntro();
		if (PlayerStates[0])
		{
			LivesLost = 3;
			Score = 88888;
		}
		else
		{
			LivesLost = 0;
			Score = 0;
		}
	}
	//every segment lights up together, the scores only while they do
	int32_t shown = (Score > 0) ? (int32_t)Score : -1;
	DrawGame(shown, shown, LivesLost);
}
