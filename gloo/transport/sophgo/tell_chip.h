#ifndef TELL_CHIP
#define TELL_CHIP

#include <asm/ioctl.h>
#include <bmlib_runtime.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define ATTR_FAULT_VALUE (int)0xFFFFFC00
#define ATTR_NOTSUPPORTED_VALUE (int)0xFFFFFC01
#define MAX_NUM_VPU_CORE 5        /* four wave cores */
#define MAX_NUM_VPU_CORE_BM1686 3 /* four wave cores */
#define MAX_NUM_JPU_CORE 4
#define VPP_CORE_MAX 2
#define BMCTL_GET_DRIVER_VERSION _IOR('q', 0x06, unsigned long)
#define BMCTL_GET_SMI_ATTR _IOWR('q', 0x01, unsigned long)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef struct bm_smi_attr {
  int dev_id;
  int chip_id;
  int chip_mode; /*0---pcie; 1---soc*/
  int domain_bdf;
  int status;
  int card_index;
  int chip_index_of_card;

  int mem_used;
  int mem_total;
  int tpu_util;

  int board_temp;
  int chip_temp;
  int board_power;
  u32 tpu_power;
  int fan_speed;

  int vdd_tpu_volt;
  int vdd_tpu_curr;
  int atx12v_curr;

  int tpu_min_clock;
  int tpu_max_clock;
  int tpu_current_clock;
  int board_max_power;

  int ecc_enable;
  int ecc_correct_num;

  char sn[18];
  char board_type[6];

  /* vpu mem and instant info*/
  int vpu_instant_usage[MAX_NUM_VPU_CORE];
  signed long long vpu_mem_total_size[2];
  signed long long vpu_mem_free_size[2];
  signed long long vpu_mem_used_size[2];

  int jpu_core_usage[MAX_NUM_JPU_CORE];

  /*if or not to display board endline and board attr*/
  int board_endline;
  int board_attr;

  int vpp_instant_usage[VPP_CORE_MAX];
  int vpp_chronic_usage[VPP_CORE_MAX];

  bm_dev_stat_t stat;
} BM_SMI_ATTR, *BM_SMI_PATTR;

static struct bm_smi_attr g_attr[64];

/* get attibutes for the specified device*/
static void bm_smi_get_attr(int bmctl_fd, int dev_id) {
  g_attr[dev_id].dev_id = dev_id;
  if (ioctl(bmctl_fd, BMCTL_GET_SMI_ATTR, &g_attr[dev_id]) < 0) {
    g_attr[dev_id].chip_id = ATTR_FAULT_VALUE;
    g_attr[dev_id].status = ATTR_FAULT_VALUE;
    g_attr[dev_id].chip_mode =
        ATTR_FAULT_VALUE;  // 0---pcie = ATTR_FAULT_VALUE; 1---soc
    g_attr[dev_id].domain_bdf = ATTR_FAULT_VALUE;
    g_attr[dev_id].mem_used = 0;
    g_attr[dev_id].mem_total = 0;
    g_attr[dev_id].tpu_util = ATTR_FAULT_VALUE;
    g_attr[dev_id].board_temp = ATTR_FAULT_VALUE;
    g_attr[dev_id].chip_temp = ATTR_FAULT_VALUE;
    g_attr[dev_id].board_power = ATTR_FAULT_VALUE;
    g_attr[dev_id].tpu_power = ATTR_FAULT_VALUE;
    g_attr[dev_id].fan_speed = ATTR_FAULT_VALUE;
    g_attr[dev_id].vdd_tpu_volt = ATTR_FAULT_VALUE;
    g_attr[dev_id].vdd_tpu_curr = ATTR_FAULT_VALUE;
    g_attr[dev_id].atx12v_curr = ATTR_FAULT_VALUE;
    g_attr[dev_id].tpu_min_clock = ATTR_FAULT_VALUE;
    g_attr[dev_id].tpu_max_clock = ATTR_FAULT_VALUE;
    g_attr[dev_id].tpu_current_clock = ATTR_FAULT_VALUE;
    g_attr[dev_id].board_max_power = ATTR_FAULT_VALUE;
    g_attr[dev_id].ecc_enable = ATTR_FAULT_VALUE;
    g_attr[dev_id].ecc_correct_num = ATTR_FAULT_VALUE;
    g_attr[dev_id].card_index = ATTR_FAULT_VALUE;
  }
}

