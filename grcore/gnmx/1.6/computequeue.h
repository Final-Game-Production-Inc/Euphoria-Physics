/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNMX_COMPUTEQUEUE_H_
#define _SCE_GNMX_COMPUTEQUEUE_H_

#include <gnm.h>
#include "computecontext.h"

namespace sce
{
	namespace Gnmx
	{
		/** @brief Encapsulates the "setup and submit" functionality of compute queues in a single C++ object. */
		class SCE_GNMX_EXPORT ComputeQueue
		{
		public:
			typedef enum SubmissionStatus
			{
				kSubmitOK			   = 0,
				kSubmitFailQueueIsFull = 0x80000001
			} SubmissionStatus;


			/** @brief  Initializes a compute queue object.
				@param  pipeId   Pipe to use for this compute queue (<c><i>pipeId</i></c>: 0 -> 6).
				@param  queueId  Queue to use for this compute queue (<c><i>queueId</i></c>: 0 -> 7).
			*/
			void initialize(uint32_t pipeId, uint32_t queueId);

#ifndef SCE_GNM_OFFLINE_MODE
			/** @brief  Maps a ring buffer address to the compute queue.
				@param  ringBaseAddr Base address of the ring buffer.
				@param  ringSizeInDW     Size of the ring buffer in dwords. The ring size must be a power of two.
				@param  readPtrAddr  Memory address where the hardware is going to write the current 32-bit 'read pointer' value.
				@return Success or failure to map the address.
			*/
			bool map(void *ringBaseAddr, uint32_t ringSizeInDW, void *readPtrAddr);

			/** @brief Unmap the queue */
			void unmap();

			/** @brief Tests to see if this queue is currently mapped. 
				@return Returns true if this queue is currently mapped. 
			*/
			bool isMapped() const { return (m_vqueueId != 0); }

			/** @brief Submits a compute context into this queue.

				@param cmpc  Pointer to the compute context to execute.
				@return Status of the submission.
			*/
			SubmissionStatus submit(ComputeContext *cmpc);

			/** @brief Submits a DispatchCommandBuffer into this queue.

				@param dcb  Pointer to the dispatch command buffer to execute.
				@return Status of the submission.
			*/
			inline SubmissionStatus submit(Gnm::DispatchCommandBuffer *dcb)
			{
				return submit(dcb->m_beginptr, (uint32_t)(dcb->m_cmdptr - dcb->m_beginptr)*4);
			}

			/** @brief Submits a DispatchCommandBuffer into this queue.

				@param pDcb  Pointer to the dispatch command buffer commands to execute.
				@param sizeInBytes  Size in bytes of the dispatch command buffer commands to execute.
				@return Status of the submission.
			*/
			inline SubmissionStatus submit(void const *pDcb, uint32_t sizeInBytes)
			{
				return submit(1, &pDcb, &sizeInBytes);
			}

			/** @brief Submits an array of dispatch command buffers into this queue.

				@param numBuffers  Number of dispatch command buffers to insert into this queue.
				@param apDcb  Array of <c><i>numBuffers</i></c> pointers to command buffers to execute.
				@param aDcbSizes  Array of <c><i>numBuffers</i></c> sizes in bytes for the command buffers to execute.
				@return Status of the submission.
			*/
			SubmissionStatus submit(uint32_t numBuffers, void const*const* apDcb, uint32_t const *aDcbSizes);
#endif // !SCE_GNM_OFFLINE_MODE

			uint32_t  m_pipeId;				  ///< Pipe ID of this compute queue.
			uint32_t  m_queueId;			  ///< Queue ID of this compute queue.
			
			uint32_t  m_vqueueId;			  ///< Virtual queue (vqueue) ID used to unmap, submit, and so on.
			volatile uint32_t* m_readPtrAddr; ///< Read pointer address.

#if !defined(DOXYGEN_IGNORE)
			// Internal use only.
			Gnm::DispatchCommandBuffer m_dcbRoot;
#endif // !defined(DOXYGEN_IGNORE)
		};
	};
}

#endif /* _SCE_GNMX_COMPUTEQUEUE_H_ */
