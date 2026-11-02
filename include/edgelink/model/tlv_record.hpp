#pragma once

#include "edgelink/model/metric_type.hpp"
#include <vector>
#include <cstdint>

namespace edgelink::model {

struct TlvRecord {
    MetricType type;
    uint16_t length{0};
    double value{0.0};
};

}
