//
//  log_ios.h
//  
//
//  Apple System Log facility, used in C
//
//

#ifndef SIP_LOG_IOS_H
#define SIP_LOG_IOS_H

#define CLog CLog_sip

void CLog( const char * fmt, ... );

void CLogEmergency( char * fmt, ... );

void CLogAlert( char * fmt, ... );

void CLogCritical( char * fmt, ... );

void CLogError( char * fmt, ... );

void CLogWarning( char * fmt, ... );

void CLogNotice( char * fmt, ... );

void CLogInfo( char * fmt, ... );

void CLogDebug( char * fmt, ... );

#endif

