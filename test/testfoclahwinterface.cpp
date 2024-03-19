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
/***************************************************************//**
 * @file testfoclahwinterface.cpp
 * @brief This file is to test Espia::Focla hardware interface
 *
 * This test is based on testfreloninterface.cpp by A.Homs
 *
 * @author A.Kirov
 * @date 15/07/2009
 *******************************************************************/

#include <iostream>

#include "FoclaHwInterface.h"
#include "EspiaBufferMgr.h"
#include "processlib/PoolThreadMgr.h"
#include "lima/CtSaving.h"
#include "processlib/Data.h"

using namespace lima;
using namespace std;


class FoclaFrameCallback : public HwFrameCallback
{
public:
	FoclaFrameCallback( Espia::Focla::Interface &hw_inter, 
	                    CtSaving &buffer_save, Cond &acq_finished ) 
		: m_hw_inter(hw_inter), m_buffer_save(buffer_save), 
		  m_acq_finished(acq_finished) {}
protected:
	virtual bool newFrameReady(const HwFrameInfoType& frame_info);
private:
	Espia::Focla::Interface &m_hw_inter;
	CtSaving &m_buffer_save;
	Cond &m_acq_finished;
};


bool FoclaFrameCallback::newFrameReady( const HwFrameInfoType &frame_info )
{
	int nb_acq_frames = m_hw_inter.getNbAcquiredFrames();
	HwInterface::Status status;
	m_hw_inter.getStatus(status);

	cout << "In callback:" << endl
	     << "  frame_info=" << frame_info << endl
	     << "  nb_acq_frames=" << nb_acq_frames << endl
	     << "  status=" << status << endl;

	Data aNewData = Data();
	aNewData.frameNumber = frame_info.acq_frame_nb;
	const Size &aSize = frame_info.frame_dim.getSize();
	aNewData.dimensions.push_back(aSize.getWidth());
	aNewData.dimensions.push_back(aSize.getHeight());
	aNewData.type = Data::UINT16;
	
	std::function<void(void *)> empty_deleter;
	BufferBase *aNewBuffer = new MappedBuffer(frame_info.frame_ptr, empty_deleter);
	aNewData.setBuffer(aNewBuffer);
	aNewBuffer->unref();

	m_buffer_save.frameReady(aNewData);

	HwSyncCtrlObj *hw_sync;
	m_hw_inter.getHwCtrlObj(hw_sync);
	int nb_frames;
	hw_sync->getNbFrames(nb_frames);
	if (frame_info.acq_frame_nb == nb_frames - 1)
		m_acq_finished.signal();

	return true;
}


void print_status(Espia::Focla::Interface& hw_inter)
{
	HwInterface::Status status;

	hw_inter.getStatus(status);
	cout << "status=" << status << endl;
}


void test_focla_hw_interface()
{
	Espia::Dev espia_dev(0);

	Espia::Focla::Dev focla(espia_dev);

	Espia::Acq espia_acq(espia_dev);
	Espia::BufferMgr espia_buffer_mgr(espia_acq);
	BufferCtrlMgr buffer_mgr(espia_buffer_mgr);


	cout << "Creating Focla Hw Interface ... " << endl;
	Espia::Focla::Interface hw_inter( espia_acq, buffer_mgr, focla );
	cout << " Done!" << endl;


	HwBufferCtrlObj *hw_buffer;
	hw_inter.getHwCtrlObj(hw_buffer);

	FrameDim frame_dim(1024, 1024, Bpp16);
	hw_buffer->setFrameDim(frame_dim);
	int max_nb_buffers;
	hw_buffer->getMaxNbBuffers(max_nb_buffers);
	cout << "MaxNbBuffers " << max_nb_buffers << endl;
	int nb_concat_frames = 1;
	hw_buffer->setNbConcatFrames(nb_concat_frames);  // ???
	int nb_buffers = max_nb_buffers/2;
	hw_buffer->setNbBuffers(nb_buffers);
	cout << "Allocated " << nb_buffers << " buffers" << endl << flush;

	hw_buffer->getFrameDim(frame_dim);
	hw_buffer->getNbBuffers(nb_buffers);
	hw_buffer->getNbConcatFrames(nb_concat_frames);
	cout << "FrameDim " << frame_dim << ", "
	     << "NbBuffers " << nb_buffers << ", "
	     << "NbConcatFrames " << nb_concat_frames << endl << flush;


	CtControl aControl(&hw_inter);
	CtSaving buffer_save(aControl);
	CtSaving::Parameters saving_par;
	saving_par.fileFormat = CtSaving::EDF;
	saving_par.directory = ".";
	saving_par.prefix = "img";
	saving_par.suffix = ".edf";
	saving_par.nextNumber = 0;
	saving_par.savingMode = CtSaving::AutoFrame;
	saving_par.overwritePolicy = CtSaving::Overwrite;
	saving_par.framesPerFile = 1;
	buffer_save.setParameters(saving_par);


	Cond acq_finished;
	FoclaFrameCallback cb(hw_inter, buffer_save, acq_finished);
	hw_buffer->registerFrameCallback(cb);


	HwSyncCtrlObj *hw_sync;
	hw_inter.getHwCtrlObj(hw_sync);


#if 1
	hw_sync->setNbFrames(10);
#else
	espia_acq.setNbFrames(10);
#endif /* 0 */

	print_status(hw_inter);
	hw_inter.startAcq();

#if 1
	acq_finished.wait();
	PoolThreadMgr::get().wait();
#else
	Sleep(10);
#endif /* 0 */

	print_status(hw_inter);
	hw_inter.stopAcq();
	print_status(hw_inter);

}


int main(int argc, char *argv[])
{

	try {
		test_focla_hw_interface();
	} catch (Exception e) {
		cerr << "LIMA Exception: " << e << endl;
		return -1;
	}

	return 0;
}
