/*
* Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved.
*
* Not to be copied, adapted, modified, used, distributed, sold,
* licensed or commercially exploited in any manner without the
* written consent of NaturalMotion.
*
* All non public elements of this software are the confidential
* information of NaturalMotion and may not be disclosed to any
* person nor used for any purpose not expressly approved by
* NaturalMotion in writing.
*
*/



#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

#include "art/MessageParams.h"

namespace ART
{
  /**
   * general note - masking is performed on a part basis.  to mask a particular joint,
   * indicate the joint's child part.
   */

  /**
   * expressionToMask - helper function to convert from a string expression to a mask
   * Worth noting that white spaces are not supported and will result in an empty mask
   * Similarly, parentheses are not supported
   * Case insensitive.
   */

  /**
   * Note For GTA Human, MP3 Human and rdrHorse ONLY.
   * mmmmtodo remove rdrHorse from this code or SWITCH in? 
   * rdr humans have a different root that is not taken into account here mmmmtodo SWITCH in code for different root?
   */

  BehaviourMask NmRsCharacter::expressionToMask(const char* const expression)
  {
    // early out if empty
    if(!expression)
      return bvmask_Full;

    int nLength = istrlen(expression);
    Assert(nLength < ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS);

    // Make sure we don't write past the end of our buffer:
    if(nLength >= ART_PARAMETER_NAME_LENGTH_IN_MESSAGEPARAMS)
      return bvmask_Full;

    // buffer expression and convert to lower case
    char buffer[ART_PARAMETER_STRING_LENGTH_IN_MESSAGEPARAMS];
    int i;
    for(i = 0; i < nLength; ++i)
      buffer[i] = (char)tolower(expression[i]);
    buffer[i] = 0; // replace trailing null

    // check for hex string case
    if(buffer[0] == '0' && buffer[1] == 'x')
      return hexToMask(buffer+2);

    // check for binary string case
    if(buffer[0] == '0' && buffer[1] == 'b')
      return binToMask(buffer+2);

    return evaluateMaskExpression(buffer, 0, nLength);
  }

  //
  // nameToMask - helper function to convert from mask names, e.g. "UK", to the
  // bitmask.
  // 
  // Should not be used inside behaviour code, as there are perfectly good mask
  // types for this.
  BehaviourMask NmRsCharacter::nameToMask(const char* const maskName)
  {
    // early out if empty
    if(!maskName || getBodyIdentifier() == notSpecified)
      return bvmask_Full;

    BehaviourMask mask = bvmask_None;

    // strip out a possible '~' one's compliment character
    int offset = 0;
    bool invert = false;
    if(maskName[0] == '~')
    {
      invert = true;
      offset = 1;
    }

    if (maskName[offset] == 'f' &&  maskName[offset+1] == 'b')
      mask = bvmask_Full;
    else if (maskName[offset] == 'n' &&  maskName[offset+1] == 'a')
      mask = bvmask_None;

    if (maskName[offset] == 'u')
    {
      if (maskName[offset+1] == 'w')
        mask |= bvmask_HandLeft | bvmask_HandRight;
      if (maskName[offset+1] == 'c')
        mask |= bvmask_ClavicleLeft | bvmask_ClavicleRight;
      if (maskName[offset+1] == 'a' || maskName[offset+1] == 'b' || maskName[offset+1] == 't' || maskName[offset+1] == 'l')
        mask |= bvmask_ArmLeft;
      if (maskName[offset+1] == 'a' || maskName[offset+1] == 'b' || maskName[offset+1] == 't' || maskName[offset+1] == 'r')
        mask |= bvmask_ArmRight;
      if (maskName[offset+1] == 's' || maskName[offset+1] == 'b' || maskName[offset+1] == 't' || maskName[offset+1] == 'k')
        mask |= bvmask_LowSpine;
      if (maskName[offset+1] == 'n' || maskName[offset+1] == 'b' || maskName[offset+1] == 'k')
        mask |= bvmask_CervicalSpine;
    }
    else if (maskName[offset] == 'l')
    {
      if (maskName[offset+1] == 'b')
        mask |= bvmask_Pelvis;
      if (maskName[offset+1] == 'l' || maskName[offset+1] == 'b')
        mask |= bvmask_LegLeft;
      if (maskName[offset+1] == 'r' || maskName[offset+1] == 'b')
        mask |= bvmask_LegRight;
      if (maskName[offset+1] == 'a')
        mask |= bvmask_FootLeft | bvmask_FootRight;
    }
    else if (maskName[offset] == 's')
    {
      if (maskName[offset+1] == '1' || maskName[offset+1] == '3' || maskName[offset+1] == '5')
        mask |= bvmask_Pelvis | bvmask_Spine0 | bvmask_Spine1; //MP3/GTA - irrelevant as is root, Ok for MP3/GTA as << 0 has no effect.
      if (maskName[offset+1] == '1' || maskName[offset+1] == '3')
        mask |= bvmask_LegLeft | bvmask_LegRight;
      if (maskName[offset+1] == '2' || maskName[offset+1] == '3' || maskName[offset+1] == '4' || maskName[offset+1] == '5')
        mask |= bvmask_HighSpine;
      if (maskName[offset+1] == '2')
        mask |= bvmask_ArmLeft | bvmask_ArmRight;
    }

    if(invert)
      mask = ~mask;

    return mask;
  }

