
#ifndef _NEO_DATE_H_
#define _NEO_DATE_H_ 1

/* UTC time_t -> struct tm in local timezone */
void neo_time_expand (const time_t tt, char *timezone, struct tm *ttm);

/* local timezone struct tm -> time_t UTC */
time_t neo_time_compact (struct tm *ttm, char *timezone);

#endif /* _NEO_DATE_H_ */
