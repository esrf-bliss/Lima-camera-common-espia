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
#include "EspiaAcq.h"
#include "lima/MemUtils.h"

#include <sstream>

using namespace lima;
using namespace lima::Espia;
using namespace std;

#define CHECK_CALL(ret)		ESPIA_CHECK_CALL(ret)

#define ESPIA_MIN_BUFFER_SIZE	(128 * 1024)

AcqEndCallback::AcqEndCallback()
	: m_acq(NULL)
{
	DEB_CONSTRUCTOR();
}

AcqEndCallback::~AcqEndCallback()
{
	DEB_DESTRUCTOR();

	if (m_acq) {
		DEB_TRACE() << "Unregistering from Acq";
		m_acq->unregisterAcqEndCallback(*this);
	}
}

Acq *AcqEndCallback::getAcq() const
{ 
	return m_acq; 
}

void AcqEndCallback::setAcq(Acq *acq)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(acq);

	if (acq && m_acq) {
		THROW_HW_ERROR(InvalidValue) << "Acquisition already set";
	} else if (!acq && !m_acq) {
		THROW_HW_ERROR(InvalidValue) << "Acquisition already reset";
	}

	m_acq = acq;
}


Acq::Acq(Dev& dev)
	: m_dev(dev)
{
	DEB_CONSTRUCTOR();
	DEB_PARAM() << DEB_VAR1(dev.getDevNb());

	ostringstream os;
	if (dev.isMeta())
		os << "MetaAcq";
	else
		os << "Acq#" << dev.getDevNb();
	DEB_SET_OBJ_NAME(os.str());

	m_sg_img_config = SGImgNorm;
	m_nb_buffers = m_nb_buffer_frames = 0;
	m_real_frame_factor = m_real_frame_size = 0;
	
	m_nb_frames = 0;
	m_started = false;

	resetFrameInfo(m_last_frame_info);
	m_frame_cb_nb = Invalid;
	m_user_frame_cb_act = false;

	m_acq_end_cb = NULL;

	enableFrameCallback();
}

Acq::~Acq()
{
	DEB_DESTRUCTOR();

	if (m_acq_end_cb)
		unregisterAcqEndCallback(*m_acq_end_cb);

	bufferFree();
	disableFrameCallback();
}

Dev& Acq::getDev()
{
	return m_dev;
}

int Acq::dispatchFrameCallback(struct espia_cb_data *cb_data)
{
	DEB_STATIC_FUNCT();
	DEB_PARAM() << DEB_VAR1(cb_data->cb_nr);

	Acq *espia = (Acq *) cb_data->data;
	DEB_TRACE() << DEB_VAR1(DEB_OBJ_NAME(espia));

	bool (Acq::*method)(struct espia_cb_data *cb_data) = NULL;

	int& cb_nb = cb_data->cb_nr;
	if (cb_nb == espia->m_frame_cb_nb) {
		DEB_TRACE() << "Processing FrameCallback";
		method = &Acq::processFrameCallback;
	}

	string err_msg;

	if (method && (cb_data->ret == 0)) {
		try {
			bool cont_cb = (espia->*method)(cb_data);
			if (!cont_cb)
				DEB_ERROR() << "Aborting acquisition!";
			int ret = cont_cb ? ESPIA_OK : ESPIA_ERR_ABORT;
			DEB_RETURN() << DEB_VAR1(ret);
			return ret;
		} catch (Exception& e) {
			err_msg = e.getErrMsg();
		} catch (...) {
			err_msg = "Unknown Error in frame callback";
		}
	} else if (!method) {
		ostringstream os;
		os << "Unexpected frame callback: " << DEB_VAR1(cb_nb);
		err_msg = os.str();
	} else {
		ostringstream os;
		os << "Espia callback error: " << StrError(cb_data->ret);
		err_msg = os.str();
	}

	string espia_name = DEB_OBJ_NAME(espia);
	err_msg.insert(0, espia_name + ": ");
	string overrun_msg = StrError(SCDXIPCI_ERR_OVERRUN);
	bool overrun = (err_msg.find(overrun_msg) != string::npos);
	Event::Code err_code = overrun ? Event::CamOverrun : 
					 Event::CamFault;
	Event *event = new Event(Hardware, Event::Error, Event::Camera, 
				 err_code, err_msg);
	DEB_EVENT(*event) << DEB_VAR1(*event);

	espia->reportEvent(event);

	DEB_ERROR() << "Aborting acquisition!";
	int ret = ESPIA_ERR_ABORT;
	DEB_RETURN() << DEB_VAR1(ret);
	return ret;
}

