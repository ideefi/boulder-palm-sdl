/* 
 * Boulder Palm
 * Copyright (C) 2001-2020 by Wojciech Martusewicz <martusewicz@interia.pl>
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#define LEVELS_WIDTH        40
#define LEVELS_HIGH         22

#define TILE_SIZE           30
#define BITMAP_MAX          14

#define SCREEN_SIZE_X       640
#define SCREEN_SIZE_Y       480

#define X_MARGIN            5
#define Y_MARGIN            5

#if (SCREEN_SIZE_X / TILE_SIZE) < LEVELS_WIDTH
    #define BOARD_WIDTH     (SCREEN_SIZE_X / TILE_SIZE)
#else
    #define BOARD_WIDTH     LEVELS_WIDTH
#endif
#if (SCREEN_SIZE_Y / TILE_SIZE) < LEVELS_HIGH - 1 // -1 for bottom margin
    #define BOARD_HIGH      ((SCREEN_SIZE_Y / TILE_SIZE) - 1)
#else
    #define BOARD_HIGH      LEVELS_HIGH
#endif

#define STANDARD_DELAY      1000
#define INTER_TIME          60

enum tile {TUNNEL, WALL, HERO, ROCK, DIAMOND, GROUND, METAL, BOX, DOOR, FLY, 
           CRASH};
enum hero {KILLED, FACE1, FACE2, RIGHT, LEFT};
enum sound {SOUND_NONE, SOUND_MOVE, SOUND_DIAMOND, SOUND_EXPLOSION};
enum direction {NORTH, EAST, SOUTH, WEST};
enum move {REAL, GHOST};
enum box_state {STILL, MOVING};
enum side {FALL_LEFT = -1, FALL_RIGHT = 1};

struct game
{
    int current_level;
    int level_diamonds;   // Total number of diamonds to pick up
    int level_time;       // Total time to pass the board
    int diamonds;         // Diamonds left
    int time;             // Time left
    enum hero hero_state; // Direction of player
    enum move move_mode;  // Move mode (real move or action without move)
    int lastposx, lastposy;
    int move_time;        // Time of last move (impatience feature)
    int sound_mode;
    enum sound sound_to_play;
};

struct board_mem
{
    unsigned char board:4;
    unsigned char rock_move:1;
    unsigned char box_move:1;
    unsigned char box_dir:2;
};

/********************
 * Global variables *
 ********************/
SDL_Event Event;
SDL_Renderer *Renderer;
SDL_Surface *Surface;
SDL_Texture *Tiles[BITMAP_MAX];
TTF_Font *Font;

struct game Game;
unsigned char Mem[LEVELS_HIGH][LEVELS_WIDTH];

const char BitmapFile[BITMAP_MAX][32] = {"res/tunnel.bmp", "res/wall.bmp",
    "res/heror.bmp", "res/herol.bmp", "res/hero1.bmp", "res/hero2.bmp",
    "res/rock.bmp", "res/diamond.bmp", "res/ground.bmp", "res/metal.bmp",
    "res/door.bmp", "res/box.bmp", "res/crash.bmp", "res/fly.bmp"};


/*********************************************
 * Access (get/set) to game board properties *
 *********************************************/
int GetBoard(int h, int w)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    return b->board;
}

void SetBoard(int h, int w, int v)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    b->board = v;
}

int GetRockMove(int h, int w)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    return b->rock_move;
}

void SetRockMove(int h, int w, int v)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    b->rock_move = v;
}

int GetBoxMove(int h, int w)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    return b->box_move;
}

void SetBoxMove(int h, int w, int v)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    b->box_move = v;
}

int GetBoxDir(int h, int w)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    return b->box_dir;
}

void SetBoxDir(int h, int w, int v)
{
    struct board_mem *b = (struct board_mem*)&(Mem[h][w]);
    b->box_dir = v;
}


/*****************
 * Loading level *
 *****************/
int LoadLevel(int level)
{
    FILE *fp = NULL;
    char line[LEVELS_WIDTH + 1];
    int i = 0, j = 0;

    snprintf(line, sizeof(line), "res/%d.lvl", level + 1);
    fp = fopen(line, "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == 0 || line[0] == 10)
            continue;
        if (line[0] == '.')
        {
            if (line[1] == 'd' && line[2] == '=')
                Game.level_diamonds = atoi(line + 3);
            else
            if (line[1] == 't' && line[2] == '=')
                Game.level_time = atoi(line + 3);
            continue;
        }

        for (i = 0; i < LEVELS_WIDTH; i++)
            SetBoard(j, i, line[i] - 48);

        if (++j > LEVELS_HIGH)
            break;
    }

    fclose(fp);
    return 0;
}


