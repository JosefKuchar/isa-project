/**
 * @author Josef Kuchař (xkucha28)
 */

#include <arpa/inet.h>

// Buffer size
const size_t BUFSIZE = 65535;

// Default block size (from RFC 1350), in bytes
const int DEFAULT_BLOCK_SIZE = 512;

// Default port
const int DEFAULT_PORT = 69;

// Max packet re-transmissions
const int RETRY_COUNT = 3;

// Default timeout, in seconds
const int DEFAULT_TIMEOUT = 1;
