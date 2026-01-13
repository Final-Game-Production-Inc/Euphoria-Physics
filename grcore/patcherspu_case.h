#if SPU_GCM_FIFO
case grcDevice__BeginFrame:
	SPU_BOOKMARK(0x1000);
	pSpuGcmState->FlashEnable = any->subcommand & 1;
	pSpuGcmState->DipSwitches = any->subcommand >> 4;
#	if DEBUG_ALLOW_CACHED_STATES_TOGGLE
		pSpuGcmState->DebugSetAllStates = (any->subcommand >> 1) & 1;
#	endif
	break;

case grcDevice__EndFrame:
	SPU_BOOKMARK(0x1010);
	// NOP for now.
	break;
#endif

BEGIN_SPU_SIMPLE_COMMAND(grcVertexProgram__SetFlag)
	grcVertexProgram::SetFlag(any->subcommand & 31,(any->subcommand & 0x80) != 0);
END_SPU_COMMAND

// EOF
