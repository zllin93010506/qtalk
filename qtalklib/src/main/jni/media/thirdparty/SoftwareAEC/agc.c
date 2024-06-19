#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// AGC from http://hawksoft.com, modify by oran huang.
#include "agc.h" 
void hvdiNewAGC(hvdi_agc *agc, double level)
{

    if(agc == NULL)
    {
        return ;
    }
    agc->sample_max = 1;
    agc->counter = 0;
    agc->igain = 65536;
    if(level > 1.0)
    {
        level = 1.0;
    }
    else if(level < 0.5)
    {
        level = 0.5;
    }
	
    agc->ipeak = (int)(SHRT_MAX * level * 32768.0);

    agc->silence_counter = 0;
    return ;
}


void hvdiDeleteAGC(hvdi_agc *agc)
{
    free(agc);
}

void hvdiAGC(hvdi_agc *agc, short *buffer, int len)
{
    int i;
	int tmp;

    for(i=0;i<len;i++)
    {
        long gain_new;
        int sample;

        /* get the abs of buffer[i] */
        sample = buffer[i];
        sample = (sample < 0 ? -(sample):sample);

        if(sample > (int)agc->sample_max)
        {
            /* update the max */
            agc->sample_max = (unsigned int)sample;
        }
        agc->counter ++;


        /* Calculate new gain factor 10x per second */
		
        if (agc->counter >= 16000/50) // calculate AGC gain every 20ms
        {
            if (agc->sample_max > 1000)        /* speaking? */
            {
                gain_new = ((agc->ipeak / agc->sample_max) * 62259) >> 16;
                

                if(agc->sample_max > agc->ipeak){
			    	if (agc->silence_counter > 40)  /* pause -> speaking */{
                    	agc->igain += (gain_new - agc->igain) >> 3;
						printf("pause -> speaking!!\n");
					}
					else
	                    agc->igain += (gain_new - agc->igain)/4;
					}
				else
					agc->igain = 65536;

                agc->silence_counter = 0;
            }
            agc->silence_counter++;
            agc->counter = 0;
            agc->sample_max = 1;


        }

        buffer[i] = (short) ((buffer[i] * agc->igain) >> 16);
    }
}