static void bm_smi_fetch_all(int fd, int dev_cnt, int start_dev) {
  for (int i = start_dev; i < start_dev + dev_cnt; i++) {
    bm_smi_get_attr(fd, i);
    if (dev_cnt == 1) {
      g_attr[i].board_endline = 1;
      g_attr[i].board_attr = 1;
    } else if ((i > start_dev) &&
               (g_attr[i - 1].card_index) == (g_attr[i].card_index)) {
      g_attr[i].board_endline = 1;
      g_attr[i].board_attr = 0;
      g_attr[i - 1].board_endline = 0;
    } else {
      g_attr[i].board_endline = 1;
      g_attr[i].board_attr = 1;
    }
    if (g_attr[i].chip_id == ATTR_FAULT_VALUE) {
      printf("error! [%d]chip_id is ATTR_FAULT_VALUE\r\n", i);
      continue;
    }
    if (g_attr[i].chip_id == ATTR_NOTSUPPORTED_VALUE) {
      printf("error! [%d]chip_id is ATTR_NOTSUPPORTED_VALUE\r\n", i);
      continue;
    }
    printf("chip[0x%X]:%d -> card:%d[%d] mode:%s\r\n", g_attr[i].chip_id, i,
           g_attr[i].card_index, g_attr[i].chip_index_of_card,
           g_attr[i].chip_mode == 0 ? "pcie" : "soc");
  }
}

int get_card_id(int fd, int dev_id) {
  bm_smi_get_attr(fd, dev_id);
  return g_attr[dev_id].card_index;
}

int bm_smi_open_bmctl(int* driver_version) {
  char dev_ctl_name[20];
  int fd;
  snprintf(dev_ctl_name, sizeof(dev_ctl_name), "/dev/bmdev-ctl");
  fd = open(dev_ctl_name, O_RDWR);
  if (fd == -1) {
    perror("no sophon device found on this PC or Server\n");
    exit(EXIT_FAILURE);
  } else {
    if (ioctl(fd, BMCTL_GET_DRIVER_VERSION, driver_version) < 0)
      *driver_version = 1 << 16;
  }
  return fd;
}

bool is_on_same_card(int dev1, int dev2) {
  int g_driver_version = 0;
  int fd = 0;
  int dev_cnt = 1;
  const char* CNfilename = "/proc/bmsophon/chip_num";
  FILE* file = fopen(CNfilename, "r");
  if (file != NULL) {
    if (fscanf(file, "%d", &dev_cnt) != 1) {
      perror("Error reading chip_num file");
      fclose(file);
      dev_cnt = 1;
    }
    fclose(file);
  }
  fd = bm_smi_open_bmctl(&g_driver_version);
  // dev1
  int card_id1 = get_card_id(fd, dev1);
  // dev2
  int card_id2 = get_card_id(fd, dev2);
  return card_id1 == card_id2;
}



// HOW TO USE: ./tell_chip dev_id1 dev_id2
// int main(int argc, char* argv[]) {
//   int g_driver_version = 0;
//   int fd = 0;
//   int dev_cnt = 1;
//   const char* CNfilename = "/proc/bmsophon/chip_num";
//   FILE* file = fopen(CNfilename, "r");
//   if (file != NULL) {
//     if (fscanf(file, "%d", &dev_cnt) != 1) {
//       perror("Error reading chip_num file");
//       fclose(file);
//       dev_cnt = 1;
//     }
//     fclose(file);
//   }
//   //   if (argc > 1) dev_cnt = atoi(argv[1]);
//   int dev1 = atoi(argv[1]);
//   int dev2 = atoi(argv[2]);
//   fd = bm_smi_open_bmctl(&g_driver_version);
//   //   printf("#Driver Version:0x%X\r\n", g_driver_version);
//   //   bm_smi_fetch_all(fd, dev_cnt, 0);
//   bool ans = is_on_same_card(fd, dev1, dev2);
//   printf("dev%d and dev%d is on the same card?: %d\n", dev1, dev2, ans);
//   return 0;
// }

#endif