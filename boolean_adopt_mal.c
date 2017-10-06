#include "kilolib.h"
#include <stdlib.h>
#include <math.h>

// Default values for core definitions
#define SITE_NUM 2
#define MAX_MSG_SIZE 100
#define MIN_DISTANCE 100

#define BELIEF_BYTES 1
#define BELIEF_PRECISION 1

// Individual variables for bots
int updateTicks = 16; // 32 updates per second
int initialDelay = 0;
int lastUpdate = -1;
int messageCount = 0;
int nestQualities[SITE_NUM] = {7, 9};
int loopCounter = 0;
int isMalicious = 0;

uint8_t beliefs[SITE_NUM];
int beliefStart = 2;

// Frank's T-norm:
// 0.0 = min
// 1.0 = product
// +inf = Luk
//
double baseParam = 1.0;


uint8_t messages[MAX_MSG_SIZE][2 + SITE_NUM];
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
    uint8_t state;
    uint8_t duration;
};

// Kilobee state/belief variables

struct NestSite nest = {0, 0};
struct State danceState = {0, 0};

void setDanceState(uint8_t state, uint8_t duration)
{
    danceState.state = state;
    danceState.duration = duration;
}

void setNestSite(uint8_t site, uint8_t quality)
{
    nest.site = site;
    nest.siteQuality = quality;
}

uint8_t getSiteToVisit(uint8_t *beliefs)
{
    int siteToVisit = -1;

    for (int i = 0; i < SITE_NUM; i++)
    {
        if (beliefs[i] == 1)
        {
            siteToVisit = i;
            break;
        }
    }

    return (uint8_t) siteToVisit;
}

double franksTNorm(double belief1, double belief2, double p)
{
    if (p == 0)
    {
        return fmin(belief1, belief2);
    }
    else if (p == 1)
    {
        return belief1 * belief2;
    }
    else
    {
        return log(1.0 + (pow(exp(p), belief1) - 1.0) * (pow(exp(p), belief2) - 1.0) / (exp(p) - 1.0) ) / p;
    }
}

void consensus(uint8_t *beliefs1, uint8_t *beliefs2)
{
    for (int b = 0; b < SITE_NUM; b++)
    {
        if (beliefs1[b] == 2)
        {
            if (beliefs2[b] == 0)
            {
                beliefs1[b] = 1;
            }
        }
        else if (beliefs1[b] == 0)
        {
            if (beliefs2[b] == 2)
            {
                beliefs1[b] = 1;
            }
        }
        else if (beliefs1[b] == 1)
        {
            if (beliefs2[b] == 2)
            {
                beliefs1[b] = 2;
            }
            else if (beliefs2[b] == 0)
            {
                beliefs1[b] = 0;
            }
        }
    }
}

int generate = 0;
double u1 = 0.0;
double u2 = 0.0;
double z1 = 0.0;
double z2 = 0.0;

double get_noise()
{
    if (generate == 0)
    {
        u1 = (double) (1 + rand_hard()) / 256;
        u2 = (double) (1 + rand_hard()) / 256;

        z1 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        z2 = sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2);

        z1 = (0 * z1);
        z2 = (0 * z2);

        generate++;
        return z2;
    }
    else
    {
        generate--;
        return z1;
    }
}

void set_bot_colour(uint8_t site)
{
    switch (site)
    {
        case 0:
            set_color(RGB(3, 0, 0));
            break;
        case 1:
            set_color(RGB(0, 0, 3));
            break;
        case 2:
            set_color(RGB(0, 3, 0));
            break;
    }
}

message_t *tx_message()
{
    return &msg;
}

void rx_message(message_t *m, distance_measurement_t *d)
{
    //int distance = estimate_distance(d);
    if (1)// distance < min_distance)
    {
        // Dance state
        messages[messageCount][0] = m->data[0];
        // Dance site
        messages[messageCount][1] = m->data[1];
        // Beliefs
        for (int b = 0; b < SITE_NUM; b++)
        {
            messages[messageCount][2 + b] = m->data[beliefStart + b];
        }
        messageCount++;
    }
}

void setup()
{
    if (initialDelay == 0)
    {
        initialDelay = 1;
        delay(30000);   // Delay initial loop for 30 seconds to allow a fairly
                        // equal start time and easy recording.
    }

    // Setup code here; run once at the beginning
    kilo_message_tx = (message_tx_t)&tx_message;
    kilo_message_rx = (message_rx_t)&rx_message;

    // Construct a valid message
    // Format will be:
    // [0] - dance state; 1 (true) if bee is dancing, 0 (false) if not.
    // [1] - dance site; the site that is currently being danced for.
    // [2] - belief[0]; site value "x" (for 0:x, 1: 1 - x).
    // [3] - belief[1]; optional for 3-sites (language size: 3).
    // [4]
    // [5]
    // [6]
    // [7]
    // [8]
    // [9]
    // msg.type = NORMAL;
    // msg.crc = message_crc(&msg);

    /*
     * MALICIOUS AGENT DETERMINATION
     */

    if ((rand_hard() / 255.0) < 0.1)
    {
        isMalicious = 1;
    }

    for (int b = 0; b < SITE_NUM; b++)
    {
        beliefs[b] = 0;
    }

    beliefs[rand_hard() % SITE_NUM] = 1;

    uint8_t siteToVisit = getSiteToVisit(beliefs);
    setNestSite(siteToVisit, nestQualities[siteToVisit]);
    int danceDuration = nestQualities[siteToVisit] + round(get_noise());
    if (danceDuration <= 0)
    {
        danceDuration = 1;
    }
    setDanceState(1, danceDuration);

    double probNotDancing = rand_hard() / 255.0;
    if (probNotDancing <= 0.5)
    {
        setDanceState(0, 0);
    }

    // Generate random integers to fill both ID bytes, leading to a 16-bit
    // number with 65,536 possible values: 255 (8-bit) x 255 (8-bit).
    //msg.data[0] = rand_hard();
    //msg.data[1] = rand_hard();
    // Dance state
    msg.data[0] = danceState.state;
    msg.data[1] = nest.site;
    // Beliefs
    for (int b = 0; b < SITE_NUM; b++)
    {
        msg.data[beliefStart + b] = beliefs[b];
    }

    msg.type = NORMAL;
    msg.crc = message_crc(&msg);

    if (isMalicious == 1)
    {
        set_color(RGB(0, 2, 0));
    }
    else if (danceState.state == 1)
    {
        set_bot_colour(nest.site);
    }
    else
    {
        // Set colour to black; not dancing
        set_color(RGB(0, 0, 0));
    }
}

