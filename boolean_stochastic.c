#include "kilolib.h"
#include <stdlib.h>
#include <math.h>

// Default values for core definitions
#define LANG_SIZE 3
#define POPUL_SIZE 200
#define MIN_DISTANCE = 100;

#define BOOLEAN 1.0;

// Individual variables for bots
int initialDelay = 0;
int messageCount = 0;
int nestQualities[LANG_SIZE] = {7, 9, 10};

double belief[LANG_SIZE - 1];
int lastUpdate = -1;

uint8_t messages[POPUL_SIZE][2];
message_t msg;

/*
 * Internal structures for 'kilobees'
 */

struct NestSite
{
    uint8_t site;
    uint8_t siteQuality;
};

struct State
{
    int state;
    uint8_t duration;
};

// Kilobee state/belief variables

struct NestSite nest = {0, 0};
struct State danceState = {0, 0};

void setDanceState(int state, uint8_t duration)
{
    danceState.state = state;
    danceState.duration = duration;
}

void setNestSite(uint8_t site, uint8_t quality)
{
    nest.site = site;
    nest.siteQuality = quality;
}

int getSiteToVisit()
{

}
