#ifndef CLOCK_H
#define CLOCK_H

#include <stmcpp/error.hpp>

namespace clock {
    enum class error {
        ldo_timeout,
        vos1_timeout,
        vos0_timeout,
        hse_timeout,
        hsi48_timeout,
        csi_timeout,
        flash_latency_timeout,
        pll1_timeout,
        domain_source_switch_timeout,
        clock_compensation_timeout,
        clock_mux_timeout,
        other
    };

    static stmcpp::error::handler<error, "clock"> errorHandler;

    void init();
} 


#endif