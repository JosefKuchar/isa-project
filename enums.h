#pragma once

enum class Opcode {
    RRQ = 1,    // Read request (RRQ)
    WRQ = 2,    // Write request (WRQ)
    DATA = 3,   // Data (DATA)
    ACK = 4,    // Acknowledgment (ACK)
    ERROR = 5,  // Error (ERROR)
    OACK = 6,   // Option Acknowledgment (OACK)
};

enum class Mode { Netascii, Octet, Mail };

enum class ErrorCode {
    NotDefined = 0,         // Not defined, see error message (if any)
    FileNotFound = 1,       // File not found
    AccessViolation = 2,    // Access violation
    DiskFull = 3,           // Disk full or allocation exceeded
    IllegalOperation = 4,   // Illegal TFTP operation
    UnknownTID = 5,         // Unknown transfer ID
    FileAlreadyExists = 6,  // File already exists
    NoSuchUser = 7,         // No such user
    InvalidOption = 8,      // Invalid option
};

/**
 * RFC 2347, RFC 2348, RFC 2349 options
 */
enum class Option {
    Blksize,  // Block size
    Timeout,  // Timeout
    Tsize,    // Transfer size
};

const int DEFAULT_PORT = 69;
