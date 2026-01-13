// 
// grcore/locktypes.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_LOCKTYPES_H
#define GRCORE_LOCKTYPES_H

namespace rage
{
	enum grcLockType
	{
		grcsRead			= 0x01,
		grcsWrite			= 0x02,
		grcsDoNotWait		= 0x04,	// Will return a failed lock if unsuccessful
		grcsDiscard			= 0x08,	// Discard contents of locked region entire buffer
		grcsNoOverwrite		= 0x10, // Indicates that the region locked should not be in use by the GPU - Can cause visual artifacts if not used properly
		grcsNoDirty			= 0x20,	// Prevents driver from marking the region as dirty
		grcsAllowVRAMLock	= 0x40, // don't assert VRAM lock if you know what you're doing
	};

	enum grcTextureCreateType
	{
		grcsTextureCreate_NeitherReadNorWrite	= 0x0, // Expects data to be available upon creation (equivalent to D3D11_USAGE_IMMUTABLE)
		grcsTextureCreate_ReadWriteHasStaging	= 0x1, // Read access via backing store. Write access via backing store then UpdateSubresource(). Has staging texture for GPU->CPU operations.
		grcsTextureCreate_Write					= 0x2, // Write access via backing store then UpdateSubresource().
		grcsTextureCreate_ReadWriteDynamic		= 0x3, // Read access via backing store. Write access via backing store then Map()/Unmap() of dynamic texture.
		grcsTextureCreate_WriteDynamic			= 0x4, // Write access via backing store then Map()/Unmap() of dynamic texture.
		grcsTextureCreate_WriteOnlyFromRTDynamic= 0x5, // Write access via Map()/Unmap() of dynamic texture.
	};

	enum grcsTextureSyncType
	{
		grcsTextureSync_Mutex			= 0x0, // Uses a mutex on the buffer, minimal stalls, can be called only once per frame, "frame skips" in GPU copy possible.
		grcsTextureSync_DirtySemaphore	= 0x1, // Uses a semaphore to prevent overwriting data before the GPU has been updated. Supports calling once per frame only, could incur long stalls.
		grcsTextureSync_CopyData		= 0x2, // Makes a copy of the data. Supports multiple calls per frame but has to copy buffer contents.
		grcsTextureSync_None			= 0x3, // Expects only to be updated on the render thread or not at all.
	};

} // namespace rage

#endif
