// 
// fragmentnm/nmbehavior.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 


#include "nmbehavior.h"
#include "nmbehavior_parser.h"

#include "assert.h"
#include "atl/array.h"
#include "bank/bank.h"
#include "bank/button.h"
#include "bank/group.h"
#include "bank/slider.h"
#include "bank/text.h"
#include "bank/toggle.h"
#include "bank/bkvector.h"
#include "file/asset.h"
#include "parser/manager.h"
#include "art/messageparams.h"

using namespace rage;

namespace rage
{

//#############################################################################

NMValue::NMValue()
{
	m_name[0] = 0;
}

NMValue::NMValue( const char* name )
{
	safecpy( m_name, name, 64 );
}

NMValue::~NMValue()
{
}

//#############################################################################

char NMValueInt::m_stringValue[64];

NMValueInt::NMValueInt()
: NMValue()
, m_value(0)
{
}

NMValueInt::NMValueInt( const char* name, int i )
: NMValue(name)
, m_value(i)
{
}

NMValueInt::NMValueInt( const char* name, const char* value )
: NMValue(name)
{
	SetValueFromString( value );
}

NMValueInt::~NMValueInt()
{
}

const char* NMValueInt::GetStringValue() const
{
	sprintf( m_stringValue, "%d", m_value );
	return m_stringValue;
}

void NMValueInt::SetValueFromString( const char* value )
{
	if ( value != NULL )
	{
		m_value = atoi(value);
	}
	else
	{
		m_value = 0;
	}
}

#if __BANK
bkWidget* NMValueInt::AddWidget( bkBank &bk, const NMParam &p )
{
	return bk.AddSlider( m_name, &m_value, (int)p.GetMinValue(), (int)p.GetMaxValue(), (int)p.GetStepValue(), NullCB, p.GetDescription() ) ;
}
#endif

//#############################################################################

NMValueBool::NMValueBool()
: NMValue()
, m_value(false)
{
}

NMValueBool::NMValueBool( const char* name, bool b )
: NMValue(name)
, m_value(b)
{
}

NMValueBool::NMValueBool( const char* name, const char* value )
: NMValue(name)
{
	SetValueFromString( value );
}

NMValueBool::~NMValueBool()
{
}

const char* NMValueBool::GetStringValue() const
{
	return m_value ? "true" : "false";
}

void NMValueBool::SetValueFromString( const char* value )
{
	if ( value != NULL )
	{
		m_value = strcmpi(value,"true") == 0;
	}
	else
	{
		m_value = false;
	}
}

#if __BANK
bkWidget* NMValueBool::AddWidget( bkBank &bk, const NMParam &p )
{
	return bk.AddToggle( m_name, &m_value, NullCB, p.GetDescription() );
}
#endif

//#############################################################################

char NMValueFloat::m_stringValue[64];

NMValueFloat::NMValueFloat()
: NMValue()
, m_value(0.0f)
{
}

NMValueFloat::NMValueFloat( const char* name, float f )
: NMValue(name)
, m_value(f)
{
}

NMValueFloat::NMValueFloat( const char* name, const char* value )
: NMValue(name)
{
	SetValueFromString( value );
}

NMValueFloat::~NMValueFloat()
{
}

const char* NMValueFloat::GetStringValue() const
{
	sprintf( m_stringValue, "%f", m_value );
	return m_stringValue;
}

void NMValueFloat::SetValueFromString( const char* value )
{
	if ( value != NULL )
	{
		m_value = static_cast<float>( atof(value) );
	}
	else
	{
		m_value = 0.0f;
	}
}

#if __BANK
bkWidget* NMValueFloat::AddWidget( bkBank &bk, const NMParam &p )
{
	return bk.AddSlider( m_name, &m_value, p.GetMinValue(), p.GetMaxValue(), p.GetStepValue(), NullCB, p.GetDescription()) ;
}
#endif

//#############################################################################

char NMValueVector3::m_stringValue[64];

NMValueVector3::NMValueVector3()
: NMValue()
, m_value(ORIGIN)
{
}

NMValueVector3::NMValueVector3( const char* name, const Vector3 & v )
: NMValue(name)
, m_value(v)
{
}

NMValueVector3::NMValueVector3( const char* name, const char* value )
: NMValue(name)
{
	SetValueFromString( value );
}

NMValueVector3::~NMValueVector3()
{
}

const char* NMValueVector3::GetStringValue() const
{
	sprintf( m_stringValue, "%f %f %f", m_value.x, m_value.y, m_value.z );
	return m_stringValue;
}

void NMValueVector3::SetValueFromString( const char* value )
{
	if ( value != NULL )
	{
		// The input string may or may not be comma-delimeted.
		if( strstr(value, ",") )
		{
			ASSERT_ONLY(int numSet =)
				sscanf(value, "%f, %f, %f", &m_value.x, &m_value.y, &m_value.z);
			Assert(numSet==3);
		}
		else
		{
			ASSERT_ONLY(int numSet =)
				sscanf(value, "%f %f %f", &m_value.x, &m_value.y, &m_value.z);
			Assert(numSet==3);
		}
	}
	else
	{
		m_value.Zero();
	}
}

#if __BANK
bkWidget* NMValueVector3::AddWidget( bkBank &bk, const NMParam &p )
{
	return bk.AddVector( m_name, &m_value, p.GetMinValue(), p.GetMaxValue(), p.GetStepValue(), NullCB, p.GetDescription() ) ;
}
#endif

//#############################################################################

NMValueString::NMValueString()
: NMValue()
{
	m_value[0] = 0;
}

NMValueString::NMValueString( const char* name, const char* s )
: NMValue(name)
{
	SetValue( s );
}

NMValueString::~NMValueString()
{
}

void NMValueString::SetValueFromString( const char* value )
{
	SetValue( value );
}

void NMValueString::SetValue( const char* s )
{
	if ( s != NULL )
	{
		safecpy( m_value, s, 64 );
	}
	else
	{
		m_value[0] = 0;
	}
}

#if __BANK
bkWidget* NMValueString::AddWidget( bkBank &bk, const NMParam & /*p*/ )
{
	// FIXME: apparently can't add description to Text Widgets
	return bk.AddText( m_name, m_value, 255, false, NullCB );
}
#endif

//#############################################################################

NMParam::NMParam()
: m_min(-1000000)
, m_max(1000000)
, m_step(0.0f)
{
	SetName( NULL );
	SetType( NULL );
	SetDescription( NULL );
	SetInitValue( NULL );
}

NMParam::NMParam( const char* name, const char* type, const char* description, char *init, float min, float max, float step )
: m_min(min)
, m_max(max)
, m_step(step)
{
	SetName( name );
	SetType( type );
	SetDescription( description );
	SetInitValue( init );
}

NMParam::~NMParam()
{
}

void NMParam::SetName( const char *name )
{
	if ( name != NULL )
	{
		safecpy( m_name, name, 64 );
	}
	else
	{
		m_name[0] = 0;
	}
}

bool NMParam::IsBool() const
{
	return strnicmp(m_type,"bool",4) == 0;
}

bool NMParam::IsInt() const
{
	return strnicmp(m_type,"int",3) == 0;
}

bool NMParam::IsFloat() const
{
	return strnicmp(m_type,"float",5) == 0; 
}

bool NMParam::IsVector3() const
{
	return strnicmp(m_type,"vector3",7) == 0;
}

bool NMParam::IsString() const
{
	return strnicmp(m_type,"string",6) == 0;
}

void NMParam::SetType( const char* type )
{
	if ( type != NULL )
	{
		safecpy( m_type, type, 8 );
	}
	else
	{
		m_type[0] = 0;
	}
}

void NMParam::SetDescription( const char* description )
{
	m_description.SetFromString(description);
}

void NMParam::SetInitValue( const char* init )
{
	if ( init != NULL )
	{
		safecpy( m_init, init, 64 );
	}
	else
	{
		m_init[0] = 0;
	}
}

float NMParam::GetMinValue() const
{
	assert( IsFloat() || IsInt() || IsVector3() );
	return m_min;
}

void NMParam::SetMinValue( float min )
{
	assert( IsFloat() || IsInt() || IsVector3() );
	m_min = min;
}

float NMParam::GetMaxValue() const
{
	assert( IsFloat() || IsInt() || IsVector3() );
	return m_max;
}

void NMParam::SetMaxValue( float max )
{
	assert( IsFloat() || IsInt() || IsVector3() );
	m_max = max;
}

float NMParam::GetStepValue() const
{
	assert( IsFloat() || IsInt() || IsVector3() );
	return m_step;
}

void NMParam::SetStepValue( float step )
{
	assert( IsFloat() || IsInt() || IsVector3() );
	m_step = step;
}

//#############################################################################

NMBehavior::NMBehavior()
{
	SetName( NULL );
	SetDescription( NULL );
}

NMBehavior::NMBehavior( const char *name, const char *description )
{
	SetName( name );
	SetDescription( description );

	// make sure we always have "start"
	NMParam *p = new NMParam( "start", "bool" );
	m_params.Grow() = p;
}

NMBehavior::~NMBehavior()
{
	int count = m_params.GetCount();
	for (int i = 0; i < count; ++i)
	{
		delete m_params[i];
	}
	m_params.clear();
}

void NMBehavior::SetName( const char* name )
{
	if ( name != NULL )
	{
		safecpy( m_name, name, 64 );
	}
	else
	{
		m_name[0] = 0;
	}
}

void NMBehavior::SetDescription( const char* description )
{
	m_description.SetFromString(description);
}

void NMBehavior::AddParam( NMParam *pParam )
{
	const NMParam *pExists = GetParam( pParam->GetName() );
	if ( pExists != NULL )
	{
		// don't allow duplicate names
		return;
	}

	m_params.Grow() = pParam;
}

const NMParam* NMBehavior::GetParam( const char* name ) const
{
	int count = m_params.GetCount();
	for (int i = 0; i < count; ++i)
	{
		NMParam *p = m_params[i];
		if ( strcmp(p->GetName(),name) == 0 )
		{
			return p;
		}
	}

	return NULL;
}

const NMParam* NMBehavior::GetParam( int index ) const
{
	if ( index < m_params.GetCount() )
	{
		return m_params[index];
	}

	return NULL;
}

int NMBehavior::GetNumParams() const
{
	return m_params.GetCount();
}

//#############################################################################

NMBehaviorPool::NMBehaviorPool()
{
}

NMBehaviorPool::~NMBehaviorPool()
{
	int count = m_behaviors.GetCount();
	for (int i = 0; i < count; ++i)
	{
		delete m_behaviors[i];
	}
	m_behaviors.clear();
}

void NMBehaviorPool::AddBehavior( NMBehavior *pBehavior )
{
	const NMBehavior *pExists = GetBehavior( pBehavior->GetName() );
	if ( pExists != NULL )
	{
		// don't allow duplicate names
		return;
	}

	m_behaviors.Grow() = pBehavior;
}

const NMBehavior* NMBehaviorPool::GetBehavior( const char* name ) const
{
	int count = m_behaviors.GetCount();
	for (int i = 0; i < count; ++i)
	{
		NMBehavior *b = m_behaviors[i];
		if ( strcmp(b->GetName(),name) == 0 )
		{
			return b;
		}
	}

	return NULL;
}

const NMBehavior* NMBehaviorPool::GetBehavior( int index ) const
{
	if ( index < m_behaviors.GetCount() )
	{
		return m_behaviors[index];
	}

	return NULL;
}

int NMBehaviorPool::GetNumBehaviors() const
{
	return m_behaviors.GetCount();
}

bool NMBehaviorPool::LoadFile( const char* file )
{
	const char *ext = ASSET.FindExtensionInPath( file );
	if ( ext != NULL )
	{
		return PARSER.LoadObject( file, "", *this );
	}
	else
	{
		return PARSER.LoadObject( file, "xml", *this );
	}
}

#if __BANK
bool NMBehaviorPool::SaveFile( const char* file )
{
	const char *ext = ASSET.FindExtensionInPath( file );
	if ( ext != NULL )
	{
		return PARSER.SaveObject( file, "", this, parManager::XML );
	}
	else
	{
		return PARSER.SaveObject( file, "xml", this, parManager::XML );
	}
}
#endif // __BANK

//#############################################################################

NMBehaviorInst::NMBehaviorInst()
: m_pBehavior(NULL)
#if __BANK
, m_pBkGroup(NULL)
#endif
{
	SetName( NULL );
	SetBehavior( NULL );
	SetDescription( NULL );

}

NMBehaviorInst::NMBehaviorInst( const char* name, const NMBehavior *pBehavior, const char* description )
#if __BANK
: m_pBkGroup(NULL)
#endif
{
	SetName( name );
	SetBehavior( pBehavior );
	SetDescription( description );
}

NMBehaviorInst::~NMBehaviorInst()
{
	int count = m_values.GetCount();
	for (int i = 0; i < count; ++i)
	{
		delete m_values[i];
	}
	m_values.clear();
}

void NMBehaviorInst::SetName( const char* name )
{
	if ( name != NULL )
	{
		safecpy( m_name, name, 64 );
	}
	else
	{
		m_name[0] = 0;
	}
}

void NMBehaviorInst::SetDescription( const char* description )
{
	m_description.SetFromString(description);
}

void NMBehaviorInst::SetDescriptionHash( const atHashString& description )
{
	m_description = description;
}

void NMBehaviorInst::AddValue( NMValue *pValue )
{
	NMValue *pExists = GetValue( pValue->GetName() );
	if ( pExists != NULL )
	{
		// don't allow duplicate names
		return;
	}

	if ( m_pBehavior != NULL )
	{
		// make sure value matches param
		ASSERT_ONLY(const NMParam *p = m_pBehavior->GetParam( pValue->GetName() );)
		Assert( p != NULL );
		Assert(
			(p->IsInt() == pValue->IsInt()) &&
			(p->IsBool() == pValue->IsBool()) &&
			(p->IsFloat() == pValue->IsFloat()) &&
			(p->IsVector3() == pValue->IsVector3()) &&
			(p->IsString() == pValue->IsString()) );
	}

	m_values.Grow() = pValue;
}

NMValue* NMBehaviorInst::GetValue( const char* name ) const
{
	int count = m_values.GetCount();
	for (int i = 0; i < count; ++i)
	{
		NMValue *v = m_values[i];
		if ( strcmp(v->GetName(),name) == 0 )
		{
			return v;
		}
	}

	return NULL;
}

NMValue* NMBehaviorInst::GetValue( int index ) const
{
	if ( index < m_values.GetCount() )
	{
		return m_values[index];
	}

	return NULL;
}

int NMBehaviorInst::GetNumValues() const
{
	return m_values.GetCount();
}

void NMBehaviorInst::ConfigureMessage( ART::MessageParamsBase * msg ) const
{
	int count = m_pBehavior->GetNumParams();
	for (int i = 0; i < count; ++i)
	{
		const NMParam *p = m_pBehavior->GetParam( i );
		NMValue *v = GetValue( p->GetName() );
		if ( v != NULL )
		{
			if ( p->IsInt() )
			{
				NMValueInt *i = dynamic_cast<NMValueInt *>( v );
				msg->addInt( i->GetName(), i->GetValue() );
			}
			else if ( p->IsBool() )
			{
				NMValueBool *b = dynamic_cast<NMValueBool *>( v );
				msg->addBool( b->GetName(), b->GetValue() );
			}
			else if ( p->IsFloat() )
			{
				NMValueFloat *f = dynamic_cast<NMValueFloat *>( v );
				msg->addFloat( f->GetName(), f->GetValue() );
			}
			else if ( p->IsVector3() )
			{
				NMValueVector3 *v3 = dynamic_cast<NMValueVector3 *>( v );
				msg->addVector3( v3->GetName(), v3->GetValue().x, v3->GetValue().y, v3->GetValue().z );
			}
			else if ( p->IsString() )
			{
				NMValueString *s = dynamic_cast<NMValueString *>( v );
				msg->addString( s->GetName(), s->GetValue() );
			}
		}
	}
}

void NMBehaviorInst::VerifyParamValues()
{
	if ( m_pBehavior == NULL )
	{
		return;
	}

	// make sure all values match a param
	int count = m_values.GetCount();
	for (int i = 0; i < count; ++i)
	{
		ASSERT_ONLY(NMValue *v = m_values[i];)
		ASSERT_ONLY(const NMParam *p = m_pBehavior->GetParam( v->GetName() );)	  
		Assert( p != NULL );
		Assert(
			(p->IsInt() == v->IsInt()) &&
			(p->IsBool() == v->IsBool()) &&
			(p->IsFloat() == v->IsFloat()) &&
			(p->IsVector3() == v->IsVector3()) &&
			(p->IsString() == v->IsString()) );
	}

	// make sure we have a value created for each param, if not, create one with the init value
	count = m_pBehavior->GetNumParams();
	for (int i = 0; i < count; ++i)
	{
		const NMParam *p = m_pBehavior->GetParam( i );
		NMValue *v = GetValue( p->GetName() );
		if ( v == NULL )
		{
			if( p->IsInt() )
			{
				v = rage_new NMValueInt( p->GetName(), p->GetInitValue() );
			}
			else if ( p->IsBool() )
			{
				v = rage_new NMValueBool( p->GetName(), p->GetInitValue() );
			}
			else if ( p->IsFloat() )
			{
				v = rage_new NMValueFloat( p->GetName(), p->GetInitValue() );
			}
			else if ( p->IsVector3() )
			{
				v = rage_new NMValueVector3( p->GetName(), p->GetInitValue() );
			}
			else if ( p->IsString() )
			{
				v = rage_new NMValueString( p->GetName(), p->GetInitValue() );
			}
			else
			{
				continue;
			}

			m_values.Grow() = v;
		}
	}
}

void NMBehaviorInst::Init()
{
	if ( m_pBehavior == NULL )
	{
		return;
	}
	
	SetName( m_pBehavior->GetName() );
	SetDescriptionHash( m_pBehavior->GetDescriptionHash() );

	int count = m_values.GetCount();
	for (int i = 0; i < count; ++i)
	{
		const NMParam *pParam = m_pBehavior->GetParam( m_values[i]->GetName() );
		if ( pParam == NULL )
		{
			continue;
		}

		m_values[i]->SetValueFromString( pParam->GetInitValue() );
	}
}

#if __BANK
void NMBehaviorInst::AddWidgets( bkBank &bk )
{
	RemoveWidgets( bk );

	m_pBkGroup = bk.PushGroup( m_name, true, GetDescription() );
	{
		m_widgets.Grow() = bk.AddButton( "Reset To Default", datCallback(MFA(NMBehaviorInst::Init),this) );
		m_widgets.Grow() = bk.AddText( "Instance Name", m_name, 64 );
		m_widgets.Grow() = bk.AddText( "Instance Description", &m_description );

		int count = m_pBehavior->GetNumParams();
		for (int i = 0; i < count; ++i)
		{
			const NMParam *p = m_pBehavior->GetParam( i );
			NMValue *v = GetValue( p->GetName() );
			if( !v )
			{
				Warningf("Skipping parameter %s, type %s", p->GetName(), p->GetType());
			}
			else
			{
				m_widgets.Grow() = v->AddWidget( bk, *p );
			}
		}
	}
	bk.PopGroup();
}

void NMBehaviorInst::RemoveWidgets( bkBank &bk )
{
	if ( m_pBkGroup != NULL )
	{
		int count = m_widgets.GetCount();
		for (int i = count-1; i >= 0; --i)
		{
			bk.Remove( *(m_widgets[i]) );
		}
		m_widgets.clear();
		
		bk.Remove( *m_pBkGroup );
		m_pBkGroup = NULL;
	}
}
#endif // __BANK

const char* NMBehaviorInst::GetBehaviorName( NMBehavior *pBehavior )
{
	if ( pBehavior != NULL )
	{
		return pBehavior->GetName();
	}

	return NULL;
}

NMBehavior* NMBehaviorInst::FindBehavior( const char* name )
{
	if ( NMBEHAVIORPOOL::IsInstantiated() )
	{
		return const_cast<NMBehavior *>( NMBEHAVIORPOOL::GetInstance().GetBehavior(name) );
	}

	return NULL;
}

//#############################################################################

} // namespace rage
