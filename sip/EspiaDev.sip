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

class Dev
{
%TypeHeaderCode
#include <EspiaDev.h>
using namespace lima;
%End
 public:
	Dev(int dev_nb);
	virtual ~Dev();

	int getDevNb();
	bool isMeta();

	virtual void resetLink();
	void getCcdStatus(int& ccd_status);

	void setDebugLevels(int  lib_deb_lvl,       int  drv_deb_lvl);
	void getDebugLevels(int& lib_deb_lvl /Out/, int& drv_deb_lvl /Out/);

	void getDrvOption(const std::string &opt_name, int& val /Out/);
	void setDrvOption(const std::string &opt_name, int val );

	void writeReg(Espia::Register reg, unsigned int& val /In,Out/, 
		      unsigned int mask=0xffffffff);
	void readReg (Espia::Register reg, unsigned int& val /Out/);

	void getParam(const std::string &param_name, int& val /Out/);
	void setParam(const std::string &param_name, int val );

	void getChanUpLed(int &chan_up_led /Out/);

 private:
	Dev(const Espia::Dev&);
};

class Meta : public Espia::Dev
{
%TypeHeaderCode
#include <EspiaDev.h>
using namespace lima;
%End
 public:
	Meta(std::vector<int> dev_nb_list);

	virtual void resetLink();

	int getNbDevs();
	Espia::Dev& getDev(int dev_idx);

 private:
	Meta(const Espia::Meta&);
};

}; // namespace Espia

