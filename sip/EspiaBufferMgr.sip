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

namespace Espia
{

class BufferMgr : BufferCbMgr
{

%TypeHeaderCode
#include "EspiaBufferMgr.h"
#include "lima/MemUtils.h"
%End

 public:
	enum CamMultiFrameXferMode {
		NoCamMultiFrameXfer,
		CamMultiFrameXfer,
	};

	BufferMgr(Espia::Acq& acq);
	virtual ~BufferMgr();

	virtual Cap getCap();

	virtual int getMaxNbBuffers(const FrameDim& frame_dim,
				    int nb_concat_frames);
	virtual void allocBuffers(int nb_buffers, int nb_concat_frames, 
				  const FrameDim& frame_dim);
	virtual const FrameDim& getFrameDim();
	virtual void getNbBuffers(int& nb_buffers /Out/);
	virtual void getNbConcatFrames(int& nb_concat_frames /Out/);
	virtual void releaseBuffers();

	virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb);

	virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info /Out/);

	virtual void setStartTimestamp(Timestamp  start_ts);
	virtual void getStartTimestamp(Timestamp& start_ts /Out/);

	void setCamMultiFrameXferMode(CamMultiFrameXferMode  cam_xfer_mode);
	void getCamMultiFrameXferMode(CamMultiFrameXferMode& cam_xfer_mode /Out/);
	void setNbFramesPerXfer(int  frames_per_xfer);
	void getNbFramesPerXfer(int& frames_per_xfer /Out/);

 protected:
/* XXX This gives errors in SIP
	class FrameCallback : public HwFrameCallback
	{
	public:
		FrameCallback(BufferMgr& buffer_mgr) 
			: m_buffer_mgr(buffer_mgr) {}

		virtual bool newFrameReady(const HwFrameInfoType& frame_info)
		{
			return m_buffer_mgr.newFrameReady(frame_info);
		}
			
	protected:
		BufferMgr& m_buffer_mgr;
	private:
		FrameCallback(const Espia::FrameCallback&);
	};
*/
	virtual void setFrameCallbackActive(bool cb_active);
private:
	BufferMgr(const Espia::BufferMgr&);
};


}; // namespace Espia
