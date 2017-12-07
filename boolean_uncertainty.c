#include "kilolib.h"
#include <stdlib.h>
#include <math.h>

// Default values for core definitions
#define SITE_NUM 2
#define MAX_MSG_SIZE 100
#define MIN_DISTANCE 100

#define BELIEF_BYTES 2
#define BELIEF_PRECISION 4

// Individual variables for bots
int updateTicks = 16; // 32 updates per second
int initialDelay = 0;
int lastUpdate = -1;
int messageCount = 0;
int nestQualities[SITE_NUM] = {7, 9};
int loopCounter = 0;

double beliefs[SITE_NUM];
int beliefStart = 2;

// Frank's T-norm:
// 0.0 = min
// 1.0 = product
// +inf = Luk
//
double baseParam = 1.0;


uint8_t messages[MAX_MSG_SIZE][2 + (BELIEF_BYTES * (SITE_NUM - 1))];
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

double bytesToDouble(uint8_t *msgData)
{
    uint32_t reformedBytes = 0;
    int b = 0;
    for (int i = 8 * (BELIEF_BYTES - 1); i >= 0; i -= 8)
    {
        reformedBytes += (*(msgData + b++) << i);
    }

    return (double) reformedBytes / pow(10, BELIEF_PRECISION);
}

void doubleToBytes(double belief, uint8_t *convertedBytes)
{
    uint32_t convertedBelief = (uint32_t) ((belief * pow(10, BELIEF_PRECISION)) + 0.5);
    int b = 0;
    for (int i = 8 * (BELIEF_BYTES - 1); i >= 0; i -= 8)
    {
        *(convertedBytes + b++) = (uint8_t) (convertedBelief >> i) & 0xFF;
    }
}