void Acq::enableFrameCallback()
{
	DEB_MEMBER_FUNCT();

	int& cb_nb = m_frame_cb_nb;
	if (cb_nb != Invalid) {
		DEB_TRACE() << "Callback already enabled";
		return;
	}

	struct espia_cb_data cb_data;
	cb_data.type    = ESPIA_CB_ACQ;
	cb_data.cb      = dispatchFrameCallback;
	cb_data.data    = this;
	cb_data.timeout = SCDXIPCI_BLOCK_FOREVER;

	struct img_frame_info& req_finfo = cb_data.info.acq.req_finfo;
	req_finfo.buffer_nr    = ESPIA_ACQ_ANY;
	req_finfo.frame_nr     = ESPIA_ACQ_ANY;
	req_finfo.round_count  = ESPIA_ACQ_ANY;
	req_finfo.acq_frame_nr = ESPIA_ACQ_EACH;

	DEB_TRACE() << "Registering frame callback";
	m_dev.registerCallback(cb_data, cb_nb);
}

void Acq::disableFrameCallback()
{
	DEB_MEMBER_FUNCT();

	int& cb_nb = m_frame_cb_nb;
	if (cb_nb == Invalid) {
		DEB_TRACE() << "Callback already disabled";
		return;
	}

	DEB_TRACE() << "Unregistering frame callback";
	m_dev.unregisterCallback(cb_nb);
}

void Acq::setFrameCallbackActive(bool cb_active)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(cb_active);
	m_user_frame_cb_act = cb_active;
}

bool Acq::processFrameCallback(struct espia_cb_data *cb_data)
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = acqLock();

	struct img_frame_info& cb_finfo = cb_data->info.acq.cb_finfo;
	bool aborted = finished_espia_frame_info(&cb_finfo, cb_data->ret);
	bool finished = aborted;
	if (!aborted) {
		m_last_frame_info = cb_finfo;
		bool endless = (m_nb_frames == 0);
		unsigned long last_frame = m_nb_frames - 1;
		finished = (!endless && (cb_finfo.acq_frame_nr == last_frame));
	}

	DEB_TRACE() << DEB_VAR2(aborted, finished);

	if (finished)
		m_started = false;

	l.unlock();

	bool cont_cb = true;
	if (aborted)
		return cont_cb;

	HwFrameInfo hw_finfo;
	real2virtFrameInfo(cb_finfo, hw_finfo);
	DEB_TRACE() << DEB_VAR1(hw_finfo);

	if (m_user_frame_cb_act) {
		DEB_TRACE() << "Calling user FrameCallback";
		cont_cb = newFrameReady(hw_finfo);
	}

	if ((m_acq_end_cb != NULL) && finished) {
		DEB_TRACE() << "Calling AcqEndCallback";
		m_acq_end_cb->acqFinished(hw_finfo);
	}

	DEB_RETURN() << DEB_VAR1(cont_cb);
	return cont_cb;
}

