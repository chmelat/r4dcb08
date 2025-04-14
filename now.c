/*
 *  Aktualni datum a cas, ISO 8601
 */
#include <stdlib.h>  /* Standard lib */
#include <stdio.h>  /* Standard input/output definitions */
#include <time.h>  /* Time function */
#include <sys/time.h>  /* Time function */

char *now(void)
{
  struct tm  *ts;  /* Cas */
  static char buf[40];  /* Textova promenna pro zapis casu */
  struct timeval now;  /* Argument fce gettimeofday */

/* Nacteni casu a jeho zapis do promenne buf v pozadovanem formatu */
  if (gettimeofday(&now, NULL) == -1) {; /* vterin od r. 1970 a ppm sec*/
    fprintf(stderr, "now: Chyba pri volani gettimeofday(&now,NULL)\n");
    exit (EXIT_FAILURE);
  }
  ts = localtime(&now.tv_sec);  /* Prevod vterin na casovou strukturu */
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ts);  /* Konverze casove struktury */
  sprintf(buf, "%s.%02u", buf, (unsigned)(now.tv_usec/1E4));  /* Pripojeni desetinne casti*/

  return buf;
}
