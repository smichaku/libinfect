#pragma once

#include <string>
#include <sstream>
#include <exception>

class InjectorException : public std::exception {
public:
    InjectorException(const std::string &msg, int err) :
        message(msg),
        error_code(err)
    {
        std::ostringstream stream;
        stream << message << " (error 0x" << std::hex << error_code << ")";

        m_text = stream.str();
    }

    const char *what() const noexcept override
    {
        return m_text.c_str();
    }

    const std::string message;
    const int error_code;

private:
    std::string m_text;
};
