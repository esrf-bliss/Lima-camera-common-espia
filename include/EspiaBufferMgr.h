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
#ifndef ESPIABUFFERMGR_H
#define ESPIABUFFERMGR_H

#include "lima/HwBufferMgr.h"
#include "EspiaAcq.h"

namespace lima
{

namespace Espia
{

class BufferMgr : public BufferCbMgr
{
	DEB_CLASS_NAMESPC(DebModEspia, "BufferMgr", "Espia");

 public:
	typedef BufferHelper::Parameters AllocParameters;

	enum CamMultiFrameXferMode {
		NoCamMultiFrameXfer,
		CamMultiFrameXfer,
	};

	BufferMgr(Acq& acq);
	virtual ~BufferMgr();

	virtual Cap getCap();

	virtual void setAllocParameters(const AllocParameters& alloc_params);
	virtual void getAllocParameters(      AllocParameters& alloc_params);

	virtual int getMaxNbBuffers(const FrameDim& frame_dim,
				    int nb_concat_frames);
	virtual void allocBuffers(int nb_buffers, int nb_concat_frames, 
				  const FrameDim& frame_dim);
	virtual const FrameDim& getFrameDim();
	virtual void getNbBuffers(int& nb_buffers);
	virtual void getNbConcatFrames(int& nb_concat_frames);
	virtual void releaseBuffers();

	virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb);

	virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info);

	virtual void setStartTimestamp(Timestamp  start_ts);
	virtual void getStartTimestamp(Timestamp& start_ts);

	void setCamMultiFrameXferMode(CamMultiFrameXferMode cam_xfer_mode);
	void getCamMultiFrameXferMode(CamMultiFrameXferMode& cam_xfer_mode);
	void setNbFramesPerXfer(int  frames_per_xfer);
	void getNbFramesPerXfer(int& frames_per_xfer);

 protected:
	class FrameCallback : public HwFrameCallback
	{
		DEB_CLASS_NAMESPC(DebModEspia, "BufferMgr::FrameCallback", 
				  "Espia");

	public:
		FrameCallback(BufferMgr& buffer_mgr);
		virtual ~FrameCallback();

		virtual bool newFrameReady(const HwFrameInfoType& frame_info);
			
	protected:
		BufferMgr& m_buffer_mgr;
	};

	virtual void setFrameCallbackActive(bool cb_active);
	
 private:
	void getXferParams(const FrameDim& frame_dim, int nb_concat_frames,
			   FrameDim& xfer_frame_dim, int& xfer_concat_frames);
	
	Acq& m_acq;
	FrameCallback m_frame_cb;
	int m_frames_per_xfer;
	CamMultiFrameXferMode m_cam_xfer_mode;
	FrameDim m_frame_dim;
	AllocParameters m_alloc_params;
};

} // namespace Espia

} // namespace lima

#endif // ESPIABUFFERMGR_H
