//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#ifndef ESPIA_H
#define ESPIA_H

#include "espia_lib.h"
#include <errno.h>
#include "lima/Exceptions.h"
#include "lima/Debug.h"

#include <string>

namespace lima
{

namespace Espia
{

enum {
	Invalid = -1,
	NoBlock = 0,
	BlockForever = -1,
	MetaDev = SCDXIPCI_META_DEV,
};

enum Register {
	RegStatus		= 0x00, //  0 * 4
	RegControl		= 0x04, //  1 * 4
	RegDMAAddr		= 0x08, //  2 * 4
	RegDMASize		= 0x0c, //  3 * 4
	RegSGTabAddr		= 0x10, //  4 * 4
	RegFlashAdd		= 0x1c, //  7 * 4
	RegFlashData		= 0x20, //  8 * 4
	RegSerialStatus		= 0x24, //  9 * 4
	RegPixelCount		= 0x28, // 10 * 4
};

enum SGImgConfig {
	SGImgNorm,
	SGImgFlipVert1,
	SGImgFlipVert2,
	SGImgConcatVert2,
	SGImgConcatVertInv2,
};

std::ostream& operator <<(std::ostream& os, SGImgConfig img_config);

inline unsigned long Sec2USec(double sec)
{
	if (sec > 0)
		sec *= 1e6;
	return (unsigned long) sec;
}

inline double USec2Sec(unsigned long usec)
{
	if (usec > 0)
		return usec * 1e-6;
	return usec;
}

std::string StrError(int ret);

#define ESPIA_CHECK_CALL(espia_ret)					\
	do {								\
		int aux_ret = (espia_ret);				\
		if (aux_ret < 0)					\
			THROW_HW_ERROR(Error) << "Espia error: "	\
					      << StrError(aux_ret);	\
	} while (0)


} // namespace Espia

} // namespace lima


#endif // ESPIA_H
