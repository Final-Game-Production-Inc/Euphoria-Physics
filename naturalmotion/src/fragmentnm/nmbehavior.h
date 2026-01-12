// 
// fragmentnm/nmbehavior.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef FRAGMENTNM_NMBEHAVIOR_H 
#define FRAGMENTNM_NMBEHAVIOR_H 

#include "atl/array.h"
#include "atl/hashstring.h"
#include "atl/singleton.h"
#include "data/base.h"
#include "parser/macros.h"
#include "vector/vector3.h"

#include <string.h>

namespace ART
{
	class MessageParamsBase;
}

namespace rage 
{

class bkBank;
class bkGroup;
class bkWidget;
class parTreeNode;
class NMParam;

//#############################################################################

// PURPOSE:
//  Base class to hold the value of a NMParam.  Derived classes specify the
//  type and override the virtual functions.  These are used by the NMBehaviorInst
//  class to provide a value to go with a NMBehavior's NMParam.  The names of 
//  the NMParam and the NMValue need to match exactly in order to be associated
//  correctly.
class NMValue
{
public:
	NMValue();  // only to be called by the parser
	NMValue( const char* name );

	virtual ~NMValue();

	const char* GetName() const { return m_name; }

	// PURPOSE: Converts the value from a derived class to a string
	// RETURNS: string value
	virtual const char* GetStringValue() const { return NULL; }

	// PURPOSE: Converts the string value into the appropriate value type in the derived class
	// PARAMS: string value
	virtual void SetValueFromString( const char* /*value*/ ) {} 

	// PURPOSE: Derived classes override the one they want and make it return true
	//  according to the derived class's value type
	// RETURNS: boolean
	virtual bool IsInt() const { return false; }
	virtual bool IsBool() const { return false; }
	virtual bool IsFloat() const { return false; }
	virtual bool IsVector3() const { return false; }
	virtual bool IsString() const { return false; }

#if __BANK
	// PURPOSE: Adds a single widget according to the value type
	// PARAMS:
	//	bk - bank to add this widget to
	//	p - The parameter for this value.  
	// RETURNS: the widget created
	virtual bkWidget* AddWidget( bkBank &bk, const NMParam &p ) = 0;
#endif

	PAR_PARSABLE;

protected:
	char m_name[64];
};

//#############################################################################

// PURPOSE:
//  Integer value of a NMParam.  AddWidgets adds a slider.
class NMValueInt : public NMValue
{
public:
	NMValueInt();  // only to be called by the parser
	NMValueInt( const char* name, int i=0 );
	NMValueInt( const char* name, const char* value );

	~NMValueInt();

	const char* GetStringValue() const;
	void SetValueFromString( const char* value );

	bool IsInt() const { return true; }

	int GetValue() const { return m_value; }
	void SetValue( int i ) { m_value = i; }

#if __BANK
	// PURPOSE: Adds a Slider Widget
	// PARAMS:
	//	bk - bank to add this widget to
	//	p - The parameter for this value.  
	// RETURNS: the widget created
	virtual bkWidget* AddWidget( bkBank &bk, const NMParam &p );
#endif

	PAR_PARSABLE;

private:
	int m_value;

	static char m_stringValue[64];
};

//#############################################################################

// PURPOSE:
//  Boolean value of a NMParam.  AddWidgets adds a toggle.
class NMValueBool : public NMValue
{
public:
	NMValueBool();  // only to be called by the parser
	NMValueBool( const char* name, bool b=false );
	NMValueBool( const char* name, const char* value );

	~NMValueBool();

	const char* GetStringValue() const;
	void SetValueFromString( const char* value );

	bool IsBool() const { return true; }

	bool GetValue() const { return m_value; }
	void SetValue( bool b ) { m_value = b; }

#if __BANK
	// PURPOSE: Adds a Toggle Widget
	// PARAMS:
	//	bk - bank to add this widget to
	//	p - The parameter for this value.  
	// RETURNS: the widget created
	virtual bkWidget* AddWidget( bkBank &bk, const NMParam &p );
#endif

	PAR_PARSABLE;

private:
	bool m_value;
};

//#############################################################################

// PURPOSE:
//  Float value of a NMParam.  AddWidgets adds a slider.
class NMValueFloat : public NMValue
{
public:
	NMValueFloat(); // only to be called by the parser
	NMValueFloat( const char* name, float f=0.0f );
	NMValueFloat( const char* name, const char* value );

	~NMValueFloat();

	const char* GetStringValue() const;
	void SetValueFromString( const char* value );

	bool IsFloat() const { return true; }

