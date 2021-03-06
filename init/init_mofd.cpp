/*
** Copyright 2016, The CyanogenMod Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "log.h"
#include "property_service.h"
#include "util.h"
#include "vendor_init.h"

#define PHONE_INFO "/factory/PhoneInfodisk/PhoneInfo_inf"

/* Serial number */
#define SERIAL_PROP "ro.serialno"
#define SERIAL_OFFSET 0x00
#define SERIAL_LENGTH 17

/* Zram */
#define ZRAM_PROP "ro.config.zram"
#define MEMINFO_FILE "/proc/meminfo"
#define MEMINFO_KEY "MemTotal:"
#define ZRAM_MEM_THRESHOLD 3000000

/* ProjectID */
#define PRODUCT_PROP "ro.build.product"
#define MODEL_PROP "ro.product.model"
#define DEVICE_PROP "ro.product.device"
#define DESCRIPTION_PROP "ro.build.description"
#define FINGERPRINT_PROP "ro.build.fingerprint"
#define PRODUCT_ID_FILE "/sys/module/intel_mid_sfi/parameters/project_id"
#define ID_LENGTH 2

static int read_file2(const char *fname, char *data, int max_size)
{
    int fd, rc;

    if (max_size < 1)
        return 0;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s'\n", fname);
        return 0;
    }

    rc = read(fd, data, max_size -1);
    if ((rc > 0) && (rc < max_size ))
        data[rc] = '\0';
    else
        data[0] = '\0';
    close(fd);

    return 1;
}

static void get_serial()
{
    int ret = 0;
    char const *path = PHONE_INFO;
    char buf[SERIAL_LENGTH + 1];
    prop_info *pi;

    if(read_file2(path, buf, sizeof(buf))) {
        if (strlen(buf) > 0) {
            pi = (prop_info*) __system_property_find(SERIAL_PROP);
            if(pi)
                ret = __system_property_update(pi, buf,
                        strlen(buf));
            else
                ret = __system_property_add(SERIAL_PROP,
                        strlen(SERIAL_PROP),
                        buf, strlen(buf));
        }
    }
}

static void configure_zram() {
    char buf[128];
    FILE *f;

    if ((f = fopen(MEMINFO_FILE, "r")) == NULL) {
        ERROR("%s: Failed to open %s\n", __func__, MEMINFO_FILE);
        return;
    }

    while (fgets(buf, sizeof(buf), f) != NULL) {
        if (strncmp(buf, MEMINFO_KEY, strlen(MEMINFO_KEY)) == 0) {
            int mem = atoi(&buf[strlen(MEMINFO_KEY)]);
            const char *mode = mem < ZRAM_MEM_THRESHOLD ? "true" : "false";
            INFO("%s: Found total memory to be %d kb, zram enabled: %s\n", __func__, mem, mode);
            property_set(ZRAM_PROP, mode);
            break;
        }
    }

    fclose(f);
}

static void get_projectID() {
    int ret = 0;
    char const *path = PRODUCT_ID_FILE;
    char buf[ID_LENGTH + 1];
    prop_info *pi;

    if(read_file2(path, buf, sizeof(buf))) {
        if (strstr(buf, "23")) {
            property_set("ro.product.model", "ASUS_Z008");
            property_set("ro.build.fingerprint", "asus/WW_Z008/Z008:5.0/LRX21V/2.20.40.138_20160107_6192_user:user/release-keys");
            property_set("ro.build.description", "asusmofd_hd-user 5.0 LRX21V 2.20.40.138_20160107_6192_user release-keys");
            property_set("ro.product.device", "Z008");
            property_set("ro.build.product", "Z008");
        } else if (strstr(buf, "27") || strstr(buf, "28") || strstr(buf, "30") || strstr(buf, "31")){
            property_set("ro.product.model", "ASUS_Z00A");
            property_set("ro.build.fingerprint", "asus/WW_Z00A/Z00A:5.0/LRX21V/2.20.40.165_20160118_6541_user:user/release-keys");
            property_set("ro.build.description", "asusmofd_fhd-user 5.0 LRX21V 2.20.40.165_20160118_6541_user release-keys");
            property_set("ro.product.device", "Z00A");
            property_set("ro.build.product", "Z00A");
        } else {
            property_set("ro.product.model", "UNKNOWN");
            property_set("ro.build.fingerprint", "UNKNOWN");
            property_set("ro.build.description", "UNKNOWN");
            property_set("ro.product.device", "UNKNOWN");
            property_set("ro.build.product", "UNKNOWN");
            ERROR("Unkown product_ID %s found \n", buf);
        }
    }
}

void vendor_load_properties()
{
    get_serial();
    configure_zram();
    get_projectID();
}
