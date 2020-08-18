/*
 * utility.hpp - System utilities
 */

#pragma once

#include <stdexcept>
#include <system_error>

#include <sys/ioctl.h>

namespace vmm::utility {

/**
 * Runs an ioctl.
 *
 * By default, the ioctl ran is equivalent to one with no arguments.
 *
 * Examples
 * ========
 * ```
 * #include <vmm/utility.hpp>
 * #include <fcntl.h>
 * #include <linux/kvm.h>
 * #include <sys/stat.h>
 *
 * auto fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
 * auto version = vmm::utility::ioctl(fd, KVM_GET_API_VERSION);
 * auto supported = vmm::utility::ioctl(fd, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY);
 * ```
 */
template<typename T=int>
auto ioctl(const int fd, const unsigned long req, T arg=T{}) -> unsigned int {
    const auto ret = ::ioctl(fd, req, arg);
    if (ret < 0) throw std::system_error{errno, std::system_category()};
    return ret;
}

auto close(const int fd) -> void;

}  // namespace vmm::utility
