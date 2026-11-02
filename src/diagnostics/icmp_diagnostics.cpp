#include "edgelink/diagnostics/icmp_diagnostics.hpp"
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <poll.h>

namespace edgelink::diagnostics {

IcmpDiagnostics::IcmpDiagnostics() {
    process_id_ = static_cast<uint16_t>(::getpid() & 0xFFFF);
    try {
        socket_ = network::Socket::create_raw_icmp();
    } catch (const std::exception& e) {
        init_error_ = e.what();
    }
}

bool IcmpDiagnostics::is_available() const noexcept {
    return socket_.has_value() && socket_->is_valid();
}

const std::string& IcmpDiagnostics::init_error() const noexcept {
    return init_error_;
}

uint16_t IcmpDiagnostics::calculate_checksum(const void* data, size_t length) noexcept {
    const auto* words = static_cast<const uint16_t*>(data);
    uint32_t sum = 0;

    while (length > 1) {
        sum += *words++;
        length -= 2;
    }

    if (length == 1) {
        sum += *reinterpret_cast<const uint8_t*>(words);
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

IcmpPingResult IcmpDiagnostics::ping(const std::string& target_host_or_ip, uint32_t timeout_ms, uint16_t seq) {
    IcmpPingResult result;
    result.target_ip = target_host_or_ip;
    result.sequence = seq;

    if (!is_available()) {
        result.error_message = "RAW ICMP socket unavailable: " + init_error_;
        return result;
    }

    std::string ip = network::Socket::resolve_host(target_host_or_ip);
    result.target_ip = ip;

    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    if (::inet_pton(AF_INET, ip.c_str(), &dest_addr.sin_addr) <= 0) {
        result.error_message = "Invalid IP address: " + ip;
        return result;
    }

    struct Packet {
        icmphdr hdr{};
        uint64_t timestamp_ns{0};
    } packet;

    packet.hdr.type = ICMP_ECHO;
    packet.hdr.code = 0;
    packet.hdr.un.echo.id = htons(process_id_);
    packet.hdr.un.echo.sequence = htons(seq);
    packet.hdr.checksum = 0;

    auto start_time = std::chrono::steady_clock::now();
    packet.timestamp_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count()
    );

    packet.hdr.checksum = calculate_checksum(&packet, sizeof(packet));

    ssize_t sent = ::sendto(
        socket_->native_handle(),
        &packet,
        sizeof(packet),
        0,
        reinterpret_cast<const sockaddr*>(&dest_addr),
        sizeof(dest_addr)
    );

    if (sent < 0) {
        result.error_message = "sendto failed: " + std::string(strerror(errno));
        return result;
    }

    pollfd pfd{};
    pfd.fd = socket_->native_handle();
    pfd.events = POLLIN;

    auto deadline = start_time + std::chrono::milliseconds(timeout_ms);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            result.error_message = "Request timed out";
            return result;
        }

        int remaining_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count()
        );

        int poll_res = ::poll(&pfd, 1, remaining_ms);
        if (poll_res <= 0) {
            result.error_message = (poll_res == 0) ? "Request timed out" : "poll error: " + std::string(strerror(errno));
            return result;
        }

        uint8_t recv_buf[1024];
        sockaddr_in reply_addr{};
        socklen_t addr_len = sizeof(reply_addr);

        ssize_t bytes_recv = ::recvfrom(
            socket_->native_handle(),
            recv_buf,
            sizeof(recv_buf),
            0,
            reinterpret_cast<sockaddr*>(&reply_addr),
            &addr_len
        );

        if (bytes_recv < static_cast<ssize_t>(sizeof(iphdr) + sizeof(icmphdr))) {
            continue;
        }

        const auto* ip_hdr = reinterpret_cast<const iphdr*>(recv_buf);
        size_t ip_hdr_len = ip_hdr->ihl * 4;

        if (bytes_recv < static_cast<ssize_t>(ip_hdr_len + sizeof(icmphdr))) {
            continue;
        }

        const auto* icmp_reply = reinterpret_cast<const icmphdr*>(recv_buf + ip_hdr_len);

        if (icmp_reply->type == ICMP_ECHOREPLY) {
            if (ntohs(icmp_reply->un.echo.id) == process_id_ && ntohs(icmp_reply->un.echo.sequence) == seq) {
                auto end_time = std::chrono::steady_clock::now();
                double rtt = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                result.success = true;
                result.rtt_ms = rtt;
                result.ttl = ip_hdr->ttl;
                return result;
            }
        }
    }
}

}
