#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <scsi/sg_lib.h>
#include <scsi/sg_io_linux.h>
#include "mam-info.h"

//---------------Usage--------------
static void usage()
{
    fprintf(stderr, 
          "LTO Medium Access Memory tool\n"
          "Usage: \n"
          "lto-cm -f device [-c] [-v]\n"
          " where:\n"
          "    -f device        is a sg device                        \n"
          "    -c               compact mode\n"
          "    -v               increase verbosity \n"
          "    -h/?             display usage\n"
         );
}

int att_read(int fd, void* data, int command, int len, int data_type){
    int ok;
    unsigned char rAttCmdBlk[READ_ATT_CMD_LEN] = {0x8C, 0x00, 0, 0, 0, 0, 0, 0, 0x04, 0x00, 0, 0, 159,0, 0, 0};
    unsigned char inBuff[READ_ATT_REPLY_LEN];
    unsigned char sense_buffer[32];
    sg_io_hdr_t io_hdr;

    rAttCmdBlk[8]  = 0xff & (command >> 8);
    rAttCmdBlk[9]  = 0xff & command;
    rAttCmdBlk[12] = 0xff & len;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(rAttCmdBlk);
    io_hdr.mx_sb_len = sizeof(sense_buffer);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = READ_ATT_REPLY_LEN;
    io_hdr.dxferp = inBuff;
    io_hdr.cmdp = rAttCmdBlk;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = 20000;     

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        if(globalArgs.verbose)perror("SG_READ_ATT_0803: Inquiry SG_IO ioctl error");
        close(fd);
        return -1;
    }

    ok = 0;
    switch (sg_err_category3(&io_hdr)) {
    case SG_LIB_CAT_CLEAN:
        ok = 1;
        break;
    case SG_LIB_CAT_RECOVERED:
        if(globalArgs.verbose)printf("Recovered error on SG_READ_ATT, continuing\n");
        ok = 1;
        break;
    default: /* won't bother decoding other categories */
        printf("ERROR : Problem reading attribute %04x\n", command);
        if(globalArgs.verbose)sg_chk_n_print3("SG_READ_ATT command error", &io_hdr, 1);
        return -1;
    }

    if (ok) { /* output result if it is available */

        if(globalArgs.verbose)printf("SG_READ_ATT command=%0x duration=%u millisecs, resid=%d, msg_status=%d\n",
                command, io_hdr.duration, io_hdr.resid, (int)io_hdr.msg_status);

        if(globalArgs.verbose) {
        int i;
        printf("Raw value for attribute %04x: ", command);
        for(i=0;i<len;i++) {
          printf("%02x ", inBuff[9+i]);
        }
        printf("\n");
        }

        if(data_type == MAM_TYPE_BINARY) {
           int i;
           uint64_t out = 0;
           for(i=0;i<len;i++) {
             out *= 256;
             out += inBuff[9+i];
           }
           memcpy( data, (void*)&out,len );
        }

        if(data_type == MAM_TYPE_ASCII) {
                memcpy(data, &inBuff[9],len );
                ((char*)data)[len] = 0;
        }
   }

return 0;
}


