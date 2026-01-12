TABLE_DECL(g_UpdateContacts) = {
	{CNST_FNC(NullFunc),							CNST_FNC(UpdateContactsFixAndMov),				CNST_FNC(UpdateContactsFixAndArt)},				// collisions), first object fixed
	{CNST_FNC(NullFunc),							CNST_FNC(UpdateContactsFixedPointFixAndMov),	CNST_FNC(UpdateContactsFixedPointFixAndArt)},	// fixed point constraints), first object fixed
	{CNST_FNC(NullFunc),							CNST_FNC(UpdateContactsFixAndMov),				CNST_FNC(UpdateContactsFixAndArt)},				// sliding point constraints (same as collisions)), first object fixed
	{CNST_FNC(NullFunc),							CNST_FNC(UpdateContactsRotationFixAndMov),		CNST_FNC(UpdateContactsRotationFixAndArt)},		// fixed rotation constraints), first object fixed
	{CNST_FNC(NullFunc),							CNST_FNC(UpdateContactsRotationFixAndMov),		CNST_FNC(UpdateContactsRotationFixAndArt)},		// sliding rotation constraints), first object fixed
	{CNST_FNC(NullFunc),							CNST_FNC(UpdateContactsRotationFixAndMov),		CNST_FNC(UpdateContactsRotationFixAndArt)},		// pivoting rotation constraints), first object fixed
	{CNST_FNC(UpdateContactsMovAndFix),				CNST_FNC(UpdateContactsMovAndMov),				CNST_FNC(UpdateContactsMovAndArt)},				// collisions), first object rigid
	{CNST_FNC(UpdateContactsFixedPointMovAndFix),	CNST_FNC(UpdateContactsFixedPointMovAndMov),	CNST_FNC(UpdateContactsFixedPointMovAndArt)},	// fixed point constraints), first object rigid
	{CNST_FNC(UpdateContactsMovAndFix),				CNST_FNC(UpdateContactsMovAndMov),				CNST_FNC(UpdateContactsMovAndArt)},				// sliding point constraints (same as collisions)), first object rigid
	{CNST_FNC(UpdateContactsRotationMovAndFix),		CNST_FNC(UpdateContactsRotationMovAndMov),		CNST_FNC(UpdateContactsRotationMovAndArt)},		// fixed rotation constraints), first object rigid
	{CNST_FNC(UpdateContactsRotationMovAndFix),		CNST_FNC(UpdateContactsRotationMovAndMov),		CNST_FNC(UpdateContactsRotationMovAndArt)},		// sliding rotation constraints), first object rigid
	{CNST_FNC(UpdateContactsRotationMovAndFix),		CNST_FNC(UpdateContactsRotationMovAndMov),		CNST_FNC(UpdateContactsRotationMovAndArt)},		// pivoting rotation constraints), first object rigid
	{CNST_FNC(UpdateContactsArtAndFix),				CNST_FNC(UpdateContactsArtAndMov),				CNST_FNC(UpdateContactsArtAndArt)},				// collisions), first object articulated
	{CNST_FNC(UpdateContactsFixedPointArtAndFix),	CNST_FNC(UpdateContactsFixedPointArtAndMov),	CNST_FNC(UpdateContactsFixedPointArtAndArt)},	// fixed point constraints), first object articulated
	{CNST_FNC(UpdateContactsArtAndFix),				CNST_FNC(UpdateContactsArtAndMov),				CNST_FNC(UpdateContactsArtAndArt)},				// sliding point constraints (same as collisions)), first object articulated
	{CNST_FNC(UpdateContactsRotationArtAndFix),		CNST_FNC(UpdateContactsRotationArtAndMov),		CNST_FNC(UpdateContactsRotationArtAndArt)},		// fixed rotation constraints), first object articulated
	{CNST_FNC(UpdateContactsRotationArtAndFix),		CNST_FNC(UpdateContactsRotationArtAndMov),		CNST_FNC(UpdateContactsRotationArtAndArt)},		// sliding rotation constraints), first object articulated
	{CNST_FNC(UpdateContactsRotationArtAndFix),		CNST_FNC(UpdateContactsRotationArtAndMov),		CNST_FNC(UpdateContactsRotationArtAndArt)},		// pivoting rotation constraints), first object articulated
};

