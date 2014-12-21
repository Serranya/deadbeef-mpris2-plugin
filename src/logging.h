#ifndef LOGGING_H_
#define LOGGING_H_

#define MPRIS__DEBUG

#ifndef MPRIS__DEBUG
	#define debug(...)
#else
	#define debug(...) logDebug(__VA_ARGS__)
#endif

#define error(...) logError(__VA_ARGS__)

#endif
