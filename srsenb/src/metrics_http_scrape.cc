#include "srsenb/hdr/metrics_http_scrape.h"
#include <iomanip>
#include <float.h>
#include "srsran/phy/utils/vector.h"

using namespace std;

namespace srsenb {

static char const* const prefixes[2][9] = {
    {
        "",
        "m",
        "u",
        "n",
        "p",
        "f",
        "a",
        "z",
        "y",
    },
    {
        "",
        "k",
        "M",
        "G",
        "T",
        "P",
        "E",
        "Z",
        "Y",
    },
};

static std::string toPrometheusLabel(const std::string & stackLabel, const std::string & metricLabel) {
    std::string result;
    result = std::string("srsenb_") + stackLabel + "_" + metricLabel;
    return result;
}


static std::string getEnbSelector(const std::string & enbId) {
    std::string result;
    result = std::string("enb_id=\"") + enbId + "\"";
    return result;
}

 static std::string getUeSelector(const std::string & ueId) {
     std::string result;
     result = std::string("ue_id=\"") + ueId + "\"";
     return result;
 }

 static std::string getDrbSelector(const std::string & drbId) {
	 std::string result;
	 result =std::string("drb_id=\"") + drbId + "\"";
	return result;
 }

 static std::string float_to_string(float f, int digits, int field_width) {
     std::ostringstream os;
     int precision;
     if (isnan(f) or fabs(f) < 0.0001) {
         f = 0.0;
         precision = digits - 1;
     } else {
         precision = digits - (int) (log10f(fabs(f)) - 2 * DBL_EPSILON);
     }

     if (precision == -1) {
         precision = 0;
     }
     os << std::setw(field_width) << std::fixed << std::setprecision(precision) << f;
     return os.str();
 }

 static std::string int_to_hex_string(int value, int field_width) {
     std::ostringstream os;
     os << std::hex << std::setw(field_width) << value;
     return os.str();
 }

