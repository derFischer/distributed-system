#include "rte_common.h"
#include "rte_mbuf.h"
#include "rte_meter.h"
#include "rte_red.h"

#include "qos.h"

#define CIR0 0
#define CBS0 0
#define EBS0 0

#define CIR1 0
#define CBS1 0
#define EBS1 0

#define CIR2 0
#define CBS2 0
#define EBS2 0

#define CIR3 0
#define CBS3 0
#define EBS3 0

#define GREENWQ 0
#define GREENMINTH 0
#define GREENMAXTH 0
#define GREENMAXP 0

#define YELLOWWQ 0
#define YELLOWMINTH 0
#define YELLOWMAXTH 0
#define YELLOWMAXP 0

#define REDWQ 0
#define REDMINTH 0
#define REDMAXTH 0
#define REDMAXP 0

struct rte_meter_srtcm_params srtcmParams[APP_FLOWS_MAX];
struct rte_meter_srtcm srtcmFlows[APP_FLOWS_MAX];

/**
 * srTCM
 */
void qos_param_init(uint64_t CIR, uint64_t CBS, uint64_t EBS, int index)
{
    srtcmParams[index].cir = CIR;
    srtcmParams[index].cbs = CBS;
    srtcmParams[index].ebs = EBS;
    rte_meter_srtcm_config(&srtcmFlows[index], &srtcmParams[index]);
}

int qos_meter_init(void)
{
    /* to do */
    qos_param_init(CIR0, CBS0, EBS0, 0);
    qos_param_init(CIR1, CBS1, EBS1, 1);
    qos_param_init(CIR2, CBS2, EBS2, 2);
    qos_param_init(CIR3, CBS3, EBS3, 0);
    return 0;
}

enum qos_color
qos_meter_run(uint32_t flow_id, uint32_t pkt_len, uint64_t time)
{
    /* to do */
    return rte_meter_srtcm_color_blind_check(&strcmFlows[flow_id], time, pkt_len);
}

/**
 * WRED
 */
struct rte_red_config redParams[e_RTE_METER_COLORS];
struct rte_red redFlows[APP_FLOWS_MAX][e_RTE_METER_COLORS];

void ret_params_init(const uint16_t wq_log2, const uint16_t min_th, const uint16_t max_th, const uint16_t maxp_inv, enum qos_color color)
{
    rte_red_config_init(&redParams[color], wq_log2, min_th, max_th, maxp_inv);
    for (int i = 0; i < APP_FLOWS_MAX; i++)
    {
        rte_red_rt_data_init(&app_red[i][color]);
    }
}

int qos_dropper_init(void)
{
    /* to do */
    ret_params_init(GREENWQ, GREENMINTH, GREENMAXTH, GREENMAXP, GREEN);
    ret_params_init(YELLOWWQ, YELLOWMINTH, YELLOWMAXTH, YELLOWMAXP, YELLOW);
    ret_params_init(REDWQ, REDMINTH, REDMAXTH, REDMAXP, RED);

}

uint64_t time;
unsigned queue[APP_FLOWS_MAX][e_RTE_METER_COLORS];
int qos_dropper_run(uint32_t flow_id, enum qos_color color, uint64_t time)
{
    /* to do */
    if (app_time_stamp != time)
    {
        app_time_stamp = time;
        red_queue_clear(time);
    }

    if (!rte_red_enqueue(&redParams[color], &redFlows[flow_id][color], queue[flow_id][color], time))
    {
        queue[flow_id][color] += 1;
    }
    return 0;
}