  /**
   * hexToMask - create bit-mask from hex string of the format 0x********
   * should already be all lower case. limited to 32 bits.
   */
  BehaviourMask NmRsCharacter::hexToMask(const char* xs) const
  {
    BehaviourMask result = 0;
    int szlen = istrlen(xs);
    int i, xv, fact;
    if (szlen > 0)
    {
      // Converting more than 32bit hexadecimal value?
      if (szlen > 8)
      {
        Assert(false);
        return 0; // return empty mask
      }

      fact = 1;
      for(i=szlen-1; i>=0 ;i--)
      {
        if(*(xs+i) >= '0' && *(xs+i) <= '9' )       // 0 through 9
          xv = *(xs+i) - '0';
        else if(*(xs+i) >= 'a' && *(xs+i) <= 'f' )  // A through F
          xv = *(xs+i) - 'a' + 10;
        else                                        // not a valid digit.
        {
          // assert and return empty mask.
          Assert(false);
          return 0;
        }
        result += (xv * fact);
        fact *= 16;

      }
    }
    return result;
  }

  /**
  * binToMask - create bit-mask from binary string of the format 0b**...**
  * should already be all lower case. limited to 32 bits.
  */
  BehaviourMask NmRsCharacter::binToMask(const char* xs) const
  {
    BehaviourMask result = 0;
    int szlen = istrlen(xs);
    int i;
    if (szlen > 0)
    {
      // Converting more than 32bit binary value?
      if (szlen > 32)
      {
        Assert(false);
        return 0; // return empty mask
      }

      for(i=szlen-1; i>=0 ;i--)
        if(*(xs+i) != '0')
          result += 1 << i;
    }
    return result;
  }

  BehaviourMask NmRsCharacter::evaluateMaskExpression(const char* const expression, int start, int end)
  {
    Assert(start <= end);

    for(int i = start; i < end; ++i)
    {
      if(expression[i] == '|')
        return evaluateMaskExpression(expression, start, i) | evaluateMaskExpression(expression, i+1, end);
      if(expression[i] == '&')
        return evaluateMaskExpression(expression, start, i) & evaluateMaskExpression(expression, i+1, end);
    }

    return nameToMask(expression+start);
  }

  BehaviourMask NmRsCharacter::partToMask(const int partIndex) const
  {
    Assert(partIndex >= 0 && partIndex < 32);//mmmmmtodo isn't this 32 for horses?
    return 1 << partIndex;
  }

  bool NmRsCharacter::isPartInMask(const BehaviourMask mask, int partIndex) const
  {
    Assert(partIndex >= 0 && partIndex < 32);
    return (mask == bvmask_Full) || (mask & partToMask(partIndex));
  }

  bool NmRsCharacter::isEffectorInMask(const BehaviourMask mask, int effectorIndex) const
  {
    return isPartInMask(mask, effectorIndex+1);
  }
} // namespace ART
