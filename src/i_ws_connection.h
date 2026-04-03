#pragma once

#include <string>

class IWebSocketConnection {
public:
    // Pure virtual methods (= 0): subclasses MUST implement these.
    // Like @abstractmethod in Python or abstract methods in C#.
    virtual void connect(const std::string& host,
                         const std::string& port,
                         const std::string& path) = 0;

    virtual std::string read() = 0;

    virtual void write(const std::string& message) = 0;

    virtual void close() = 0;

    // Virtual destructor: ensures the right destructor runs
    // when deleting through a base class pointer.
    // = default means "use the compiler-generated one."
    virtual ~IWebSocketConnection() = default;
};
