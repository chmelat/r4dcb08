/*
 *  Function send instruction 'inst' with argument 'arg[]' length 'in_dat'.
 *  Return pointer to output data, length 'out_dat'.
 *  'verb' {1,0} - write " ... OK" acknowledge or silent output
 *  V1.0/2016-02-08
 *  V1.1/2019-10-28
 *  V1.2/2019-10-30  Autonomic file
 *  V1.3/2019-10-31  Add p_r (pointer to received packet) and  verb as parameter
 */

#include <stdlib.h>  /* Standard lib */
#include <stdio.h>  /* Standard input/output definitions */
#include <stdint.h>  /* Standard input/output definitions */

#include "typedef.h"  /* Definice datovych typu */
#include "packet.h"  /* Deklarace packetovych fci */

/*
 *  Global variable
 */

/*
 *  Local function declaration
 */

/*
 *  Monada
 *  mode 0 Read temperature
 *       1 Write new address to device
 */

uint8_t *monada(int fd, uint8_t adr, uint8_t inst, int in_len, /*@null@*/ uint8_t *arg, /*@out@*/ PACKET *p_r, int verb, char *msg, int mode)
{
  #if (VERBOSE == 1)
    char lmsg[] = "monada"; /* Local message */
  #endif 
  static uint8_t data[DMAX];  /* Array of data */
  uint8_t *p_data = data;
  static PACKET ps;  /* Send packet */
  static PACKET pr;  /* Received packet */
  PACKET *p_ps = &ps;
  PACKET *p_pr = &pr;
  int i;

  if (in_len > DMAX) {
    fprintf(stderr,"In %s too length data: %d > %d!\n",msg,in_len,DMAX);
    exit (EXIT_FAILURE);	  
  }

  for (i=0; i<in_len; i++)
    data[i] = *(arg++);

  form_packet(adr, inst, p_data, in_len, p_ps);
 
  #if (VERBOSE == 1)
    printf("%s: Send packet:\n", lmsg);
    print_packet(p_ps);
  #endif

  if (send_packet(fd, p_ps) < 0) {
    fprintf(stderr,"In %s send error!\n", msg);
    exit (EXIT_FAILURE);	  
  }
  if (received_packet(fd, p_pr, mode) < 0) {
    fprintf(stderr,"In %s received error!\n", msg);	  
    exit (EXIT_FAILURE);
  }

  #if (VERBOSE == 1)
    printf("%s: Received packet:\n", lmsg);
    print_packet(p_pr);
  #endif 

  if (verb)
    printf("%s ... OK\n", msg);

  *p_r = pr;
  return p_pr->data;
}
