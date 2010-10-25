/* Copyright (C) 2010 Zsolt Sz Sztupák
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "install.h"
#include "minui/minui.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"

#include "extendedcommands.h"
#include "commands.h"
#include "lagfixutils.h"
#include "nandroid.h"

extern char **environ;

static char* startval[] = {"DATA_FS=","CACHE_FS=","DBDATA_FS=","DATA_LOOP=","CACHE_LOOP=","DBDATA_LOOP=","BIND_DATA_TO_DBDATA=", NULL};
static int catvals[] = { 3,6,7,999 };
static char* categories[][5] = {
    {"rfs","jfs","ext4",NULL,NULL},
    {"false","ext2",NULL,NULL},
    {"false","data",NULL,NULL},
    {NULL,NULL,NULL,NULL}
  };



void advanced_lagfix_menu() {
  static char* headers[] = { "Advanced Lagfix Menu",
                             "", NULL };

  int chosen_item = 0;
  for (;;)
    {
        char* list[8];
        FILE* f = fopen("/system/etc/lagfix.conf","r");
        int i=0;
        for (i=0;i<7;i++) {
          list[i] = malloc(64);
        }
        list[7] = NULL;
        if (f==NULL) {
          LOGE("Could not open lagfix.conf!\n");
        } else {
          ui_print("Current configuration is:\n");
          i=0;
          while ((i<7) && (fgets(list[i],63,f))) {
            ui_print(list[i]);
            list[i][strlen(list[i])-1]='\0'; // remove trailing newline
            i++;
          }
          fclose(f);
        }

        chosen_item = get_menu_selection(headers, list, 0);
        if (chosen_item == GO_BACK)
            break;

        int category=0;
        while (chosen_item>=catvals[category]) {
          category++;
        }

        ui_print("%d\n",category);

        int curval=0;
        while ((categories[category][curval]) && (memcmp(list[chosen_item]+strlen(startval[chosen_item]),categories[category][curval],strlen(categories[category][curval]))!=0))
          curval++;

        ui_print("%d\n",curval);

        if (categories[category][curval]) {
          curval++;
          if (!categories[category][curval]) {
            curval = 0;
          }
        } else {
          curval=0;
        }

        ui_print("%d\n",curval);

        sprintf(list[chosen_item],"%s%s",startval[chosen_item],categories[category][curval]);

        f = fopen("/system/etc/lagfix.conf","w+");
        for (i=0; i<7; i++) {
          fprintf(f,"%s\n",list[i]);
        }
        fclose(f);
    }
}

void show_lagfix_menu() {
  static char* headers[] = {  "Lagfix Menu",
                              "d: /data; o: /cache+/dbdata; a: d+o",
                              "binds: /data/data moved to dbdata",
                              NULL
  };

  static char* list[] = { "Disable lagfix",
                          "Use OCLF (d=rfs+e2;o=rfs)",
                          "Use Voodoo (d=e4;o=rfs)",
                          "Use JFS (d=jfs;o=rfs)",
                          "Use NO-RFS standard (d=rfs+e2;o=e4)",
                          "Use NO-RFS advanced (a=e4)",
                          "Use NO-RFS advanced JFS (a=jfs)",
                          "Use NO-RFS overkill (a=e4+e2;binds)",
                          "Use NO-RFS overkill JFS (a=e4+e2;binds)",
                          "Advanced options",
                          NULL
    };

    for (;;)
    {
        FILE* f = fopen("/system/etc/lagfix.conf","r");
        if (f==NULL) {
          LOGE("Could not open lagfix.conf!");
        } else {
          char buf[64];
          ui_print("\n\nCurrent configuration is:\n");
          while (fgets(buf,63,f)) {
            ui_print(buf);
          }
          fclose(f);
        }

        int chosen_item = get_menu_selection(headers, list, 0);
        if (chosen_item == GO_BACK)
            break;
        switch (chosen_item)
        {
          case 0:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=rfs\nCACHE_FS=rfs\nDBDATA_FS=rfs\nDATA_LOOP=false\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 1:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=rfs\nCACHE_FS=rfs\nDBDATA_FS=rfs\nDATA_LOOP=ext2\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 2:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=ext4\nCACHE_FS=rfs\nDBDATA_FS=rfs\nDATA_LOOP=false\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 3:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=jfs\nCACHE_FS=rfs\nDBDATA_FS=rfs\nDATA_LOOP=false\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 4:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=rfs\nCACHE_FS=ext4\nDBDATA_FS=ext4\nDATA_LOOP=ext2\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 5:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=ext4\nCACHE_FS=ext4\nDBDATA_FS=ext4\nDATA_LOOP=false\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 6:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=jfs\nCACHE_FS=jfs\nDBDATA_FS=jfs\nDATA_LOOP=false\nCACHE_LOOP=false\nDBDATA_LOOP=false\nBIND_DATA_TO_DBDATA=false\n");fclose(f);break;
          case 7:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=ext4\nCACHE_FS=ext4\nDBDATA_FS=ext4\nDATA_LOOP=ext2\nCACHE_LOOP=ext2\nDBDATA_LOOP=ext2\nBIND_DATA_TO_DBDATA=data\n");fclose(f);break;
          case 8:f = fopen("/system/etc/lagfix.conf","w+");fprintf(f,"DATA_FS=jfs\nCACHE_FS=jfs\nDBDATA_FS=jfs\nDATA_LOOP=ext2\nCACHE_LOOP=ext2\nDBDATA_LOOP=ext2\nBIND_DATA_TO_DBDATA=data\n");fclose(f);break;
          case 9:advanced_lagfix_menu(); break;
        }
    }
}

int searchfor_in_config_file(const char* searchfor, int category) {
  int res = -1;
  char buf[64]; 
  FILE *f = fopen("/system/etc/lagfix.conf.old","r");
  if (!f) return -1;
  while (fgets(buf,63,f)) {
    if (memcmp(searchfor,buf,strlen(searchfor))==0) {
      res++;
      while (categories[category][res] && (memcmp(buf+strlen(searchfor),categories[category][res],strlen(categories[category][res]))!=0)) {
        res++;
      }
      if (!categories[category][res]) res=-1;
    }
  }
  fclose(f);
  return res;

}

int get_loop_options(const char * name) {
  char searchfor[64];
  sprintf(searchfor,"%s_LOOP=",name);
  return searchfor_in_config_file(searchfor,1);
}

int get_fs_options(const char * name) {
  char searchfor[64];
  sprintf(searchfor,"%s_FS=",name);
  return searchfor_in_config_file(searchfor,0);
}

int dont_use_bind_options = 0;
int get_bind_options() {
  if (dont_use_bind_options) return 0;
  return searchfor_in_config_file("BIND_DATA_TO_DBDATA=",2);
}

void mount_block(const char* name, const char* blockname, const char* loopblock, const char* destnoloop, const char* destloop) {
  int getfsopts,getloopopts,bindopts;
  getfsopts = get_fs_options(name);
  getloopopts = get_loop_options(name);
  char tmp[256];
  if (getloopopts) {
    sprintf(tmp,"mkdir %s",destloop);__system(tmp);
    sprintf(tmp,"chmod 700 %s",destloop);__system(tmp);
    if (getfsopts==0) {
      sprintf(tmp,"mount -t rfs -o nosuid,nodev,check=no %s %s",blockname,destloop);
    } else if (getfsopts==1) {
      sprintf(tmp,"mount -t jfs -o noatime,nodiratime %s %s",blockname,destloop);
    } else if (getfsopts==2) {
      sprintf(tmp,"mount -t ext4 -o noatime,barrier=0,noauto_da_alloc %s %s",blockname,destloop);
    } else if (getfsopts==3) {
      sprintf(tmp,"mount -t ext2 -o noatime,nodiratime %s %s",blockname,destloop);
    }
    __system(tmp);
    sprintf(tmp,"losetup %s %s/.extfs",loopblock,destloop);__system(tmp);
    sprintf(tmp,"mount -t ext2 %s %s",loopblock,destnoloop);__system(tmp);
  } else {
    if (getfsopts==0) {
      sprintf(tmp,"mount -t rfs -o nosuid,nodev,check=no %s %s",blockname,destnoloop);
    } else if (getfsopts==1) {
      sprintf(tmp,"mount -t jfs -o noatime,nodiratime %s %s",blockname,destnoloop);
    } else if (getfsopts==2) {
      sprintf(tmp,"mount -t ext4 -o noatime,barrier=0,noauto_da_alloc %s %s",blockname,destnoloop);
    } else if (getfsopts==3) {
      sprintf(tmp,"mount -t ext2 -o noatime,nodiratime %s %s",blockname,destnoloop);
    }
    __system(tmp);
  }
}

// there should be some checks whether the action actually succeded
int ensure_lagfix_mount_points(const RootInfo *info) {
  int bindopts;
  bindopts = get_bind_options();
  if (strcmp(info->name,"DATA:")==0) {
    mount_block("DATA","/dev/block/mmcblk0p2","/dev/block/loop1","/data","/res/odata");
    if (bindopts) {
      ensure_root_path_mounted("DATADATA:");
      __system("mkdir -p /dbdata/.data/data");
      //__system("mkdir -p /dbdata/.data/dalvik-cache");
      __system("mkdir -p /data/data");
      //__system("mkdir -p /data/dalvik-cache");
      __system("mount -o bind /dbdata/.data/data /data/data");
      //__system("mount -o bind /dbdata/.data/dalvik-cache /data/dalvik-cache");
    }
  } else if (strcmp(info->name,"DATADATA:")==0) {
    mount_block("DBDATA","/dev/block/stl10","/dev/block/loop2","/dbdata","/res/odbdata");
  } else if (strcmp(info->name,"CACHE:")==0) {
    mount_block("CACHE","/dev/block/stl11","/dev/block/loop3","/cache","/res/ocache");
  } else {
    return 1;
  }
  return 0;
}

// not implemented
int ensure_lagfix_unmount_points(const RootInfo *info) {
  if (strcmp(info->name,"DATA:")==0) {
    return -1;
  } else if (strcmp(info->name,"DATADATA:")==0) {
    return -1;
  } else if (strcmp(info->name,"CACHE:")==0) {
    return -1;
  } else return 1;
}

int ensure_lagfix_formatted(const RootInfo *info) {
  // we won't remove hidden files in root yet
  if (strcmp(info->name,"DATA:")==0) {
    __system("rm -rf /data/*");
    return 0;
  } else if (strcmp(info->name,"DATADATA:")==0) {
    __system("rm -rf /dbdata/*");
    return 0;
  } else if (strcmp(info->name,"CACHE:")==0) {
    __system("rm -rf /cache/*");
    return 0;
  } else return 1;
}

int unmount_all_lagfixed() {
  sync();
  __system("umount -f /data/data");
  __system("umount -f /data/dalvik-cache");
  __system("umount -f -d /dev/block/loop3");
  __system("umount -f -d /dev/block/loop2");
  __system("umount -f -d /dev/block/loop1");
  __system("umount -f /res/ocache");
  __system("umount -f /res/odata");
  __system("umount -f /res/odbdata");
  __system("umount -f /cache");
  __system("umount -f /data");
  __system("umount -f /dbdata");
  return 0;
}

int create_lagfix_partition(int id) {
  char loopname[64],blockname[64];
  char looppos[64],blockpos[64];
  char name[64];
  int loopsize;
  if (id==0) {
    strcpy(loopname,"/dev/block/loop1");
    strcpy(blockname,"/dev/block/mmcblk0p2");
    strcpy(looppos,"/res/odata");
    strcpy(blockpos,"/data");
    strcpy(name,"DATA");
    loopsize = 1831634944;
  } else if (id==1) {
    strcpy(loopname,"/dev/block/loop2");
    strcpy(blockname,"/dev/block/stl10");
    strcpy(looppos,"/res/odbdata");
    strcpy(blockpos,"/dbdata");
    strcpy(name,"DBDATA");
    loopsize = 128382976;
  } else {
    strcpy(loopname,"/dev/block/loop3");
    strcpy(blockname,"/dev/block/stl11");
    strcpy(looppos,"/res/ocache");
    strcpy(blockpos,"/cache");
    strcpy(name,"CACHE");
    loopsize = 29726720;
  }
  int ft = get_fs_options(name);
  int loop = get_loop_options(name);
  char tmp[256];
  if (ft==0) {
    if (id==0) {
      sprintf(tmp,"/sbin/fat.format -l %s -F 32 -S 4096 -s 4 %s",name,blockname);
      // we can't create small partitions that are valid as rfs with fat.format, so we'll use some compressed pre-made valid rfs images
    } else {
      if (id==1) {
        sprintf(tmp,"gunzip -c /res/misc/dbdata.rfs.gz | dd of=%s",blockname);
      } else {
        sprintf(tmp,"gunzip -c /res/misc/cache.rfs.gz | dd of=%s",blockname);
      }
    }
    __system(tmp);
  } else if (ft==1) {
    sprintf(tmp,"/sbin/mkfs.jfs -L %s %s",name,blockname);
    __system(tmp);
  } else if (ft==2) {
    sprintf(tmp,"/sbin/mkfs.ext4 -L %s -b 4096 -m 0 -F %s",name,blockname);
    __system(tmp);
  } else if (ft==3) {
    sprintf(tmp,"/sbin/mkfs.ext2 -L %s -b 4096 -m 0 -F %s",name,blockname);
    __system(tmp);
  }

  if (loop) {
    sprintf(tmp,"mount %s %s",blockname,looppos);
    __system(tmp);
    sprintf(tmp,"%s/.extfs",looppos);
    FILE*f = fopen(tmp,"w+");fclose(f);
    truncate(tmp,loopsize);
    sprintf(tmp,"losetup /dev/block/loop0 %s/.extfs",looppos);
    __system(tmp);
    __system("/sbin/mkfs.ext2 -b 4096 -m 0 -F /dev/block/loop0");
    __system("losetup -d /dev/block/loop0");
    sprintf(tmp,"umount %s",blockname);
    __system(tmp);
  }
  return 0;
}

int do_lagfix() {
  ui_print("checking mounts available\n");
  if (ensure_root_path_mounted("DATA:")!=0) return -1;
  if (ensure_root_path_mounted("DATADATA:")!=0) return -1;
  if (ensure_root_path_mounted("CACHE:")!=0) return -1;
  if (ensure_root_path_mounted("SDCARD:")!=0) return -1;

  char tmp[PATH_MAX];
  nandroid_generate_timestamp_path(tmp);
  ui_print("Creating a nandroid backup at %s\n",tmp);
  if (nandroid_backup(tmp)!=0) return -1;

  ui_print("Backup completed, recreating file systems\n");

  ui_print("Unmounting\n");
  unmount_all_lagfixed();

  ui_print("Switching to new config\n");
  __system("cp /system/etc/lagfix.conf /system/etc/lagfix.conf.old");

  ui_print("Creating /data\n");
  create_lagfix_partition(0);
  ui_print("Creating /dbdata\n");
  create_lagfix_partition(1);
  ui_print("Creating /cache\n");
  create_lagfix_partition(2);

  ui_print("Mounting to test\n");
  dont_use_bind_options = 1;
  if (ensure_root_path_mounted("DATA:")!=0) return -1;
  if (ensure_root_path_mounted("DATADATA:")!=0) return -1;
  if (ensure_root_path_mounted("CACHE:")!=0) return -1;
  dont_use_bind_options = 0;
  __system("mount"); // for debug purposes
  if (get_bind_options()) {
    ui_print("Creating bind directories\n");
    __system("mkdir -p /dbdata/.data/data");
    //__system("mkdir -p /dbdata/.data/dalvik-cache");
    __system("mkdir -p /data/data");
    //__system("mkdir -p /data/dalvik-cache");
  }

  ui_print("Unmounting again\n");
  unmount_all_lagfixed();

  ui_print("Restoring data\n");
  nandroid_restore(tmp,0,0,1,1,0);

  // restore might have brought some .data into dbdata, clear them
  if (!get_bind_options()) {
    if (ensure_root_path_mounted("DATADATA:")!=0) return -1;
    __system("rm -rf /dbdata/.data");
  }

  __system("mount");
  ui_print("Unmounting again to be sure\n");
  sync();
  sleep(5);
  unmount_all_lagfixed();
  sync();
  return 0;
}

int lagfixer_main(int argc, char** argv) {
  ui_init();
  ui_print(EXPAND(RECOVERY_VERSION)" - lagfixer\n");
  create_fstab();
  ui_set_show_text(1);

  int res = do_lagfix();
  if (res) {
    ui_print("Something went wrong while doing the lagfix, sorry.\n");
  } else {
    ui_print("Done. Your device will reboot soon or enter recovery mode to debug.\n");
  }
  sleep(5);

  gr_exit();
  ev_exit();
  return 0;
}
