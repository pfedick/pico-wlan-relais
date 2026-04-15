#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Allgemeine lwIP-Optionen
#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_ARP 1
#define LWIP_ETHERNET 1
#define LWIP_ICMP 1
#define LWIP_RAW 1
#define TCP_MSS 1460
#define TCP_WND (8 * TCP_MSS)
#define TCP_SND_BUF (8 * TCP_MSS)
#define TCP_SND_QUEUELEN 16
#define LWIP_HTTPD_CGI 1
#define LWIP_HTTPD_SSI 1

// Speicher-Optimierungen
#define MEM_LIBC_MALLOC 0
#define MEM_ALIGNMENT 4
#define MEM_SIZE 16000
#define MEMP_NUM_TCP_SEG 32
#define PBUF_POOL_SIZE 24

// TCP/IP Optionen
#define LWIP_CHKSUM_ALGORITHM 3
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_DNS 1
#define LWIP_TCP_KEEPALIVE 1

// DHCP
#define LWIP_DHCP 1

// Erforderlich für pico_cyw43_arch_lwip_threadsafe_background
#define MEMCPY(dst, src, len) memcpy(dst, src, len)
#define SMEMCPY(dst, src, len) memcpy(dst, src, len)

#endif