TABLE_DECL(g_PreResponse) = {
	{CNST_FNC(NullFunc),						CNST_FNC(PreResponseFixAndMov),				CNST_FNC(PreResponseFixAndArt)},			
	{CNST_FNC(NullFunc),						CNST_FNC(PreResponseFixedPointFixAndMov),	CNST_FNC(PreResponseFixedPointFixAndArt)},
	{CNST_FNC(NullFunc),						CNST_FNC(PreResponseFixAndMov),				CNST_FNC(PreResponseFixAndArt)},			
	{CNST_FNC(NullFunc),						CNST_FNC(PreResponseRotationFixAndMov),		CNST_FNC(PreResponseRotationFixAndArt)},	
	{CNST_FNC(NullFunc),						CNST_FNC(PreResponseRotationFixAndMov),		CNST_FNC(PreResponseRotationFixAndArt)},
	{CNST_FNC(NullFunc),						CNST_FNC(PreResponseRotationFixAndMov),		CNST_FNC(PreResponseRotationFixAndArt)},
	{CNST_FNC(PreResponseMovAndFix),			CNST_FNC(PreResponseMovAndMov),				CNST_FNC(PreResponseMovAndArt)},				
	{CNST_FNC(PreResponseFixedPointMovAndFix),	CNST_FNC(PreResponseFixedPointMovAndMov),	CNST_FNC(PreResponseFixedPointMovAndArt)},
	{CNST_FNC(PreResponseMovAndFix),			CNST_FNC(PreResponseMovAndMov),				CNST_FNC(PreResponseMovAndArt)},				
	{CNST_FNC(PreResponseRotationMovAndFix),	CNST_FNC(PreResponseRotationMovAndMov),		CNST_FNC(PreResponseRotationMovAndArt)},	
	{CNST_FNC(PreResponseRotationMovAndFix),	CNST_FNC(PreResponseRotationMovAndMov),		CNST_FNC(PreResponseRotationMovAndArt)},
	{CNST_FNC(PreResponseRotationMovAndFix),	CNST_FNC(PreResponseRotationMovAndMov),		CNST_FNC(PreResponseRotationMovAndArt)},
	{CNST_FNC(PreResponseArtAndFix),			CNST_FNC(PreResponseArtAndMov),				CNST_FNC(PreResponseArtAndArt)},			
	{CNST_FNC(PreResponseFixedPointArtAndFix),	CNST_FNC(PreResponseFixedPointArtAndMov),	CNST_FNC(PreResponseFixedPointArtAndArt)},
	{CNST_FNC(PreResponseArtAndFix),			CNST_FNC(PreResponseArtAndMov),				CNST_FNC(PreResponseArtAndArt)},			
	{CNST_FNC(PreResponseRotationArtAndFix),	CNST_FNC(PreResponseRotationArtAndMov),		CNST_FNC(PreResponseRotationArtAndArt)},	
	{CNST_FNC(PreResponseRotationArtAndFix),	CNST_FNC(PreResponseRotationArtAndMov),		CNST_FNC(PreResponseRotationArtAndArt)},	
	{CNST_FNC(NullFunc),						CNST_FNC(NullFunc),							CNST_FNC(NullFunc)},	
};

