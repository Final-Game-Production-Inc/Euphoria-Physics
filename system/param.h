//
// system/param.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_PARAM_H
#define SYSTEM_PARAM_H

#include <stddef.h>

namespace rage {

#if __WIN32
#pragma warning(disable: 4514)
#endif

// Alternate version of command line processing

// TODO: Consider adding __FILE__ and/or __LINE__ here
// as well so people can find where in the code a particular
// option is used.

#define CAN_STRIP_PARAMS 1
#if __NO_OUTPUT
#if CAN_STRIP_PARAMS
	struct stripped_param {
		bool Get() const { return false; }
		bool Get(const char * &) const { return false; }
		bool GetParameter(const char* &) const {return false;}
		void GetDebug(int &) {}
		void GetDebug(u32&) {}
		template <typename T> bool Get(T &) const { return false; }
		template <typename T> int GetArray(T *,int ) const { return 0; }
		int GetArray(const char **,int ,char *,int ) const { return 0; }
		// this function is intentionally NOT implemented!!  
		// if you find yourself compiling some code that is trying to set a stripped param:
		// it should either be wrapped in !__FINAL or implemented as a NOSTRIP_PARAM
		// void Set(const char*) {} 
	};
#define PARAM(x,desc,...)		::rage::stripped_param PARAM_##x // ::rage::sysParam PARAM_##x(0,#x,false)
#define XPARAM(x)			extern ::rage::stripped_param PARAM_##x
#if __WIN32PC
#define NOSTRIP_PC_PARAM(x,desc,...)	::rage::sysParam PARAM_##x(0,#x,false)
#define NOSTRIP_PC_XPARAM(x)		extern ::rage::sysParam PARAM_##x
#else
#define NOSTRIP_PC_PARAM(x,desc,...)	PARAM(x,desc)
#define NOSTRIP_PC_XPARAM(x)		XPARAM(x)
#endif
#else // CAN_STRIP_PARAMS
// PURPOSE: Declare a new named parameter
// PARAMS:
//	x - The name of the new parameter.
//	desc - The help text for the parameter
// NOTES
//	Access this parameter in your code as PARAM_x
//	On the command line, use -x to specify that parameter and give it arguments
// <FLAG Component>
#define PARAM(x,desc)		::rage::sysParam PARAM_##x(0,#x,false)
// PURPOSE: An extern declaration for a PARAM
#define XPARAM(x)			extern ::rage::sysParam PARAM_##x
#define NOSTRIP_PC_PARAM(x,desc)	PARAM(x,desc)
#define NOSTRIP_PC_XPARAM(x)		XPARAM(x)
#endif
// PURPOSE: Declare a new fixed parameter
// PARAMS:
//	i - The position of the parameter
//	x - The name of the new parameter
//	desc - The help text for the parameter
// NOTES:
//	Access this parameter in your code as PARAM_x
//	FPARAMs are used for positional parameters, for example
//	in the command "move 'src' 'dest'", 'src' is a positional
//	parameter with index 1, 'dest' is a positional parameter
//	with index 2
#define FPARAM(i,x,desc)	::rage::sysParam PARAM_##x(i,#x,false)
// PURPOSE: Declare a new required parameter
// PARAMS:
//   x - The name of the new parameter.
//   desc - The help text for the parameter
// NOTES
//  Access this parameter in your code as PARAM_X
//  On the command line, use -x to specify that parameter and give it arguments.
//  If this parameter is not on the command line, the program will print help text and exit.
// <FLAG Component>
#define REQ_PARAM(x, desc)	CompileTimeAssert(0) // Required parameters are not available for __FINAL builds
#define NOSTRIP_PARAM(x,desc,...)		::rage::sysParam PARAM_##x(0,#x,false)
#define NOSTRIP_XPARAM(x)			extern ::rage::sysParam PARAM_##x
#else
#define PARAM(x,desc,...)	::rage::sysParam PARAM_##x(0,#x,false,desc)
#define FPARAM(i,x,desc)	::rage::sysParam PARAM_##x(i,#x,false,desc)
#define REQ_PARAM(x, desc)	::rage::sysParam PARAM_##x(0,#x,true,desc)
#define XPARAM(x)			extern ::rage::sysParam PARAM_##x
#define NOSTRIP_PARAM(x,desc,...)	::rage::sysParam PARAM_##x(0,#x,false,desc)
#define NOSTRIP_XPARAM(x)			extern ::rage::sysParam PARAM_##x
#define NOSTRIP_PC_PARAM(x,desc,...)	NOSTRIP_PARAM(x,desc)
#define NOSTRIP_PC_XPARAM(x)		NOSTRIP_XPARAM(x)
#endif

#if __FINAL_LOGGING
#define NOSTRIP_FINAL_LOGGING_PARAM(x,desc) NOSTRIP_PARAM(x,desc)
#else
#define NOSTRIP_FINAL_LOGGING_PARAM(x,desc) PARAM(x,desc)
#endif

/*
PURPOSE
This class is designed to process the command line parameters.
<FLAG Component>
*/
class sysParam {
public:

	// PURPOSE
	//   Creates a new sysParam and adds it to the global list.
	// PARAMS
	//   index - The location of a fixed parameter
	//   name - The parameter name
	//   desc - The help text for the parameter
	sysParam(
		int index,
		const char *name,
		bool required
#if !__NO_OUTPUT
		,const char *desc
#endif
		);

	//PURPOSE
	//	Scan all command line parameters and initialize all sysParam objects.
	//	Called automatically by startup code.
	//PARAMS
	//	argc - argument count.
	//	argv - argument list.
	static void Init(int argc,char **argv);

#if !__NO_OUTPUT

	//PURPOSE: Display usage help by iterating over all registered sysParam objects.
	//PARAMS
	//	section - Which section to restrict help output for (gfx, simple, etc). Must start with an equals , e.g. '=simple'.
	//  sectionNamesOnly - Only print the section names
	//NOTES
	//	A 'section' is simply any identifier in [brackets] at the start of
	//	the parameter help text; it is stripped off here while printing.
	static void Help(const char *section = NULL, bool sectionNamesOnly = false);
#endif

	// RETURNS: True if flag is present on command line
	bool Get() const;

	// PURPOSE
	//   Get a string parameter from the command line
	// PARAMS
	//   ptr - The pointer that should be modified to point at the command line argument
	// RETURNS
	//   True if flag is present on command line, and stores any parameter to
	//   the flag in 'ptr'.  ptr will contain null string if flag is present but it
	//   has no parameters.
	bool Get(const char * &ptr) const;

	// PURPOSE
	//   Get a string parameter from the command line
	// PARAMS
	//   ptr - The pointer that should be modified to point at the command line argument
	// RETURNS
	//   True if flag is present on command line and it contains a parameter, in which
	//   case ptr will contain its value.  This is different from the normal Get because
	//   it will not modify ptr (or return true) if only a flag alone is on the command
	//   line.  This makes it easier to have an optional parameter that otherwise has
	//   a default value.
	bool GetParameter(const char * &ptr) const;

	// PURPOSE
	//   Get an integer parameter value from the command line.
	// PARAMS
	//	 i - the variable to store the integer value in
	// RETURNS
	//   True if there was a value to retrieve, else false
	bool Get(int &i) const;

	// PURPOSE
	//   Get a size_t parameter value from the command line.
	// PARAMS
	//	 i - the variable to store the size_t value in
	// RETURNS
	//   True if there was a value to retrieve, else false
	bool Get(u32 &i) const;

	// PURPOSE
	//   Get a float parameter value from the command line
	// PARAMS
	//	 f - the variable to store the float value in
	// RETURNS
	//   True if there was a value to retrieve, else false
	bool Get(float &f) const;

	// PURPOSE
	//   Safer form of Get(int &i)
	// PARAMS
	//	 i - the output. Receives 1 if the parameter is on the command line but has no int argument
	void GetDebug(int &i) const;

	// PURPOSE
	//   Safer form of Get(size_t &i)
	// PARAMS
	//	 i - the output. Receives 1 if the parameter is on the command line but has no int argument
	void GetDebug(u32 &i) const;

	// PURPOSE
	//   Retrieve multiple comma-separated values from the parameter.
	// PARAMS
	//   array - Pointer to storage for retrieved values
	//   count - Number of values to retrieve
	// RETURNS
	//   Number of values stored in array
	int GetArray(int *array,int count) const;
	int GetArray(float *array,int count) const;

	// PURPOSE
	//   Retrieve multiple comma-separated values from the parameter.
	// PARAMS
	//   array - Pointers to each null-terminated suboption
	//   count - Maximum number of suboption values to retrieve
	//   buffer - Storage area, supplied by caller; each 'array' entry points into this buffer.
	//   bufferSize - Size of the storage area.
	// RETURNS
	//   Number of suboption values stored in the array.
	int GetArray(const char **array,int count,char *buffer,int bufferSize) const;

	// PURPOSE
	//   Set a command line parameter
	// PARAMS
	//   ptr - Pointer to the new value of the parameter
	// NOTES
	//   The pointer must point into statically allocated storage!
	void Set(const char * ptr);

	// PURPOSE
	//   Get the name of the program
	// RETURNS
	//   The name of the program
	// NOTES
	//   This is the same as GetArg(0)
	static const char *GetProgramName();

	// PURPOSE
	//   Get the number of arguments on the argument list
	// RETURNS
	//   The number of arguments the user supplied (just like argc in a normal C main entry point)
	static int GetArgCount();

	// PURPOSE
	//   Get the ith argument on the argument list
	// PARAMS
	//   i - The index of the arguement you want
	// RETURNS
	//   A pointer to the parameter
	// NOTES
	//   i must be non-negative and less than GetArgCount
	static const char *GetArg(int i);