uint8_t getSiteToVisit(double *beliefs)
{
    int siteToVisit = -1;

    double randomSite = (double) rand_hard() / 255.0;

    for (int i = 0; i < SITE_NUM - 1; i++)
    {
        if (randomSite < beliefs[i])
        {
            siteToVisit = i;
            break;
        }
    }

    if (siteToVisit == -1)
    {
        siteToVisit = SITE_NUM - 1;
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

void consensus(double *beliefs1, double *beliefs2, double *newBeliefs)
{
    for (int b = 0; b < SITE_NUM - 1; b++)
    {
        double numerator = franksTNorm(beliefs1[b], beliefs2[b], baseParam);
        double denominator = 1.0 - beliefs1[b] - beliefs2[b] + (2.0 * numerator);

        double newValue = 0.0;

        // Undefined behaviour of D-S rule of combination for inconsistent beliefs
        if (denominator == 0.0)
        {
            newValue = 0.5;
        }
        else if (numerator == 0.0)
        {
            newValue = 0.0;
        }
        else
        {
            newValue = (numerator / denominator);
        }

        newBeliefs[b] = newValue;
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
        u1 = (double) rand_hard() / 256;
        u2 = (double) rand_hard() / 256;

        z1 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        z2 = sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2);

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
    //std::cout << distance << std::endl;
    if (1)// distance < min_distance)
    {
        // Dance state
        messages[messageCount][0] = m->data[0];
        // Dance site
        messages[messageCount][1] = m->data[1];
        // Beliefs
        for (int b = 0; b < 3 * (SITE_NUM - 1); b++)
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

    // double beliefSum = 0.0;
    for (int b = 0; b < SITE_NUM - 1; b++)
    {
        uint32_t reformedBytes = 0;
        uint32_t maxValue = 0;
        for (int i = 8 * 3; i >= 0; i -= 8)
        {
            reformedBytes += (rand_hard() << i);
            maxValue += (255 << i);
        }
        beliefs[b] = (double) reformedBytes / (double) maxValue;
        // beliefSum += beliefs[b];
    }
    beliefs[SITE_NUM - 1] = 1.0 - beliefs[0];
    // beliefSum += rand_hard() / 255.0;
    // for (int b = 0; b < SITE_NUM - 1; b++)
    // {
    //     beliefs[b] = beliefs[b] / beliefSum;
    // }

    uint8_t siteToVisit = getSiteToVisit(beliefs);
    setNestSite(siteToVisit, nestQualities[siteToVisit]);
    setDanceState(1, nestQualities[siteToVisit]);

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
    uint8_t convertedBytes[BELIEF_BYTES * (SITE_NUM - 1)];
    for (int b = 0; b < SITE_NUM - 1; b++)
    {
        int byteIndex = beliefStart + (b * BELIEF_BYTES);
        doubleToBytes(beliefs[b], convertedBytes + (b * BELIEF_BYTES));
        for (int i = 0; i < BELIEF_BYTES; i++)
        {
            msg.data[byteIndex + i] = convertedBytes[(b * BELIEF_BYTES) + i];
        }
    }

    // std::cout << beliefs[0] << std::endl;
    // std::cout << bytesToDouble(&msg.data[2]) << std::endl;

    msg.type = NORMAL;
    msg.crc = message_crc(&msg);

    if (danceState.state == 1)
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

        // Dance state
        msg.data[0] = danceState.state;
        msg.data[1] = nest.site;
        // Beliefs
        uint8_t convertedBytes[BELIEF_BYTES * (SITE_NUM - 1)];
        for (int b = 0; b < SITE_NUM - 1; b++)
        {
            int byteIndex = beliefStart + (b * BELIEF_BYTES);
            doubleToBytes(beliefs[b], convertedBytes + (b * BELIEF_BYTES));
            for (int i = 0; i < BELIEF_BYTES; i++)
            {
                msg.data[byteIndex + i] = convertedBytes[(b * BELIEF_BYTES) + i];
            }
        }

        msg.type = NORMAL;
        msg.crc = message_crc(&msg);

        if (danceState.state == 0)
        {
            if (messageCount > 0)
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
                    double *dancingBees = (double *) malloc(sizeof(double) * (dancingBeeCount * SITE_NUM - 1));
                    int dbIndex = 0;
                    for (int i = 0; i < messageCount; i++)
                    {
                        // If bee's dance state is true
                        if (messages[i][0] == 1)
                        {
                            // Set the dancing bee to its beliefs
                            for (int b = 0; b < SITE_NUM - 1; b++)
                            {
                                dancingBees[dbIndex + b] = bytesToDouble(&messages[i][2 + b]);
                            }

                            dbIndex += SITE_NUM - 1;
                        }
                    }

                    double *otherBeliefs = &dancingBees[(rand_hard() % dancingBeeCount) * (SITE_NUM - 1)];
                    double newBeliefs[SITE_NUM - 1];

                    consensus(beliefs, otherBeliefs, newBeliefs);

                    for (int i = 0; i < SITE_NUM - 1; i++)
                    {
                        beliefs[i] = newBeliefs[i];
                    }
                    beliefs[SITE_NUM - 1] = 1.0 - beliefs[0];

                    uint8_t siteToVisit = getSiteToVisit(beliefs);
                    setNestSite(siteToVisit, nestQualities[siteToVisit]);
                    setDanceState(1, nestQualities[siteToVisit]);

                    free(dancingBees);
                }
            }
            else
            {
                uint8_t siteToVisit = getSiteToVisit(beliefs);
                setNestSite(siteToVisit, nestQualities[siteToVisit]);
                setDanceState(1, nestQualities[siteToVisit]);
            }

            // double probNotDancing = rand_hard() / 255.0;
            // if (probNotDancing <= 0.5)
            // {
            //     setDanceState(0, 0);
            // }

            if (danceState.state == 1)
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
                    spinup_motors();
                    set_motors(0,0);
                    break;
                case(1):
                    //if (last_output == 0) spinup_motors();
                    spinup_motors();
                    set_motors(kilo_turn_left,0); // 70
                    break;
                case(2):
                    //if (last_output == 0) spinup_motors();
                    spinup_motors();
                    set_motors(0,kilo_turn_right); // 70
                    break;
                case(3):
                    //if (last_output == 0) spinup_motors();
                    spinup_motors();
                    set_motors(kilo_straight_left, kilo_straight_right); // 65
                    break;
            }

            if (danceState.duration < 1)
            {
                // Bee no longer dances
                setDanceState(0, 0);
                set_color(RGB(0, 0, 0));
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
