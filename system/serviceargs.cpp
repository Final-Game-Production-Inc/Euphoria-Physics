#include "serviceargs.h"

#include "diag/channel.h"
#include "system/nelem.h"
#include "string/string.h"

RAGE_DEFINE_CHANNEL(service);

#define serviceAssertf(cond, fmt, ...)     RAGE_ASSERTF(service, cond, fmt, ##__VA_ARGS__)
#define serviceVerifyf(cond, fmt, ...)     RAGE_VERIFYF(service, cond, fmt, ##__VA_ARGS__)
#define serviceDebugf1(fmt, ...)           RAGE_DEBUGF1(service, fmt, ##__VA_ARGS__)

namespace rage
{

const atHashString sysServiceArgs::CLASS_PARAM_NAME = ATSTRINGHASH("class", 0xE45C07DD);
const atHashString sysServiceArgs::MODE_PARAM_NAME = ATSTRINGHASH("mode", 0x68155C83);

sysServiceArgs::sysServiceArgs()
{
	Clear();
}

void sysServiceArgs::Clear()
{
	// clear out system arguments
	sysMemSet(m_Args, 0, sizeof(m_Args));
}

const char* sysServiceArgs::GetString() const
{
	return m_Args;
}

bool sysServiceArgs::HasArgs() const
{
	return m_Args[0] != 0;
}

void sysServiceArgs::Set(const char* args)
{
	serviceDebugf1("Set sysServiceArgs[%s]", args);

	// clear, validate and copy
	Clear();
	serviceAssertf(strlen(args) <= sizeof(m_Args), "Args buffer is too short!");
	safecpy(m_Args, args);

	// convert url '+' characters to spaces
	// url encoding on the arguments converts spaces to '+'
	char* pos = m_Args;
	while (*pos != '\0')
	{
		if (*pos == '+')
		{
			*pos = ' ';
		}
		++pos;
	}
}

bool sysServiceArgs::GetParamValue(const atHashString param, char* buffer, size_t size) const
{
	return GetParamValue(m_Args, sizeof(m_Args), param, buffer, size);
}

bool sysServiceArgs::GetParamValue(const char* argsString, const size_t argsStringSize, const atHashString param, char* buffer, size_t size)
{
	serviceAssertf(size, "Invalid size!");
	sysMemSet(buffer, 0, size);

	unsigned paramLength = 0;
	// Get the start position of this param.
	const char* pos = GetParamStart(argsString, param, &paramLength);
	if (pos)
	{
		// Step over the arg to the value.
		pos += paramLength;

		// Ensure we haven't passed of the end or args (if this is the last arg and doesn't have a value).
		if ((pos - argsString) < static_cast<int>(argsStringSize))
		{
			// If this has a value.
			if (*pos == '=')
			{
				++pos;

				// Copy the value into the buffer.
				while (!isspace(*pos) && *pos != 0 && size > 1)
				{
					*buffer = *pos;
					++buffer;
					++pos;
					--size;
				}
				// Add null terminator.
				*buffer = 0;

				serviceAssertf(size > 1 && (isspace(*pos) || *pos == 0), "Buffer was too small to fit arguments value, value was truncated!");
				return true;
			}
		}
	}

	return false;
}

bool sysServiceArgs::SetParamValue(const char* param, const char* value)
{
	char newStr[MAX_ARG_LENGTH];

	if (!serviceVerifyf(param != nullptr && value != nullptr && param[0] != 0, "Invalid parameters"))
	{
		return false;
	}

	// Simple solution, clear and append at the end.
	// This means the order of the params can change.
	ClearParam(param);

	const unsigned paramLen = static_cast<unsigned>(strlen(param));
	const unsigned valueLen = static_cast<unsigned>(strlen(value));

	if (value[0] == 0 && serviceVerifyf(paramLen < MAX_ARG_LENGTH, "Param too long"))
	{
		safecpy(newStr, param);
	}
	else if (value[0] != 0 && serviceVerifyf(paramLen + valueLen + 1 < MAX_ARG_LENGTH, "Param and value too long"))
	{
		formatf(newStr, "%s=%s", param, value);
	}
	else
	{
		return false;
	}

	if (m_Args[0] == 0)
	{
		Set(newStr);
		return true;
	}

	const unsigned requiredLen = static_cast<unsigned>(strlen(newStr));
	const unsigned currentlen = static_cast<unsigned>(strlen(m_Args));
	const unsigned spaceLen = 1;

	if (serviceVerifyf(currentlen + requiredLen + spaceLen < MAX_ARG_LENGTH, "String too long"))
	{
		safecat(m_Args, " ");
		safecat(m_Args, newStr);
		return true;
	}

	return false;
}

void sysServiceArgs::ClearParam(const atHashString param)
{
	unsigned paramLen = 0;
	char* pos = const_cast<char*>(GetParamStart(m_Args, param, &paramLen));
	char* endpos = pos;

	// The param doesn't exist
	if (pos == nullptr)
	{
		return;
	}

	endpos += paramLen;

	// It's a param with a value so we remove that too
	if (*endpos == '=')
	{
		++endpos;

		while (!isspace(*endpos) && *endpos != 0 && endpos < m_Args + COUNTOF(m_Args))
		{
			++endpos;
		}
	}

	if (!serviceVerifyf(endpos < m_Args + COUNTOF(m_Args), "Out of bounds read"))
	{
		return;
	}

	// Go backwards any leading white spaces
	while (pos > m_Args && isspace(*(pos - 1)))
	{
		--pos;
	}

	// It's the last param in the args so we simply set the 0-terminator
	if (*endpos == 0)
	{
		*pos = 0;

		return;
	}

	// If we're removing the first param we drop any leading spaces from endpos
	if (pos == m_Args)
	{
		while (isspace(*endpos) && *endpos != 0 && endpos < m_Args + COUNTOF(m_Args))
		{
			++endpos;
		}
	}

	memmove(pos, endpos, strlen(endpos) + 1);
}

const char* sysServiceArgs::GetParamStart(const char* argsString, const atHashString param, unsigned* paramLength)
{
	// Skip all leading spaces at the start of the args-string
	while (isspace(*argsString) && *argsString != 0)
	{
		++argsString;
	}

	char buf[MAX_ARG_LENGTH];

	const char* pos = argsString;
	while (*pos != '\0')
	{
		const char* currentParam = pos;

		// To work with and without a '-' char at the front of the pos we ignore it.
		if (*currentParam == '-')
		{
			++currentParam;
		}

		// Find the end of this param (not including the value).
		while (!isspace(*pos) && *pos != 0 && *pos != '=') {
			++pos;
		}

		const unsigned len = static_cast<unsigned>(pos - currentParam);

		safecpy(buf, currentParam, len < MAX_ARG_LENGTH ? (len + 1) : MAX_ARG_LENGTH);
		atHashString currentParamHash(buf);

		// Check if this is the param we are after.
		if (currentParamHash == param)
		{
			if (paramLength != nullptr)
			{
				*paramLength = len;
			}

			return currentParam;
		}

		// Move over the param's value if it has one.
		if (*pos == '=')
		{
			while (!isspace(*pos) && *pos != 0)
			{
				++pos;
			}
		}

		// Move onto the next param.
		while (isspace(*pos) && *pos != 0)
		{
			++pos;
		}
	}

	return NULL;
}


} // namespace rage