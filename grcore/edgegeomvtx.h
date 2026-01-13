//
// grcore/edgegeomvtx.h
//
// Copyright (C) 2012-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_EDGEGEOMVTX_H
#define GRCORE_EDGEGEOMVTX_H

#include "fvfchannels.h"
#include "edge/geom/edgegeom_structs.h"

namespace rage
{

static uint8_t GetEdgeOutputVertexFormatId(const EdgeGeomSpuConfigInfo *spuConf, const EdgeGeomVertexStreamDescription *spuOutputStreamDesc, uint16_t vertexInputs)
{
	uint8_t outputVertexFormatId = spuConf->outputVertexFormatId;

	// If the vertex shader only wants positions, switch to a simpler spu output
	// format.  While we will disable the other streams anyways, this ensures we
	// get the stride correct.
	if (vertexInputs == grcFvf::grcfcPositionMask)
	{
		bool outputsPosition = outputVertexFormatId != 0xff ||
			(spuOutputStreamDesc && spuOutputStreamDesc->blocks[0].attributeBlock.edgeAttributeId == EDGE_GEOM_ATTRIBUTE_ID_POSITION);
		if (outputsPosition)
		{
			switch (outputVertexFormatId)
			{
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_X11Y11Z10N:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_I16Nc4:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_X11Y11Z10N_X11Y11Z10N:
				outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3;
				break;
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4_X11Y11Z10N_X11Y11Z10N:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4_X11Y11Z10N_I16Nc4:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4_X11Y11Z10N_X11Y11Z10N_X11Y11Z10N:
				outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4;
				break;
			case EDGE_GEOM_RSX_VERTEX_FORMAT_EMPTY:
				break;
			default:
				Assert(spuOutputStreamDesc);
				if (spuOutputStreamDesc->blocks[0].attributeBlock.componentCount == 3)
				{
					outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3;
				}
				else
				{
					Assert(spuOutputStreamDesc->blocks[0].attributeBlock.componentCount == 4);
					outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4;
				}
				break;
			}
		}
	}

	return outputVertexFormatId;
}

}
// namespace rage

#endif // GRCORE_EDGEGEOMVTX_H
