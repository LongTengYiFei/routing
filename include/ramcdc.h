#ifndef  RAMCDC_H
#define  RAMCDC_H

void ramcdc_init(int);
int ramcdc(const char *p, int n);
int ramcdc_avx_256(const char *p, int n);
int ramcdc_avx_512(const char *p, int n);
#endif