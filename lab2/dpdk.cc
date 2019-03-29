/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */
#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define PACKAGES 10
#define PACKAGESIZE (sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + sizeof(int))
#define DSTMAC 0xffffff
#define SRCIP 0x0a50a8c0
#define DSTIP 0x0650a8c0
#define SRCPORT 1234
#define DSTPORT 1234

static const struct rte_eth_conf port_conf_default = {
    .rxmode = {
        .max_rx_pkt_len = ETHER_MAX_LEN,
    },
};

static void etherpacket(char *packet,
                        struct ether_addr source,
                        struct ether_addr dest)
{
  struct ether_hdr *etherH = (struct ether_hdr *)packet;
  etherH->d_addr = dest;
  etherH->s_addr = source;
  etherH->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
}

static void ipv4packet(char *packet)
{
  struct ipv4_hdr *ipv4H = (struct ipv4_hdr *)(packet);
  ipv4H->version_ihl = 0x45;
  ipv4H->total_length = PACKAGESIZE - sizeof(struct ether_hdr);
  ipv4H->packet_id = 0;
  ipv4H->fragment_offset = 0;
  ipv4H->time_to_live = 100;
  ipv4H->next_proto_id = 0x11;
  ipv4H->src_addr = SRCIP;
  ipv4H->dst_addr = DSTIP;
  ipv4H->hdr_checksum = rte_ipv4_cksum(ipv4H);
}

static void udppacket(char *packet)
{
  struct udp_hdr *UDPH = (struct udp_hdr *)(packet);
  UDPH->src_port = rte_cpu_to_be_16(SRCPORT);
  UDPH->dst_port = rte_cpu_to_be_16(DSTPORT);
  UDPH->dgram_len = rte_cpu_to_be_16(PACKAGESIZE - sizeof(struct ether_hdr) - sizeof(struct ipv4_hdr));
}

static void adddata(char *packet)
{
  int *data = (int *)(packet);
  *data = 0xdead;
}

static void UDPpacket(char *packet,
                      struct ether_addr source,
                      struct ether_addr dest)
{
  etherpacket(packet, source, dest);
  ipv4packet(packet + sizeof(struct ether_hdr));
  udppacket(packet + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));
  adddata(packet + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
}

/* basicfwd.c: Basic DPDK skeleton forwarding example. */
/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
  struct rte_eth_conf port_conf = port_conf_default;
  const uint16_t rx_rings = 1, tx_rings = 1;
  int retval;
  uint16_t q;

  if (port >= rte_eth_dev_count())
    return -1;

  /* Configure the Ethernet device. */
  retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
  if (retval != 0)
    return retval;

  /* Allocate and set up 1 RX queue per Ethernet port. */
  for (q = 0; q < rx_rings; q++)
  {
    retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
                                    rte_eth_dev_socket_id(port), NULL, mbuf_pool);
    if (retval < 0)
      return retval;
  }

  /* Allocate and set up 1 TX queue per Ethernet port. */
  for (q = 0; q < tx_rings; q++)
  {
    retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
                                    rte_eth_dev_socket_id(port), NULL);
    if (retval < 0)
      return retval;
  }

  /* Start the Ethernet port. */
  retval = rte_eth_dev_start(port);
  if (retval < 0)
    return retval;

  /* Display the port MAC address. */
  struct ether_addr addr;
  rte_eth_macaddr_get(port, &addr);
  printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
         " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
         (unsigned)port,
         addr.addr_bytes[0], addr.addr_bytes[1],
         addr.addr_bytes[2], addr.addr_bytes[3],
         addr.addr_bytes[4], addr.addr_bytes[5]);
  /* Enable RX in promiscuous mode for the Ethernet device. */
  rte_eth_promiscuous_enable(port);

  return 0;
}

void writeDst(uint8_t *dst)
{
  dst[0] = DSTMAC & 0xff;
  dst[1] = (DSTMAC >> 8) & 0Xff;
  dst[2] = (DSTMAC >> 16) & 0Xff;
  dst[3] = (DSTMAC >> 24) & 0Xff;
  dst[4] = (DSTMAC >> 32) & 0Xff;
  dst[5] = (DSTMAC >> 40) & 0Xff;
  return;
}
/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int main(int argc, char *argv[])
{
  struct rte_mempool *mbuf_pool;
  unsigned nb_ports;
  uint16_t portid;
  /* Initialize the Environment Abstraction Layer (EAL). */
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
  argc -= ret;
  argv += ret;
  /* Check that there is an even number of ports to send/receive on. */
  nb_ports = rte_eth_dev_count();
  /*if (nb_ports < 2 || (nb_ports & 1))
                rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");
        /* Creates a new mempool in memory to hold the mbufs. */
  mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
                                      MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  if (mbuf_pool == NULL)
    rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
  /* Initialize all ports. */
  //RTE_ETH_FOREACH_DEV(portid)
  portid = 0;
  if (port_init(portid, mbuf_pool) != 0)
    rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n",
             portid);
  if (rte_lcore_count() > 1)
    printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");
  /* Call lcore_main on the master core only. */
  //lcore_main();
  /*
         * Check that the port is on the same NUMA node as the polling thread
         * for best performance.
         */
  for (uint16_t port = 0; port < nb_ports; port++)
    if (rte_eth_dev_socket_id(port) > 0 &&
        rte_eth_dev_socket_id(port) !=
            (int)rte_socket_id())
      printf("WARNING, port %u is on remote NUMA node to "
             "polling thread.\n\tPerformance will "
             "not be optimal.\n",
             port);
  printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
         rte_lcore_id());
  /* Run until the application is quit or killed. */
  struct rte_mbuf *bufs[PACKAGES];
  struct ether_addr src, dst;
  rte_eth_macaddr_get(0, &src);
  writeDst(dst.addr_bytes);

  for (int i = 0; i < PACKAGES; i++)
  {
    if (!(bufs[i] = rte_pktmbuf_alloc(mbuf_pool)))
    {
      rte_exit(EXIT_FAILURE, "allocate package failed\n");
    }

    char *packet = rte_pktmbuf_mtod(bufs[i], char *);
    rte_pktmbuf_append(bufs[i], PACKAGESIZE);
    UDPpacket(packet, src, dst);
  }

  const uint16_t nb_tx = rte_eth_tx_burst(0, 0,
                                          bufs, PACKAGES);
  /* Free any unsent packets. */
  if (unlikely(nb_tx < PACKAGES))
  {
    uint16_t buf;
    for (buf = nb_tx; buf < PACKAGES; buf++)
      rte_pktmbuf_free(bufs[buf]);
  }
  return 0;
}
