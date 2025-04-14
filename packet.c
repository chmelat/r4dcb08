/*
 *  Function for packet handling in rtu modbus protocol
 *  V1.0/2025-01-30
 *  V1.1/2025-03-28 Adaptation for R4DCB08 device
 */
#include <stdio.h>  /* Standard input/output definitions */
#include <stdlib.h>  /* Standard library */
#include <unistd.h>  /* UNIX standard function definitions */
#include <stdint.h>  /* Specific width integer types */

#include "typedef.h"  /* definice datovych typu */
#include "packet.h"  /* deklarace globalnich fci */

/* Maximalni delka dat DMAX je definovan v typedef.h */

/*
 *  Funkcni prototypy lokalnich fci
 */
static uint16_t CRC16_2(uint8_t *buf, int len);

/**********************************************************************/

/*
 *  Packet received
 *  mode 0 packet from temperature read
 *       1 aknown packet from set temp. correction or baudrate
 */
int received_packet (int fd, PACKET *p_RP, int mode)
{
  char msg[] = "received_packet";
  uint8_t buf[DMAX+4];
  uint8_t tmp[1];
  int res;  /* Number of read data */
  int i=1;
  int j;
  uint16_t crc_c;  /* Compute  CRC */
  int len;

/* Wait for first character of packet (adr) */
  for(;;) {
    res = read(fd, tmp, 1);
    if (res == 1) {
      buf[0] = tmp[0];
      break;
    }
  }

  if (mode == 0) {  
/* Read two bytes (fce + len) */  
  while ( i < 3 ) {
    res = read(fd, tmp, 1);
    if (res == 1) {
      buf[i++] = tmp[0];
    } else
      break;
  }
  len = buf[2];

/* Nacteni zbytku ramce (data + crc) */
  while ( i < len+5 ) {
    res = read(fd, tmp, 1);
    if (res == 1) {
      buf[i++] = tmp[0];
    } else
      break;
  }
  if (i != len+5) {
    fprintf(stderr, "%s: Incorrect data lenght, mode 0!\n",msg);
    return (-1);
  }
  } else if (mode == 1) {
/* Nacteni zbytku ramce */
    while ( i < 8 ) {
      res = read(fd, tmp, 1);
      if (res == 1) {
        buf[i++] = tmp[0];
      } else
        break;
    }
    if (i != 8) {
      fprintf(stderr, "%s: Incorrect data lenght, mode 1!\n",msg);
      return (-1);
    }
    len = 2;
  } else {
    return -2; /* unknown mode */
  }


  p_RP->addr = buf[0];
  p_RP->inst = buf[1];
  p_RP->len = len;
  for (j=0; j<len; j++) {
    if (mode == 0)
      p_RP->data[j] = buf[j+3];
    else
      p_RP->data[j] = buf[j+4];
  }
  p_RP->CRC = UINT16(buf[i-2],buf[i-1]);

/* CRC test */ 
  if (mode == 0) 
    crc_c = CRC16_2(buf, len+3);
  else
    crc_c = CRC16_2(buf, len+4);

  if (crc_c != p_RP->CRC) {
    fprintf(stderr, "%s: Incorrect CRC, %x != %x!\n",msg, crc_c, p_RP->CRC);
    print_packet(p_RP);
    return (-1);
  }

/* Prodleva, jinak se pri nulovem case cteni hrouti  */  
  usleep((useconds_t)(8E3));

  return 0;
}

/*
 *  Send packet 'p_SP' to device 'fd' (write)
 */
int send_packet (int fd, PACKET *p_SP)
{
  char msg[] = "send_packet";
  uint8_t buf[DMAX+4];  /* Buffer with send data */
  int i;

  if (p_SP->len > DMAX) {
    fprintf(stderr, "%s: Overflow length of data (%u > %d).\n", msg, p_SP->len, DMAX);
    return (-1);
  }

/*  Packet formulation  */
  buf[0] = p_SP->addr;  /* Address */
  buf[1] = p_SP->inst;  /* Instruction */
  for (i = 0; i < p_SP->len; i++)
    buf[i+2] = p_SP->data[i];
  buf[p_SP->len+2] = ( p_SP->CRC & 0xFF ); /* CRC_lo */
  buf[p_SP->len+3] = ( p_SP->CRC >> 8 );   /* CRC_hi */

/* Write packet to fd */  
  if (write (fd, buf, p_SP->len+4) < 0) {
    fprintf(stderr, "%s: It is impossible send packet to port.\n", msg);
    return (-1);
  }
  else
    return p_SP->len;  /* Lenght of corect send data */
}

/*
 *  Packet assembly
 */
void form_packet (uint8_t addr, uint8_t inst, uint8_t *p_data,
                  int len, PACKET *p_Packet)
{
  int i;
  uint8_t buf[DMAX+4];  /* Buffer */
  uint16_t crc_c; /* CRC */
  
  p_Packet->addr = addr;  /* Address */
  p_Packet->inst = inst;    /* Instruction code */
  p_Packet->len = len;    /* Lenght of data */
  buf[0] = addr;
  buf[1] = inst;
  for (i = 0; i < len; i++) {     /* Data */
    p_Packet->data[i] = *p_data;
    buf[i+2] = *p_data++;
  }

  crc_c = CRC16_2(buf, i+2); /* CRC */
  p_Packet->CRC = crc_c;
}

/*
 *  Print packet 'p_P' on terminal
 */
void print_packet(PACKET *p_P)
{
  int i;

  printf("ADR INS DATA... CRC CRC\n");
  printf("%02X %02X ",p_P->addr, p_P->inst);
  for (i = 0; i<p_P->len; i++)
    printf("%02X ", p_P->data[i]);
  printf("%02X %02X\n",(p_P->CRC) & 0xFF, (p_P->CRC) >> 8);
}


/* Local functions */

/*
 *  Modbus CRC
 */

static uint16_t CRC16_2(uint8_t *buf, int len)
{ 
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
  crc ^= (uint16_t)buf[pos];    // XOR byte into least sig. byte of crc

  for (int i = 8; i != 0; i--) {    // Loop over each bit
    if ((crc & 0x0001) != 0) {      // If the LSB is set
      crc >>= 1;                    // Shift right and XOR 0xA001
      crc ^= 0xA001;
    }
    else                            // Else LSB is not set
      crc >>= 1;                    // Just shift right
    }
  }
  return crc;
}
