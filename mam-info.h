#define READ_ATT_REPLY_LEN 512
#define READ_ATT_CMD_LEN 16
#define EBUFF_SZ 256

#define MAM_TYPE_BINARY          0x00
#define MAM_TYPE_ASCII           0x01

#define MAM_ATT_MANUFACTURER     0x0400
#define MAM_ATT_SERIAL           0x0401
#define MAM_ATT_MANUFACTURE_DATE 0x0406
#define MAM_ATT_LAST_WRITTEN     0x0804
#define MAM_ATT_BARCODE          0x0806
#define MAM_ATT_IDENTIFIER       0x0008
#define MAM_ATT_LOAD_COUNT       0x0003
#define MAM_ATT_INIT_COUNT       0x0007
#define MAM_ATT_TOTAL_MB_WRITTEN 0x0220
#define MAM_ATT_TOTAL_MB_READ    0x0221

struct globalArgs_t {
        int verbose;
        const char* device_name;
        int compact;
} globalArgs;

struct mam_medium_t {
        char manufacturer[9];
        char serial[33];
        char manufacture_date[9];
        char last_written[13];
        char barcode[33];
        char identifier[33];
        uint64_t load_count;
        uint16_t init_count;
        uint64_t total_mb_written;
        uint64_t total_mb_read;
};
