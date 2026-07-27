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
#include "EspiaBufferMgr.h"
#include "lima/MemUtils.h"
#include <sstream>

using namespace lima;
using namespace lima::Espia;
using namespace std;


BufferMgr::BufferMgr(Acq& acq) 
	: m_acq(acq), m_frame_cb(*this)
{
	DEB_CONSTRUCTOR();
       
	Dev& dev = acq.getDev();
	DEB_PARAM() << DEB_VAR1(dev.getDevNb());

	ostringstream os;
	if (dev.isMeta())
		os << "MetaBufferMgr";
	else
		os << "BufferMgr#" << dev.getDevNb();
	DEB_SET_OBJ_NAME(os.str());

	m_frames_per_xfer = 1;
	m_cam_xfer_mode = NoCamMultiFrameXfer;
}

BufferMgr::~BufferMgr() 
{
	DEB_DESTRUCTOR();
}

BufferMgr::Cap BufferMgr::getCap()
{
	return Basic | Concat;
}

void BufferMgr::setAllocParameters(const AllocParameters& alloc_params)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(alloc_params);
	m_alloc_params = alloc_params;
}

void BufferMgr::getAllocParameters(AllocParameters& alloc_params)
{
	DEB_MEMBER_FUNCT();
	alloc_params = m_alloc_params;
	DEB_RETURN() << DEB_VAR1(alloc_params);
}

int BufferMgr::getMaxNbBuffers(const FrameDim& frame_dim,
			       int nb_concat_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(frame_dim, nb_concat_frames);

	FrameDim xfer_frame_dim;
	int xfer_concat_frames;
	getXferParams(frame_dim, nb_concat_frames,
		      xfer_frame_dim, xfer_concat_frames);

	FrameDim buffer_frame_dim;
	getBufferFrameDim(xfer_frame_dim, xfer_concat_frames,
			  buffer_frame_dim);
	DEB_TRACE() << DEB_VAR1(buffer_frame_dim);

	int max_nb_buffers = m_alloc_params.getDefMaxNbBuffers(
					buffer_frame_dim.getMemSize());
	DEB_RETURN() << DEB_VAR1(max_nb_buffers);
	return max_nb_buffers;
}

void BufferMgr::allocBuffers(int nb_buffers, int nb_concat_frames, 
			     const FrameDim& frame_dim)
{
	DEB_MEMBER_FUNCT();

	FrameDim xfer_frame_dim;
	int xfer_concat_frames;
	getXferParams(frame_dim, nb_concat_frames, 
		      xfer_frame_dim, xfer_concat_frames);

	m_acq.bufferAlloc(nb_buffers, xfer_concat_frames, xfer_frame_dim);

	m_frame_dim = frame_dim;
}

const FrameDim& BufferMgr::getFrameDim()
{
	DEB_MEMBER_FUNCT();
	DEB_RETURN() << DEB_VAR1(m_frame_dim);
	return m_frame_dim;
}

void BufferMgr::getNbBuffers(int& nb_buffers)
{
	DEB_MEMBER_FUNCT();
	m_acq.getNbBuffers(nb_buffers);
}

void BufferMgr::getNbConcatFrames(int& nb_concat_frames)
{
	DEB_MEMBER_FUNCT();
	m_acq.getNbBufferFrames(nb_concat_frames);
	nb_concat_frames *= m_frames_per_xfer;
	DEB_RETURN() << DEB_VAR1(nb_concat_frames);
}

void BufferMgr::releaseBuffers()
{
	DEB_MEMBER_FUNCT();
	m_acq.bufferFree();
}

void *BufferMgr::getBufferPtr(int buffer_nb, int concat_frame_nb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(buffer_nb, concat_frame_nb);

	int xfer_frame_nb = concat_frame_nb / m_frames_per_xfer;
	int frame_offset = concat_frame_nb % m_frames_per_xfer;
	DEB_TRACE() << DEB_VAR2(xfer_frame_nb, frame_offset);

	void *ptr = m_acq.getBufferFramePtr(buffer_nb, xfer_frame_nb);
	ptr = (char *) ptr + frame_offset * m_frame_dim.getMemSize();

	DEB_RETURN() << DEB_VAR1(ptr);
	return ptr;
}

void BufferMgr::getFrameInfo(int acq_frame_nb, HwFrameInfoType& info)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(acq_frame_nb);

	int xfer_frame_nb = acq_frame_nb / m_frames_per_xfer;
	m_acq.getFrameInfo(xfer_frame_nb, info);
}

void BufferMgr::setStartTimestamp(Timestamp  start_ts)
{
	DEB_MEMBER_FUNCT();
}

void BufferMgr::getStartTimestamp(Timestamp& start_ts)
{
	DEB_MEMBER_FUNCT();
	m_acq.getStartTimestamp(start_ts);
}

void BufferMgr::setCamMultiFrameXferMode(CamMultiFrameXferMode cam_xfer_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(cam_xfer_mode);
	cam_xfer_mode = m_cam_xfer_mode;
}

void BufferMgr::getCamMultiFrameXferMode(CamMultiFrameXferMode& cam_xfer_mode)
{
	DEB_MEMBER_FUNCT();
	cam_xfer_mode = m_cam_xfer_mode;
	DEB_RETURN() << DEB_VAR1(cam_xfer_mode);
}

void BufferMgr::setNbFramesPerXfer(int frames_per_xfer)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(frames_per_xfer);
	frames_per_xfer = m_frames_per_xfer;
}

void BufferMgr::getNbFramesPerXfer(int& frames_per_xfer)
{
	DEB_MEMBER_FUNCT();
	frames_per_xfer = m_frames_per_xfer;
	DEB_RETURN() << DEB_VAR1(frames_per_xfer);
}

void BufferMgr::getXferParams(const FrameDim& frame_dim, int nb_concat_frames,
			      FrameDim& xfer_frame_dim, int& xfer_concat_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(frame_dim, nb_concat_frames);

	if (nb_concat_frames % m_frames_per_xfer != 0)
		THROW_HW_ERROR(InvalidValue) 
			<< "nb_concat_frames (" << nb_concat_frames << ") "
			<< "is not a multiple of "
			<< "frames_per_xfer (" << m_frames_per_xfer << ")";

	int width = frame_dim.getSize().getWidth();
	int height = frame_dim.getSize().getHeight() * m_frames_per_xfer;
	xfer_frame_dim = FrameDim(width, height, frame_dim.getImageType());
	xfer_concat_frames = nb_concat_frames / m_frames_per_xfer;

	DEB_RETURN() << DEB_VAR2(xfer_frame_dim, xfer_concat_frames);
}

void BufferMgr::setFrameCallbackActive(bool cb_active)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(cb_active);

	if (cb_active)
		m_acq.registerFrameCallback(m_frame_cb);
	else
		m_acq.unregisterFrameCallback(m_frame_cb);
}
	

BufferMgr::FrameCallback::FrameCallback(BufferMgr& buffer_mgr) 
	: m_buffer_mgr(buffer_mgr) 
{
	DEB_CONSTRUCTOR();
}

BufferMgr::FrameCallback::~FrameCallback() 
{
	DEB_DESTRUCTOR();
}

bool BufferMgr::FrameCallback::newFrameReady(const HwFrameInfoType& frame_info)
{
	DEB_MEMBER_FUNCT();
	return m_buffer_mgr.newFrameReady(frame_info);
}
		
