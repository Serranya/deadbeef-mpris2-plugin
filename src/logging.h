#ifndef LOGGING_H_
#define LOGGING_H_

void logDebug (const char *fmt, ...);
void logError (const char *fmt, ...);

#ifndef MPRIS__DEBUG
	#define debug(...)
#else
	#define debug(...) logDebug(__VA_ARGS__)
#endif

#define error(...) logError(__VA_ARGS__)

#endif