/**************************************
 * This function starts the new board *
 **************************************/
void StartLevel(int new_level)
{
    if (LoadLevel(new_level) < 0)
        if (Game.current_level != 0)
        {
            Game.current_level = 0;
            StartLevel(Game.current_level);
        }
    Game.time = Game.level_time;
    Game.move_time = Game.level_time;
    Game.diamonds = Game.level_diamonds;
    Game.hero_state = FACE1;
}


/******************
 * Play the sound *
 ******************/
void SoundPlay(void)
{
    if (Game.sound_mode && Game.sound_to_play != SOUND_NONE)
    {
        // TODO: Play the sound...
        switch (Game.sound_to_play)
        {
            case SOUND_MOVE:
                break;
            case SOUND_DIAMOND:
                break;
            case SOUND_EXPLOSION:
                break;
            case SOUND_NONE:
                break;
        }

        Game.sound_to_play = SOUND_NONE;
    }
}


/****************************
 * Set the sound to be play *
 ****************************/
void SoundRequest(int sound)
{
    Game.sound_to_play = sound;
}


/******************
 * Make the crash *
 ******************/
void MakeCrash(int object, int y, int x)
{
    int j, i;

    for (j = y - 1; j <= y + 1; j++)
        for (i = x - 1; i <= x + 1; i++)
            if (GetBoard(j, i) != METAL) 
                SetBoard(j, i, object);

    SoundRequest(SOUND_EXPLOSION);
}


/********************
 * Remove the crash *
 ********************/
void CrashRemove(void)
{
    int j, i;

    for (j = LEVELS_HIGH - 2; j > 0; j--)
        for (i = 0; i <= LEVELS_WIDTH - 1; i++)
            if (GetBoard(j, i) == CRASH)
                SetBoard(j, i, TUNNEL);
}


/***************************************************
 * This function control each other box and fly AI *
 ***************************************************/
int MoveBox(int j, int i, int d)
{
    int dj = j, di = i;

    if (d > WEST)
        d -= (WEST + 1);
    if (d < NORTH)
        d = WEST;

    switch (d)
    {
        case NORTH: dj -= 1; break;
        case EAST:  di += 1; break;
        case SOUTH: dj += 1; break;
        case WEST:  di -= 1; break;
    }

    if (GetBoard(dj, di) == HERO)
    {
        if (GetBoard(j, i) == BOX)
            MakeCrash(CRASH, dj, di);
        else
            MakeCrash(DIAMOND, dj, di);
        return 1;
    }

    if (GetBoard(dj, di) == TUNNEL)
    {
        if (GetBoard(j, i) == BOX)
            SetBoard(dj, di, BOX);
        else
            SetBoard(dj, di, FLY);
        SetBoard(j, i, TUNNEL);
        SetBoxMove(dj, di, MOVING);
        SetBoxDir(dj, di, d);
        return 1;
    }

    return 0;
}


/**********************************************
 * This function control boxs's and flys's AI *
 **********************************************/
void MoveBoxes(void)
{
    int j, i, d;

    for (j = LEVELS_HIGH - 2; j > 0; j--)
        for (i = 1; i < LEVELS_WIDTH - 1; i++)
            SetBoxMove(j, i, STILL);

    for (j = LEVELS_HIGH - 2; j > 0; j--)
        for (i = 1; i < LEVELS_WIDTH - 1; i++)
            if ((GetBoard(j, i) == BOX || GetBoard(j, i) == FLY) 
                && GetBoxMove(j, i) == STILL)
            {
                for (d = GetBoxDir(j, i) - 1; d <= GetBoxDir(j, i) + 2; d++)
                    if (MoveBox(j, i, d))
                        break;
            }
}


/*******************************************
 * Falling rock and diamonds on given side *
 *******************************************/
void FallingOnSide(int j, int i, int side)
{
    if (GetBoard(j, i + side) == TUNNEL 
        && GetBoard(j + 1, i + side) == TUNNEL)
    {
        SetBoard(j, i + side, GetBoard(j, i));
        SetBoard(j, i, TUNNEL);
        SetRockMove(j, i + side, MOVING);
    }
}


/***************************************************
 * This function control rock and diamonds falling *
 ***************************************************/