TABLE_DECL(g_ApplyImpulse) = {
	{CNST_FNC(NullFunc),							CNST_FNC(ApplyImpulseFixAndMov),				CNST_FNC(ApplyImpulseFixAndArt)},			
	{CNST_FNC(NullFunc),							CNST_FNC(ApplyImpulseFixedPointFixAndMov),		CNST_FNC(ApplyImpulseFixedPointFixAndArt)},
	{CNST_FNC(NullFunc),							CNST_FNC(ApplyImpulseFixAndMov),				CNST_FNC(ApplyImpulseFixAndArt)},			
	{CNST_FNC(NullFunc),							CNST_FNC(ApplyImpulseFixedRotationFixAndMov),	CNST_FNC(ApplyImpulseFixedRotationFixAndArt)},	
	{CNST_FNC(NullFunc),							CNST_FNC(ApplyImpulseSlideRotationFixAndMov),	CNST_FNC(ApplyImpulseSlideRotationFixAndArt)},
	{CNST_FNC(NullFunc),							CNST_FNC(ApplyImpulsePivotRotationFixAndMov),	CNST_FNC(ApplyImpulsePivotRotationFixAndArt)},
	{CNST_FNC(ApplyImpulseMovAndFix),				CNST_FNC(ApplyImpulseMovAndMov),				CNST_FNC(ApplyImpulseMovAndArt)},			
	{CNST_FNC(ApplyImpulseFixedPointMovAndFix),		CNST_FNC(ApplyImpulseFixedPointMovAndMov),		CNST_FNC(ApplyImpulseFixedPointMovAndArt)},
	{CNST_FNC(ApplyImpulseMovAndFix),				CNST_FNC(ApplyImpulseMovAndMov),				CNST_FNC(ApplyImpulseMovAndArt)},			
	{CNST_FNC(ApplyImpulseFixedRotationMovAndFix),	CNST_FNC(ApplyImpulseFixedRotationMovAndMov),	CNST_FNC(ApplyImpulseFixedRotationMovAndArt)},	
	{CNST_FNC(ApplyImpulseSlideRotationMovAndFix),	CNST_FNC(ApplyImpulseSlideRotationMovAndMov),	CNST_FNC(ApplyImpulseSlideRotationMovAndArt)},
	{CNST_FNC(ApplyImpulsePivotRotationMovAndFix),	CNST_FNC(ApplyImpulsePivotRotationMovAndMov),	CNST_FNC(ApplyImpulsePivotRotationMovAndArt)},
	{CNST_FNC(ApplyImpulseArtAndFix),				CNST_FNC(ApplyImpulseArtAndMov),				CNST_FNC(ApplyImpulseArtAndArt)},
	{CNST_FNC(ApplyImpulseFixedPointArtAndFix),		CNST_FNC(ApplyImpulseFixedPointArtAndMov),		CNST_FNC(ApplyImpulseFixedPointArtAndArt)},
	{CNST_FNC(ApplyImpulseArtAndFix),				CNST_FNC(ApplyImpulseArtAndMov),				CNST_FNC(ApplyImpulseArtAndArt)},
	{CNST_FNC(ApplyImpulseFixedRotationArtAndFix),	CNST_FNC(ApplyImpulseFixedRotationArtAndMov),	CNST_FNC(ApplyImpulseFixedRotationArtAndArt)},
	{CNST_FNC(ApplyImpulseSlideRotationArtAndFix),	CNST_FNC(ApplyImpulseSlideRotationArtAndMov),	CNST_FNC(ApplyImpulseSlideRotationArtAndArt)},
	{CNST_FNC(ApplyImpulsePivotRotationArtAndFix),	CNST_FNC(ApplyImpulsePivotRotationArtAndMov),	CNST_FNC(ApplyImpulsePivotRotationArtAndArt)},
};

