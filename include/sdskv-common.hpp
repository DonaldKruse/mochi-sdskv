#ifndef __SDSKV_COMMON_HPP
#define __SDSKV_COMMON_HPP

#include <sdskv-common.h>

namespace sdskv {

const char* const sdskv_error_messages[] = {
    "",
    "Allocation error",
    "Invalid argument",
    "Mercury error",
    "Could not create database",
    "Invalid database name",
    "Invalid database id",
    "Invalid provider id",
    "Error writing in the database",
    "Unknown key",
    "Provided buffer size too small",
    "Error erasing from the database",
    "Migration error",
    "Function not implemented",
    "Invalid comparison function",
    "REMI error",
    "Argobots error"
};

class exception : public std::exception {

    std::string m_msg;
    int m_error;

    public:

    exception(int error)
    : m_msg(std::string("[SDSKV] ") + sdskv_error_messages[-error])
    , m_error(error) {}

    virtual const char* what() const noexcept override {
        return m_msg.c_str();
    }

    int error() const {
        return m_error;
    }
};

}

#endif