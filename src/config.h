#define USE_BUSYBOX_ASH  0
#define USE_DEBUG        1
#define USE_ERROR        1
#define USE_DEVTMPFS     0
#define DISPLAY_SVG      1
#define DISPLAY_SCALE    0.75
#define SVG_DPI          200
#define SVG_PRE_PATH     "/splash-starting.svg"
#define SVG_POST_PATH    "/splash-started.svg"
#define SERIAL_NUMBER    "1234567890"
#define USB_ID_PRODUCT   "0x1234"
#define USB_ID_VENDOR    "0x05F9"
#define USB_MANUFACTURER "Linux"
#define USB_PRODUCT      "Mass Storage"
#define USB_CONFIG       "USB SCSI"
#define TARGET_NAA       "naa."SERIAL_NUMBER

/* do not change */
#define PATH_DEV "/dev"
#define PATH_SYS "/sys"
#define PATH_SYS_DEV PATH_SYS"/dev"
#define PATH_UDC PATH_SYS"/class/udc"
#define PATH_NULL PATH_DEV"/null"
#define PATH_KMSG PATH_DEV"/kmsg"
#define PATH_CONSOLE PATH_DEV"/console"
#define PATH_BLOCK PATH_SYS"/block"
#define PATH_CONFIGFS PATH_SYS"/kernel/config"
#define PATH_GADGET PATH_CONFIGFS"/usb_gadget"
#define PATH_TARGET PATH_CONFIGFS"/target"
#define PATH_TARGET_CORE PATH_TARGET"/core/iblock_0"
#define PATH_FABRIC PATH_TARGET"/usb_gadget"
#define PATH_NAA PATH_FABRIC"/"TARGET_NAA
#define PATH_TPGT PATH_NAA"/tpgt_1"
#define PATH_FBDEV PATH_DEV"/fb0"
