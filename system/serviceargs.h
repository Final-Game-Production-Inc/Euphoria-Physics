#ifndef SYSTEM_SERVICE_PARAM_H
#define SYSTEM_SERVICE_PARAM_H

#include "atl/hashstring.h"

namespace rage
{

class sysServiceArgs
{
public:
	// PURPOSE:	The maximum arguments length from the system (including NULL terminator).
	static const u32 MAX_ARG_LENGTH = 256u;
	static const atHashString CLASS_PARAM_NAME;
	static const atHashString MODE_PARAM_NAME;

	sysServiceArgs();

	// PURPOSE: Clears the launch args
	void Clear();

	// PURPOSE: Returns the raw string
	const char* GetString() const;

	// PURPOSE: Returns true if an arg is set
	bool HasArgs() const;

	// PURPOSE: Set new launch args
	// PARAMS: the args in the format "argkeya=valX argkeyb=valY"
	void Set(const char* args);

	// PURPOSE: Retrieves a specified params value.
	// PARAMS:	param - the param to check.
	//			buffer - the buffer to receive the value.
	//			bufferSize - the size of buffer.
	// RETURNS:	true if the param exists.
	bool GetParamValue(const atHashString param, char* buffer, size_t size) const;
	template <size_t _Size>
	bool GetParamValue(const atHashString param, char(&buffer)[_Size]) const;

	// PURPOSE : Sets or modifies a parameter on the launch args
	bool SetParamValue(const char* param, const char* value);

	// PURPOSE: Removes a parameter from the launch args
	void ClearParam(const atHashString param);

	// PURPOSE:	Returns true if the specified param exists.
	// PARAMS:	param - the param to check.
	// RETURNS:	true if the param exists.
	bool HasParam(const atHashString param) const;

	// PURPOSE: Retrieves a specified params value from an external argString
	// PARAMS:	argsString - the string containing the args
	//			param - the param to check.
	//			buffer - the buffer to receive the value.
	//			bufferSize - the size of buffer.
	// RETURNS:	true if the param exists.
	static bool GetParamValue(const char* argsString, const size_t argsStringSize, const atHashString param, char* buffer, size_t size);
	template <size_t _Size>
	static bool GetParamValue(const char* argsString, const size_t argsStringSize, const atHashString param, char(&buffer)[_Size]);

	// PURPOSE:	Returns true if the specified param exists in argsString
	static bool HasParam(const char* argsString, const atHashString param);

	// PURPOSE: Gets the start of a given param without the '-' char.
	// PARAMS:	argsString - the string containing the args
	//			param - the param to search for
	//			paramLength - the length of the param name (does not include the param value or the = character)
	// RETURN:	The start of a given param or NULL if it does not exist.
	// NOTE:	The string may still have other params after the requested one.
	static const char* GetParamStart(const char* argsString, const atHashString param, unsigned* paramLength);

public:
	// PURPOSE:	The buffer containing the last argument string from the console.
	char m_Args[MAX_ARG_LENGTH];
};

inline bool sysServiceArgs::HasParam(const atHashString param) const
{
	return GetParamStart(m_Args, param, nullptr) != NULL;
}

inline bool sysServiceArgs::HasParam(const char* argsString, const atHashString param)
{
	return GetParamStart(argsString, param, nullptr) != nullptr;
}

template <size_t _Size>
inline bool sysServiceArgs::GetParamValue(const atHashString param, char(&buffer)[_Size]) const
{
	return GetParamValue(param, buffer, _Size);
}

template <size_t _Size>
inline bool sysServiceArgs::GetParamValue(const char* argsString, const size_t argsStringLength, const atHashString param, char(&buffer)[_Size])
{
	return GetParamValue(argsString, argsStringLength, param, buffer, _Size);
}


} //namespace rage


#endif //SYSTEM_SERVICE_PARAM_H