void loop()
{
    if (kilo_ticks > lastUpdate + updateTicks)
    {
        lastUpdate = kilo_ticks;

        set_motors(0,0);

        // Dance state
        msg.data[0] = danceState.state;
        msg.data[1] = nest.site;
        // Beliefs
        for (int b = 0; b < SITE_NUM; b++)
        {
            msg.data[beliefStart + b] = beliefs[b];
        }

        msg.type = NORMAL;
        msg.crc = message_crc(&msg);

        if (danceState.state == 0)
        {
            if (isMalicious == 1)
            {
                for (int b = 0; b < SITE_NUM; b++)
                {
                    beliefs[b] = 0;
                }

                beliefs[rand_hard() % SITE_NUM] = 1;

                uint8_t siteToVisit = getSiteToVisit(beliefs);
                setNestSite(siteToVisit, nestQualities[siteToVisit]);
                setDanceState(1, nestQualities[siteToVisit]);
            }
            else if (messageCount > 0)
            {
                int dancingBeeCount = 0;
                for (int i = 0; i < messageCount; i++)
                {
                   // If bee's dance state is true
                   if (messages[i][0] == 1)
                   {
                        dancingBeeCount++;
                   }
                }

                if (dancingBeeCount > 0)
                {
                    uint8_t *dancingBees = (uint8_t *) malloc(sizeof(uint8_t) * (dancingBeeCount * SITE_NUM));
                    int dbIndex = 0;
                    for (int i = 0; i < messageCount; i++)
                    {
                        // If bee's dance state is true
                        if (messages[i][0] == 1)
                        {
                            // Set the dancing bee to its beliefs
                            for (int b = 0; b < SITE_NUM; b++)
                            {
                                dancingBees[dbIndex + b] = messages[i][2 + b];
                            }

                            dbIndex += SITE_NUM;
                        }
                    }

                    uint8_t *otherBeliefs = &dancingBees[(rand_hard() % dancingBeeCount) * (SITE_NUM)];

                    for (int i = 0; i < SITE_NUM; i++)
                    {
                        beliefs[i] = otherBeliefs[i];
                    }

                    uint8_t siteToVisit = getSiteToVisit(beliefs);
                    setNestSite(siteToVisit, nestQualities[siteToVisit]);
                    int danceDuration = nestQualities[siteToVisit] + round(get_noise());
                    if (danceDuration <= 0)
                    {
                        danceDuration = 1;
                    }
                    setDanceState(1, danceDuration);

                    free(dancingBees);
                }
            }
            else
            {
                uint8_t siteToVisit = getSiteToVisit(beliefs);
                setNestSite(siteToVisit, nestQualities[siteToVisit]);
                int danceDuration = nestQualities[siteToVisit] + round(get_noise());
                if (danceDuration <= 0)
                {
                    danceDuration = 1;
                }
                setDanceState(1, danceDuration);
            }

            // double probNotDancing = rand_hard() / 255.0;
            // if (probNotDancing <= 0.5)
            // {
            //     setDanceState(0, 0);
            // }

            if (isMalicious == 1)
            {
                set_color(RGB(0, 2, 0));
            }
            else if (danceState.state == 1)
            {
                // Set colour to respective site
                set_bot_colour(nest.site);
            }
            else
            {
                // Set colour to black; not dancing
                set_color(RGB(0, 0, 0));
            }
        }

        else
        {
            // Bee is dancing, so decrement dance duration.
            setDanceState(1, danceState.duration - 1);

            // Random movement
            switch(rand_hard() % 4)
            {
                case(0):
                    set_motors(0,0);
                    break;
                case(1):
                    //if (last_output == 0) spinup_motors();
                    set_motors(kilo_turn_left,0); // 70
                    break;
                case(2):
                    //if (last_output == 0) spinup_motors();
                    set_motors(0,kilo_turn_right); // 70
                    break;
                case(3):
                    //if (last_output == 0) spinup_motors();
                    set_motors(kilo_straight_left, kilo_straight_right); // 65
                    break;
            }

            if (danceState.duration < 1)
            {
                // Bee no longer dances
                setDanceState(0, 0);
                set_color(RGB(0, 0, 0));
                set_motors(0,0);
            }
        }

        messageCount = 0;
        loopCounter++;
    }
}

int main()
{
    kilo_init();
    kilo_start(setup, loop);

    return 0;
}
