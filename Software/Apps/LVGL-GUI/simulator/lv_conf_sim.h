// Simulator-only LVGL config: the SDK's, with the pool scaled for 64-bit
// pointers and snapshots enabled for ScreenshotTour.
#include UNA_SDK_LV_CONF
#undef  LV_MEM_SIZE
#define LV_MEM_SIZE (40U * 1024U * sizeof(void*) / 4U)

#undef  LV_USE_SNAPSHOT
#define LV_USE_SNAPSHOT 1