	float GetValue() const { return m_value; }
	void SetValue( float f ) { m_value = f; }

#if __BANK
	// PURPOSE: Adds a Slider Widget
	// PARAMS:
	//	bk - bank to add this widget to
	//	p - The parameter for this value.  
	// RETURNS: the widget created
	virtual bkWidget* AddWidget( bkBank &bk, const NMParam &p );
#endif

	PAR_PARSABLE;

private:
	float m_value;

	static char m_stringValue[64];
};

//#############################################################################

// PURPOSE:
//  Vector3 value of a NMParam.  AddWidgets adds a slider.
class NMValueVector3 : public NMValue
{
public:
	NMValueVector3(); // only to be called by the parser
	NMValueVector3( const char* name, const Vector3 & value );
	NMValueVector3( const char* name, const char* value );

	~NMValueVector3();

	const char* GetStringValue() const;
	void SetValueFromString( const char* value );

	bool IsVector3() const { return true; }

	const Vector3 & GetValue() const { return m_value; }
	void SetValue( const Vector3 &v ) { m_value.Set(v); }

#if __BANK
	// PURPOSE: Adds a Slider Widget
	// PARAMS:
	//	bk - bank to add this widget to
	//	p - The parameter for this value.  
	// RETURNS: the widget created
	virtual bkWidget* AddWidget( bkBank &bk, const NMParam &p );
#endif

	PAR_PARSABLE;

private:
	Vector3 m_value;

	static char m_stringValue[64];
};

//#############################################################################

// PURPOSE:
//  String value of a NMParam.  AddWidgets adds a text box.
class NMValueString : public NMValue
{
public:
	NMValueString();	// only to be called by the parser
	NMValueString( const char* name, const char* s=NULL );

	~NMValueString();

	const char* GetStringValue() const { return m_value; }
	void SetValueFromString( const char* value );

	bool IsString() const { return true; }

	const char* GetValue() const { return m_value; }
	void SetValue( const char* s );

#if __BANK
	// PURPOSE: Adds a Text Widget
	// PARAMS:
	//	bk - bank to add this widget to
	//	p - The parameter for this value.  
	// RETURNS: the widget created
	virtual bkWidget* AddWidget( bkBank &bk, const NMParam &p );
#endif

	PAR_PARSABLE;

private:
	char m_value[64];
};

//#############################################################################

// PURPOSE:
//  NMParam holds the definition of a particular NMBehavior's parameter need by the 
//  Natural Motion system.  These are used by the NMBehavior class to provide 
//  specify a parameter needed by the Natural Motion behavior.  The names of 
//  the NMParam and the NMValue need to match exactly in order to be associated
//  correctly.  The Min, Max, and Step values are only used by NMParams with type
//  "float".
class NMParam
{
public:
	NMParam();  // only to be called by the parser
	NMParam( const char* name, const char* type, const char* description=NULL, char *init=NULL, float min=0.0f, float max=1.0f, float step=0.1f );

	virtual ~NMParam();

	const char* GetName() const { return m_name; }

	// PURPOSE: Tells us what type the NMValue should be
	// RETURNS: boolean
	bool IsInt() const;
	bool IsBool() const;
	bool IsFloat() const;
	bool IsVector3() const;
	bool IsString() const;

	const char* GetType() const { return m_type; }

#if __BANK
	const char* GetDescription() const { return m_description.GetCStr(); }
#endif
	const atHashString& GetDescriptionHash() const { return m_description; }
	void SetDescription( const char* description );

	const char* GetInitValue() const { return m_init; }
	void SetInitValue( const char* init );

	float GetMinValue() const;
	void SetMinValue( float min );

	float GetMaxValue() const;
	void SetMaxValue( float max );

	float GetStepValue() const;
	void SetStepValue( float step );

	PAR_PARSABLE;

private:
	void SetName( const char *name );
	void SetType( const char* type );

	char m_name[64];
	char m_type[8];
	char m_init[64];
	atHashString m_description;
	float m_min;
	float m_max;
	float m_step;
};

//#############################################################################

// PURPOSE:
//  This class defines a built-in Natural Motion behavior, providing its name
//  a helpful description, and a list of all NMParams.  The NMBehaviorPool
//  manages these and provides a pointer to NMBehaviorInsts.
class NMBehavior
{
public:
	NMBehavior();   // only to be called by the parser
	NMBehavior( const char *name, const char *description=0 );

	virtual ~NMBehavior();

