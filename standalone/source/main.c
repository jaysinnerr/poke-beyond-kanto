#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_types.h>
#include <string.h>

#define SCREEN_W 240
#define SCREEN_H 160
#define TILE 8
#define VIEW_W (SCREEN_W / TILE)
#define VIEW_H (SCREEN_H / TILE)
#define WORLD_W 64
#define WORLD_H 48

#define SRAM ((volatile u8*)0x0E000000)
#define SAVE_MAGIC 0x4B42
#define SAVE_VERSION 1

#define COL_BLACK   RGB5(0,0,0)
#define COL_WHITE   RGB5(31,31,31)
#define COL_GRASS   RGB5(7,22,7)
#define COL_GRASS2  RGB5(5,18,6)
#define COL_PATH    RGB5(24,20,12)
#define COL_PATH2   RGB5(20,16,9)
#define COL_WATER   RGB5(5,13,27)
#define COL_WATER2  RGB5(7,17,31)
#define COL_TREE    RGB5(3,14,5)
#define COL_TREE2   RGB5(6,20,8)
#define COL_ROOF    RGB5(25,7,6)
#define COL_ROOF2   RGB5(18,4,4)
#define COL_WALL    RGB5(25,22,16)
#define COL_WALL2   RGB5(20,17,11)
#define COL_SIGN    RGB5(17,11,5)
#define COL_PLAYER  RGB5(5,10,24)
#define COL_SKIN    RGB5(31,23,17)
#define COL_FOLLOW  RGB5(30,24,4)
#define COL_RED     RGB5(28,4,4)
#define COL_GREEN   RGB5(5,26,8)
#define COL_BLUE    RGB5(5,11,27)
#define COL_UI      RGB5(28,28,25)
#define COL_SHADOW  RGB5(7,7,7)

typedef enum
{
    STATE_TITLE,
    STATE_WORLD,
    STATE_MENU,
    STATE_MESSAGE,
    STATE_BATTLE
} GameState;

typedef enum
{
    TILE_GRASS,
    TILE_PATH,
    TILE_WATER,
    TILE_TREE,
    TILE_HOUSE,
    TILE_SIGN,
    TILE_FLOWER
} TileType;

typedef struct
{
    u16 magic;
    u16 version;
    u16 playerX;
    u16 playerY;
    u16 leaguePoints;
    u16 wins;
    u16 losses;
    u16 streak;
    u16 bestStreak;
    u16 reputation;
    u32 rng;
    u16 checksum;
} SaveData;

typedef struct
{
    const char* name;
    int maxHp;
    int attack;
    u16 body;
    u16 accent;
} Creature;

static volatile u16* const fb = (volatile u16*)VRAM;

static GameState state = STATE_TITLE;
static GameState returnState = STATE_WORLD;
static SaveData saveData;
static int playerX = 11;
static int playerY = 31;
static int followerX = 10;
static int followerY = 31;
static int prevPlayerX = 11;
static int prevPlayerY = 31;
static int facing = 1;
static int frameCounter = 0;
static int stepCounter = 0;
static const char* messageLine1 = "";
static const char* messageLine2 = "";
static int playerHp = 30;
static int enemyHp = 20;
static int battleEnemy = 0;
static int battleTurnDelay = 0;
static int battleMessageFrames = 0;
static const char* battleMessage = "";

static const Creature creatures[] =
{
    {"SPROUTLE", 22, 5, RGB5(7,24,9), RGB5(18,30,9)},
    {"EMBERLING", 20, 6, RGB5(29,10,4), RGB5(31,22,6)},
    {"AQUAPUP", 24, 4, RGB5(5,15,28), RGB5(10,25,31)}
};

static const u8 font5x7[39][5] =
{
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x00,0x36,0x36,0x00,0x00}, {0x00,0x60,0x60,0x00,0x00},
    {0x08,0x08,0x08,0x08,0x08}
};

