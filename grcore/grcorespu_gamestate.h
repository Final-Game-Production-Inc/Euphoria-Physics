
struct spuGcmState : public spuGcmStateBase
{
#if HACK_GTA4
	spuMatrix44	shadowMatrix ;
	
	void*		tintDescriptorPtr;
	u8			tintDescriptorCount;
	u8			isGPADCapturing;

#if !__FINAL
	u8			_pad0[1];
	u8			FragmentStrippingFlags;
#else
	u8			_pad0[2];
#endif
#elif HACK_MC4
	Float16 damageParams[4];

	const void* damageTexture;
	const spuMatrix44* restMtx;
	int transformType;
	float transformParam;

	EdgeGeomLocalToWorldMatrix bones[20];
#endif
};

enum spuCommands {
#if HACK_GTA4
	GTA4__SetShadowType = GRCORE_COMMAND_BASE_COUNT,
	GTA4__SetShadowMatrix,
	GTA4__SetDamageTexture,
	GTA4__SetDamageTextureOffset,
	GTA4__SetCharacterClothData,
	GTA4__SetTintDescriptor,
	GTA4__SetWriteRsxLabel,

	GRCORE_COMMAND_COUNT
#elif HACK_MC4
	MC4__SetDamageTexture = GRCORE_COMMAND_BASE_COUNT,
	MC4__SetDamageParams,
	MC4__SetRestMatrix,
	MC4__SetTransformType,
	MC4__SetTransformParam,
	MC4__SetBoneIndex,
	MC4__SendBones,

	GRCORE_COMMAND_COUNT
#else
	GRCORE_COMMAND_COUNT = GRCORE_COMMAND_BASE_COUNT
#endif // HACK_MC4
};

#if HACK_GTA4
struct spuCmd_GTA4__SetShadowType: public spuCmd_Any {
	u32 shadowType; // eEdgeShadowType
};

struct spuCmd_GTA4__SetShadowMatrix: public spuCmd_Any {
	spuMatrix44 shadowMatrix;
};

struct spuCmd_GTA4__SetDamageTexture: public spuCmd_Any {
	float boundRadius;
	void *damageTexture;
};

struct spuCmd_GTA4__SetDamageTextureOffset: public spuCmd_Any {
	Vector3 damageTexOffset;
};

struct spuCmd_GTA4__SetCharacterClothData: public spuCmd_Any {
	void *morphData;
};
struct spuCmd_GTA4__SetTintDescriptor: public spuCmd_Any {
	void	*tintDescriptorPtr;
	u8		tintDescriptorCount;
	u8		tintShaderIdx;	// n: 0x00=none, 0xff=reset, shaderIdx=n-1
	u8		pad0[2];
};

struct spuCmd_GTA4__SetWriteRsxLabel : public spuCmd_Any {
	//u8		index;	// subcommand
	u32			value;
};
#elif HACK_MC4
struct spuCmd_MC4__SetDamageTexture: public spuCmd_Any {
	void *damageTexture;
};
struct spuCmd_MC4__SetDamageParams: public spuCmd_Any {
	float damageParams[4];
};
struct spuCmd_MC4__SetRestMatrix: public spuCmd_Any {
	const spuMatrix44* mtx;
};
struct spuCmd_MC4__SetTransformType: public spuCmd_Any {
	int value;
};
struct spuCmd_MC4__SetTransformParam: public spuCmd_Any {
	float value;
};
struct spuCmd_MC4__SendBones: public spuCmd_Any {
	float ox, oy, oz;
	EdgeGeomLocalToWorldMatrix mtxs[0];
};
#endif // HACK_MC4