	// PURPOSE: Name used to execute the Natural Motion Behavior
	// RETURNS: string
	const char* GetName() const { return m_name; }

#if __BANK
	const char* GetDescription() const { return m_description.GetCStr(); }
#endif
	const atHashString GetDescriptionHash() const { return m_description; }

	void SetDescription( const char* description );

	void AddParam( NMParam *pParam );
	const NMParam* GetParam( const char* name ) const;
	const NMParam* GetParam( int index ) const;

	int GetNumParams() const;

	bool AllowDuplicates() const { return m_allowDuplicates; }
	bool IsTaskMessage() const { return m_taskMessage; }

	PAR_PARSABLE;

private:
	void SetName( const char* name );

	char m_name[64];
	atHashString m_description;
	bool m_allowDuplicates;
	bool m_taskMessage;
	atArray<NMParam *> m_params;
};

//#############################################################################

// PURPOSE:
//  NMBehaviorPool manages all NMBehaviors.  The NMBehaviors can be defined in
//  an xml file or in code.
class NMBehaviorPool
{
public:
	NMBehaviorPool();
	virtual ~NMBehaviorPool();

	void AddBehavior( NMBehavior *pBehavior );
	const NMBehavior* GetBehavior( const char* name ) const;
	const NMBehavior* GetBehavior( int index ) const;

	int GetNumBehaviors() const;

	bool LoadFile( const char* file );
#if __BANK
	bool SaveFile( const char* file );
#endif

	PAR_PARSABLE;

private:
	atArray<NMBehavior *> m_behaviors;
};

typedef atSingleton<NMBehaviorPool> NMBEHAVIORPOOL;

//#############################################################################

// PURPOSE:
//  NMBehaviorInst is just that: an instance of an NMBehavior.  It holds the
//  particular values associated with an NMBehavior so that one can reuse
//  the same behavior with different settings.
// NOTES:
//  NMBEHAVIORPOOL needs to be initialized and loaded before any attempt to
//  load with the PARSER is made.
class NMBehaviorInst : public datBase
{
public:
	NMBehaviorInst();
	NMBehaviorInst( const char* name, const NMBehavior *pBehavior=NULL, const char* description=NULL );
	
	virtual ~NMBehaviorInst();

	// PURPOSE: Name of this behavior instance.  NOT used to execute your Natural Motion Behavior.
	// RETURNS: string
	const char* GetName() const { return m_name; }
	void SetName( const char* name );

	const NMBehavior* GetBehavior() const { return m_pBehavior; }
	void SetBehavior( const NMBehavior *pBehavior ) { m_pBehavior = const_cast<NMBehavior *>( pBehavior ); }

#if __BANK
	const char* GetDescription() const { return m_description.GetCStr(); }
#endif
	const atHashString& GetDescriptionHash() const { return m_description; }

	void SetDescription( const char* description );
	void SetDescriptionHash( const atHashString& description );

	void AddValue( NMValue *pValue );
	NMValue* GetValue( const char* name ) const;
	NMValue* GetValue( int index ) const;

	int GetNumValues() const;

	// PURPOSE: Reads the NMValues and sets up the MessageParams.  Call this before executing your Natural Motion Behavior
	// PARAMS: msg
	void ConfigureMessage( ART::MessageParamsBase * msg ) const;
	
	// PURPOSE: Checks that all value names match parameter names in the associated behavior.  
	//  For every parameter not provided, creates one with the default value.
	void VerifyParamValues();

	// PURPOSE: Initializes this instance with all of the behavior's default parameter values
	void Init();

#if __BANK
	// PURPOSE: Adds a group named after this instance.  The group holds all widgets for manipulating NMParam.
	// PARAMS: bk
	void AddWidgets( bkBank &bk );
	void RemoveWidgets( bkBank &bk );
#endif // __BANK

	// PURPOSE: Used by the PARSER.  
	// PARAMS: pBehavior
	// RETURNS: name of the behavior
	static const char* GetBehaviorName( NMBehavior *pBehavior );

	// PURPOSE: Used by the PARSER.  Searches NMBEHAVIORPOOL so you need to have it initialized and loaded first.
	// PARAMS: name
	// RETURNS: the behavior
	static NMBehavior* FindBehavior( const char* name );

	PAR_PARSABLE;

private:
	char m_name[64];
	atHashString m_description;
	NMBehavior *m_pBehavior;
	atArray<NMValue *> m_values;

#if __BANK
	bkGroup *m_pBkGroup;
	atArray<bkWidget *> m_widgets;
#endif
};

//#############################################################################

} // namespace rage

#endif // FRAGMENTNM_NMBEHAVIOR_H 
