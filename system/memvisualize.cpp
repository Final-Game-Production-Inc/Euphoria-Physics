
#include "memvisualize.h"

#if RAGE_TRACKING
#include "system/memory.h"

namespace rage {

sysMemVisualize::sysMemVisualize() : m_bits(0) { }

sysMemVisualize& sysMemVisualize::GetInstance()
{
	static sysMemVisualize s_instance;
	return s_instance;
}

bool sysMemVisualize::Get(size_t pos) const
{ 
	const size_t mask = (static_cast<size_t>(1) << pos);
	bool value = (mask == (m_bits & mask));
	return value;
}

void sysMemVisualize::Set(const char* psz)
{
	char ch = *psz;
	while (ch != ' ' && ch != '\0')
	{
		Set(ch);
		ch = *(++psz);
	}
}

void sysMemVisualize::Set(char ch)
{
#if __XENON || RSG_DURANGO || RSG_PC
	const char* pszFilter = "adfghmnprsuvxy";
#else
	const char* pszFilter = "adfghmnprsuy";
#endif

	const char* psz = strchr(pszFilter, ch);

	if (psz)
	{
		const size_t pos = psz - pszFilter;
		if (pos < static_cast<size_t>(MAX))
			m_bits |= (static_cast<size_t>(1) << pos);
	}
}

void sysMemVisualize::SetAll()
{
	m_bits = ~static_cast<size_t>(0);
}
} // namespace rage

#endif // RAGE_TRACKING
