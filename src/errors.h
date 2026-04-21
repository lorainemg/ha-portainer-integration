#pragma once

#include <stdexcept>
#include <string>

// Base exception for all ha-portainer errors.
// Inherits from std::runtime_error so existing catch sites still work.
class HaPortainerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Network-level failures: DNS, TCP, SSL, WebSocket handshake
class ConnectionError : public HaPortainerError {
public:
    using HaPortainerError::HaPortainerError;
};

// HA rejected authentication or response was unparseable during auth
class AuthError : public HaPortainerError {
public:
    using HaPortainerError::HaPortainerError;
};

// HA command returned success: false
class CommandError : public HaPortainerError {
public:
    using HaPortainerError::HaPortainerError;
};

// YAML file missing, parse failure, missing fields, malformed URL
class ConfigError : public HaPortainerError {
public:
    using HaPortainerError::HaPortainerError;
};

// Unexpected dashboard shape (e.g., missing "views" key)
class DashboardError : public HaPortainerError {
public:
    using HaPortainerError::HaPortainerError;
};

// Portainer API failures: connection, non-2xx HTTP, malformed JSON
class PortainerError : public HaPortainerError {
public:
    using HaPortainerError::HaPortainerError;
};
