#ifndef NM_ART_H
#define NM_ART_H

#include "art/ARTBaseDefs.h"
#include "nmutils/NMTypes.h"

#define NM_ANIM_MATRICES 0
#define NM_TESTING_ANIM_MATRICES 0

namespace NMutils
{
  struct MemoryConfiguration;
}

namespace ART
{
  struct MemoryConfiguration;
  class ARTFeedbackInterface;
  class MessageParamsBase;
  class MemoryManager;

  /**
   * \brief %ART Asset Data
   *
   * The %AssetBlock represents a collection of ART assets.
   *
   * It is created using AssetBlockWriter and AssetWriter.
   */
  struct AssetBlock
  {
    void*  data;
    size_t dataSize;
  };

  /**
   * Behaviour flag sent with incoming transform pointer, can help TransformSource work with data more effectively
   * see class TransformSource for details.
   */
  enum IncomingTransformStatus
  {
    kITSNone = 0,     /**< No special status */
    kITSDisable,      /**< Last frame that will be available */ // restore rigid bodies for simulation, etc
  };

  /**
   * Which transform source is being supplied - current frame or previous frame?
   */
  enum IncomingTransformSource
  {
    kITSourceCurrent = 0, /**< Transforms are for the current frame */
    kITSourcePrevious,    /**< Transforms are for the previous frame */
#if NM_ANIM_MATRICES
    kITSourceAnimation,
#endif // NM_ANIM_MATRICES
    KITSourceCount        /* how many transform sources will be prepared */
  };

  enum IncomingTransformApplyMode
  {
    kDisabled,        // turned off
    kSingleFrame,     // apply once, then disable
    kEnabling,        // apply initial tm, then continue
    kContinuous,      // apply until told otherwise
    kDisabling,       // frame before disabled, used to set physics back to normal
  };

}

#endif // NM_ART_H