void Acq::bufferAlloc(int& nb_buffers, int nb_buffer_frames,
		      const FrameDim& frame_dim)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR3(nb_buffers, nb_buffer_frames, frame_dim);

	if (!frame_dim.isValid() || (nb_buffers <= 0) || 
	    (nb_buffer_frames <= 0))
		THROW_HW_ERROR(InvalidValue) << "Invalid parameters: "
					     << DEB_VAR3(frame_dim, nb_buffers,
							 nb_buffer_frames);

	if ((frame_dim == m_frame_dim) && (nb_buffers == m_nb_buffers) &&
	    (nb_buffer_frames == m_nb_buffer_frames)) {
		DEB_TRACE() << "Requested buffers already allocated";
		return;
	}

	bufferFree();

	int& virt_buffers   = nb_buffers;
	int& virt_frames    = nb_buffer_frames;

	int real_buffers    = virt_buffers;
	int real_frames     = virt_frames;
	int real_frame_size = frame_dim.getMemSize();
	if (virt_frames == 1) {
		int page_size;
		GetPageSize(page_size);
		real_frame_size += page_size - 1;
		real_frame_size &= ~(page_size - 1);
	}

	int real_buffer_size = real_frame_size * real_frames;
	int frame_factor = 1;
	if (real_buffer_size < ESPIA_MIN_BUFFER_SIZE) {
		frame_factor  = ESPIA_MIN_BUFFER_SIZE / real_buffer_size;
		real_frames  *= frame_factor;
		real_buffers += frame_factor - 1;
		real_buffers /= frame_factor;
		virt_buffers  = real_buffers * frame_factor;

		DEB_TRACE() << DEB_VAR3(frame_factor, virt_buffers, 
					virt_frames);
	}

	DEB_TRACE() << "Calling espia_buffer_alloc";
	DEB_TRACE() << DEB_VAR3(real_buffers, real_frames, real_frame_size); 
	CHECK_CALL(espia_buffer_alloc(m_dev, real_buffers, real_frames,
				      real_frame_size));

	m_frame_dim         = frame_dim;
	m_nb_buffers        = virt_buffers;
	m_nb_buffer_frames  = virt_frames;
	m_real_frame_factor = frame_factor;
	m_real_frame_size   = real_frame_size;

	setupSG();

	DEB_RETURN() << DEB_VAR1(nb_buffers);
}

void Acq::bufferFree()
{
	DEB_MEMBER_FUNCT();

	if ((m_nb_buffers == 0) || (m_nb_buffer_frames == 0)) {
		DEB_TRACE() << "Buffers already freed";
		return;
	}

	stop();

	DEB_TRACE() << "Calling espia_buffer_free";
	CHECK_CALL(espia_buffer_free(m_dev));

	m_frame_dim = FrameDim();
	m_nb_buffers = m_nb_buffer_frames = 0;
	m_real_frame_factor = m_real_frame_size = 0;
}

const FrameDim& Acq::getFrameDim()
{
	DEB_MEMBER_FUNCT();
	const FrameDim& frame_dim = m_frame_dim;
	DEB_RETURN() << DEB_VAR1(frame_dim);
	return frame_dim;
}

void Acq::getNbBuffers(int& nb_buffers)
{
	DEB_MEMBER_FUNCT();
	nb_buffers = m_nb_buffers;
	DEB_RETURN() << DEB_VAR1(nb_buffers);
}

void Acq::getNbBufferFrames(int& nb_buffer_frames)
{
	DEB_MEMBER_FUNCT();
	nb_buffer_frames = m_nb_buffer_frames;
	DEB_RETURN() << DEB_VAR1(nb_buffer_frames);
}

void *Acq::getBufferFramePtr(int buffer_nb, int frame_nb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(buffer_nb, frame_nb);

	int real_buffer = realBufferNb(buffer_nb, frame_nb);
	int real_frame  = realFrameNb(buffer_nb, frame_nb);
	void *ptr;
	DEB_TRACE() << "Calling espia_frame_address";
	CHECK_CALL(espia_frame_address(m_dev, real_buffer, real_frame, &ptr));
	DEB_RETURN() << DEB_VAR1(ptr);
	return ptr;
}

void *Acq::getAcqFramePtr(int acq_frame_nb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(acq_frame_nb);

	unsigned long buffer_nb = ESPIA_ACQ_ANY;
	void *ptr;
	DEB_TRACE() << "Calling espia_frame_address";
	CHECK_CALL(espia_frame_address(m_dev, buffer_nb, acq_frame_nb, &ptr));
	DEB_RETURN() << DEB_VAR1(ptr);
	return ptr;
}

