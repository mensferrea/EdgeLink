#include "edgelink/diagnostics/icmp_diagnostics.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::string target_ip = "127.0.0.1";
    int count = 5;

    if (argc >= 2) {
        target_ip = argv[1];
    }
    if (argc >= 3) {
        count = std::stoi(argv[2]);
    }

    std::cout << "EdgeLink RAW Socket ICMP Diagnostics\n";
    std::cout << "Target: " << target_ip << ", Count: " << count << "\n";

    edgelink::diagnostics::IcmpDiagnostics diag;
    if (!diag.is_available()) {
        std::cerr << "Diagnostics initialization failed: " << diag.init_error() << "\n";
        std::cerr << "Note: RAW sockets require root privileges or CAP_NET_RAW capability.\n";
        return 1;
    }

    int received = 0;
    double total_rtt = 0.0;

    for (int i = 1; i <= count; ++i) {
        auto res = diag.ping(target_ip, 1000, static_cast<uint16_t>(i));
        if (res.success) {
            ++received;
            total_rtt += res.rtt_ms;
            std::cout << "Reply from " << res.target_ip 
                      << ": seq=" << res.sequence 
                      << " ttl=" << static_cast<int>(res.ttl) 
                      << " time=" << std::fixed << std::setprecision(2) << res.rtt_ms << " ms\n";
        } else {
            std::cout << "Request to " << target_ip 
                      << " seq=" << res.sequence 
                      << " failed: " << res.error_message << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\n--- " << target_ip << " ping statistics ---\n";
    std::cout << count << " packets transmitted, " << received << " received, "
              << ((count - received) * 100 / count) << "% packet loss\n";

    if (received > 0) {
        std::cout << "avg RTT = " << std::fixed << std::setprecision(2) 
                  << (total_rtt / received) << " ms\n";
    }

    return 0;
}