static inline void pset(int x, int y, u16 c)
{
    if((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        fb[y * SCREEN_W + x] = c;
}

static void fillRect(int x, int y, int w, int h, u16 c)
{
    int yy, xx;
    for(yy = 0; yy < h; ++yy)
        for(xx = 0; xx < w; ++xx)
            pset(x + xx, y + yy, c);
}

static void clearScreen(u16 c)
{
    fillRect(0, 0, SCREEN_W, SCREEN_H, c);
}

static int glyphIndex(char ch)
{
    if(ch >= 'A' && ch <= 'Z') return ch - 'A';
    if(ch >= '0' && ch <= '9') return 26 + (ch - '0');
    if(ch == ':') return 36;
    if(ch == '.') return 37;
    if(ch == '-') return 38;
    return -1;
}

static void drawChar(int x, int y, char ch, u16 c, int scale)
{
    int gi = glyphIndex(ch);
    int col, row;
    if(gi < 0) return;

    for(col = 0; col < 5; ++col)
    {
        u8 bits = font5x7[gi][col];
        for(row = 0; row < 7; ++row)
        {
            if(bits & (1 << row))
                fillRect(x + col * scale, y + row * scale, scale, scale, c);
        }
    }
}

static void drawText(int x, int y, const char* s, u16 c, int scale)
{
    int cx = x;
    while(*s)
    {
        if(*s == ' ')
            cx += 4 * scale;
        else
        {
            drawChar(cx, y, *s, c, scale);
            cx += 6 * scale;
        }
        ++s;
    }
}

static void drawNumber(int x, int y, unsigned value, u16 c, int scale)
{
    char buf[12];
    int n = 0, i;
    if(value == 0)
        buf[n++] = '0';
    else
    {
        char rev[12];
        int r = 0;
        while(value && r < 11)
        {
            rev[r++] = '0' + (value % 10);
            value /= 10;
        }
        while(r)
            buf[n++] = rev[--r];
    }
    buf[n] = 0;
    for(i = 0; i < n; ++i)
        drawChar(x + i * 6 * scale, y, buf[i], c, scale);
}

static u32 nextRand(void)
{
    saveData.rng = saveData.rng * 1664525u + 1013904223u;
    return saveData.rng;
}

static u16 calcChecksum(const SaveData* d)
{
    const u8* p = (const u8*)d;
    u16 sum = 0;
    unsigned i;
    for(i = 0; i < sizeof(SaveData) - sizeof(u16); ++i)
        sum = (u16)(sum + p[i] * 17u + i);
    return sum;
}

static void saveGame(void)
{
    unsigned i;
    saveData.magic = SAVE_MAGIC;
    saveData.version = SAVE_VERSION;
    saveData.playerX = (u16)playerX;
    saveData.playerY = (u16)playerY;
    saveData.checksum = calcChecksum(&saveData);

    const u8* src = (const u8*)&saveData;
    for(i = 0; i < sizeof(SaveData); ++i)
        SRAM[i] = src[i];
}

static void newSave(void)
{
    memset(&saveData, 0, sizeof(saveData));
    saveData.magic = SAVE_MAGIC;
    saveData.version = SAVE_VERSION;
    saveData.playerX = 11;
    saveData.playerY = 31;
    saveData.leaguePoints = 1000;
    saveData.rng = 0xBEEFC0DEu;
    playerX = saveData.playerX;
    playerY = saveData.playerY;
    followerX = playerX - 1;
    followerY = playerY;
}

static void loadGame(void)
{
    unsigned i;
    u8* dst = (u8*)&saveData;
    for(i = 0; i < sizeof(SaveData); ++i)
        dst[i] = SRAM[i];

    u16 stored = saveData.checksum;
    if(saveData.magic != SAVE_MAGIC || saveData.version != SAVE_VERSION || stored != calcChecksum(&saveData))
    {
        newSave();
        saveGame();
    }
    else
    {
        playerX = saveData.playerX;
        playerY = saveData.playerY;
        followerX = playerX - 1;
        followerY = playerY;
    }
}

static TileType worldTile(int x, int y)
{
    if(x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H)
        return TILE_TREE;

    if(x < 2 || y < 2 || x >= WORLD_W - 2 || y >= WORLD_H - 2)
        return TILE_TREE;

    if(x >= 34 && x <= 44 && y >= 7 && y <= 20)
    {
        if(x == 34 || x == 44 || y == 7 || y == 20)
            return TILE_WATER;
        if((x + y) % 9 == 0)
            return TILE_FLOWER;
    }

    if((x >= 28 && x <= 31) || (y >= 28 && y <= 31))
        return TILE_PATH;

    if(x >= 8 && x <= 15 && y >= 23 && y <= 27)
        return TILE_HOUSE;
    if(x >= 18 && x <= 25 && y >= 23 && y <= 27)
        return TILE_HOUSE;

    if((x == 16 && y == 29) || (x == 26 && y == 29) || (x == 33 && y == 27))
        return TILE_SIGN;

    if((x * 13 + y * 7) % 23 == 0)
        return TILE_TREE;
    if((x * 5 + y * 11) % 31 == 0)
        return TILE_FLOWER;

    return TILE_GRASS;
}

static int walkable(TileType t)
{
    return t == TILE_GRASS || t == TILE_PATH || t == TILE_SIGN || t == TILE_FLOWER;
}

static void drawTile(int sx, int sy, TileType t, int wx, int wy)
{
    int px = sx * TILE;
    int py = sy * TILE;
    int i;

    switch(t)
    {
    case TILE_GRASS:
        fillRect(px, py, 8, 8, COL_GRASS);
        pset(px + ((wx + wy) & 7), py + 2, COL_GRASS2);
        pset(px + 6, py + ((wx * 3 + wy) & 7), COL_GRASS2);
        break;
    case TILE_PATH:
        fillRect(px, py, 8, 8, COL_PATH);
        pset(px + ((wx + 3) & 7), py + ((wy + 5) & 7), COL_PATH2);
        pset(px + 6, py + 1, COL_PATH2);
        break;
    case TILE_WATER:
        fillRect(px, py, 8, 8, COL_WATER);
        for(i = 0; i < 8; i += 3)
            pset(px + i, py + ((frameCounter / 16 + i + wy) & 7), COL_WATER2);
        break;
    case TILE_TREE:
        fillRect(px, py, 8, 8, COL_GRASS2);
        fillRect(px + 1, py, 6, 6, COL_TREE2);
        fillRect(px + 2, py + 2, 4, 5, COL_TREE);
        pset(px + 3, py + 7, COL_SIGN);
        pset(px + 4, py + 7, COL_SIGN);
        break;
    case TILE_HOUSE:
        if(wy <= 24)
        {
            fillRect(px, py, 8, 8, COL_ROOF);
            fillRect(px, py + 5, 8, 3, COL_ROOF2);
        }
        else
        {
            fillRect(px, py, 8, 8, COL_WALL);
            fillRect(px + 1, py + 1, 2, 3, COL_BLUE);
            fillRect(px + 5, py + 1, 2, 3, COL_BLUE);
            fillRect(px + 3, py + 4, 2, 4, COL_WALL2);
        }
        break;
    case TILE_SIGN:
        fillRect(px, py, 8, 8, COL_PATH);
        fillRect(px + 1, py + 1, 6, 4, COL_SIGN);
        fillRect(px + 3, py + 5, 2, 3, COL_SIGN);
        break;
    case TILE_FLOWER:
        fillRect(px, py, 8, 8, COL_GRASS);
        pset(px + 2, py + 3, COL_WHITE);
        pset(px + 3, py + 2, COL_RED);
        pset(px + 4, py + 3, COL_WHITE);
        pset(px + 3, py + 4, COL_RED);
        break;
    }
}

static void drawPlayerSprite(int x, int y)
{
    fillRect(x + 2, y + 1, 4, 3, COL_RED);
    fillRect(x + 1, y + 3, 6, 3, COL_SKIN);
    fillRect(x + 2, y + 6, 4, 4, COL_PLAYER);
    fillRect(x + 1, y + 10, 2, 2, COL_SHADOW);
    fillRect(x + 5, y + 10, 2, 2, COL_SHADOW);
    pset(x + 2, y + 4, COL_BLACK);
    pset(x + 5, y + 4, COL_BLACK);
}

static void drawFollowerSprite(int x, int y)
{
    fillRect(x + 2, y + 2, 5, 5, COL_FOLLOW);
    pset(x + 1, y + 1, COL_FOLLOW);
    pset(x + 7, y + 1, COL_FOLLOW);
    pset(x + 3, y + 3, COL_BLACK);
    pset(x + 6, y + 3, COL_BLACK);
    fillRect(x + 3, y + 7, 3, 1, COL_SHADOW);
}

static void drawWorld(void)
{
    int camX = playerX - VIEW_W / 2;
    int camY = playerY - VIEW_H / 2;
    int sx, sy;

    if(camX < 0) camX = 0;
    if(camY < 0) camY = 0;
    if(camX > WORLD_W - VIEW_W) camX = WORLD_W - VIEW_W;
    if(camY > WORLD_H - VIEW_H) camY = WORLD_H - VIEW_H;

    for(sy = 0; sy < VIEW_H; ++sy)
        for(sx = 0; sx < VIEW_W; ++sx)
            drawTile(sx, sy, worldTile(camX + sx, camY + sy), camX + sx, camY + sy);

    int fx = (followerX - camX) * TILE;
    int fy = (followerY - camY) * TILE - 4;
    int px = (playerX - camX) * TILE;
    int py = (playerY - camY) * TILE - 4;

    drawFollowerSprite(fx, fy + 4);
    drawPlayerSprite(px, py);

    fillRect(0, 0, 240, 10, RGB5(2,4,2));
    drawText(4, 2, "BEYOND KANTO", COL_WHITE, 1);
    drawText(110, 2, "RP", COL_WHITE, 1);
    drawNumber(126, 2, saveData.leaguePoints, COL_WHITE, 1);
}

static void drawPanel(int x, int y, int w, int h)
{
    fillRect(x, y, w, h, COL_BLACK);
    fillRect(x + 2, y + 2, w - 4, h - 4, COL_UI);
    fillRect(x + 4, y + 4, w - 8, h - 8, RGB5(4,7,5));
}

static void drawTitle(void)
{
    clearScreen(RGB5(3,8,5));
    fillRect(0, 112, SCREEN_W, 48, COL_GRASS2);
    fillRect(0, 126, SCREEN_W, 34, COL_GRASS);
    fillRect(0, 143, SCREEN_W, 17, COL_PATH);

    drawText(28, 30, "BEYOND", COL_WHITE, 3);
    drawText(49, 56, "KANTO", COL_FOLLOW, 3);
    drawText(54, 94, "A GBA ADVENTURE", RGB5(22,29,24), 1);
    if((frameCounter / 30) & 1)
        drawText(74, 126, "PRESS START", COL_WHITE, 1);
}

static void drawMenu(void)
{
    drawWorld();
    drawPanel(28, 18, 184, 126);
    drawText(48, 28, "TRAINER STATUS", COL_FOLLOW, 1);

    drawText(44, 47, "LEAGUE RP", COL_WHITE, 1);
    drawNumber(148, 47, saveData.leaguePoints, COL_WHITE, 1);

    drawText(44, 61, "GLOBAL RANK", COL_WHITE, 1);
    unsigned rank = 10000;
    if(saveData.leaguePoints > 1000)
    {
        unsigned progress = ((unsigned)(saveData.leaguePoints - 1000) * 9999u) / 8999u;
        rank = 10000u - progress;
        if(rank < 1) rank = 1;
    }
    drawNumber(154, 61, rank, COL_WHITE, 1);

    drawText(44, 75, "WINS", COL_WHITE, 1);
    drawNumber(148, 75, saveData.wins, COL_WHITE, 1);
    drawText(44, 89, "LOSSES", COL_WHITE, 1);
    drawNumber(148, 89, saveData.losses, COL_WHITE, 1);
    drawText(44, 103, "STREAK", COL_WHITE, 1);
    drawNumber(148, 103, saveData.streak, COL_WHITE, 1);
    drawText(44, 117, "REPUTATION", COL_WHITE, 1);
    drawNumber(148, 117, saveData.reputation, COL_WHITE, 1);
    drawText(43, 132, "A SAVE   B CLOSE", RGB5(20,25,20), 1);
}

static void showMessage(const char* l1, const char* l2, GameState ret)
{
    messageLine1 = l1;
    messageLine2 = l2;
    returnState = ret;
    state = STATE_MESSAGE;
}

static void drawMessage(void)
{
    drawWorld();
    drawPanel(8, 106, 224, 48);
    drawText(18, 118, messageLine1, COL_WHITE, 1);
    drawText(18, 133, messageLine2, COL_WHITE, 1);
}

static void drawCreature(int cx, int cy, const Creature* c, int flip)
{
    int dir = flip ? -1 : 1;
    (void)dir;
    fillRect(cx - 8, cy - 8, 17, 15, c->body);
    fillRect(cx - 5, cy - 13, 11, 8, c->body);
    fillRect(cx - 8, cy - 15, 4, 6, c->accent);
    fillRect(cx + 5, cy - 15, 4, 6, c->accent);
    pset(cx - 3, cy - 10, COL_BLACK);
    pset(cx + 3, cy - 10, COL_BLACK);
    fillRect(cx - 5, cy + 7, 4, 3, c->accent);
    fillRect(cx + 2, cy + 7, 4, 3, c->accent);
}

static void drawHpBar(int x, int y, int hp, int maxHp)
{
    int width = 68;
    int fill = hp * width / maxHp;
    if(fill < 0) fill = 0;
    fillRect(x, y, width + 4, 8, COL_BLACK);
    fillRect(x + 2, y + 2, width, 4, RGB5(8,8,8));
    fillRect(x + 2, y + 2, fill, 4, hp > maxHp / 3 ? COL_GREEN : COL_RED);
}

static void drawBattle(void)
{
    const Creature* e = &creatures[battleEnemy];
    clearScreen(RGB5(18,27,28));

    fillRect(0, 83, 240, 77, RGB5(21,27,18));
    fillRect(0, 107, 240, 53, RGB5(14,22,11));

    drawText(12, 10, e->name, COL_BLACK, 1);
    drawHpBar(12, 21, enemyHp, e->maxHp);
    drawCreature(181, 58, e, 0);

    drawText(127, 93, "VOLTIKIN", COL_WHITE, 1);
    drawHpBar(127, 104, playerHp, 30);
    drawFollowerSprite(55, 87);

    drawPanel(8, 119, 224, 35);
    if(battleMessageFrames > 0)
        drawText(18, 132, battleMessage, COL_WHITE, 1);
    else
    {
        drawText(18, 128, "A ATTACK", COL_WHITE, 1);
        drawText(137, 128, "B RUN", COL_WHITE, 1);
        drawText(18, 141, "WIN TO GAIN LEAGUE RP", RGB5(20,25,20), 1);
    }
}

static void startBattle(void)
{
    battleEnemy = nextRand() % 3;
    const Creature* e = &creatures[battleEnemy];
    enemyHp = e->maxHp;
    playerHp = 30;
    battleTurnDelay = 0;
    battleMessageFrames = 0;
    state = STATE_BATTLE;
}

static void winBattle(void)
{
    saveData.wins++;
    if(saveData.streak < 9999) saveData.streak++;
    if(saveData.streak > saveData.bestStreak) saveData.bestStreak = saveData.streak;
    if(saveData.leaguePoints <= 9974) saveData.leaguePoints += 25;
    else saveData.leaguePoints = 9999;
    if(saveData.reputation <= 9996) saveData.reputation += 3;
    else saveData.reputation = 9999;
    saveGame();
    showMessage("BATTLE WON. RP PLUS 25", "YOUR RANK IS CLIMBING.", STATE_WORLD);
}

static void loseBattle(void)
{
    saveData.losses++;
    saveData.streak = 0;
    if(saveData.leaguePoints > 1010) saveData.leaguePoints -= 10;
    else saveData.leaguePoints = 1000;
    saveGame();
    showMessage("YOU LOST THE BATTLE.", "TRAIN AND TRY AGAIN.", STATE_WORLD);
}

static void updateBattle(u16 down)
{
    const Creature* e = &creatures[battleEnemy];

    if(battleMessageFrames > 0)
    {
        battleMessageFrames--;
        if(battleMessageFrames == 0 && battleTurnDelay == 2)
        {
            if(enemyHp <= 0)
            {
                winBattle();
                return;
            }

            int dmg = e->attack + (nextRand() % 4);
            playerHp -= dmg;
            battleMessage = "THE WILD CREATURE ATTACKS.";
            battleMessageFrames = 40;
            battleTurnDelay = 3;
        }
        else if(battleMessageFrames == 0 && battleTurnDelay == 3)
        {
            if(playerHp <= 0)
            {
                loseBattle();
                return;
            }
            battleTurnDelay = 0;
        }
        return;
    }

    if(down & KEY_A)
    {
        int dmg = 6 + (nextRand() % 7);
        enemyHp -= dmg;
        battleMessage = "VOLTIKIN STRIKES.";
        battleMessageFrames = 35;
        battleTurnDelay = 2;
    }
    else if(down & KEY_B)
    {
        if((nextRand() & 3) != 0)
        {
            showMessage("YOU ESCAPED SAFELY.", "THE JOURNEY CONTINUES.", STATE_WORLD);
        }
        else
        {
            battleMessage = "CANNOT ESCAPE.";
            battleMessageFrames = 30;
            battleTurnDelay = 3;
        }
    }
}

static void interact(void)
{
    int tx = playerX;
    int ty = playerY;
    if(facing == 0) ty--;
    else if(facing == 1) ty++;
    else if(facing == 2) tx--;
    else tx++;

    TileType t = worldTile(tx, ty);
    if(t == TILE_SIGN)
    {
        if(tx == 16)
            showMessage("CEDAR TOWN TRAINER BOARD", "WIN BATTLES. CLIMB RANK.", STATE_WORLD);
        else if(tx == 26)
            showMessage("ROUTE 1 NORTH", "WILDS AHEAD.", STATE_WORLD);
        else
            showMessage("LAKE AURORA", "RARE CREATURES GATHER HERE.", STATE_WORLD);
    }
    else
    {
        showMessage("THE WORLD FEELS ALIVE.", "KEEP EXPLORING.", STATE_WORLD);
    }
}

static void updateWorld(u16 down)
{
    int nx = playerX;
    int ny = playerY;

    if(down & KEY_UP) { ny--; facing = 0; }
    else if(down & KEY_DOWN) { ny++; facing = 1; }
    else if(down & KEY_LEFT) { nx--; facing = 2; }
    else if(down & KEY_RIGHT) { nx++; facing = 3; }
    else if(down & KEY_A) { interact(); return; }
    else if(down & KEY_START) { state = STATE_MENU; return; }

    if(nx != playerX || ny != playerY)
    {
        if(walkable(worldTile(nx, ny)))
        {
            prevPlayerX = playerX;
            prevPlayerY = playerY;
            playerX = nx;
            playerY = ny;
            followerX = prevPlayerX;
            followerY = prevPlayerY;
            stepCounter++;
            saveData.playerX = playerX;
            saveData.playerY = playerY;

            if(worldTile(playerX, playerY) == TILE_GRASS && stepCounter > 3 && (nextRand() % 18) == 0)
            {
                stepCounter = 0;
                startBattle();
            }
        }
    }
}

static void updateMenu(u16 down)
{
    if(down & KEY_A)
    {
        saveGame();
        showMessage("GAME SAVED.", "YOUR JOURNEY IS SECURE.", STATE_MENU);
    }
    else if(down & KEY_B || down & KEY_START)
        state = STATE_WORLD;
}

int main(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);
    REG_DISPCNT = MODE_3 | BG2_ENABLE;

    loadGame();

    while(1)
    {
        VBlankIntrWait();
        scanKeys();
        u16 down = keysDown();
        frameCounter++;

        switch(state)
        {
        case STATE_TITLE:
            if(down & KEY_START)
                state = STATE_WORLD;
            drawTitle();
            break;

        case STATE_WORLD:
            updateWorld(down);
            if(state == STATE_WORLD)
                drawWorld();
            break;

        case STATE_MENU:
            updateMenu(down);
            if(state == STATE_MENU)
                drawMenu();
            break;

        case STATE_MESSAGE:
            if(down & (KEY_A | KEY_B | KEY_START))
                state = returnState;
            if(state == STATE_MESSAGE)
                drawMessage();
            break;

        case STATE_BATTLE:
            updateBattle(down);
            if(state == STATE_BATTLE)
                drawBattle();
            break;
        }
    }

    return 0;
}