	// PURPOSE
	//   Return the entire argv array
	// RETURNS
	//   The argv array, just like normal C main entry point.
	static char **GetArgArray();

	// PURPOSE
	//   Search the command line (including response files) for a named parameter outside of the normal sysParam system.
	//   Intended for use by script, etc.
	// RETURNS
	//    Pointer to the parameter value (or empty string if the parameter is there but no value, or NULL if parameter is not present at all)
	static const char *SearchArgArray(const char* param);

	// PURPOSE
	//   Retrieve an existing sysParam object's value by name
	// PARAMS
	//   arg - Arg to match (not case-sensitive, ignores trailing s)
	//   defValue - Default value to return if the flag is present
	//				but doesn't have any data (ie <C>-flag</C> instead of
	//				<C>-flag=something</C>.
	// RETURNS
	//   Value of the flag if it was present (defValue if it was
	//   present but has no data), or NULL if the flag was not present
	//   or is an unknown parameter entirely.
	static const char *GetByName(const char *arg,const char *defValue);
	static sysParam *GetParamByName(const char *arg);

	// PURPOSE
	//   Check whether this class has been initialized.
	// RETURNS
	//   True if the class has been initialized, false otherwise.
	static bool IsInitialized()
	{
		return (sm_Initialized);
	}

	// PURPOSE
	//	Access the static list of PARAM objects.
	// RETURNS
	//	Pointer to the first static parameter object, from which all of them may be iterated via GetNext().
	static const sysParam* GetFirst()
	{
		return sm_First;
	}

	// PURPOSE
	//	Access the next static PARAM object.
	// RETURNS
	//	Pointer to the next parameter object linked to this one, or NULL.
	const sysParam* GetNext() const
	{
		return m_Next;
	}

	// PURPOSE
	//	Procedural access to the parameter name.
	// RETURNS
	//	Null terminated pointer to the name of this parameter object.
	const char* GetName() const
	{
		return m_Name;
	}


#if !__NO_OUTPUT
	// PURPOSE
	//	Procedural access to the static parameter description.
	// RETURNS
	//	Null terminated pointer to the description for this parameter object.
	const char* GetDescription() const
	{
		return m_Desc;
	}
#endif

#if !__FINAL
	// EJ: Command-line functions for use when sysParam isn't loaded into memory
	static bool FindCommandLineArg(const char* s, char** ppValue = NULL);
	static int GetCommandLineArgInt(const char* s, int defaultValue = 0);
	static float GetCommandLineArgFloat(const char* s, float defaultValue = 0.0f);	
#endif

private:
	// PURPOSE: Copy the section name (the beginning of the description, inside [brackets] into dest.
	// PARAMS:
	//	dest - Destination buffer
	//	destSize - max size of the dest buffer
	// RETURN: A pointer to the first non-section character in the description
	// NOTES:
	//	The value in dest will be the section name without the brackets.
	const char *GetSectionName(char *dest,int destSize);

	const char *m_Name;
#if !__NO_OUTPUT
	const char *m_Desc;
#endif
	const char *m_Value;
	int m_Index;
	sysParam *m_Next;
	bool m_Required;

	static sysParam *sm_First;
	public:
	static const char *sm_ProgName;
	private:
	static int sm_Argc;
	static char **sm_Argv;
	static bool sm_Initialized;
};


// Implementation

inline bool sysParam::Get() const
{
	return m_Value != 0;
}

inline bool sysParam::Get(const char * &ptr) const 
{
	FastAssert(sm_Initialized);
	if (m_Value)
	{
		ptr = m_Value;
		return true;
	}
	else
	{
		return false;
	}
}

inline bool sysParam::GetParameter(const char * &ptr) const 
{
	FastAssert(sm_Initialized);
	if (m_Value && *m_Value)
	{
		ptr = m_Value;
		return true;
	}
	else
	{
		return false;
	}
}

inline void sysParam::GetDebug(int &i) const {
	FastAssert(sm_Initialized);
	if (!Get(i) && Get())
	{
		i = 1;
	}
}

inline void sysParam::GetDebug(u32 &i) const {
	FastAssert(sm_Initialized);
	if (!Get(i) && Get())
	{
		i = 1;
	}
}

inline void sysParam::Set(const char * ptr)
{
	m_Value = ptr;
}

inline const char* sysParam::GetProgramName()
{
	return sm_ProgName;
}

inline int sysParam::GetArgCount()
{
	FastAssert(sm_Initialized);
	return sm_Argc;
}

inline const char* sysParam::GetArg(int i)
{
	FastAssert(sm_Initialized);
	FastAssert(i>=0&&i<sm_Argc);
	return sm_Argv[i];
}

inline char** sysParam::GetArgArray()
{
	FastAssert(sm_Initialized);
	return sm_Argv;
}

} // namespace rage

#endif
