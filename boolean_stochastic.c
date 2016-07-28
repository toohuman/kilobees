#include "kilolib.h"
#include <stdlib.h>
#include <math.h>

// Default values for core definitions
#define LANG_SIZE 3
#define POPUL_SIZE 200
#define MIN_DISTANCE = 100;

#define BOOLEAN 1.0;

// Individual variables for bots
int updateTicks = 4;
int initialDelay = 0;
int lastUpdate = -1;
int messageCount = 0;
int nestQualities[LANG_SIZE] = {7, 9, 10};

double belief[LANG_SIZE - 1];

// Frank's T-norm:
// 0.0 = min
// 1.0 = product
// +inf = Luk
//
double baseParam = 1.0;


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
    uint8_t state;
    uint8_t duration;
};

double beliefs[LANG_SIZE];

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

uint8_t getSiteToVisit(double *beliefs)
{
    uint8_t siteToVisit = -1;

    double randomSite = (double) rand_hard() / 255;

    for (int i = 0; i < LANG_SIZE - 1; i++)
    {
        if (randomSite <= beliefs[i])
        {
            siteToVisit = i;
            break;
        }
    }

    if (siteToVisit == -1)
    {
        siteToVisit = LANG_SIZE - 1;
    }

    return siteToVisit;
}

// static double frankTNorm(T, L)(T belief1, T belief2, L p)
// {
//  T function(T value1, T value2, L p) func;

//  if (p == 0) func = (x, y, p) => (x <= y) ? x : y;
//  else if (p == 1) func = (x, y, p) => x * y;
//  else
//  {
//      func = (x, y, p)
//      {
//          return log(1 + ( ((pow(exp(p), x) - 1) * (pow(exp(p), y) - 1) ) / (exp(p) - 1)) ) / p;
//      };
//  }

//  return func(belief1, belief2, p);
// }

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
        return log(1 + (pow(exp(p), belief1) - 1) * (pow(exp(p), belief2) - 1) / (exp(p) - 1) ) / p;
    }
}

// auto beliefs1 = agent1.getBeliefs;
// auto beliefs2 = agent2.getBeliefs;
// auto newBeliefs = beliefs1.dup;
// foreach (int b, ref belief; newBeliefs)
// {
//  auto numerator = Operators.frankTNorm(beliefs1[b][0], beliefs2[b][0], baseParam);
//  auto denominator = 1.0 - beliefs1[b][0] - beliefs2[b][0] + (2 * Operators.frankTNorm(beliefs1[b][0], beliefs2[b][0], baseParam));

//  // Undefined behaviour of D-S rule of comb. for inconsistent beliefs
//  if (denominator == 0.0)
//      belief[0] = belief[1] = 0.5;
//  else if (numerator == 0.0)
//      belief[0] = belief[1] = 0.0;
//  else
//      belief[0] = belief[1] = (numerator / denominator);
// }

void consensus(double *beliefs1, double *beliefs2, double *newBeliefs)
{
    for (int b = 0; b < LANG_SIZE - 1; b++)
    {
        double numerator = franksTNorm(beliefs1[0], beliefs2[0], baseParam);
        double denominator = 1.0 - beliefs1[0] - beliefs2[0] + (2 * numerator);

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

        z1 = sqrt(-2 * log(u1)) * cos(2 * M_PI * u2);
        z2 = sqrt(-2 * log(u1)) * sin(2 * M_PI * u2);

        generate++;
        return z2;
    }
    else
    {
        generate--;
        return z1;
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
        messages[messageCount][0] = m->data[0];
        messages[messageCount][1] = m->data[1];
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
    // [0] - first 8-bits of ID.
    // [1] - final 8-bits of ID.
    // [2] - dance state; 1 (true) if bee is dancing, 0 (false) if not.
    // [3] - dance site; the site that is currently being danced for.
    // [4] - belief[0]; site value "x" (for 0:x, 1: 1 - x).
    // [5] - belief[1]; optional for 3-sites (language size: 3).
    // [6]
    // [7]
    // [8]
    // [9]
    // msg.type = NORMAL;
    // msg.crc = message_crc(&msg);

    while (1)
    {
        for (int b = 0; b < LANG_SIZE - 1; b++)
        {
            beliefs[b] = rand_hard() / 255;
        }

        uint8_t exitScope = 1;
        double prevBelief = beliefs[0];

        for (int b = 1; b < LANG_SIZE - 1; b++)
        {
           if (prevBelief <= beliefs[b])
           {
                exitScope = 0;
                break;
           }
        }

        if (exitScope == 1)
            break;
    }

    uint8_t siteToVisit = getSiteToVisit(beliefs);
    setNestSite(siteToVisit, nestQualities[siteToVisit]);
    setDanceState(1, nestQualities[siteToVisit]);

    double probNotDancing = rand_hard() / 255;
    if (probNotDancing <= 0.5 && nestQualities[siteToVisit] > 0)
    {
        setDanceState(1, nestQualities[siteToVisit]);
    }
    else
    {
        setDanceState(0, 0);
    }

    // Generate random integers to fill both ID bytes, leading to a 16-bit
    // number with 65,536 possible values: 255 (8-bit) x 255 (8-bit).
    msg.data[0] = rand_hard();
    msg.data[1] = rand_hard();
    // Dance state
    msg.data[3] = danceState.state;
    msg.data[4] = nest.site;

    msg.type = NORMAL;
    msg.crc = message_crc(&msg);
}

void loop()
{
    if (kilo_ticks > lastUpdate + updateTicks)
    {
        lastUpdate = kilo_ticks;

        // Random movement
        /*switch(rand_hard() % 4)
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
        }*/


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
                    double *dancingBees = (double *) malloc(sizeof(double) * (dancingBeeCount * LANG_SIZE - 1));
                    int dbIndex = 0;
                    for (int i = 0; i < messageCount; i++)
                    {
                        // If bee's dance state is true
                        if (messages[i][0] == 1)
                        {
                            // Set the dancing bee to its opinion index
                            dancingBees[dbIndex] = messages[i][1];
                            dbIndex += LANG_SIZE - 1;
                        }
                    }

                    double *otherBeliefs = &dancingBees[(rand_hard() % dancingBeeCount) * (LANG_SIZE - 1)];
                    double newBeliefs[LANG_SIZE - 1];

                    consensus(beliefs, otherBeliefs, newBeliefs);

                    for (int i = 0; i < LANG_SIZE; i++)
                    {
                        beliefs[i] = newBeliefs[i];
                    }

                    uint8_t siteToVisit = getSiteToVisit(beliefs);
                    setNestSite(siteToVisit, nestQualities[siteToVisit]);
                    setDanceState(1, nestQualities[siteToVisit]);

                    free(dancingBees);
                }
            }

            double probNotDancing = rand_hard() / 255;
            if (probNotDancing <= 0.5 && nestQualities[nest.site] > 0)
            {
                setDanceState(1, nestQualities[nest.site]);
            }
            else
            {
                setDanceState(0, 0);
            }

            if (danceState.state == 1)
            {
                switch ((uint8_t) nest.site)
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

            if (danceState.duration < 1)
            {
                // Bee no longer dances
                setDanceState(0, 0);
                set_color(RGB(0, 0, 0));
            }
        }

        messageCount = 0;
    }
}

int main()
{
    kilo_init();
    kilo_start(setup, loop);

    return 0;
}
