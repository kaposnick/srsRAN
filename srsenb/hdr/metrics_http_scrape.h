#ifndef SRSENB_METRICS_HTTP_SCRAPE_H
#define SRSENB_METRICS_HTTP_SCRAPE_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sstream>

#define MHD_PLATFORM_H

#include <microhttpd.h>

#include "srsran/interfaces/enb_metrics_interface.h"

namespace srsenb {

class metrics_http_scrape : public srsran::metrics_listener<enb_metrics_t>
{
public:
    metrics_http_scrape();
    ~metrics_http_scrape();

    void set_metrics(const enb_metrics_t& m, const uint32_t period_usec);
    void set_handle(const uint32_t enb_id); 

    void init(enb_metrics_interface* m_, const uint32_t bindPort);
    void stop();

    std::string& getEnbId();
    const srsenb::enb_metrics_t & getLatestMetrics();

private:
    enb_metrics_interface* m = nullptr;
    std::string enbId;
    srsenb::enb_metrics_t metrics;
    struct MHD_Daemon * d;
};

}

#endif  // SRSENB_METRICS_HTTP_SCRAPE_H
