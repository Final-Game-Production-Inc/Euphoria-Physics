// GCM_REPORT_AREA_SIZE should be a multiple of 128 so as not to throw off the CellSpurs alignment.
#if __BANK
#define GCM_REPORT_COUNT				1408 // Higher count for shmoo : we use those a lot.
#else // __BANK
#define GCM_REPORT_COUNT				1152 // Bumping by +128, primarily to support all our timebars as well as the shmoo system
#endif // __BANK
#define GCM_OCCLUSION_QUERY_COUNT		128
#define GCM_CONDITIONAL_QUERY_COUNT		4096
#define GCM_REPORT_AREA_SIZE 	((GCM_REPORT_COUNT+GCM_OCCLUSION_QUERY_COUNT+GCM_CONDITIONAL_QUERY_COUNT)*sizeof(CellGcmReportData))
#define SPU_SPURS_AREA_SIZE		(2*sizeof(CellSpurs))
#define MAPPED_GCM_AREA_SIZE	(1024*1024)
