//
// system/appcontent.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_APP_CONTENT_H
#define SYSTEM_APP_CONTENT_H

namespace rage
{
	class sysAppContent
	{
	public:
		enum BuildVersion
		{
			Build_Unknown = 0,
			Build_European = 0x01,
			Build_American = 0x02,
			Build_Japanese = 0x04
		};

		// PURPOSE
		// Initializes the App Content library
		//	PS4:: loading the required module
		static bool Init();

		// PURPOSE
		// Shuts down the App Content library
		//	PS4: unloading the module
		static bool Shutdown();

		// PURPOSE
		//	Returns true if initialized
		static bool IsInitialized() { return sm_bIsInitialized; }

#if RSG_DURANGO
		static void SetTitleId(unsigned titleId) { m_TitleId = titleId; }
		static void SetJapaneseTitleId(unsigned titleId) { m_JapaneseTitleId = titleId; }
#endif

		// PURPOSE
		// Returns true if the build is American/European etc
		static bool IsAmericanBuild();
		static bool IsEuropeanBuild();
		static bool IsJapaneseBuild();

		static const char* GetBuildLocaleCode();

#if !__NO_OUTPUT
		// PURPOSE
		// Returns the output string of the current build language
		static const char* GetBuildLocaleString();
#endif

	private:
		static bool sm_bIsInitialized;
		static int sm_BuildVersion;

#if RSG_DURANGO
		static unsigned m_TitleId;
		static unsigned m_JapaneseTitleId;
#endif
	};
}

#endif // SYSTEM_APP_CONTENT_H