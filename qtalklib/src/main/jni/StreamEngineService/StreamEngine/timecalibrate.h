//
//  TimeCalibrate.h
//  QOCA
//
//  Created by Corey on 11/21/13.
//
//

#ifndef QOCA_TimeCalibrate_h
#define QOCA_TimeCalibrate_h


#ifdef __cplusplus
extern "C"{
#endif

#ifndef WIN32
typedef struct _timeCalibrate timeCalibrate;

struct _timeCalibrate* time_calibrator_create(int isCaller, char* call_id, char* local_id, char* remote_id, char* serverIP, int serverPort, int hostPort);
void time_calibrator_start(timeCalibrate* aCalibrator);
void time_calibrator_end(timeCalibrate* );
int time_calibrator_destroy(struct _timeCalibrate* aCalibrator);

#endif // #ifndef WIN32


#ifdef __cplusplus
}
#endif

#endif
