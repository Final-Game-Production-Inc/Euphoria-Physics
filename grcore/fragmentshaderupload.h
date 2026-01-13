#include "effect_config.h"
#include "system/dma.h"

#define UPLOAD_FRAGMENT_PROGRAMS_WITH_EDGE (0)

#if PATCH_ON_SPU || SPU_GCM_FIFO

namespace rage {

void grcFragmentProgram::Upload(void* destAddr, u32 tag) const
{
#if __SPU    
    Upload(destAddr, Program, CompressedProgramSize, ProgramSize, tag);
#else
    // Tell SPU to transfer the program
    SPU_COMMAND(patcher__DecompressMicrocode,0);
    cmd->MicrocodeSrcAddr = Program;
    cmd->MicrocodeDestAddr = destAddr;
    Assign(cmd->ProgramSize, ProgramSize);
    Assign(cmd->CompressedProgramSize, CompressedProgramSize);
#endif
}

#if __SPU

//#if COMPRESSED_FRAGMENT_PROGRAMS
static const u32 FRAGMENT_DECOMP_SLOP = 128;
extern "C" u32 datDecompressInPlace(void *dest,const u32 destSize,const u32 offsetOfSrc,const u32 srcSize);
//#endif

extern char* spuScratch;
extern char* spuScratchTop;

void grcFragmentProgram::Upload(void* destAddr, void* program, u32 compressedsize, u32 programSize, u32 tag)
{
#if SPU_GCM_FIFO && !defined(NO_GCM_ALLOCATE)
    if (!destAddr)
        destAddr = gcmAllocateVram(pSpuGcmState->LastMicrocodeOffset, (programSize + 63) & ~63);
#endif

#if COMPRESSED_FRAGMENT_PROGRAMS
    int compSize = (compressedsize + 15) & ~15;
    int compOffset = programSize + FRAGMENT_DECOMP_SLOP - compSize;
    if ((u32)program >= 256*1024)
    {
        Assert(spuScratch + programSize + FRAGMENT_DECOMP_SLOP <= spuScratchTop);
        sysDmaGet(spuScratch + compOffset, (u32)program, compSize, tag);
        sysDmaWaitTagStatusAll(1<<tag);
        program = spuScratch;
    }
    AssertVerify(datDecompressInPlace(program, programSize, compOffset, compressedsize) == programSize);
#else
    if ((u32)program >= 256*1024)
    {
        Assert(spuScratch + programSize <= spuScratchTop);
        sysDmaGet(spuScratch, (u32)program, programSize, tag);  
        sysDmaWaitTagStatusAll(1<<tag);
        program = spuScratch;
    }  
#endif

#if UPLOAD_FRAGMENT_PROGRAMS_WITH_EDGE	// Write ucode only, not patches..  (ColinH)
	qword *ptr = (qword *)program;
    qword *end = (qword *)((char*)program + programSize);
	qword imm_mask = si_ilhu( 3 );
	qword imm_val = si_ilhu( 2 );
	qword msb = (qword){0,4,8,12,0,4,8,12,0,4,8,12,0,4,8,12};

	imm_mask = si_rotqmbyi( imm_mask,-4 );		// Not first field..
	qword splat = si_ilh( 0xa0a );
	qword perm = si_ilhu( 4 );
	perm = si_iohl( perm,0x80c );
	qword *base = ptr;
	u32 write = (u32)destAddr;
	int size = 0;

	do
	{
		qword instruction = ptr[0];
		qword zero = ptr[1];
        // set valid to -1 if MSB of byte 10 is clear else 0
		qword valid = si_cgtbi( instruction,-1 );
		valid = si_shufb( valid,valid,splat );
        // mask off type field from each of 3 inputs
		qword immediate = si_and( instruction,imm_mask );
        // any of them set to 2, i.e. an immediate ?
		immediate = si_ceq( immediate,imm_val ); 
        // if yes, set immediate to -1, otherwise 0
        immediate = si_cgti(si_shufb(immediate,zero,msb), 0); // matts - added this missing splat
        // only -1 if valid also -1
		immediate = si_and( immediate,valid );

        // if we had an immediate instrqwords is 2, otherwise 1
        int instrqwords = 1 - si_to_int(immediate);
        ptr += instrqwords;
        int instrbytes = instrqwords * 16;
        size += instrbytes;

        // is the immediate value 0?
		zero = si_ceqi( zero,0 );
		zero = si_shufb( zero,zero,msb );
		zero = si_ceqi( zero,-1 ); // only if all xyzw are 0
		zero = si_and( zero,immediate );
        int iszero = si_to_int(zero); // -1 if there is a zero constant

		// 3 cases here - no imm, imm, or zero imm

        if (iszero || size > 0x3fe0 || ptr >= end)
        {
		    sysDmaPutb(base, write, size + iszero * 16, tag); // Don't send zero constant
		    base=ptr;
		    write+=size;
		    size=0;
        }
	}
    while (ptr < end);
#else
	sysDmaLargePutb(program,(u32)destAddr,programSize,tag);
#endif
}
#endif // __SPU

} // namespace rage

#endif // PATCH_ON_SPU || SPU_GCM_FIFO
