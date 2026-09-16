#include <stdint.h>
#include <string.h>
#include "commonvars.h"
#include "sound.h"

//helper defines
#define PAUSE 235
#define PAUSE2 120

uint8_t music_note, music_tempo, music_loop, music_on, sound_on, prev_music;
uint8_t selecting_music;
uint16_t music_length;
//points straight at the PLATFORM_PROGMEM tables, they are read with PLATFORM_READ_WORD
const uint16_t* musicArray = nullptr;

const uint16_t PLATFORM_PROGMEM music_crashed[] ={
    850,PAUSE*2,
    0,PAUSE,
    850,PAUSE*1,
	0,PAUSE,
	850,PAUSE*1,
	0,PAUSE,
	850,PAUSE*2,
	0,PAUSE,
};

const uint16_t PLATFORM_PROGMEM music_gameover[] ={
    1400, PAUSE2,
    1100, PAUSE2,
    0,    PAUSE2,
    1100, PAUSE2,
    0,    PAUSE2,
    1200, PAUSE2,
    1200, PAUSE2,
    1100, PAUSE2,
    1100, PAUSE2,
    0,    PAUSE2*3,
    1000, PAUSE2,
    1000, PAUSE2,
    1000, PAUSE2,
    1000, PAUSE2,
    1000, PAUSE2,
    0,    PAUSE2
};


void setMusicOn(uint8_t value)
{
    music_on = value;
    if (!music_on)
        Platform_StopTone();
}


void SelectMusic(uint8_t musicFile, uint8_t loop)
{
    if (prev_music != musicFile)
    {
        selecting_music = 1;
		prev_music = musicFile;
		musicArray = nullptr;
		music_length = 0;
		switch (musicFile)
		{
			case musCrash:
				musicArray = music_crashed;
				music_length = sizeof(music_crashed) / sizeof(music_crashed[0]);
                break;
            case musGameOver:
				musicArray = music_gameover;
				music_length = sizeof(music_gameover) / sizeof(music_gameover[0]);
				break;
			default:
				Platform_StopTone();
				break;
		}
		music_note = 0;
		music_tempo = 0;
		music_loop = loop;
        selecting_music = 0;
    }
}

void playNote()
{
    if(musicArray && (music_note + 1 < music_length))
    {
        Platform_PlayTone(PLATFORM_READ_WORD(&musicArray[music_note]), 0);

        //Set the new delay to wait, the tables are in milliseconds
        music_tempo = PLATFORM_READ_WORD(&musicArray[music_note + 1]) * 60/1000/(60/FRAMERATE) ;

        //Skip to the next note
        music_note += 2;

        if (music_note > music_length - 1)
        {
            if(music_loop)
            {
                music_note = 0;
            }
            else
            {
                Platform_StopTone();
            }
        }
    }
}


void musicTimer()
{
    //for nintendo systems as sega one checks it earlier
    if (selecting_music)
    {
        return;
    }

    //Play some music
    if (music_tempo == 0)
    {
        if(music_on)
        {
            playNote();
        }
    }
    //Else wait for the next note to play
    else
    {
        music_tempo--;
    }
}

void initMusic()
{
	prev_music = 0;
	music_note = 0;
	music_length = 0;
	music_tempo = 0;
	music_loop = 0;
	musicArray = nullptr;
	//set to 1 so nothing plays until a music was selected
	selecting_music = 1;
}

void setSoundOn(uint8_t value)
{
    sound_on = value;
}

uint8_t isMusicOn()
{
    return music_on;
}

uint8_t isSoundOn()
{
    return sound_on;
}

void initSound()
{
    sound_on = 0;
}


void playTickSound()
{
    if (sound_on)
    {
        Platform_PlayTone(750, 100);
    }
}

void processSound()
{
    if (selecting_music)
    {
        return;
    }

    musicTimer();
}