//----------------------------- MAIN FUNCTION---------------------------------
int main(int argc, char * argv[])
{
    int sg_fd;
    int k,i,l;
    char * file_name = 0;
    char ebuff[EBUFF_SZ];
    char messageout[160] ;
    int c=0;
    struct mam_medium_t medium;

   globalArgs.verbose=0;
   globalArgs.device_name=NULL;
   globalArgs.compact=0;

    while (1) {
   
        c = getopt(argc, argv, "f:h?vc");

        if (c == -1)
            break;

        switch (c) {
        case 'f':
             if ((globalArgs.device_name=(char*)optarg)==NULL) {
                fprintf(stderr, "ERROR : Specify a device\n");
                usage();
                return SG_LIB_SYNTAX_ERROR;
            }
            break;
        case 'h':
        case '?':
            usage();
            return 0;
        case 'v':
            ++globalArgs.verbose;
            break;
        case 'c':
            ++globalArgs.compact;
            break;
        default:
            fprintf(stderr, "ERROR : Unrecognised option code 0x%x ??\n", c);
            usage();
            return SG_LIB_SYNTAX_ERROR;
        }
        
    }

    if (argc == 1) {
        usage();
        return 1;
    }   

        if (optind < argc) {
            for (; optind < argc; ++optind)
                fprintf(stderr, "ERROR : Unexpected extra argument: %s\n",
                        argv[optind]);
            usage();
            return SG_LIB_SYNTAX_ERROR;
        }
        
        if(!(globalArgs.device_name)){
                usage();
                return SG_LIB_SYNTAX_ERROR;
        }
        
 
    if ((sg_fd = open(globalArgs.device_name, O_RDWR)) < 0) {
        snprintf(ebuff, EBUFF_SZ,
                 "ERROR : opening file: %s", file_name);
        perror(ebuff);
        return -1;
    }
    /* Just to be safe, check we have a new sg device by trying an ioctl */
    if ((ioctl(sg_fd, SG_GET_VERSION_NUM, &k) < 0) || (k < 30000)) {
        printf("ERROR :  %s doesn't seem to be an new sg device\n",
               globalArgs.device_name);
        close(sg_fd);
        return -1;
    }

        if(att_read(sg_fd, medium.manufacturer, MAM_ATT_MANUFACTURER, 8, MAM_TYPE_ASCII) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
        if(att_read(sg_fd, medium.serial, MAM_ATT_SERIAL, 32, MAM_TYPE_ASCII) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }

        if(att_read(sg_fd, medium.manufacture_date, MAM_ATT_MANUFACTURE_DATE, 8, MAM_TYPE_ASCII) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
/*
        if(att_read(sg_fd, medium.last_written, MAM_ATT_LAST_WRITTEN, 12, MAM_TYPE_ASCII) < 0) {
                medium.last_written[0] = '\0';
        }
*/
        if(att_read(sg_fd, medium.barcode, MAM_ATT_BARCODE, 12, MAM_TYPE_ASCII) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
/*
        if(att_read(sg_fd, medium.identifier, MAM_ATT_IDENTIFIER, 12, MAM_TYPE_ASCII) < 0) {
                medium.identifier[0] = '\0';
        }
*/
        if(att_read(sg_fd, &medium.load_count, MAM_ATT_LOAD_COUNT, 8, MAM_TYPE_BINARY) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
        if(att_read(sg_fd, &medium.init_count, MAM_ATT_INIT_COUNT, 2, MAM_TYPE_BINARY) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
        if(att_read(sg_fd, &medium.total_mb_written, MAM_ATT_TOTAL_MB_WRITTEN, 8, MAM_TYPE_BINARY) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
        if(att_read(sg_fd, &medium.total_mb_read, MAM_ATT_TOTAL_MB_READ, 8, MAM_TYPE_BINARY) < 0) {
                printf("ERROR : Read failed (try verbose opt)\n");
                close(sg_fd);
                return -1;
        }
    close(sg_fd);

        char fmtString[] = 
                "Barcode:      %.8s\n"
                "Manufacturer: %.8s\n"
                "Serial:       %.32s\n"
                "Manuf. Date:  %.8s\n"
                "Load count:   %d\n"
                "Init count:   %d\n"
                "MB written:   %d\n"
                "MB read:      %d\n";

        char compactFmtString[] =
                "%.8s "
                "%.8s "
                "%.10s "
                "%.8s\t"
                "%d\t"
                "%d\t"
                "%d\t"
                "%d\n";

        printf(globalArgs.compact ? compactFmtString : fmtString, \
                medium.barcode,
                medium.manufacturer,
                medium.serial,
                medium.manufacture_date,
                medium.load_count,
                medium.init_count,
                medium.total_mb_written,
                medium.total_mb_read
                );
    return 0;
}