void MoveRocks(void)
{
    int j, i;

    for (j = LEVELS_HIGH - 2; j > 0; j--)
        for (i = (j % 2) ? LEVELS_WIDTH - 2 : 1;
             (j % 2) ? i > 0 : i < LEVELS_WIDTH - 1;
             (j % 2) ? i-- : i++)
        {
            if (GetBoard(j, i) == ROCK || GetBoard(j, i) == DIAMOND)
            {
                // Falling rock or diamond on right or left
                if (GetBoard(j + 1, i) == ROCK
                    || GetBoard(j + 1, i) == DIAMOND
                    || GetBoard(j + 1, i) == WALL
                    || GetBoard(j + 1, i) == DOOR
                    || GetBoard(j + 1, i) == METAL)
                {
                    if (rand() & 1)
                        FallingOnSide(j, i, FALL_RIGHT);                        
                    else
                        FallingOnSide(j, i, FALL_LEFT);
                }

                // Falling down
                if (GetBoard(j + 1, i) == TUNNEL)
                {
                    SetBoard(j + 1, i, GetBoard(j, i));         
                    SetBoard(j, i, TUNNEL);
                    SetRockMove(j + 1, i, MOVING);
                }

                // Rock or diamond kills the player
                if (GetBoard(j + 1, i) == HERO && GetRockMove(j, i) == MOVING)
                    MakeCrash(CRASH, j + 1, i);
                
                // Rock or diamond kills the BOX
                if (GetBoard(j + 1, i) == BOX)
                    MakeCrash(CRASH, j + 1, i);
                if (GetBoard(j + 1, i) == FLY)
                    MakeCrash(DIAMOND, j + 1, i);
                
                SetRockMove(j, i, STILL);
            }
        }
}


/**********************************
 * This function finds the object *
 **********************************/
int FindObject(int object, int *y, int *x)
{
    int j, i;

    for (j = 1; j < LEVELS_HIGH - 1; j++)
        for (i = 1; i < LEVELS_WIDTH - 1; i++)
            if (GetBoard(j, i) == object)
            {
                if (y != 0)
                    *y = j;
                if (x != 0)
                    *x = i;
                return object; // Object found
            }
    return (-1); // Object not found
}


/**********************************
 * This function moves the player *
 **********************************/
void MoveHero(int y, int x)
{
    int j, i, o;

    if (FindObject(HERO, &j, &i) != HERO)
        return;

    o = GetBoard(j + y, i + x);

    switch (o)
    {
        case DIAMOND: // Get the diamond
            if (Game.diamonds)
                Game.diamonds--;
            SoundRequest(SOUND_DIAMOND);
            break;      
        case ROCK: // Push the rock
            if (x > 0)
                if (GetBoard(j, i + x + 1) == TUNNEL)
                {
                    SetBoard(j, i + x, TUNNEL);
                    SetBoard(j, i + x + 1, ROCK);
                }
            if (x < 0)
                if (GetBoard(j, i + x - 1) == TUNNEL)
                {
                    SetBoard(j, i + x, TUNNEL);
                    SetBoard(j, i + x - 1, ROCK);
                }
            o = GetBoard(j + y, i + x);
            break;
        case BOX:
            MakeCrash(CRASH, j + y, i + x);
            return;
        case FLY:
            MakeCrash(DIAMOND, j + y, i + x);
            return;
    }

    // Move player if it's possible
    if (o != WALL && o != ROCK && o != METAL
         && j + y >= 0 && i + x >= 0 
         && j + y < LEVELS_HIGH && i + x < LEVELS_WIDTH
         && (o != DOOR || !Game.diamonds))
    {
        if (Game.move_mode == REAL)
        {
            SetBoard(j, i, TUNNEL);
            SetBoard(j + y, i + x, HERO);
        } else
        {
            SetBoard(j + y, i + x, TUNNEL);
        }
        if (Game.sound_to_play == SOUND_NONE)
            SoundRequest(SOUND_MOVE);
    }

    Game.move_mode = REAL;
    Game.move_time = Game.time;
    return;
}


/***************
 * Kill player *
 ***************/
void KillHero(void)
{
    int y, x;

    if (FindObject(HERO, &y, &x) == HERO)
        MakeCrash(CRASH, y, x);
}


/******************
 * Print the text *
 ******************/
void PrintText(char *fstr, int value, int x, int y)
{
    int w, h;
    char msg[64];

    if (!Font)
        return;

    snprintf(msg, 64, fstr, value);
    TTF_SizeText(Font, msg, &w, &h);
    SDL_Surface *msgsurf = TTF_RenderText_Blended(
        Font, msg, (SDL_Color){255, 0, 0});
    SDL_Texture *msgtex = SDL_CreateTextureFromSurface(Renderer, msgsurf);
    SDL_Rect fromrec = {0, 0, msgsurf->w, msgsurf->h};
    SDL_Rect torec = {x, y, msgsurf->w, msgsurf->h};
    SDL_RenderCopy(Renderer, msgtex, &fromrec, &torec);
    SDL_DestroyTexture(msgtex);
    SDL_FreeSurface(msgsurf);
}


