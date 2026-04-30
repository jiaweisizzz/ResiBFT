#ifndef CONFIG_H
#define CONFIG_H

#include "parameters.h"

// Debug
#define DEBUG_BASIC true	// Print basic messages
#define DEBUG_HELP false	// Print help messages
#define DEBUG_SERVER false	// Print server messages
#define DEBUG_CLIENT false	// Print client messages
#define DEBUG_MODULES false // Print modules messages
#define DEBUG_TEE false		// Print TEE messages
#define DEBUG_LOG false		// Print log messages
#define DEBUG_TIME false	// Print time messages

#define MAXLINE 256
#define MAX_SIZE_PAYLOAD 4096

// Colours
#define COLOUR_NORMAL "\x1B[0m"
#define COLOUR_RED "\x1B[49m\x1B[91m"	  // For basic messages
#define COLOUR_GREEN "\x1B[49m\x1B[92m"	  // For log messages
#define COLOUR_ORANGE "\x1B[49m\x1B[93m"  // For server messages and client messages
#define COLOUR_BLUE "\x1B[49m\x1B[94m"	  // For normal help messages and time messages
#define COLOUR_MAGENTA "\x1B[49m\x1B[95m" // For special help messages
#define COLOUR_CYAN "\x1B[49m\x1B[96m"	  // For modules messages and TEE messages
#define COLOUR_WHITE "\x1B[49m\x1B[97m"

#endif