 static std::string float_to_eng_string(float f, int digits) {
     const int degree = (f == 0.0) ? 0 : lrint(floor(log10f(fabs(f)) / 3));

     std::string factor;

     if (abs(degree) < 9) {
         if (degree < 0) {
             factor = prefixes[0][abs(degree)];
         } else {
             factor = prefixes[1][abs(degree)];
         }
     } else {
         return "failed";
     }

     const double scaled = f * pow(1000.0, -degree);
     if (degree != 0) {
         return float_to_string(scaled, digits, 5);
     } else {
         return " " + float_to_string(scaled, digits, 5- factor.length());
     }
 }

static int request_handler(void* cls,
    struct MHD_Connection* connection,
    const char* url,
    const char* method,
    const char* version,
    const char* upload_data,
    size_t* upload_data_size,
    void** ptr) {
        cout << "Request: " << url << ", Method: " << method << endl;;

        if (strcmp(method, "GET") != 0) {
        	struct MHD_Response* response = MHD_create_response_from_buffer(0, 0, MHD_RESPMEM_PERSISTENT);
			return MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
        }

        if (strcmp(url, "/metrics") != 0) {
        	struct MHD_Response* response = MHD_create_response_from_buffer(0, 0, MHD_RESPMEM_PERSISTENT);
        	return MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        }

        // only GET /metrics allowed
        metrics_http_scrape* metrics_provider = (metrics_http_scrape*) cls;

        std::string enbId = metrics_provider->getEnbId();
        const enb_metrics_t metrics = metrics_provider->getLatestMetrics();

        std::string resBody;
		resBody = toPrometheusLabel("rf", "error_overflow") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string((int) metrics.rf.rf_o) + "\n";
		resBody += (toPrometheusLabel("rf", "error_underflow") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string((int) metrics.rf.rf_u) + "\n");
		resBody += (toPrometheusLabel("rf", "error_late") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string((int) metrics.rf.rf_l) + "\n");

		resBody += (toPrometheusLabel("system", "cpu_usage") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string(metrics.sys.current_cpu_usage) + "\n");
		resBody += (toPrometheusLabel("system", "io_cpu_usage") + "{" + getEnbSelector(enbId) + "}" + " " + std::to_string(metrics.sys.current_io_usage) + "\n");

		cout << "Number of UEs " << metrics.stack.rrc.ues.size() << endl;

		for (int ueId = 0; ueId < (int) metrics.stack.rrc.ues.size(); ueId++) {
			std::string ue_rnti = int_to_hex_string((int) metrics.stack.mac.ues[ueId].rnti, 4);
			resBody += (toPrometheusLabel("mac", "tx_errors") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + std::to_string(metrics.stack.mac.ues[ueId].tx_errors) + "\n");
			resBody += (toPrometheusLabel("mac", "tx_pkts") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + std::to_string(metrics.stack.mac.ues[ueId].tx_pkts) + "\n");
			resBody += (toPrometheusLabel("mac", "rx_errors") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + std::to_string(metrics.stack.mac.ues[ueId].rx_errors) + "\n");
			resBody += (toPrometheusLabel("mac", "rx_pkts") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + std::to_string(metrics.stack.mac.ues[ueId].rx_pkts) + "\n");
			resBody += (toPrometheusLabel("mac", "dl_cqi") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + float_to_string(SRSRAN_MAX(0.1, metrics.stack.mac.ues[ueId].dl_cqi), 1, 3) + "\n");
			resBody += (toPrometheusLabel("mac", "dl_ri") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + float_to_string(metrics.stack.mac.ues[ueId].dl_ri, 1, 4) + "\n");

			std::string rssi;
			if (not isnan(metrics.phy[ueId].ul.rssi)){
				rssi = float_to_string(SRSRAN_MAX(0.1, metrics.phy[ueId].ul.rssi), 1, 4);
			} else {
				rssi = float_to_string(0, 2, 4);
			}
			resBody += (toPrometheusLabel("phy", "rssi") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + rssi + "\n");

			std::string turbo_iters;
			if (not isnan(metrics.phy[ueId].ul.turbo_iters)){
				turbo_iters = float_to_string(SRSRAN_MAX(0.1, metrics.phy[ueId].ul.turbo_iters), 1, 4);
			} else {
				turbo_iters = float_to_string(0, 2, 4);
			}
			resBody += (toPrometheusLabel("phy", "turbo_iters") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + turbo_iters + "\n");

			std::string mcs;
			if (not isnan(metrics.phy[ueId].dl.mcs)) {
				mcs = float_to_string(SRSRAN_MAX(0.1, metrics.phy[ueId].dl.mcs), 1, 4);
			} else {
				mcs = float_to_string(0, 2, 4);
			}
			resBody += (toPrometheusLabel("mac", "dl_mcs") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + mcs + "\n");

			std::string tx_brate;
			if (metrics.stack.mac.ues[ueId].tx_brate > 0) {
				tx_brate = float_to_eng_string(SRSRAN_MAX(0.1, (float) metrics.stack.mac.ues[ueId].tx_brate / (metrics.stack.mac.ues[ueId].nof_tti * 1e-3)), 1);
			} else {
				tx_brate = float_to_string(0, 1, 6);
			}
			resBody += (toPrometheusLabel("mac", "dl_brate") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + tx_brate + "\n");

			std::stringstream stringStream;

			stringStream << std::setw(5) << metrics.stack.mac.ues[ueId].tx_pkts - metrics.stack.mac.ues[ueId].tx_errors;
			std::string ok = stringStream.str();

			resBody += (toPrometheusLabel("mac", "dl_ok") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + ok + "\n");

			stringStream = stringstream();
			stringStream << std::setw(5) << metrics.stack.mac.ues[ueId].tx_errors;
			std::string not_ok = stringStream.str();
			resBody += (toPrometheusLabel("mac", "dl_not_ok") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + not_ok + "\n");

			std::string percentage;
			if (metrics.stack.mac.ues[ueId].tx_pkts > 0 && metrics.stack.mac.ues[ueId].tx_errors) {
				percentage = float_to_string(SRSRAN_MAX(0.1, (float) 100 * metrics.stack.mac.ues[ueId].tx_errors / metrics.stack.mac.ues[ueId].tx_pkts), 1, 4);
			} else {
				percentage = float_to_string(0, 1, 4);
			}
			resBody += (toPrometheusLabel("mac", "dl_percentage") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + percentage + "\n");

			std::string pusch_sinr;
			if (not isnan(metrics.phy[ueId].ul.pusch_sinr)) {
				pusch_sinr = float_to_string(SRSRAN_MAX(0.1, metrics.phy[ueId].ul.pusch_sinr), 2, 4);
			} else {
				pusch_sinr = float_to_string(0, 1, 4);
			}
			resBody += (toPrometheusLabel("mac", "ul_pusch_sinr") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + pusch_sinr + "\n");

			std::string pucch_sinr;
			if (not isnan(metrics.phy[ueId].ul.pucch_sinr)) {
				pucch_sinr = float_to_string(SRSRAN_MAX(0.1, metrics.phy[ueId].ul.pucch_sinr), 2, 4);
			} else {
				pucch_sinr = float_to_string(0, 1, 4);
			}
			resBody += (toPrometheusLabel("mac", "ul_pucch_sinr") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + pucch_sinr + "\n");

			resBody += (toPrometheusLabel("mac", "phr") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + float_to_string(metrics.stack.mac.ues[ueId].phr, 2, 5) + "\n");

			std::string ul_mcs;
			if (not isnan(metrics.phy[ueId].ul.mcs)) {
				ul_mcs = float_to_string(SRSRAN_MAX(0.1, metrics.phy[ueId].ul.mcs), 1, 4);
			} else {
				ul_mcs = float_to_string(0, 1, 4);
			}
			resBody += (toPrometheusLabel("mac", "ul_mcs") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + ul_mcs + "\n");

			std::string rx_brate;
			if (metrics.stack.mac.ues[ueId].rx_brate > 0) {
				rx_brate = float_to_eng_string(SRSRAN_MAX(0.1, (float) metrics.stack.mac.ues[ueId].rx_brate / (metrics.stack.mac.ues[ueId].nof_tti * 1e-3)), 1);
			} else {
				rx_brate = float_to_string(0, 1, 4);
			}
			resBody += (toPrometheusLabel("mac", "ul_brate") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + rx_brate + "\n");

			stringStream = stringstream();
			stringStream << std::setw(5) << metrics.stack.mac.ues[ueId].rx_pkts - metrics.stack.mac.ues[ueId].rx_errors;
			std::string ul_ok = stringStream.str();
			resBody += (toPrometheusLabel("mac", "ul_ok") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + ul_ok + "\n");

			stringStream = stringstream();
			stringStream << std::setw(5) << metrics.stack.mac.ues[ueId].rx_errors;
			std::string ul_not_ok = stringStream.str();
			resBody += (toPrometheusLabel("mac", "ul_not_ok") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + ul_not_ok + "\n");

			std::string ul_percentage;
			if (metrics.stack.mac.ues[ueId].rx_pkts > 0 && metrics.stack.mac.ues[ueId].rx_errors) {
				ul_percentage = float_to_string(SRSRAN_MAX(0.1, (float) 100 * metrics.stack.mac.ues[ueId].rx_errors / metrics.stack.mac.ues[ueId].rx_pkts), 1, 4);
			} else {
				ul_percentage = float_to_string(0, 1, 4);
			}
			resBody += (toPrometheusLabel("mac", "ul_percentage") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + ul_percentage + "\n");

			resBody += (toPrometheusLabel("mac", "ul_buffer") + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + "} " + float_to_eng_string(metrics.stack.mac.ues[ueId].ul_buffer, 2) + "\n");

			for (const auto& drb : metrics.stack.rrc.ues[ueId].drb_qci_map) {
				const uint32_t bearer_id = drb.first;
				const auto drb_id = std::to_string(bearer_id);

				const uint32_t qci = drb.second;
				resBody += (toPrometheusLabel("bearer", "qci")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(qci) + "\n";

				const auto& rlc_bearer = metrics.stack.rlc.ues[ueId].bearer[bearer_id];
				const auto& pdcp_bearer = metrics.stack.pdcp.ues[ueId].bearer[bearer_id];

				const uint64_t dl_total_bytes = pdcp_bearer.num_tx_acked_bytes;
				resBody += (toPrometheusLabel("bearer", "dl_total_bytes")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(dl_total_bytes) + "\n";

				const uint64_t ul_total_bytes = pdcp_bearer.num_rx_pdu_bytes;
				resBody += (toPrometheusLabel("bearer", "ul_total_bytes")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(ul_total_bytes) + "\n";

				const float dl_latency = pdcp_bearer.tx_notification_latency_ms / 1e3;
				resBody += (toPrometheusLabel("bearer", "dl_latency")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(dl_latency) + "\n";

				const float ul_latency = rlc_bearer.rx_latency_ms / 1e3;
				resBody += (toPrometheusLabel("bearer", "ul_latency")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(ul_latency) + "\n";

				const uint32_t dl_buffered_bytes = pdcp_bearer.num_tx_buffered_pdus_bytes;
				resBody += (toPrometheusLabel("bearer", "dl_buffered_bytes")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(dl_buffered_bytes) + "\n";

				const uint32_t ul_buffered_bytes = rlc_bearer.rx_buffered_bytes;
				resBody += (toPrometheusLabel("bearer", "ul_buffered_bytes")) + "{" + getEnbSelector(enbId) + ", " + getUeSelector(ue_rnti) + ", " + getDrbSelector(drb_id) + "}" + std::to_string(ul_buffered_bytes) + "\n";
			}
		}

		struct MHD_Response* response;
        response = MHD_create_response_from_buffer(resBody.length(), (void *)resBody.c_str(), MHD_RESPMEM_MUST_COPY);
        int result = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return result;
    }


metrics_http_scrape::metrics_http_scrape() {

}

metrics_http_scrape::~metrics_http_scrape() {
    stop();
}

void metrics_http_scrape::init (enb_metrics_interface* m_, const uint32_t bindPort) {
    d = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION,
        bindPort,
        NULL, NULL,
        &srsenb::request_handler, this,
        MHD_OPTION_END);
    if (d != NULL) {
        cout << "HTTP Server started for port " << bindPort << endl;
    }
    m = m_;
}

void metrics_http_scrape::stop() {
    if (d != NULL) {
        MHD_stop_daemon(d);
        cout << "HTTP Server stopped" << endl;
        d = NULL;
    }
}

void metrics_http_scrape::set_handle(const uint32_t enbId_) {
    std::stringstream ss;
    ss << std::string("0x") << std::hex << (int) enbId_ ;

    std::string s = ss.str();
    enbId = s;

    cout  << "enbId: " << enbId << endl;
}

std::string& metrics_http_scrape::getEnbId() {
    return enbId;
}

enb_metrics_t & metrics_http_scrape::getLatestMetrics() {
    metrics = {};
    m->get_metrics(&metrics);
    return metrics;
}

void metrics_http_scrape::set_metrics(const enb_metrics_t& m_, const uint32_t period_usec) {
    metrics = m_;
}

}