void Acq::getFrameInfo(int acq_frame_nb, HwFrameInfoType& info)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(acq_frame_nb);

	struct img_frame_info finfo;
	finfo.buffer_nr    = ESPIA_ACQ_ANY;
	finfo.frame_nr     = ESPIA_ACQ_ANY;
	finfo.round_count  = ESPIA_ACQ_ANY;
	finfo.acq_frame_nr = acq_frame_nb;

	DEB_TRACE() << "Calling espia_get_frame";
	int ret = espia_get_frame(m_dev, &finfo, SCDXIPCI_NO_BLOCK);
	if (ret == SCDXIPCI_ERR_NOTREADY) {
		DEB_TRACE() << "Frame not ready yet";
		info = HwFrameInfo();
		return;
	}
	CHECK_CALL(ret);

	real2virtFrameInfo(finfo, info);
	DEB_RETURN() << DEB_VAR1(info);
}

void Acq::real2virtFrameInfo(const struct img_frame_info& real_info, 
			     HwFrameInfoType& virt_info)
{
	DEB_MEMBER_FUNCT();

	char *buffer_ptr = (char *) real_info.buffer_ptr;
	int frame_offset = real_info.frame_nr * m_real_frame_size;

	virt_info.acq_frame_nb    = real_info.acq_frame_nr;
	virt_info.frame_ptr       = buffer_ptr + frame_offset;
	virt_info.frame_dim       = m_frame_dim;
	virt_info.frame_timestamp = USec2Sec(real_info.time_us);
	virt_info.valid_pixels    = real_info.pixels;
}

void Acq::resetFrameInfo(struct img_frame_info& frame_info)
{
	DEB_MEMBER_FUNCT();

	frame_info.buffer_ptr   = NULL;
	frame_info.buffer_nr    = SCDXIPCI_INVALID;
	frame_info.frame_nr     = SCDXIPCI_INVALID;
	frame_info.round_count  = SCDXIPCI_INVALID;
	frame_info.acq_frame_nr = SCDXIPCI_INVALID;
	frame_info.time_us      = 0;
	frame_info.pixels       = 0;
}

void Acq::setNbFrames(int nb_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(nb_frames);

	if (nb_frames < 0)
		THROW_HW_ERROR(InvalidValue) << "Invalid " 
					     << DEB_VAR1(nb_frames);

	m_nb_frames = nb_frames;
}

void Acq::getNbFrames(int& nb_frames)
{
	DEB_MEMBER_FUNCT();
	nb_frames = m_nb_frames;
	DEB_RETURN() << DEB_VAR1(nb_frames);
}

void Acq::start()
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = acqLock();

	if (m_started) 
		THROW_HW_ERROR(Error) << "Acquisition already running";

	resetFrameInfo(m_last_frame_info);

	DEB_TRACE() << "Calling espia_start_acq, m_nb_frames=" << m_nb_frames;
	CHECK_CALL(espia_start_acq(m_dev, 0, m_nb_frames, SCDXIPCI_NO_BLOCK));

	m_start_ts = Timestamp::now();
	m_started = true;
}

void Acq::stop()
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = acqLock();
	if (!m_started)
		return;

	l.unlock();
	DEB_TRACE() << "Calling espia_stop_acq";
	CHECK_CALL(espia_stop_acq(m_dev));
	l.lock();

	m_started = false;
}

void Acq::getStatus(StatusType& status)
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = acqLock();

	DEB_TRACE() << "Calling espia_acq_active";
	unsigned long acq_run_nb;
	int acq_running = espia_acq_active(m_dev, &acq_run_nb);
	CHECK_CALL(acq_running);

	status.running = m_started;
	status.run_nb  = acq_run_nb;
	status.last_frame_nb = m_last_frame_info.acq_frame_nr;

	DEB_RETURN() << DEB_VAR1(status);
}

void Acq::registerAcqEndCallback(AcqEndCallback& acq_end_cb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(&acq_end_cb, m_acq_end_cb);

	if (m_acq_end_cb)
		THROW_HW_ERROR(InvalidValue) 
			<< "AcqEndCallback already registered";

	acq_end_cb.setAcq(this);
	m_acq_end_cb = &acq_end_cb;
}