/*******************************
 * Select the appropriate tile *
 *******************************/
int SelectTile(int item, int posx, int posy)
{
    int t;

    switch (item)
    {
        case TUNNEL: t = 0; break;
        case WALL:   t = 1; break;
        case HERO:
            switch (Game.hero_state)
            {
                case RIGHT: t = 2; break;
                case LEFT:  t = 3; break;
                case FACE1: t = 4; break;
                case FACE2: t = 5; break;
                default:    t = 4;
            }
            break;
        case ROCK:    t = 6; break;
        case DIAMOND: t = 7; break;
        case GROUND:  t = 8; break;
        case METAL:   t = 9; break;
        case BOX:     t = 11; break;
        case DOOR:    t = 10; break;
        case FLY:     t = 13; break;
        case CRASH:   t = 12; break;
        default:      t = 0;
    }

    return t;
}


/**********************************************************
 * This function draw currently visable part of the board *
 **********************************************************/
void ShowView(void)
{
    int starty, startx, posy, posx, y, x;

    // Find the player
    if (FindObject(HERO, &starty, &startx) < 0)
    {
        startx = Game.lastposx;
        starty = Game.lastposy;
        Game.hero_state = KILLED;
    } else
    {
        Game.lastposx = startx;
        Game.lastposy = starty;
    }

    // Scrolling the board
    startx -= BOARD_WIDTH / 2;
    if (startx < 0)
        startx = 0;
    if (startx > LEVELS_WIDTH - BOARD_WIDTH)
        startx = LEVELS_WIDTH - BOARD_WIDTH;

    starty -= BOARD_HIGH / 2;
    if (starty < 0)
        starty = 0;
    if (starty > LEVELS_HIGH - BOARD_HIGH)
        starty = LEVELS_HIGH - BOARD_HIGH;

    // Draw the board
    posy = starty;
    for (y = 0; y < BOARD_HIGH; y++)
    {
        posx = startx;
        for (x = 0; x < BOARD_WIDTH; x++)
        {
            int t = SelectTile(GetBoard(posy, posx), x, y);
            SDL_RenderCopy(Renderer, Tiles[t], NULL, 
                &(SDL_Rect){x * TILE_SIZE + X_MARGIN, y * TILE_SIZE + Y_MARGIN, 
                TILE_SIZE, TILE_SIZE});
            posx++;
        }
        posy++;
    }

    SDL_RenderPresent(Renderer);
}


/*******************************************************
 * Write time, score, etc and end of the Game checking *
 *******************************************************/
void ShowStatus(void)
{
    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(Renderer, 
        &(SDL_Rect){0, SCREEN_SIZE_Y - TILE_SIZE + Y_MARGIN, 
        SCREEN_SIZE_X, TILE_SIZE - Y_MARGIN});

    if (!Game.time || Game.hero_state == KILLED)
    {
        if (Game.hero_state != KILLED)
            KillHero();
        PrintText(" * Game Over * ", 0, (int)(SCREEN_SIZE_X / 3), 
            (int)(SCREEN_SIZE_Y - TILE_SIZE / 1.5));
        SDL_RenderPresent(Renderer);
    } else
    if (!Game.diamonds && FindObject(DOOR, 0, 0) < 0)
    {
        SDL_Delay(STANDARD_DELAY);
        PrintText(" * Level %d * ", Game.current_level + 2, 
            (int)(SCREEN_SIZE_X / 3), (int)(SCREEN_SIZE_Y - TILE_SIZE / 1.5));
        SDL_RenderPresent(Renderer);
        SDL_Delay(STANDARD_DELAY);
        StartLevel(++Game.current_level);
    } else
    {
        PrintText("    Level  %2u", Game.current_level + 1, 
            TILE_SIZE + X_MARGIN, 
            (int)(SCREEN_SIZE_Y - TILE_SIZE / 1.5));
        PrintText("   Diam  %3u", Game.diamonds, 
            (int)(TILE_SIZE + SCREEN_SIZE_X / 3), 
            (int)(SCREEN_SIZE_Y - TILE_SIZE / 1.5));
        PrintText("Time  %3u", Game.time, 
            (int)(TILE_SIZE + SCREEN_SIZE_X / 1.5), 
            (int)(SCREEN_SIZE_Y - TILE_SIZE / 1.5));
        PrintText("%c", (Game.sound_mode)?' ':'M', SCREEN_SIZE_X - TILE_SIZE, 
            (int)(SCREEN_SIZE_Y - TILE_SIZE / 1.5));
        SDL_RenderPresent(Renderer);
    }
}


