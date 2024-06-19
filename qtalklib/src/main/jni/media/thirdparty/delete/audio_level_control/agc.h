// AGC source code frm http://hawksoft.com, modify by oran huang
typedef struct{
    unsigned int    sample_max;
    int             counter;
    long            igain;
    int             ipeak;
    int             silence_counter;
}hvdi_agc;