void Acq::unregisterAcqEndCallback(AcqEndCallback& acq_end_cb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(&acq_end_cb, m_acq_end_cb);

	if (m_acq_end_cb != &acq_end_cb)
		THROW_HW_ERROR(InvalidValue) 
			<< "Specified AcqEndCallback not registered";

	stop();

	m_acq_end_cb = NULL;
	acq_end_cb.setAcq(NULL);
}

void Acq::setSGImgConfig(SGImgConfig sg_img_config,
			 const Size& det_frame_size)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR4(sg_img_config, det_frame_size,
				m_det_frame_size, m_sg_roi);

	bool roi_active = isSGRoiActive(m_det_frame_size, m_sg_roi);
	if (roi_active && (sg_img_config != SGImgNorm))
		THROW_HW_ERROR(NotSupported)
			<< "SGRoi is active: only available with SGImgNorm";

	m_sg_img_config = sg_img_config;
	m_det_frame_size = det_frame_size;
	setupSG();
}

void Acq::getSGImgConfig(SGImgConfig& sg_img_config,
			 Size& det_frame_size)
{
	DEB_MEMBER_FUNCT();
	sg_img_config = m_sg_img_config;
	det_frame_size = m_det_frame_size;
	DEB_RETURN() << DEB_VAR2(sg_img_config, det_frame_size);
}

void Acq::setSGRoi(const Size& det_frame_size, const Roi& sg_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR3(m_sg_img_config, det_frame_size, sg_roi);

	bool roi_active = isSGRoiActive(det_frame_size, sg_roi);
	if (roi_active && (m_sg_img_config != SGImgNorm))
		THROW_HW_ERROR(NotSupported)
			<< "SGRoi only available with SGImgNorm";

	if (roi_active)
		m_det_frame_size = det_frame_size;
	m_sg_roi = sg_roi;
	setupSG();
}

void Acq::getSGRoi(Size& det_frame_size, Roi& sg_roi)
{
	DEB_MEMBER_FUNCT();
	det_frame_size = m_det_frame_size;
	sg_roi = m_sg_roi;
	DEB_RETURN() << DEB_VAR2(det_frame_size, sg_roi);
}

void Acq::setupSG()
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR4(m_sg_img_config, m_det_frame_size, m_sg_roi,
				m_frame_dim);

	if ((m_nb_buffers == 0) || !isSGRoiValid(m_det_frame_size, m_sg_roi))
		return;

	struct scdxipci_sg_table *sg_list;
	struct espia_frame_dim eframe_dim;
	struct espia_roi eroi, *eroi_ptr;

	eframe_dim.width  = m_det_frame_size.getWidth();
	eframe_dim.height = m_det_frame_size.getHeight();
	eframe_dim.depth  = m_frame_dim.getDepth();

	if (isSGRoiActive(m_det_frame_size, m_sg_roi)) {
		Size roi_size = m_sg_roi.getSize();
		eroi.left   = m_sg_roi.getTopLeft().x;
		eroi.top    = m_sg_roi.getTopLeft().y;
		eroi.width  = roi_size.getWidth();
		eroi.height = roi_size.getHeight();
		eroi_ptr    = &eroi;
	} else {
		eroi_ptr = NULL;
	}

	int nr_dev;
	CHECK_CALL(espia_create_sg(m_dev, m_sg_img_config, &eframe_dim,
				   eroi_ptr, &sg_list, &nr_dev));
	try {
		for (int i = 0; i < nr_dev; ++i)
			CHECK_CALL(espia_set_sg(m_dev, i, &sg_list[i]));
	} catch (...) {
		espia_release_sg(m_dev, sg_list, nr_dev);
		throw;
	}

	CHECK_CALL(espia_release_sg(m_dev, sg_list, nr_dev));
}


ostream& lima::operator <<(ostream& os, const Acq::StatusType& status)
{
	os << "<"
	   << "running=" << status.running << ", "
	   << "run_nb=" << status.run_nb << ", "
	   << "last_frame_nb=" << status.last_frame_nb
	   << ">";

	return os;

}