/**********************
 * Refreash the Board *
 **********************/
void RefreashBoard(void)
{
    static int t = 0;

    if (!t--)
    {
        CrashRemove();
        MoveRocks();
        MoveBoxes();
        ShowStatus();
        ShowView();
        SoundPlay();
        t = INTER_TIME / 5; // The speed of moving objects
    }
}


/*********************
 * Time decrementing *
 *********************/
void DecrementTime(void)
{
    static int t = INTER_TIME;

    if (Game.time <= 0 || Game.hero_state == KILLED)
        return;

    switch (t--)
    {
        case 0:
            Game.time--;
            t = INTER_TIME;
        case INTER_TIME / 2:
            if (Game.hero_state == FACE1 && Game.move_time - Game.time > 5)
                Game.hero_state = FACE2;
            else
                Game.hero_state = FACE1;
    }
}


/******************
 * Show the intro *
 ******************/
void ShowIntro(void)
{
    SDL_Texture *intro;

    Surface = SDL_LoadBMP("res/intro.bmp");
    intro = SDL_CreateTextureFromSurface(Renderer, Surface);
    SDL_RenderCopy(Renderer, intro, NULL, &(SDL_Rect){0, 0, 
        SCREEN_SIZE_X, SCREEN_SIZE_Y});
    SDL_RenderPresent(Renderer);

    SDL_Delay(STANDARD_DELAY * 2);
    SDL_RenderClear(Renderer);
    SDL_FreeSurface(Surface);
}


/********************
 * Start aplication *
 ********************/
void StartAplication(void)
{
    int i;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("Boulder Palm on PC",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        SCREEN_SIZE_X, SCREEN_SIZE_Y, SDL_WINDOW_SHOWN);
    SDL_Surface* icon = SDL_LoadBMP("res/app_icon.bmp");
    SDL_SetWindowIcon(win, icon);
    SDL_FreeSurface(icon);
    Renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!Renderer)
        exit(fprintf(stderr, "Could not create SDL Renderer\n"));

    TTF_Init();
    Font = TTF_OpenFont("res/font.ttf", TILE_SIZE / 2);

    Game.current_level = 0;
    Game.diamonds      = 0;
    Game.move_mode     = REAL;
    Game.sound_mode    = 1;
    Game.sound_to_play = SOUND_NONE;

    for (i = 0; i < BITMAP_MAX; i++)
    {
        Surface = SDL_LoadBMP(BitmapFile[i]);
        Tiles[i] = SDL_CreateTextureFromSurface(Renderer, Surface);
        SDL_FreeSurface(Surface);
    }

    ShowIntro();
    StartLevel(Game.current_level);
}


/*************************************
* Handle a key press from the player *
 *************************************/
void KeyDown(void)
{
    switch (Event.key.keysym.sym)
    {
        case SDLK_LEFT: case SDLK_a: 
            MoveHero(0, -1);
            Game.hero_state = LEFT;
            break;
        case SDLK_RIGHT: case SDLK_d:
            MoveHero(0, 1);
            Game.hero_state = RIGHT;
            break;
        case SDLK_UP: case SDLK_w: 
            MoveHero(-1, 0);
            break;
        case SDLK_DOWN: case SDLK_s:
            MoveHero(1, 0);
            break;
        case SDLK_SPACE: case SDLK_RETURN:
            if (Game.hero_state == KILLED)
                StartLevel(Game.current_level);
            else
                Game.move_mode = GHOST;
            break;
        case SDLK_m:
            Game.sound_mode ^= 1;
            break;
        case SDLK_n:
            StartLevel(++Game.current_level);
            break;
        case SDLK_p:
            if (Game.current_level > 0)
                StartLevel(--Game.current_level);
            break;
        case SDLK_r:
            KillHero();
            break;
        case SDLK_j: // Respawn cheat
            SetBoard(Game.lastposy, Game.lastposx, HERO);
            Game.hero_state = FACE1;
            break;
        case SDLK_t: // Time cheat
            Game.time = Game.level_time;
            break;
        case SDLK_q:
            exit(0);
    }
}


/******************
 * Main game loop *
 ******************/
int main()
{
    StartAplication();

    for (;;)
    {
        if (SDL_PollEvent(&Event))
        {
            switch (Event.type)
            {
                case SDL_QUIT:
                    exit(0);
                case SDL_KEYDOWN:
                    KeyDown();
                    break;
            }

            ShowView();
            SoundPlay();
        }

        DecrementTime();
        RefreashBoard();

        SDL_Delay(1000 / 60);
    }
}

