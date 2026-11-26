#include "metrics.h"
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
int cpu_ticks(unsigned long long *total,unsigned long long *idle){
 FILE *f=fopen("/proc/stat","r"); if(!f)return 0;
 unsigned long long u,n,s,i,w,h,q,t;
 int ok=fscanf(f,"cpu %llu %llu %llu %llu %llu %llu %llu %llu",&u,&n,&s,&i,&w,&h,&q,&t)==8; fclose(f);
 if(ok){*total=u+n+s+i+w+h+q+t; *idle=i+w;} return ok;
}
int memory_info(double *ram,double *swap){
 FILE *f=fopen("/proc/meminfo","r"); if(!f)return 0;
 char line[256],key[64]; unsigned long long v,mt=0,ma=0,st=0,sf=0; int available=0;
 while(fgets(line,sizeof line,f))if(sscanf(line,"%63s %llu",key,&v)==2){
 if(!strcmp(key,"MemTotal:"))mt=v; else if(!strcmp(key,"MemAvailable:")){ma=v;available=1;} else if(!strcmp(key,"SwapTotal:"))st=v; else if(!strcmp(key,"SwapFree:"))sf=v; }
 fclose(f); if(!mt||!available)return 0; *ram=100.0*(mt-ma)/mt; *swap=st?100.0*(st-sf)/st:0; return 1;
}
int disk_info(double *used){struct statvfs s; if(statvfs("/",&s)||!s.f_blocks)return 0; *used=100.0*(s.f_blocks-s.f_bfree)/s.f_blocks; return 1;}
double uptime_seconds(void){double v=-1; FILE *f=fopen("/proc/uptime","r"); if(f){if(fscanf(f,"%lf",&v)!=1)v=-1; fclose(f);}return v;}
