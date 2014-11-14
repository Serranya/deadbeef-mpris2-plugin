#ifndef LOGGING_H_
#define LOGGING_H_

#define MPRIS__DEBUG 1

#ifndef MPRIS__DEBUG
	#define debug(msg)
#else
	#define debug(...) logDebug(__VA_ARGS__)
#endif

#define error(...) logError(__VA_ARGS__)

#endif
