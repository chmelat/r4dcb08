/*
 *  Data type definition
 *  V1.0/2025-01-29 
 */

#define DMAX 253  /* Maximal data length */
#define UINT16(lo,hi) ( (uint16_t)(lo) + 256*(uint16_t)(hi) ) /* For CRC from buf */
#define INT16(lo,hi) ( (int16_t)((uint16_t)(lo) + 256*(uint16_t)(hi)) ) 

#define VERBOSE 0

/*
 *  Modbus RTU frame
 */
typedef struct {
  uint8_t addr;           /* Address (8 bit) */
  uint8_t inst;           /* Instruction (8 bit) */
  uint8_t len;            /* Data lenght */ 
  uint8_t data[DMAX+2];   /* Data */
  uint16_t CRC;           /* CRC (16 bit) */
} PACKET;
