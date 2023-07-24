// Enable this to do inference on embedded images
// #define CLI_ONLY_INFERENCE 1

// Enable this to get cpu stats
#define COLLECT_CPU_STATS 1

#if !defined(CLI_ONLY_INFERENCE)
// Enable this for display
// #define DISPLAY_SUPPORT 1
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void run_inference(void *ptr);
#ifdef __cplusplus
}
#endif
