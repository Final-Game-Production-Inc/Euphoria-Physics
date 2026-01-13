#include <sdk_version.h>

#if CELL_SDK_VERSION < 0x210000
	__asm__ volatile(
		".align   3\n"
		"rotqbyi $78,%[stack_size],-4\n"
		"rotqbyi $77,$1,-4\n"
		"ila     $79,(8*1024)>>4\n"
		"ceqi    $78,$78,0\n"
		"selb    $79,%[stack_size],$79,$78\n"
		"shli    $79,$79,4\n"
		"sf      $78,$1,$77\n"
		"ai      $77,$1,-0x30\n"
		"a       $78,$79,$78\n"
		"fsmbi   $79,0xf0ff\n"
		"andi    $78,$78,0x1ff\n"
		"ceqi    $78,$78,0\n"
		"or      $79,$79,$78\n"
		"selb    $1,$77,$1,$79\n"
		::[stack_size]"r"(job->header.sizeStack):"$77", "$78","$79");
#endif
