#include "kilolib.h"
#include <stdlib.h>
#include <math.h>

#define LANG_SIZE 3
#define POPUL_SIZE 200

#define BOOLEAN 1.0

// Default values for core variables
double min_distance = 100;
#ifdef BOOLEAN
int legalOpinions[3][LANG_SIZE] = {
    {2,0,0},
    {0,2,0},
    {0,0,2}
};
#else
int legalOpinions[7][LANG_SIZE] = {
    {2,0,0},
    {0,2,0},
    {0,0,2},
    {1,1,0},
    {1,0,1},
    {0,1,1},
    {1,1,1}
};
#endif
int nestQualities[LANG_SIZE] = {7, 9, 10};

// Individual vars for bots
int initialDelay = 0;
int messageCount = 0;
uint8_t messages[POPUL_SIZE][2];
message_t msg;
int opinionIndex;
int opinion[LANG_SIZE];
int last_update = -1;

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

int getOpinionIndex(int *opinion)
{
    int index = -1;
    // Loop through each opinion
    for (int i = 0; i < sizeof(legalOpinions) / sizeof(legalOpinions[0]); i++)
    {
        int found = 1;
        // Loop through values in opinion
        for (int j = 0; j < LANG_SIZE; j++)
        {
            if (legalOpinions[i][j] != opinion[j])
            {
                found = 0;
                break;
            }
        }

        // If found is not true, continue the loop
        if (found != 1)
            continue;
        else
        {
            index = i;
            break;
        }
    }

    return index;
}

int getSiteToVisit(int *opinion)
{
    int siteToVisit = -1;
    int siteChoices[LANG_SIZE] = {-1, -1, -1};
    int siteCount = 0;

    for (int i = 0; i < LANG_SIZE; i++)
    {
        if (opinion[i] == 2)
        {
            siteToVisit = i;
            break;
        }
        else if (opinion[i] == 1)
        {
            siteChoices[siteCount] = i;
            siteCount++;
        }
    }

    if (siteToVisit == -1)
    {
        // Choose a random site to visit, out of all 1/2 values
        siteToVisit = siteChoices[(rand_hard() % siteCount)];
    }

    return siteToVisit;
}

void consensus(int *opinion1, int *opinion2, int *newOpinion)
{
    for (int i = 0; i < LANG_SIZE; i++)
    {
        int sum = opinion1[i] + opinion2[i];
        if (sum <= 1)
            newOpinion[i] = 0;
        else if (sum >= 3)
            newOpinion[i] = 2;
        else if (sum == 2)
            newOpinion[i] = 1;
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
        u1 = (double) (1 + rand_hard())/ 256;
        u2 = (double) (1 + rand_hard())/ 256;

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
        if (1)//distance < min_distance)
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
        delay(30000); // Delay initial loop for 10 seconds to allow a fairly equal start time and easy recording.
    }

    // put your setup code here, will be run once at the beginning
    kilo_message_tx = (message_tx_t)&tx_message;
    kilo_message_rx = (message_rx_t)&rx_message;

	// Construct a valid message
	msg.type    = NORMAL;
	msg.crc     = message_crc(&msg);
	*msg.data 	= (long) 0;

	messageCount = 0;

    opinionIndex = rand_hard() % (sizeof(legalOpinions) / sizeof(legalOpinions[1]));
    for (int i = 0; i < LANG_SIZE; i++)
    {
        opinion[i] = legalOpinions[opinionIndex][i];
    }

    if (opinionIndex == -1)
    {
        setNestSite(-1, 0);
        setDanceState(0, 0);
        // Set dance state in the msg
        msg.data[0] = (uint8_t) 0;
        // Set the opinion index
        msg.data[1] = (uint8_t) -1;
    }
    else
    {
        int siteToVisit = getSiteToVisit(opinion);
        setNestSite(siteToVisit, nestQualities[siteToVisit]);
        setDanceState(1, nestQualities[siteToVisit]);
        // Set dance state in the msg
        msg.data[0] = (uint8_t) 1;
        // Set the opinion index
        msg.data[1] = (uint8_t) opinionIndex;
    }
	msg.crc = message_crc(&msg);
}

