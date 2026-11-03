#ifndef ROBOT_WIFI_AP_H
#define ROBOT_WIFI_AP_H

#include <WiFi.h>
#include <stdarg.h>

/**
   @brief Provides one TCP client connection after joining the computer hotspot.

   The class joins a WiFi network and owns a single active TCP client.
   It is a transport helper: callers decide the meaning and format of transmitted
   bytes. Call update() regularly to accept or retire client connections.
*/
class RobotWifiAP_c {
  public:
    /**
       @brief Configures the network to join and the TCP listening port.
       @param ssid Computer hotspot name.
       @param password Computer hotspot password.
       @param port TCP port on which the server listens.
    */
    RobotWifiAP_c(const char* ssid, const char* password, uint16_t port);

    /**
       @brief Begins joining WiFi asynchronously.
       @return True when connection setup was requested, not proof of association.
       @note update() starts the TCP server after WiFi receives an IP address.
    */
    bool begin();

    /**
       @brief Maintains the current client and accepts a replacement when needed.

       This method never interprets application data. It only manages connection
       state and should be called regularly from loop().
    */
    void update();

    /** @brief Returns whether a TCP client is currently connected. */
    bool clientConnected();

    /**
       @brief Returns the number of bytes waiting to be read from the client.
       @return Zero when no client is connected or no bytes are available.
    */
    size_t available();

    /**
       @brief Reads one byte from the active client.
       @return The byte value, or -1 when no byte is available.
    */
    int read();

    /**
       @brief Reads available bytes into a caller-provided buffer.
       @param buffer Destination buffer.
       @param maxLen Maximum number of bytes to copy.
       @return Number of bytes copied; zero for invalid input or no client data.
       @note This method does not add a null terminator.
    */
    size_t readBytes(char* buffer, size_t maxLen);

    /**
       @brief Sends a null-terminated string without adding a newline.
       @param msg String to send.
       @return true when at least one byte was written.
    */
    bool send(const char* msg);

    /**
       @brief Sends a string followed by a newline.
       @param msg String to send.
       @return true when at least one byte was written.
    */
    bool sendLine(const char* msg);

    /**
       @brief Sends a binary buffer unchanged.
       @param data Bytes to send.
       @param len Number of bytes to send.
       @return true only when every byte was written.
    */
    bool sendBuffer(const uint8_t* data, size_t len);

    /** @brief Closes the active client connection, if any. */
    void disconnectClient();

    /** @brief Returns the DHCP-assigned station IP address. */
    IPAddress ip() const;

    /** @brief Returns one when associated with WiFi, otherwise zero. */
    int stationCount() const;

    /**
       @brief Sets the minimum interval between TCP-client accept attempts.
       @param intervalMs Interval in milliseconds.
    */
    void setAcceptIntervalMs(unsigned long intervalMs);

    /**
       @brief Formats and sends a string to the active client.
       @param format printf-style format string.
       @return true when all retained buffer bytes were written to the client.
       @warning Output beyond 255 payload bytes is silently truncated. A true
       result does not guarantee that the entire requested message was retained.
    */
    bool printf(const char* format, ...);

    /**
       @brief Formats and sends one newline-terminated string to the active client.
       @param format printf-style format string.
       @return true when the prepared buffer bytes, including newline, were written.
       @warning Keep the formatted payload to at most 253 bytes before newline.
       The current implementation mishandles longer output: it can send an
       embedded null byte and, above 254 payload bytes, an uninitialised byte.
       A true result does not detect truncation or these formatting errors.
    */
    bool printlnf(const char* format, ...);

  private:
    /** @brief Returns whether it is time to look for a new TCP client. */
    bool shouldCheckAccept() const;

    const char* _ssid;
    const char* _password;
    uint16_t _port;
    WiFiServer _server;
    WiFiClient _client;
    bool _started = false;
    bool _listening = false;
    unsigned long _lastAcceptCheckMs = 0;
    unsigned long _acceptIntervalMs = 100;
};

#endif