TABLE_DECL(g_ApplyImpulseAndPush) = {
	{CNST_FNC(NullFunc),									CNST_FNC(ApplyImpulseAndPushFixAndMov),					CNST_FNC(ApplyImpulseAndPushFixAndArt)},			
	{CNST_FNC(NullFunc),									CNST_FNC(ApplyImpulseAndPushFixedPointFixAndMov),		CNST_FNC(ApplyImpulseAndPushFixedPointFixAndArt)},
	{CNST_FNC(NullFunc),									CNST_FNC(ApplyImpulseAndPushFixAndMov),					CNST_FNC(ApplyImpulseAndPushFixAndArt)},			
	{CNST_FNC(NullFunc),									CNST_FNC(ApplyImpulseAndPushFixedRotationFixAndMov),	CNST_FNC(ApplyImpulseAndPushFixedRotationFixAndArt)},	
	{CNST_FNC(NullFunc),									CNST_FNC(ApplyImpulseAndPushSlideRotationFixAndMov),	CNST_FNC(ApplyImpulseAndPushSlideRotationFixAndArt)},
	{CNST_FNC(NullFunc),									CNST_FNC(ApplyImpulseAndPushPivotRotationFixAndMov),	CNST_FNC(ApplyImpulseAndPushPivotRotationFixAndArt)},
	{CNST_FNC(ApplyImpulseAndPushMovAndFix),				CNST_FNC(ApplyImpulseAndPushMovAndMov),					CNST_FNC(ApplyImpulseAndPushMovAndArt)},			
	{CNST_FNC(ApplyImpulseAndPushFixedPointMovAndFix),		CNST_FNC(ApplyImpulseAndPushFixedPointMovAndMov),		CNST_FNC(ApplyImpulseAndPushFixedPointMovAndArt)},
	{CNST_FNC(ApplyImpulseAndPushMovAndFix),				CNST_FNC(ApplyImpulseAndPushMovAndMov),					CNST_FNC(ApplyImpulseAndPushMovAndArt)},			
	{CNST_FNC(ApplyImpulseAndPushFixedRotationMovAndFix),	CNST_FNC(ApplyImpulseAndPushFixedRotationMovAndMov),	CNST_FNC(ApplyImpulseAndPushFixedRotationMovAndArt)},	
	{CNST_FNC(ApplyImpulseAndPushSlideRotationMovAndFix),	CNST_FNC(ApplyImpulseAndPushSlideRotationMovAndMov),	CNST_FNC(ApplyImpulseAndPushSlideRotationMovAndArt)},
	{CNST_FNC(ApplyImpulseAndPushPivotRotationMovAndFix),	CNST_FNC(ApplyImpulseAndPushPivotRotationMovAndMov),	CNST_FNC(ApplyImpulseAndPushPivotRotationMovAndArt)},
	{CNST_FNC(ApplyImpulseAndPushArtAndFix),				CNST_FNC(ApplyImpulseAndPushArtAndMov),					CNST_FNC(ApplyImpulseAndPushArtAndArt)},
	{CNST_FNC(ApplyImpulseAndPushFixedPointArtAndFix),		CNST_FNC(ApplyImpulseAndPushFixedPointArtAndMov),		CNST_FNC(ApplyImpulseAndPushFixedPointArtAndArt)},
	{CNST_FNC(ApplyImpulseAndPushArtAndFix),				CNST_FNC(ApplyImpulseAndPushArtAndMov),					CNST_FNC(ApplyImpulseAndPushArtAndArt)},
	{CNST_FNC(ApplyImpulseAndPushFixedRotationArtAndFix),	CNST_FNC(ApplyImpulseAndPushFixedRotationArtAndMov),	CNST_FNC(ApplyImpulseAndPushFixedRotationArtAndArt)},
	{CNST_FNC(ApplyImpulseAndPushSlideRotationArtAndFix),	CNST_FNC(ApplyImpulseAndPushSlideRotationArtAndMov),	CNST_FNC(ApplyImpulseAndPushSlideRotationArtAndArt)},
	{CNST_FNC(ApplyImpulseAndPushPivotRotationArtAndFix),	CNST_FNC(ApplyImpulseAndPushPivotRotationArtAndMov),	CNST_FNC(ApplyImpulseAndPushPivotRotationArtAndArt)},
};