void loop()
{
    // put your main code here, will be run repeatedly
    if (kilo_ticks > last_update + 4)
    {
        last_update = kilo_ticks;

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


        // Find opinion index
        opinionIndex = getOpinionIndex(opinion);
        //std::cout << opinionIndex << std::endl;
        //std::cout << (int) nest.site << std::endl;

        // Set the opinion index
        msg.data[1] = (uint8_t) opinionIndex;

        if (danceState.state == 1)
        {
            // setNestSite(-1, 0);
            // setDanceState(0, 0);
            // Set dance state in msg to NOT_DANCING
            msg.data[0] = (uint8_t) 1;
        }
        else
        {
            // int siteToVisit = getSiteToVisit(opinion);
            // setNestSite(siteToVisit, nestQualities[siteToVisit]);
            // setDanceState(1, nestQualities[siteToVisit]);
            // Set dance state in msg to DANCING
            msg.data[0] = (uint8_t) 0;
        }
	    msg.crc = message_crc(&msg);

		// std::cout << "+Messages received:" << loopCounter << ":" << (int) danceState.state << ":" << messageCount << std::endl;
		// std::cout << "+Opinion index:" << loopCounter << ":" << (int) danceState.state << ":" << (int) opinionIndex << std::endl;
		// std::cout << "+Nest site:" << loopCounter << ":" << (int) danceState.state << ":" << (int) nest.site << std::endl;

		// std::cout << "+:" << loopCounter << ":" << (int) danceState.state << ":" << messageCount << ":" << (int) opinionIndex << ":" << (int) nest.site << std::endl;

        if (danceState.state == 1)
        {
            /*switch ((uint8_t) opinionIndex)
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
                case 3:
                    set_color(RGB(3, 0, 3));
                    break;
                case 4:
                    set_color(RGB(3, 3, 0));
                    break;
                case 5:
                    set_color(RGB(0, 3, 3));
                    break;
                case 6:
                    set_color(RGB(3, 3, 3));
                    break;
                case (uint8_t) -1:
                    set_color(RGB(0, 0, 0));
                    break;
            }*/
            switch ((uint8_t) opinionIndex)
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
                case 3:
                    set_color(RGB(0, 0, 0));
                    break;
                case 4:
                    set_color(RGB(0, 0, 0));
                    break;
                case 5:
                    set_color(RGB(0, 0, 0));
                    break;
                case 6:
                    set_color(RGB(0, 0, 0));
                    break;
                case (uint8_t) -1:
                    set_color(RGB(0, 0, 0));
                    break;
            }
        }
        else
        {
            // Set colour to black for not dancing
            set_color(RGB(0, 0, 0));
        }

        if (danceState.state == 0)
        {
            //std::cout << "Bee is not dancing..." << std::endl;
            if (messageCount > 0)
            {
                //std::cout << "Messages received..." << std::endl;

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
               		//std::cout << "Bees dancing..." << std::endl;
                    int *dancingBees = (int *) malloc(sizeof(int) * dancingBeeCount);
                    int dbIndex = 0;
                    for (int i = 0; i < messageCount; i++)
                    {
                        // If bee's dance state is true
                        if (messages[i][0] == 1)
                        {
                            // Set the dancing bee to its opinion index
                            dancingBees[dbIndex] = messages[i][1];
                            dbIndex++;
                        }
                    }

                    int otherOpinion = dancingBees[rand_hard() % dancingBeeCount];
                    int newOpinion[LANG_SIZE];
                    //std::cout << "Consensus forming..." << std::endl;
                    consensus(opinion, legalOpinions[otherOpinion], newOpinion);
                    #ifndef BOOLEAN
                    for (int i = 0; i < LANG_SIZE; i++)
                    {
                        opinion[i] = newOpinion[i];
                    }

                    #else
                    int newOpinionIndex = getSiteToVisit(newOpinion);
                    for (int i = 0; i < LANG_SIZE; i++)
                    {
                        opinion[i] = legalOpinions[newOpinionIndex][i];
                    }
                    #endif

                    opinionIndex = getOpinionIndex(opinion);
                    if (opinionIndex != -1)
                    {
                    	int siteToVisit = getSiteToVisit(opinion);
			        	setNestSite(siteToVisit, nestQualities[siteToVisit]);
			       		setDanceState(1, nestQualities[siteToVisit]);
                    }
                    else
                    {
                    	setNestSite(-1, 0);
                    }

                    free(dancingBees);
                }
            }

	        int probNotDancing = 0; //rand_hard() % 2;
	        if (probNotDancing == 0 && nestQualities[nest.site] > 0 && nest.site != (uint8_t) -1)
	        {
	            setDanceState(1, nestQualities[nest.site]);
	        }
	        else
	        {
	        	setDanceState(0, 0);
	        }
        }
        else
        {
        	// Bee is dancing, so  decrement dance duration.
			setDanceState(1, danceState.duration - 1);
			//std::cout << "Dance State: " << danceState.state << " : " << (int) danceState.duration << std::endl;
			if (danceState.duration < 1)
			{
				// Bee no longer dances
				setDanceState(0, 0);
				set_color(RGB(0, 0, 0));
			}
        }

        messageCount = 0;
	//loopCounter++;
    }
}


int main()
{
    kilo_init();
    kilo_start(setup, loop);

    return 0;
}
